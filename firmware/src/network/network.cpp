#include "network/network.h"

#include <identity/identity.h>

#include <ESPmDNS.h>
#include <WiFi.h>


namespace {

constexpr uint32_t kConnectTimeoutMs = 20000;
// How long a lost connection may last before the hotspot opens.
constexpr uint32_t kLostGraceMs = 30000;
// How often the hotspot retries the saved network while nobody is connected to it.
constexpr uint32_t kRetryIntervalMs = 120000;
// Keeps the hotspot up after a setup-page success so the phone can read the new address.
constexpr uint32_t kHotspotLingerMs = 30000;

#if CONFIG_IDF_TARGET_ESP32S3
constexpr const char* kChipFamily = "ESP32-S3";
#elif CONFIG_IDF_TARGET_ESP32C3
constexpr const char* kChipFamily = "ESP32-C3";
#else
constexpr const char* kChipFamily = "ESP32";
#endif

String jsonString(const String& value) {
  String out = "\"";
  for (size_t i = 0; i < value.length(); ++i) {
    char c = value[i];
    if (c == '"' || c == '\\') {
      out += '\\';
      out += c;
    } else if (static_cast<uint8_t>(c) < 0x20) {
      char escaped[7];
      snprintf(escaped, sizeof(escaped), "\\u%04x", c);
      out += escaped;
    } else {
      out += c;
    }
  }
  return out + "\"";
}

}  // namespace

void Network::begin(AsyncWebServer& server, const String& deviceName, const String& hostname) {
  deviceName_ = deviceName;
  hostname_ = hostname;
  uint8_t mac[6];
  WiFi.macAddress(mac);
  char suffix[5];
  snprintf(suffix, sizeof(suffix), "%02X%02X", mac[4], mac[5]);
  hotspotSsid_ = identity::hotspotName(deviceName_.c_str(), suffix).c_str();

  prefs_.begin("wifi", false);
  savedSsid_ = prefs_.getString("ssid", "");
  savedPassword_ = prefs_.getString("pass", "");

  // Credentials live in our own namespace; keep the Wi-Fi driver from storing a copy.
  WiFi.persistent(false);
  WiFi.setHostname(hostname_.c_str());

  setupRoutes(server);

  if (savedSsid_.isEmpty()) {
    Serial.println("[wifi] no saved network");
    openHotspot();
  } else {
    startAttempt(savedSsid_, savedPassword_, Source::Saved);
  }
}

void Network::loop() {
  uint32_t now = millis();
  handleImprov();

  if (setupRequested_) {
    setupRequested_ = false;
    startAttempt(setupSsid_, setupPassword_, Source::SetupPage);
  }

  switch (mode_) {
    case Mode::Connecting:
      if (WiFi.status() == WL_CONNECTED) {
        onConnected();
      } else if (now - modeSince_ > kConnectTimeoutMs) {
        onAttemptFailed();
      }
      break;

    case Mode::Connected:
      if (WiFi.status() == WL_CONNECTED) {
        lostSince_ = 0;
      } else if (lostSince_ == 0) {
        Serial.println("[wifi] connection lost, reconnecting");
        lostSince_ = now;
      } else if (now - lostSince_ > kLostGraceMs) {
        Serial.println("[wifi] still unreachable");
        WiFi.setAutoReconnect(false);
        WiFi.disconnect();
        openHotspot();
      }
      break;

    case Mode::Hotspot:
      if (!savedSsid_.isEmpty() && static_cast<int32_t>(now - nextRetryAt_) >= 0 &&
          WiFi.softAPgetStationNum() == 0) {
        startAttempt(savedSsid_, savedPassword_, Source::Saved);
      }
      break;
  }

  if (hotspotActive_) {
    dns_.processNextRequest();
    if (hotspotCloseAt_ != 0 && static_cast<int32_t>(now - hotspotCloseAt_) >= 0) closeHotspot();
  }
}

