#include "device_api.h"

#include <AsyncJson.h>
#include <Preferences.h>
#include <WiFi.h>
#include <identity/identity.h>

#include "pin_http.h"
#include "log.h"

namespace {

constexpr uint32_t kBroadcastIntervalMs = 100;
// Sent even when nothing changed, so clients can tell the robot is alive.
constexpr uint32_t kHeartbeatMs = 2000;
constexpr uint32_t kCleanupIntervalMs = 1000;

const char* activityName(cwc::Activity activity) {
  switch (activity) {
    case cwc::Activity::Manual: return "manual";
    case cwc::Activity::Stowing: return "stowing";
    case cwc::Activity::Calibrating: return "calibrating";
    case cwc::Activity::Idle: return "idle";
  }
  return "idle";
}

const char* stepName(cwc::CalibrationStep step) {
  switch (step) {
    case cwc::CalibrationStep::None: return "none";
    case cwc::CalibrationStep::HorizontalMin: return "horizontal_min";
    case cwc::CalibrationStep::HorizontalMax: return "horizontal_max";
    case cwc::CalibrationStep::VerticalMin: return "vertical_min";
    case cwc::CalibrationStep::VerticalMax: return "vertical_max";
    case cwc::CalibrationStep::Stow: return "stow";
    case cwc::CalibrationStep::Sweep: return "sweep";
    case cwc::CalibrationStep::Confirm: return "confirm";
  }
  return "none";
}

bool parseAxis(const char* name, cwc::Axis& axis) {
  if (strcmp(name, "horizontal") == 0) axis = cwc::Axis::Horizontal;
  else if (strcmp(name, "vertical") == 0) axis = cwc::Axis::Vertical;
  else return false;
  return true;
}

const char* axisName(cwc::Axis axis) { return axis == cwc::Axis::Horizontal ? "horizontal" : "vertical"; }

// Rounds for display so the JSON stays short and stable.
float tenths(float value) { return roundf(value * 10) / 10; }

void writeSettings(JsonObject out, const cwc::RobotSettings& settings) {
  out["playSpeed"] = tenths(settings.playSpeed * 100) / 100;
  out["calibrated"] = settings.calibrated;
  JsonObject axes = out["axes"].to<JsonObject>();
  for (cwc::Axis axis : cwc::kAxes) {
    const cwc::AxisSettings& a = settings.axis(axis);
    JsonObject o = axes[axisName(axis)].to<JsonObject>();
    o["min"] = tenths(a.travel.min);
    o["max"] = tenths(a.travel.max);
    o["stow"] = tenths(a.stow);
    o["maxSpeed"] = tenths(a.maxSpeed);
    o["maxAccel"] = tenths(a.maxAccel);
  }
}

// Overlays the fields present in `in` onto `settings`. `calibrated` is owned
// by Calibration and is not writable here.
void readSettings(JsonObjectConst in, cwc::RobotSettings& settings) {
  if (in["playSpeed"].is<float>()) settings.playSpeed = in["playSpeed"];
  JsonObjectConst axes = in["axes"];
  for (cwc::Axis axis : cwc::kAxes) {
    JsonObjectConst o = axes[axisName(axis)];
    if (o.isNull()) continue;
    cwc::AxisSettings& a = settings.axis(axis);
    if (o["min"].is<float>()) a.travel.min = o["min"];
    if (o["max"].is<float>()) a.travel.max = o["max"];
    if (o["stow"].is<float>()) a.stow = o["stow"];
    if (o["maxSpeed"].is<float>()) a.maxSpeed = o["maxSpeed"];
    if (o["maxAccel"].is<float>()) a.maxAccel = o["maxAccel"];
  }
}

const char* chipName(cwc::Chip chip) {
  switch (chip) {
    case cwc::Chip::Esp32S3: return "ESP32-S3";
    case cwc::Chip::Esp32C3: return "ESP32-C3";
    case cwc::Chip::Esp32: break;
  }
  return "ESP32";
}

// Lets the HTTP response reach the browser before the board restarts.
constexpr uint32_t kRestartDelayMs = 800;

void sendJson(AsyncWebServerRequest* request, int code, const JsonDocument& doc) {
  String body;
  serializeJson(doc, body);
  request->send(code, "application/json", body);
}

}  // namespace

