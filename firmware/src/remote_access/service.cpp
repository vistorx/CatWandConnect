#include "service.h"

#include <ArduinoJson.h>
#include <AsyncJson.h>
#include <Preferences.h>
#include <WiFi.h>

#include "../log.h"
#include "../pin_http.h"
#include "tailscale.h"

using remote_access::Result;
using remote_access::State;

namespace {

// Its own NVS namespace: the task writes it while the main loop writes "cwc".
class RemoteStorage : public cwc::Storage {
 public:
  void begin() { prefs_.begin("remote", false); }
  bool read(const char* key, void* data, size_t size) override {
    if (prefs_.getBytesLength(key) != size) return false;
    return prefs_.getBytes(key, data, size) == size;
  }
  bool write(const char* key, const void* data, size_t size) override {
    return prefs_.putBytes(key, data, size) == size;
  }

 private:
  Preferences prefs_;
};

class ArduinoClock : public cwc::Clock {
 public:
  uint32_t nowMs() const override { return millis(); }
};

void wipe(std::string& text) {
  text.assign(text.size(), '\0');
  text.clear();
}

constexpr uint32_t kTickMs = 250;

}  // namespace

void RemoteAccessService::begin(AsyncWebServer& server, auth::PinGuard& pin, const String& hostname) {
  pin_ = &pin;
  hostname_ = hostname.c_str();

  server.on("/api/remote-access", HTTP_GET, [this](AsyncWebServerRequest* request) { getStatus(request); });
  auto* put = new AsyncCallbackJsonWebHandler(
      "/api/remote-access", [this](AsyncWebServerRequest* request, JsonVariant& body) { putStatus(request, body); });
  put->setMethod(HTTP_PUT);
  server.addHandler(put);

  // The core reads its setting on the task, before the first status is asked for.
  xTaskCreate(taskEntry, "remote", 8192, this, 1, nullptr);
}

void RemoteAccessService::taskEntry(void* self) { static_cast<RemoteAccessService*>(self)->run(); }

void RemoteAccessService::run() {
  static RemoteStorage storage;
  static ArduinoClock clock;
  static TailscaleProvider tailscale;
  storage.begin();
  remote_access::RemoteAccess remote(tailscale, storage, clock);
  remote.begin(hostname_);

  State logged = State::Off;
  std::string loggedAddress;
  for (;;) {
    Change change;
    std::string key;
    {
      std::lock_guard<std::mutex> guard(lock_);
      change = pending_;
      pending_ = Change::None;
      key.swap(pendingKey_);
    }
    if (change == Change::On) {
      Result result = remote.turnOn(key);
      logLine("[remote] turned on (%s)%s", tailscale.id(),
              result == Result::Ok ? "" : ", could not save the setting");
    } else if (change == Change::Off) {
      remote.turnOff();
      logLine("[remote] turned off, key forgotten");
    }
    wipe(key);

    remote.tick(WiFi.status() == WL_CONNECTED);

    remote_access::Status status = remote.status();
    if (status.state != logged) {
      logged = status.state;
      logLine("[remote] %s", remote_access::stateName(status.state));
    }
    if (status.address != loggedAddress) {
      loggedAddress = status.address;
      if (!status.address.empty()) {
        notePersonal(status.address.c_str(), "tailnet-ip");
        logLine("[remote] address %s", status.address.c_str());
      }
    }
    {
      std::lock_guard<std::mutex> guard(lock_);
      published_ = status;
      // Until the task has read it, show the owner's change at once.
      if (pending_ == Change::On) published_.enabled = true;
      if (pending_ == Change::Off) published_.enabled = false;
    }
    vTaskDelay(pdMS_TO_TICKS(kTickMs));
  }
}

remote_access::Status RemoteAccessService::status() {
  std::lock_guard<std::mutex> guard(lock_);
  return published_;
}

bool RemoteAccessService::viaRemoteAccess(AsyncWebServerRequest* request) {
  IPAddress from = request->client()->remoteIP();
  if (from.type() != IPv4) return false;
  uint32_t address = uint32_t(from[0]) << 24 | uint32_t(from[1]) << 16 | uint32_t(from[2]) << 8 | from[3];
  return remote_access::isTailnetAddress(address);
}

void RemoteAccessService::getStatus(AsyncWebServerRequest* request) {
  remote_access::Status s = status();
  JsonDocument doc;
  doc["enabled"] = s.enabled;
  doc["state"] = remote_access::stateName(s.state);
  doc["provider"] = s.provider;
  doc["address"] = s.address;
  doc["deviceName"] = s.deviceName;
  if (s.keyExpiry > 0) {
    doc["keyExpiry"] = s.keyExpiry;
  } else {
    doc["keyExpiry"] = nullptr;
  }
  doc["routes"] = s.routes;
  doc["viaRemoteAccess"] = viaRemoteAccess(request);
  String body;
  serializeJson(doc, body);
  request->send(200, "application/json", body);
}

void RemoteAccessService::putStatus(AsyncWebServerRequest* request, JsonVariant& body) {
  if (!requirePin(*pin_, request)) return;
  if (!body["enabled"].is<bool>()) {
    request->send(400, "application/json", "{\"error\":\"invalid_request\"}");
    return;
  }
  const bool enable = body["enabled"];
  std::string key = body["key"] | "";
  const char* error = nullptr;
  remote_access::Status now = status();
  if (enable && now.state == State::Unsupported) {
    error = "unsupported";
  } else if (enable && key.empty() && !now.enabled) {
    error = "key_required";
  } else if (enable && !key.empty() && !remote_access::validKey(key)) {
    error = "invalid_key";
  }
  if (error != nullptr) {
    wipe(key);
    request->send(400, "application/json", String("{\"error\":\"") + error + "\"}");
    return;
  }
  if (!enable || !key.empty()) {
    std::lock_guard<std::mutex> guard(lock_);
    pending_ = enable ? Change::On : Change::Off;
    wipe(pendingKey_);
    pendingKey_.swap(key);
    published_.enabled = enable;
    if (!enable) {
      published_.state = State::Off;
      published_.address.clear();
      published_.keyExpiry = 0;
    } else {
      published_.state = State::Connecting;
    }
  }
  wipe(key);
  getStatus(request);
}
