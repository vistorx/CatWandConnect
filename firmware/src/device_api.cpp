#include "device_api.h"

#include <ArduinoJson.h>

namespace {

constexpr uint32_t kBroadcastIntervalMs = 100;
// Sent even when nothing changed, so clients can tell the robot is alive.
constexpr uint32_t kHeartbeatMs = 2000;
constexpr uint32_t kCleanupIntervalMs = 1000;

const char* activityName(cwc::Activity activity) {
  switch (activity) {
    case cwc::Activity::Manual: return "manual";
    case cwc::Activity::Idle: return "idle";
  }
  return "idle";
}

}  // namespace

void DeviceApi::begin(AsyncWebServer& server) {
  socket_.onEvent([this](AsyncWebSocket*, AsyncWebSocketClient* client, AwsEventType type, void* arg,
                         uint8_t* data, size_t len) { onEvent(client, type, arg, data, len); });
  server.addHandler(&socket_);
}

void DeviceApi::onEvent(AsyncWebSocketClient* client, AwsEventType type, void* arg, uint8_t* data, size_t len) {
  switch (type) {
    case WS_EVT_CONNECT:
      newClient_ = true;  // make loop() send the state to the newcomer
      break;

    case WS_EVT_DISCONNECT: {
      // Dead-man rule: a vanished controller must not leave the wand moving.
      std::lock_guard<std::mutex> guard(lock_);
      inbox_.stop = true;
      inbox_.move = false;
      break;
    }

    case WS_EVT_DATA: {
      auto* info = static_cast<AwsFrameInfo*>(arg);
      // Commands are small; only accept complete single-frame text messages.
      if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
        onMessage(reinterpret_cast<const char*>(data), len);
      }
      break;
    }

    default:
      break;
  }
}

void DeviceApi::onMessage(const char* text, size_t len) {
  JsonDocument doc;
  if (deserializeJson(doc, text, len)) return;
  const char* type = doc["t"] | "";

  std::lock_guard<std::mutex> guard(lock_);
  if (strcmp(type, "move") == 0) {
    inbox_.move = true;
    inbox_.horizontal = doc["h"] | 0.0f;
    inbox_.vertical = doc["v"] | 0.0f;
  } else if (strcmp(type, "stop") == 0) {
    inbox_.stop = true;
    inbox_.move = false;
  }
}

void DeviceApi::loop() {
  Inbox inbox;
  {
    std::lock_guard<std::mutex> guard(lock_);
    inbox = inbox_;
    inbox_ = Inbox{};
  }
  if (inbox.stop) robot_.manualStop();
  if (inbox.move) robot_.manualMove(inbox.horizontal, inbox.vertical);

  uint32_t now = millis();
  if (now - lastCheckMs_ >= kBroadcastIntervalMs) {
    lastCheckMs_ = now;
    broadcastState(newClient_.exchange(false) || now - lastSentMs_ >= kHeartbeatMs);
  }
  if (now - lastCleanupMs_ >= kCleanupIntervalMs) {
    socket_.cleanupClients();
    lastCleanupMs_ = now;
  }
}

void DeviceApi::broadcastState(bool force) {
  if (socket_.count() == 0) return;

  JsonDocument doc;
  doc["t"] = "state";
  doc["h"] = serialized(String(robot_.position(cwc::Axis::Horizontal), 1));
  doc["v"] = serialized(String(robot_.position(cwc::Axis::Vertical), 1));
  doc["activity"] = activityName(robot_.activity());
  String state;
  serializeJson(doc, state);

  if (!force && state == lastState_) return;
  socket_.textAll(state);
  lastState_ = state;
  lastSentMs_ = millis();
}