void DeviceApi::begin(AsyncWebServer& server, const cwc::HardwareConfig& hardware, const String& name) {
  hardware_ = hardware;
  name_ = name;

  server.on("/api/auth", HTTP_GET, [this](AsyncWebServerRequest* request) { getAuth(request); });
  auto* unlockHandler = new AsyncCallbackJsonWebHandler(
      "/api/auth/unlock", [this](AsyncWebServerRequest* request, JsonVariant& body) { unlock(request, body); });
  unlockHandler->setMethod(HTTP_POST);
  server.addHandler(unlockHandler);
  auto* pinHandler = new AsyncCallbackJsonWebHandler(
      "/api/auth/pin", [this](AsyncWebServerRequest* request, JsonVariant& body) { putPin(request, body); });
  pinHandler->setMethod(HTTP_PUT);
  server.addHandler(pinHandler);

  server.on("/api/info", HTTP_GET, [this](AsyncWebServerRequest* request) { getInfo(request); });
  auto* device = new AsyncCallbackJsonWebHandler(
      "/api/device", [this](AsyncWebServerRequest* request, JsonVariant& body) { putDevice(request, body); });
  device->setMethod(HTTP_PUT);
  server.addHandler(device);

  server.on("/api/hardware", HTTP_GET, [this](AsyncWebServerRequest* request) { getHardware(request); });
  auto* hw = new AsyncCallbackJsonWebHandler(
      "/api/hardware", [this](AsyncWebServerRequest* request, JsonVariant& body) { putHardware(request, body); });
  hw->setMethod(HTTP_PUT);
  server.addHandler(hw);

  socket_.onEvent([this](AsyncWebSocket*, AsyncWebSocketClient* client, AwsEventType type, void* arg,
                         uint8_t* data, size_t len) { onEvent(client, type, arg, data, len); });
  server.addHandler(&socket_);

  server.on("/api/stow", HTTP_POST, [this](AsyncWebServerRequest* request) {
    {
      std::lock_guard<std::mutex> guard(inboxLock_);
      inbox_.stow = true;
      inbox_.move = false;
    }
    request->send(202, "application/json", "{}");
  });

  server.on("/api/calibration", HTTP_GET, [this](AsyncWebServerRequest* request) { getCalibration(request); });
  auto* calibrate = new AsyncCallbackJsonWebHandler(
      "/api/calibration",
      [this](AsyncWebServerRequest* request, JsonVariant& body) { postCalibration(request, body); });
  calibrate->setMethod(HTTP_POST);
  server.addHandler(calibrate);

  server.on("/api/settings", HTTP_GET, [this](AsyncWebServerRequest* request) { getSettings(request); });
  auto* put = new AsyncCallbackJsonWebHandler(
      "/api/settings", [this](AsyncWebServerRequest* request, JsonVariant& body) { putSettings(request, body); });
  put->setMethod(HTTP_PUT);
  server.addHandler(put);
}

// --- REST -------------------------------------------------------------------

void DeviceApi::getSettings(AsyncWebServerRequest* request) {
  JsonDocument doc;
  {
    std::lock_guard<std::mutex> guard(robotLock_);
    writeSettings(doc.to<JsonObject>(), robot_.settings());
  }
  sendJson(request, 200, doc);
}

String DeviceApi::settingsJson() {
  JsonDocument doc;
  {
    std::lock_guard<std::mutex> guard(robotLock_);
    writeSettings(doc.to<JsonObject>(), robot_.settings());
  }
  String out;
  serializeJson(doc, out);
  return out;
}

String DeviceApi::hardwareJson() {
  JsonDocument doc;
  {
    std::lock_guard<std::mutex> guard(robotLock_);
    writeHardware(doc.to<JsonObject>(), hardware_);
  }
  String out;
  serializeJson(doc, out);
  return out;
}

void DeviceApi::putSettings(AsyncWebServerRequest* request, JsonVariant& body) {
  if (!body.is<JsonObject>()) {
    request->send(400, "application/json", "{\"error\":\"invalid_json\"}");
    return;
  }
  // The speed slider is part of play; everything else needs the PIN.
  bool onlyPlaySpeed = body.size() == 1 && body["playSpeed"].is<float>();
  if (!onlyPlaySpeed && !requirePin(pin_, request)) return;

  JsonDocument doc;
  cwc::SettingsCheck check;
  {
    std::lock_guard<std::mutex> guard(robotLock_);
    cwc::RobotSettings settings = robot_.settings();
    readSettings(body.as<JsonObjectConst>(), settings);
    check = robot_.applySettings(settings);
    if (check.ok()) writeSettings(doc.to<JsonObject>(), robot_.settings());
  }

  if (!check.ok()) {
    doc["error"] = cwc::settingsErrorName(check.error);
    if (check.error != cwc::SettingsError::PlaySpeedOutOfRange) doc["axis"] = axisName(check.axis);
    sendJson(request, 400, doc);
    return;
  }
  sendJson(request, 200, doc);
  newClient_ = true;  // push the new speed to every open interface
}

