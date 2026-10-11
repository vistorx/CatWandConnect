#pragma once

// Optional Home Assistant link over MQTT discovery (ADR 0005, docs/api.md).
// Disabled by default. Runs in its own task, so a slow or unreachable broker
// never stalls the robot's control loop; commands reach the Robot core
// through DeviceApi like any Manual control.

#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include <auth/pin_guard.h>

#include <mutex>

class DeviceApi;

struct MqttConfig {
  bool enabled = false;
  String host;
  uint16_t port = 1883;
  String user;
  String password;
};

class MqttBridge {
 public:
  // `name` and `hostname` identify the robot in Home Assistant.
  void begin(AsyncWebServer& server, auth::PinGuard& pin, DeviceApi& api, const String& name,
             const String& hostname);

 private:
  static void taskEntry(void* self);
  void run();
  void handleMessage(const char* topic, const uint8_t* payload, unsigned int length);
  bool connect();
  void publishState(bool force);

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

  String lastState_;
  uint32_t lastStateMs_ = 0;
};
