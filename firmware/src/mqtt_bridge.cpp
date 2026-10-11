#include "mqtt_bridge.h"

#include <AsyncJson.h>
#include <PubSubClient.h>
#include <Preferences.h>
#include <WiFi.h>
#include <home_assistant/home_assistant.h>

#include "device_api.h"
#include "pin_http.h"

namespace {

constexpr uint32_t kRetryMs = 10000;
constexpr uint32_t kStateIntervalMs = 500;
constexpr uint32_t kStateRefreshMs = 60000;  // republish even when nothing changed

WiFiClient tcp;
PubSubClient client(tcp);
MqttBridge* instance = nullptr;

const char* activityName(cwc::Activity activity) {
  switch (activity) {
    case cwc::Activity::Manual: return "manual";
    case cwc::Activity::Stowing: return "stowing";
    case cwc::Activity::Calibrating: return "calibrating";
    case cwc::Activity::Idle: break;
  }
  return "idle";
}

MqttConfig loadConfig() {
  MqttConfig config;
  Preferences prefs;
  if (prefs.begin("mqtt", true)) {
    config.enabled = prefs.getBool("enabled", false);
    config.host = prefs.getString("host", "");
    config.port = prefs.getUShort("port", 1883);
    config.user = prefs.getString("user", "");
    config.password = prefs.getString("pass", "");
    prefs.end();
  }
  return config;
}

void saveConfig(const MqttConfig& config) {
  Preferences prefs;
  prefs.begin("mqtt", false);
  prefs.putBool("enabled", config.enabled);
  prefs.putString("host", config.host);
  prefs.putUShort("port", config.port);
  prefs.putString("user", config.user);
  prefs.putString("pass", config.password);
  prefs.end();
}

home_assistant::Device deviceInfo(const String& name, const String& hostname) {
  return {name.c_str(), hostname.c_str(), WiFi.macAddress().c_str(), CWC_VERSION,
          ("http://" + WiFi.localIP().toString() + "/").c_str()};
}

}  // namespace

void MqttBridge::begin(AsyncWebServer& server, auth::PinGuard& pin, DeviceApi& api, const String& name,
                       const String& hostname) {
  api_ = &api;
  pin_ = &pin;
  name_ = name;
  hostname_ = hostname;
  config_ = loadConfig();
  instance = this;

  server.on("/api/mqtt", HTTP_GET, [this](AsyncWebServerRequest* request) { getConfig(request); });
  auto* put = new AsyncCallbackJsonWebHandler(
      "/api/mqtt", [this](AsyncWebServerRequest* request, JsonVariant& body) { putConfig(request, body); });
  put->setMethod(HTTP_PUT);
  server.addHandler(put);

  client.setBufferSize(1024);  // discovery payloads are several hundred bytes
  client.setSocketTimeout(3);
  client.setCallback([](char* topic, uint8_t* payload, unsigned int length) {
    instance->handleMessage(topic, payload, length);
  });
  xTaskCreate(taskEntry, "mqtt", 8192, this, 1, nullptr);
}

void MqttBridge::taskEntry(void* self) { static_cast<MqttBridge*>(self)->run(); }

void MqttBridge::run() {
  uint32_t lastAttempt = 0;
  bool attempted = false;
  for (;;) {
    bool enabled;
    bool reconfigure;
    {
      std::lock_guard<std::mutex> guard(lock_);
      enabled = config_.enabled;
      reconfigure = reconfigure_;
      reconfigure_ = false;
    }
    if (reconfigure && client.connected()) {
      client.publish(home_assistant::Topics(deviceInfo(name_, hostname_)).availability().c_str(),
                     home_assistant::kOffline, true);
      client.disconnect();
    }
    if (reconfigure) attempted = false;

    if (!enabled || WiFi.status() != WL_CONNECTED) {
      if (client.connected()) client.disconnect();
      {
        std::lock_guard<std::mutex> guard(lock_);
        connected_ = false;
      }
      vTaskDelay(pdMS_TO_TICKS(500));
      continue;
    }

    if (!client.connected() && (!attempted || millis() - lastAttempt >= kRetryMs)) {
      attempted = true;
      lastAttempt = millis();
      bool ok = connect();
      std::lock_guard<std::mutex> guard(lock_);
      connected_ = ok;
    }
    if (client.connected()) {
      client.loop();
      publishState(false);
    } else {
      std::lock_guard<std::mutex> guard(lock_);
      connected_ = false;
    }
    vTaskDelay(pdMS_TO_TICKS(20));
  }
}

