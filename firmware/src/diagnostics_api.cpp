#include "diagnostics_api.h"

#include <ArduinoJson.h>
#include <AsyncJson.h>
#include <Preferences.h>
#include <WiFi.h>
#include <esp_random.h>
#include <diagnostics/pending.h>
#include <diagnostics/report.h>

#include "device_api.h"
#include "log.h"
#include "mqtt_bridge.h"
#include "network/network.h"

namespace {

// The Pending report lives next to the settings, in the "cwc" NVS namespace.
class NvsBlobs : public diagnostics::BlobStore {
 public:
  bool read(const char* key, void* data, size_t size) override {
    Preferences prefs;
    prefs.begin("cwc", true);
    bool ok = prefs.getBytesLength(key) == size && prefs.getBytes(key, data, size) == size;
    prefs.end();
    return ok;
  }
  bool write(const char* key, const void* data, size_t size) override {
    Preferences prefs;
    prefs.begin("cwc", false);
    bool ok = prefs.putBytes(key, data, size) == size;
    prefs.end();
    return ok;
  }
};

NvsBlobs blobs;

// The Device ID, drawn once with the hardware RNG and kept in NVS.
String deviceId() {
  Preferences prefs;
  prefs.begin("cwc", false);
  String id = prefs.getString("deviceId", "");
  if (id.length() != 8) {
    id = diagnostics::deviceIdFrom(esp_random()).c_str();
    prefs.putString("deviceId", id);
  }
  prefs.end();
  return id;
}

}  // namespace

void registerDiagnostics(AsyncWebServer& server, DeviceApi& api, MqttBridge& mqtt, Network& network,
                         auth::PinGuard& pin, const String& name, const String& hostname) {
  auto build = [&api, &mqtt, &network, &pin, name, hostname](bool redacted) {
    diagnostics::ReportData d;
    d.name = name.c_str();
    d.hostname = hostname.c_str();
    d.version = CWC_VERSION;
    char chip[96];
    snprintf(chip, sizeof chip, "%s rev %u, %u x %u MHz, %u MB flash", ESP.getChipModel(), unsigned(ESP.getChipRevision()),
             unsigned(ESP.getChipCores()), unsigned(ESP.getCpuFreqMHz()), unsigned(ESP.getFlashChipSize() / (1024 * 1024)));
    d.chip = chip;
    d.resetReason = resetReason();
    d.uptimeS = millis() / 1000;
    d.freeHeap = ESP.getFreeHeap();
    d.minFreeHeap = ESP.getMinFreeHeap();
    d.temperature = temperatureRead();
    d.pinEnabled = pin.enabled();
    d.deviceId = deviceId().c_str();

    d.wifiConnected = WiFi.status() == WL_CONNECTED;
    d.wifiNetwork = network.savedNetwork().c_str();
    if (d.wifiConnected) {
      d.ip = WiFi.localIP().toString().c_str();
      d.rssi = WiFi.RSSI();
    }
    d.mac = WiFi.macAddress().c_str();

    MqttStatus link = mqtt.status();
    d.mqttEnabled = link.enabled;
    d.mqttConnected = link.connected;
    d.mqttHost = link.host.c_str();
    d.mqttPort = link.port;
    d.mqttUser = link.user.c_str();
    d.mqttError = link.error.c_str();

    d.settings = api.settingsJson().c_str();
    d.hardware = api.hardwareJson().c_str();
    d.log = recentLog();
    d.notedPersonal = notedPersonal();
    return diagnostics::reportText(d, redacted);
  };

  // Registered before /api/diagnostics, which would also match this path.
  server.on("/api/diagnostics/pending", HTTP_GET, [](AsyncWebServerRequest* request) {
    diagnostics::PendingReport pending;
    JsonDocument doc;
    doc["pending"] = diagnostics::loadPending(blobs, pending);
    if (doc["pending"]) {
      doc["description"] = pending.description;
      doc["report"] = pending.report;
    }
    String body;
    serializeJson(doc, body);
    request->send(200, "application/json", body);
  });
  server.on("/api/diagnostics/pending", HTTP_DELETE, [](AsyncWebServerRequest* request) {
    diagnostics::clearPending(blobs);
    logLine("[support] pending report discarded");
    request->send(200, "application/json", "{\"pending\":false}");
  });
  auto* save = new AsyncCallbackJsonWebHandler(
      "/api/diagnostics/pending", [build](AsyncWebServerRequest* request, JsonVariant& body) {
        const char* description = body["description"] | "";
        if (*description == 0) {
          request->send(400, "application/json", "{\"error\":\"invalid_request\"}");
          return;
        }
        // The robot builds the redacted report itself: never trust a client's copy.
        bool ok = diagnostics::savePending(blobs, description, build(true));
        logLine("[support] pending report %s", ok ? "saved" : "could not be saved");
        request->send(ok ? 200 : 500, "application/json", ok ? "{\"pending\":true}" : "{\"error\":\"storage\"}");
      });
  save->setMethod(HTTP_PUT);
  server.addHandler(save);

  server.on("/api/diagnostics", HTTP_GET, [build](AsyncWebServerRequest* request) {
    bool full = request->hasParam("full") && request->getParam("full")->value() == "1";
    request->send(200, "text/plain; charset=utf-8", build(!full).c_str());
  });
}
