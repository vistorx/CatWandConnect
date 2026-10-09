#pragma once

// The Device API over WebSocket (/ws), documented in docs/api.md. Translates
// messages into Robot core commands and broadcasts the robot's state.
//
// WebSocket events arrive on the network task while the Robot core runs on the
// main loop, so commands go through a locked inbox drained by loop().

#include <ESPAsyncWebServer.h>
#include <robot_core/robot.h>

#include <atomic>
#include <mutex>

class DeviceApi {
 public:
  explicit DeviceApi(cwc::Robot& robot) : robot_(robot) {}

  void begin(AsyncWebServer& server);

  // Applies pending commands and broadcasts state. Call before Robot::tick().
  void loop();

 private:
  struct Inbox {
    bool move = false;
    float horizontal = 0;
    float vertical = 0;
    bool stop = false;
  };

  void onEvent(AsyncWebSocketClient* client, AwsEventType type, void* arg, uint8_t* data, size_t len);
  void onMessage(const char* text, size_t len);
  void broadcastState(bool force);

  cwc::Robot& robot_;
  AsyncWebSocket socket_{"/ws"};

  std::mutex lock_;
  Inbox inbox_;

  std::atomic<bool> newClient_{false};
  uint32_t lastCheckMs_ = 0;
  uint32_t lastSentMs_ = 0;
  uint32_t lastCleanupMs_ = 0;
  String lastState_;
};