bool MqttBridge::connect() {
  MqttConfig config;
  {
    std::lock_guard<std::mutex> guard(lock_);
    config = config_;
  }
  home_assistant::Device device = deviceInfo(name_, hostname_);
  home_assistant::Topics topics(device);
  String clientId = "catwandconnect-" + WiFi.macAddress();
  clientId.replace(":", "");

  client.setServer(config.host.c_str(), config.port);
  const char* user = config.user.isEmpty() ? nullptr : config.user.c_str();
  const char* pass = config.user.isEmpty() ? nullptr : config.password.c_str();
  bool ok = client.connect(clientId.c_str(), user, pass, topics.availability().c_str(), 1, true,
                           home_assistant::kOffline);
  if (!ok) {
    int state = client.state();
    std::lock_guard<std::mutex> guard(lock_);
    // PubSubClient states: negative for network trouble, 4/5 for refused logins.
    error_ = state == MQTT_CONNECT_BAD_CREDENTIALS || state == MQTT_CONNECT_UNAUTHORIZED ? "bad_credentials"
             : state < 0                                                                ? "unreachable"
                                                                                        : "refused";
    Serial.printf("[mqtt] connection to %s:%u failed (state %d)\n", config.host.c_str(), config.port, state);
    return false;
  }

  for (const auto& message : home_assistant::discovery(device)) {
    client.publish(message.topic.c_str(), message.payload.c_str(), true);
  }
  client.subscribe(topics.commands().c_str());
  client.subscribe(topics.axisCommands().c_str());
  client.publish(topics.availability().c_str(), home_assistant::kOnline, true);
  publishState(true);
  {
    std::lock_guard<std::mutex> guard(lock_);
    error_ = "";
  }
  Serial.printf("[mqtt] connected to %s:%u\n", config.host.c_str(), config.port);
  return true;
}

void MqttBridge::publishState(bool force) {
  uint32_t now = millis();
  if (!force && now - lastStateMs_ < kStateIntervalMs) return;

  home_assistant::State state = api_->withRobot([](cwc::Robot& robot) {
    home_assistant::State s{};
    for (cwc::Axis axis : cwc::kAxes) {
      size_t i = static_cast<size_t>(axis);
      s.position[i] = robot.position(axis);
      s.maxSpeed[i] = robot.settings().axis(axis).maxSpeed;
    }
    s.activity = activityName(robot.activity());
    return s;
  });
  String payload = home_assistant::statePayload(state).c_str();
  bool stale = now - lastStateMs_ >= kStateRefreshMs;
  if (!force && payload == lastState_ && !stale) return;

  home_assistant::Topics topics(deviceInfo(name_, hostname_));
  client.publish(topics.state().c_str(), payload.c_str(), true);
  lastState_ = payload;
  lastStateMs_ = now;
}

void MqttBridge::handleMessage(const char* topic, const uint8_t* payload, unsigned int length) {
  home_assistant::Topics topics(deviceInfo(name_, hostname_));
  std::string text(reinterpret_cast<const char*>(payload), length);
  home_assistant::Command command = home_assistant::parseCommand(topics, topic, text);

  api_->withRobot([&](cwc::Robot& robot) {
    switch (command.kind) {
      case home_assistant::CommandKind::Stow:
        robot.stow();
        break;
      case home_assistant::CommandKind::Position:
        robot.manualGoto(command.axis, command.value);
        break;
      case home_assistant::CommandKind::MaxSpeed: {
        cwc::RobotSettings settings = robot.settings();
        settings.axis(command.axis).maxSpeed = command.value;
        if (!robot.applySettings(settings).ok()) Serial.printf("[mqtt] rejected maximum speed %s\n", text.c_str());
        break;
      }
      case home_assistant::CommandKind::None:
        break;
    }
    return 0;
  });
  publishState(true);
}

// --- REST: broker settings ----------------------------------------------------------

void MqttBridge::getConfig(AsyncWebServerRequest* request) {
  JsonDocument doc;
  {
    std::lock_guard<std::mutex> guard(lock_);
    doc["enabled"] = config_.enabled;
    doc["host"] = config_.host;
    doc["port"] = config_.port;
    doc["user"] = config_.user;
    doc["passwordSet"] = !config_.password.isEmpty();
    doc["connected"] = connected_;
    doc["error"] = error_;
  }
  doc["baseTopic"] = home_assistant::Topics(deviceInfo(name_, hostname_)).base;
  String body;
  serializeJson(doc, body);
  request->send(200, "application/json", body);
}

void MqttBridge::putConfig(AsyncWebServerRequest* request, JsonVariant& body) {
  if (!requirePin(*pin_, request)) return;
  MqttConfig next;
  {
    std::lock_guard<std::mutex> guard(lock_);
    next = config_;
  }
  if (body["enabled"].is<bool>()) next.enabled = body["enabled"];
  if (body["host"].is<const char*>()) {
    next.host = body["host"].as<const char*>();
    next.host.trim();
  }
  if (body["port"].is<int>()) next.port = body["port"].as<int>();
  if (body["user"].is<const char*>()) next.user = body["user"].as<const char*>();
  if (body["password"].is<const char*>()) next.password = body["password"].as<const char*>();  // omitted: kept

  if (next.enabled && next.host.isEmpty()) {
    request->send(400, "application/json", "{\"error\":\"mqtt_host_required\"}");
    return;
  }
  if (!body["port"].isNull() && (body["port"].as<int>() < 1 || body["port"].as<int>() > 65535)) {
    request->send(400, "application/json", "{\"error\":\"mqtt_port_invalid\"}");
    return;
  }
  saveConfig(next);
  {
    std::lock_guard<std::mutex> guard(lock_);
    config_ = next;
    reconfigure_ = true;
    error_ = "";
  }
  getConfig(request);
}
