#pragma once

// The Device API (docs/api.md): WebSocket /ws for Manual control and live
// state, REST /api/settings for the Robot core's settings.
//
// HTTP and WebSocket events arrive on the network task while the Robot core
// ticks on the main loop, so every access to the robot holds robotLock_.
// Manual control commands go through an inbox drained by loop().

#include <ArduinoJson.h>
#include <auth/pin_guard.h>
#include <ESPAsyncWebServer.h>
#include <robot_core/hardware.h>
#include <robot_core/robot.h>

#include <atomic>
#include <mutex>

class DeviceApi {
 public:
  DeviceApi(cwc::Robot& robot, cwc::Storage& storage, cwc::Chip chip, auth::PinGuard& pin)
      : robot_(robot), storage_(storage), chip_(chip), pin_(pin) {}

  // `hardware` is the wiring the servo driver was started with; `name` the
  // robot's name.
  void begin(AsyncWebServer& server, const cwc::HardwareConfig& hardware, const String& name);

  // Runs `f(robot)` under the robot lock, for other tasks such as MQTT.
  template <class F>
  auto withRobot(F&& f) {
    std::lock_guard<std::mutex> guard(robotLock_);
    return f(robot_);
  }

  // The settings and the saved wiring as GET /api/settings and /api/hardware
  // return them, for the Diagnostic report.
  String settingsJson();
  String hardwareJson();

  // Stops Manual control before a firmware update; safe from any task.
  void stopForUpdate();

  // Restarts the board shortly, after the current response has gone out.
  void restartSoon();

  // Applies pending commands, ticks the robot and broadcasts its state.
  // Restarts the board when a hardware change asked for it.
  void loop();

 private:
  struct Inbox {
    bool move = false;
    float horizontal = 0;
    float vertical = 0;
    bool stop = false;
    bool stow = false;
  };

  void onEvent(AsyncWebSocketClient* client, AwsEventType type, void* arg, uint8_t* data, size_t len);
  void onMessage(const char* text, size_t len);
  void broadcastState(bool force);

  void getSettings(AsyncWebServerRequest* request);
  void putSettings(AsyncWebServerRequest* request, JsonVariant& body);
  void getCalibration(AsyncWebServerRequest* request);
  void postCalibration(AsyncWebServerRequest* request, JsonVariant& body);
  void writeCalibration(JsonObject out);  // caller holds robotLock_
  void getHardware(AsyncWebServerRequest* request);
  void putHardware(AsyncWebServerRequest* request, JsonVariant& body);
  void writeHardware(JsonObject out, const cwc::HardwareConfig& hardware);
  void getInfo(AsyncWebServerRequest* request);
  void putDevice(AsyncWebServerRequest* request, JsonVariant& body);
  void getAuth(AsyncWebServerRequest* request);
  void unlock(AsyncWebServerRequest* request, JsonVariant& body);
  void putPin(AsyncWebServerRequest* request, JsonVariant& body);

  cwc::Robot& robot_;
  cwc::Storage& storage_;
  cwc::Chip chip_;
  auth::PinGuard& pin_;  // only touched from the network task
  std::mutex robotLock_;
  cwc::HardwareConfig hardware_{};       // as saved, applied after restart
  std::atomic<uint32_t> restartAt_{0};
  String name_;
  AsyncWebSocket socket_{"/ws"};

  std::mutex inboxLock_;
  Inbox inbox_;

  std::atomic<bool> newClient_{false};
  uint32_t lastCheckMs_ = 0;
  uint32_t lastSentMs_ = 0;
  uint32_t lastCleanupMs_ = 0;
  String lastState_;
};
