#pragma once

// Optional Home Assistant link over MQTT discovery (ADR 0005, docs/api.md).
// Disabled by default. Runs in its own task, so a slow or unreachable broker
// never stalls the robot's control loop; commands reach the Robot core
// through DeviceApi like any Manual control.

#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include <auth/pin_guard.h>
#include <home_assistant/home_assistant.h>

#include <mutex>

class DeviceApi;

struct MqttConfig {
  bool enabled = false;
  String host;
  uint16_t port = 1883;
  String user;
  String password;
};

struct MqttStatus {
  bool enabled = false;
  bool connected = false;
  String host;
  uint16_t port = 1883;
  String user;
  String error;
};

class MqttBridge {
 public:
  // `name` and `hostname` identify the robot in Home Assistant.
  void begin(AsyncWebServer& server, auth::PinGuard& pin, DeviceApi& api, const String& name,
             const String& hostname);

  // Broker settings and link state, without the password. Safe from any task.
  MqttStatus status();

 private:
  static void taskEntry(void* self);
  void run();
  void handleMessage(const char* topic, const uint8_t* payload, unsigned int length);
  bool connect();
  void publishState(bool force);
  void publishUpdate();
  void publishJournal();

  void getConfig(AsyncWebServerRequest* request);
  void putConfig(AsyncWebServerRequest* request, JsonVariant& body);

  DeviceApi* api_ = nullptr;
  auth::PinGuard* pin_ = nullptr;
  String name_;
  String hostname_;

  std::mutex lock_;  // guards config_, reconfigure_, connected_, error_
  MqttConfig config_;
  bool reconfigure_ = true;
  bool connected_ = false;
  String error_;

  uint32_t updateGeneration_ = 0;  // of the available update last published
  home_assistant::PlayRemainingReporter playRemaining_;  // touched by the mqtt task only
  uint32_t journalId_ = 0;         // newest Play session journal event published
  String lastState_;
  uint32_t lastStateMs_ = 0;
};