void Network::startAttempt(const String& ssid, const String& password, Source source) {
  Serial.printf("[wifi] connecting to \"%s\"\n", ssid.c_str());
  attemptSsid_ = ssid;
  attemptPassword_ = password;
  attemptSource_ = source;
  lastAttemptFailed_ = false;

  WiFi.disconnect();
  WiFi.mode(hotspotActive_ ? WIFI_AP_STA : WIFI_STA);
  WiFi.setAutoReconnect(false);
  WiFi.begin(ssid.c_str(), password.c_str());
  mode_ = Mode::Connecting;
  modeSince_ = millis();
}

void Network::onConnected() {
  Serial.printf("[wifi] connected, open %s or http://%s.local/\n", deviceUrl().c_str(), hostname_.c_str());
  mode_ = Mode::Connected;
  lostSince_ = 0;
  WiFi.setAutoReconnect(true);

  if (attemptSource_ != Source::Saved) {
    savedSsid_ = attemptSsid_;
    savedPassword_ = attemptPassword_;
    prefs_.putString("ssid", savedSsid_);
    prefs_.putString("pass", savedPassword_);
  }
  if (attemptSource_ == Source::Improv) sendImprovConnected();

  if (hotspotActive_) {
    if (attemptSource_ == Source::SetupPage) {
      hotspotCloseAt_ = millis() + kHotspotLingerMs;
    } else {
      closeHotspot();
    }
  }
  startMdns();
}

void Network::onAttemptFailed() {
  Serial.printf("[wifi] could not connect to \"%s\"\n", attemptSsid_.c_str());
  lastAttemptFailed_ = true;
  WiFi.disconnect();
  if (attemptSource_ == Source::Improv) {
    sendImprov(improv::errorPacket(improv::Error::UnableToConnect));
    sendImprov(improv::statePacket(improv::State::Authorized));
  }
  openHotspot();
}

void Network::openHotspot() {
  mode_ = Mode::Hotspot;
  modeSince_ = millis();
  nextRetryAt_ = millis() + kRetryIntervalMs;
  hotspotCloseAt_ = 0;
  if (hotspotActive_) return;

  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(hotspotSsid_.c_str());
  dns_.start(53, "*", WiFi.softAPIP());
  hotspotActive_ = true;
  Serial.printf("[wifi] hotspot \"%s\" open, setup page at http://%s/\n", hotspotSsid_.c_str(),
                WiFi.softAPIP().toString().c_str());
  startMdns();
}

void Network::closeHotspot() {
  dns_.stop();
  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_STA);
  hotspotActive_ = false;
  hotspotCloseAt_ = 0;
  Serial.println("[wifi] hotspot closed");
}

void Network::startMdns() {
  if (mdnsStarted_) return;
  if (MDNS.begin(hostname_.c_str())) {
    MDNS.addService("http", "tcp", 80);
    mdnsStarted_ = true;
  }
}

String Network::deviceUrl() const { return "http://" + WiFi.localIP().toString() + "/"; }

// --- Improv over serial ---------------------------------------------------

void Network::handleImprov() {
  while (Serial.available() > 0) {
    switch (improv_.feed(static_cast<uint8_t>(Serial.read()))) {
      case improv::Parser::Status::Request:
        answerImprov(improv_.request());
        break;
      case improv::Parser::Status::Invalid:
        sendImprov(improv::errorPacket(improv_.error()));
        break;
      case improv::Parser::Status::Pending:
        break;
    }
  }
}

void Network::answerImprov(const improv::Request& request) {
  using improv::Command;
  switch (request.command) {
    case Command::WifiSettings:
      sendImprov(improv::statePacket(improv::State::Provisioning));
      startAttempt(String(request.ssid.c_str()), String(request.password.c_str()), Source::Improv);
      break;

    case Command::GetCurrentState:
      if (mode_ == Mode::Connected) {
        sendImprovConnected();
      } else if (mode_ == Mode::Connecting && attemptSource_ == Source::Improv) {
        sendImprov(improv::statePacket(improv::State::Provisioning));
      } else {
        sendImprov(improv::statePacket(improv::State::Authorized));
      }
      break;

    case Command::GetDeviceInfo:
      sendImprov(improv::rpcResultPacket(Command::GetDeviceInfo,
                                         {"CatWandConnect", CWC_VERSION, kChipFamily, deviceName_.c_str()}));
      break;

    case Command::GetWifiNetworks: {
      int count = WiFi.scanNetworks();
      for (int i = 0; i < count; ++i) {
        sendImprov(improv::rpcResultPacket(
            Command::GetWifiNetworks,
            {WiFi.SSID(i).c_str(), std::to_string(WiFi.RSSI(i)),
             WiFi.encryptionType(i) == WIFI_AUTH_OPEN ? "NO" : "YES"}));
      }
      WiFi.scanDelete();
      sendImprov(improv::rpcResultPacket(Command::GetWifiNetworks, {}));
      break;
    }
  }
}

