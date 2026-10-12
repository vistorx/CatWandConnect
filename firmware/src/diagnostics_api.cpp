#include "diagnostics_api.h"

#include <WiFi.h>
#include <diagnostics/report.h>

#include "device_api.h"
#include "log.h"
#include "mqtt_bridge.h"
#include "network/network.h"

void registerDiagnostics(AsyncWebServer& server, DeviceApi& api, MqttBridge& mqtt, Network& network,
                         auth::PinGuard& pin, const String& name, const String& hostname) {
  server.on("/api/diagnostics", HTTP_GET, [&api, &mqtt, &network, &pin, name, hostname](AsyncWebServerRequest* request) {
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

    bool full = request->hasParam("full") && request->getParam("full")->value() == "1";
    request->send(200, "text/plain; charset=utf-8", diagnostics::reportText(d, !full).c_str());
  });
}