void DeviceApi::writeCalibration(JsonObject out) {
  out["step"] = stepName(robot_.calibrationStep());
  JsonObject draft = out["draft"].to<JsonObject>();
  for (cwc::Axis axis : cwc::kAxes) {
    const cwc::AxisSettings& a = robot_.calibrationDraft().axis(axis);
    JsonObject o = draft[axisName(axis)].to<JsonObject>();
    o["min"] = tenths(a.travel.min);
    o["max"] = tenths(a.travel.max);
    o["stow"] = tenths(a.stow);
  }
}

void DeviceApi::getCalibration(AsyncWebServerRequest* request) {
  JsonDocument doc;
  {
    std::lock_guard<std::mutex> guard(robotLock_);
    writeCalibration(doc.to<JsonObject>());
  }
  sendJson(request, 200, doc);
}

void DeviceApi::postCalibration(AsyncWebServerRequest* request, JsonVariant& body) {
  if (!requirePin(pin_, request)) return;
  const char* action = body["action"] | "";
  JsonDocument doc;
  cwc::SettingsCheck check;
  bool known = true;
  {
    std::lock_guard<std::mutex> guard(robotLock_);
    if (strcmp(action, "start") == 0) {
      robot_.startCalibration();
    } else if (strcmp(action, "nudge") == 0) {
      cwc::Axis axis;
      if (parseAxis(body["axis"] | "", axis)) robot_.nudge(axis, body["degrees"] | 0.0f);
      else known = false;
    } else if (strcmp(action, "mark") == 0) {
      check = robot_.markCalibration();
    } else if (strcmp(action, "confirm") == 0) {
      robot_.confirmCalibration();
    } else if (strcmp(action, "cancel") == 0) {
      robot_.cancelCalibration();
    } else {
      known = false;
    }
    if (known && check.ok()) writeCalibration(doc.to<JsonObject>());
  }

  if (!known) {
    request->send(400, "application/json", "{\"error\":\"invalid_request\"}");
  } else if (!check.ok()) {
    doc["error"] = cwc::settingsErrorName(check.error);
    doc["axis"] = axisName(check.axis);
    sendJson(request, 400, doc);
  } else {
    sendJson(request, 200, doc);
    newClient_ = true;
  }
}

void DeviceApi::writeHardware(JsonObject out, const cwc::HardwareConfig& hardware) {
  out["chip"] = chipName(chip_);
  JsonArray pins = out["allowedPins"].to<JsonArray>();
  cwc::PinList list = cwc::allowedPins(chip_);
  for (size_t i = 0; i < list.count; ++i) pins.add(list.pins[i]);
  JsonObject axes = out["axes"].to<JsonObject>();
  for (cwc::Axis axis : cwc::kAxes) {
    const cwc::ServoHardware& servo = hardware.axis(axis);
    JsonObject o = axes[axisName(axis)].to<JsonObject>();
    o["pin"] = servo.pin;
    o["inverted"] = servo.inverted;
    o["minPulse"] = servo.minPulseUs;
    o["maxPulse"] = servo.maxPulseUs;
  }
}

void DeviceApi::getHardware(AsyncWebServerRequest* request) {
  JsonDocument doc;
  {
    std::lock_guard<std::mutex> guard(robotLock_);
    writeHardware(doc.to<JsonObject>(), hardware_);
  }
  sendJson(request, 200, doc);
}