void Network::sendImprov(const std::vector<uint8_t>& packet) {
  Serial.write(packet.data(), packet.size());
}

void Network::sendImprovConnected() {
  sendImprov(improv::statePacket(improv::State::Provisioned));
  sendImprov(improv::rpcResultPacket(improv::Command::WifiSettings, {deviceUrl().c_str()}));
}

// --- Web server -------------------------------------------------------------

void Network::setupRoutes(AsyncWebServer& server) {
  server.on("/api/wifi/status", HTTP_GET, [this](AsyncWebServerRequest* request) {
    request->send(200, "application/json", statusJson());
  });

  server.on("/api/wifi/networks", HTTP_GET, [this](AsyncWebServerRequest* request) {
    request->send(200, "application/json", networksJson());
  });

  // Only offered while the hotspot is up; changing Wi-Fi from the home network
  // comes with the settings PIN (#12).
  server.on("/api/wifi", HTTP_POST, [this](AsyncWebServerRequest* request) {
    if (!hotspotActive_) {
      request->send(409, "text/plain", "Wi-Fi can only be set from the hotspot");
      return;
    }
    if (!request->hasParam("ssid", true) || request->getParam("ssid", true)->value().isEmpty()) {
      request->send(400, "text/plain", "Missing network name");
      return;
    }
    setupSsid_ = request->getParam("ssid", true)->value();
    setupPassword_ = request->hasParam("password", true) ? request->getParam("password", true)->value() : "";
    setupRequested_ = true;
    request->send(202, "application/json", "{}");
  });

  // Captive portal: phones probe random URLs; send them all to the setup page.
  server.onNotFound([this](AsyncWebServerRequest* request) {
    if (hotspotActive_) {
      request->redirect("http://" + WiFi.softAPIP().toString() + "/");
    } else {
      request->send(404, "text/plain", "Not found");
    }
  });
}

String Network::statusJson() const {
  const char* mode = mode_ == Mode::Connected ? "connected" : mode_ == Mode::Connecting ? "connecting" : "hotspot";
  String json = "{\"mode\":\"";
  json += mode;
  json += "\",\"failed\":";
  json += lastAttemptFailed_ ? "true" : "false";
  json += ",\"ssid\":" + jsonString(mode_ == Mode::Connected ? savedSsid_ : attemptSsid_);
  json += ",\"ip\":" + jsonString(mode_ == Mode::Connected ? WiFi.localIP().toString() : "");
  json += ",\"host\":" + jsonString(hostname_ + ".local");
  return json + "}";
}

// Scans in the background: the first calls answer {"scanning":true}.
String Network::networksJson() {
  int count = WiFi.scanComplete();
  if (count == WIFI_SCAN_FAILED) {
    WiFi.scanNetworks(true);
    return "{\"scanning\":true}";
  }
  if (count == WIFI_SCAN_RUNNING) return "{\"scanning\":true}";

  String json = "{\"scanning\":false,\"networks\":[";
  for (int i = 0; i < count; ++i) {
    if (i > 0) json += ",";
    json += "{\"ssid\":" + jsonString(WiFi.SSID(i));
    json += ",\"rssi\":" + String(WiFi.RSSI(i));
    json += ",\"secure\":";
    json += WiFi.encryptionType(i) == WIFI_AUTH_OPEN ? "false" : "true";
    json += "}";
  }
  WiFi.scanDelete();
  return json + "]}";
}