void DeviceApi::putHardware(AsyncWebServerRequest* request, JsonVariant& body) {
  if (!requirePin(pin_, request)) return;
  if (!body.is<JsonObject>()) {
    request->send(400, "application/json", "{\"error\":\"invalid_json\"}");
    return;
  }

  JsonDocument doc;
  cwc::HardwareCheck check;
  {
    std::lock_guard<std::mutex> guard(robotLock_);
    cwc::HardwareConfig next = hardware_;
    JsonObjectConst axes = body["axes"];
    for (cwc::Axis axis : cwc::kAxes) {
      JsonObjectConst o = axes[axisName(axis)];
      if (o.isNull()) continue;
      cwc::ServoHardware& servo = next.axis(axis);
      if (o["pin"].is<uint8_t>()) servo.pin = o["pin"];
      if (o["inverted"].is<bool>()) servo.inverted = o["inverted"];
      if (o["minPulse"].is<uint16_t>()) servo.minPulseUs = o["minPulse"];
      if (o["maxPulse"].is<uint16_t>()) servo.maxPulseUs = o["maxPulse"];
    }
    check = cwc::validateHardware(next, chip_);
    if (check.ok()) {
      bool recalibrate = cwc::invalidatesCalibration(hardware_, next);
      cwc::saveHardware(storage_, next, chip_);
      if (recalibrate) robot_.invalidateCalibration();
      robot_.manualStop();
      hardware_ = next;
      writeHardware(doc.to<JsonObject>(), hardware_);
      doc["restart"] = true;
      doc["calibrationReset"] = recalibrate;
    }
  }

  if (!check.ok()) {
    doc["error"] = cwc::hardwareErrorName(check.error);
    doc["axis"] = axisName(check.axis);
    sendJson(request, 400, doc);
    return;
  }
  sendJson(request, 200, doc);
  restartAt_ = millis() + kRestartDelayMs;
}

// --- Identity and info ------------------------------------------------------

void DeviceApi::getInfo(AsyncWebServerRequest* request) {
  JsonDocument doc;
  doc["name"] = name_;
  doc["hostname"] = String(identity::hostnameFor(name_.c_str()).c_str()) + ".local";
  doc["version"] = CWC_VERSION;
  doc["chip"] = ESP.getChipModel();
  doc["chipRevision"] = ESP.getChipRevision();
  doc["cores"] = ESP.getChipCores();
  doc["cpuMhz"] = ESP.getCpuFreqMHz();
  doc["flashMb"] = ESP.getFlashChipSize() / (1024 * 1024);
  doc["freeHeap"] = ESP.getFreeHeap();
  doc["mac"] = WiFi.macAddress();
  doc["ssid"] = WiFi.status() == WL_CONNECTED ? WiFi.SSID() : "";
  doc["ip"] = WiFi.status() == WL_CONNECTED ? WiFi.localIP().toString() : "";
  doc["rssi"] = WiFi.status() == WL_CONNECTED ? WiFi.RSSI() : 0;
  doc["uptime"] = millis() / 1000;
  doc["temperature"] = tenths(temperatureRead());
  sendJson(request, 200, doc);
}

void DeviceApi::putDevice(AsyncWebServerRequest* request, JsonVariant& body) {
  if (!requirePin(pin_, request)) return;
  const char* name = body["name"] | "";
  if (!identity::validName(name)) {
    request->send(400, "application/json", "{\"error\":\"invalid_name\"}");
    return;
  }
  Preferences prefs;
  prefs.begin("device", false);
  prefs.putString("name", name);
  prefs.end();
  {
    std::lock_guard<std::mutex> guard(robotLock_);
    robot_.manualStop();
  }

  JsonDocument doc;
  doc["name"] = name;
  doc["hostname"] = String(identity::hostnameFor(name).c_str()) + ".local";
  doc["restart"] = true;
  sendJson(request, 200, doc);
  restartAt_ = millis() + kRestartDelayMs;  // mDNS, DHCP and the hotspot pick up the name at boot
}

void DeviceApi::stopForUpdate() {
  std::lock_guard<std::mutex> guard(inboxLock_);
  inbox_.stop = true;
  inbox_.move = false;
}

void DeviceApi::restartSoon() { restartAt_ = millis() + kRestartDelayMs; }

// --- Settings PIN -------------------------------------------------------------

void DeviceApi::getAuth(AsyncWebServerRequest* request) {
  std::string token;
  if (request->hasHeader("Authorization")) {
    String value = request->header("Authorization");
    if (value.startsWith("Bearer ")) token = value.substring(7).c_str();
  }
  JsonDocument doc;
  doc["enabled"] = pin_.enabled();
  doc["authorized"] = pin_.authorized(token, millis());
  doc["lockedFor"] = (pin_.lockedForMs(millis()) + 999) / 1000;
  sendJson(request, 200, doc);
}

void DeviceApi::unlock(AsyncWebServerRequest* request, JsonVariant& body) {
  std::string token;
  JsonDocument doc;
  switch (pin_.unlock(body["pin"] | "", millis(), token)) {
    case auth::PinGuard::Unlock::Ok:
      doc["token"] = token;
      sendJson(request, 200, doc);
      return;
    case auth::PinGuard::Unlock::Wrong:
      doc["error"] = "wrong_pin";
      break;
    case auth::PinGuard::Unlock::LockedOut:
      doc["error"] = "locked_out";
      break;
  }
  uint32_t locked = (pin_.lockedForMs(millis()) + 999) / 1000;
  if (locked > 0) doc["retryAfter"] = locked;
  sendJson(request, locked > 0 ? 429 : 401, doc);
}

void DeviceApi::putPin(AsyncWebServerRequest* request, JsonVariant& body) {
  if (!requirePin(pin_, request)) return;
  std::string pin = body["pin"] | "";
  if (!pin_.setPin(pin)) {
    request->send(400, "application/json", "{\"error\":\"invalid_pin\"}");
    return;
  }
  // Stored as is: a 4-8 digit PIN would not resist offline guessing even if
  // hashed, and reading the flash already takes the robot in hand.
  Preferences prefs;
  prefs.begin("auth", false);
  prefs.putString("pin", pin.c_str());
  prefs.end();

  JsonDocument doc;
  doc["enabled"] = pin_.enabled();
  if (pin_.enabled()) {
    std::string token;
    pin_.unlock(pin, millis(), token);  // keep the person who set it signed in
    doc["token"] = token;
  }
  sendJson(request, 200, doc);
}

// --- WebSocket --------------------------------------------------------------

void DeviceApi::onEvent(AsyncWebSocketClient*, AwsEventType type, void* arg, uint8_t* data, size_t len) {
  switch (type) {
    case WS_EVT_CONNECT:
      newClient_ = true;  // make loop() send the state to the newcomer
      break;

    case WS_EVT_DISCONNECT: {
      // Dead-man rule: a vanished controller must not leave the wand moving.
      std::lock_guard<std::mutex> guard(inboxLock_);
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

  std::lock_guard<std::mutex> guard(inboxLock_);
  if (strcmp(type, "move") == 0) {
    inbox_.move = true;
    inbox_.horizontal = doc["h"] | 0.0f;
    inbox_.vertical = doc["v"] | 0.0f;
  } else if (strcmp(type, "stop") == 0) {
    inbox_.stop = true;
    inbox_.move = false;
  } else if (strcmp(type, "stow") == 0) {
    inbox_.stow = true;
    inbox_.move = false;
  }
}

void DeviceApi::loop() {
  uint32_t restartAt = restartAt_;
  if (restartAt != 0 && static_cast<int32_t>(millis() - restartAt) >= 0) {
    logLine("Restarting to apply the new hardware configuration");
    ESP.restart();
  }

  Inbox inbox;
  {
    std::lock_guard<std::mutex> guard(inboxLock_);
    inbox = inbox_;
    inbox_ = Inbox{};
  }
  {
    std::lock_guard<std::mutex> guard(robotLock_);
    if (inbox.stop) robot_.manualStop();
    if (inbox.stow) robot_.stow();
    if (inbox.move) robot_.manualMove(inbox.horizontal, inbox.vertical);
    robot_.tick();
  }

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
  {
    std::lock_guard<std::mutex> guard(robotLock_);
    doc["h"] = tenths(robot_.position(cwc::Axis::Horizontal));
    doc["v"] = tenths(robot_.position(cwc::Axis::Vertical));
    doc["activity"] = activityName(robot_.activity());
    doc["held"] = robot_.held(cwc::Axis::Horizontal) || robot_.held(cwc::Axis::Vertical);
    doc["playSpeed"] = tenths(robot_.settings().playSpeed * 100) / 100;
    doc["calibrated"] = robot_.settings().calibrated;
    doc["calibration"] = stepName(robot_.calibrationStep());
  }
  doc["name"] = name_;
  String state;
  serializeJson(doc, state);

  if (!force && state == lastState_) return;
  socket_.textAll(state);
  lastState_ = state;
  lastSentMs_ = millis();
}
