#include "device_api.h"

#include <AsyncJson.h>
#include <Preferences.h>
#include <WiFi.h>
#include <discovery/discovery.h>
#include <identity/identity.h>

#include "diagnostics_api.h"
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
    case cwc::Activity::TuningSpeed: return "tuning_speed";
    case cwc::Activity::Playing: return "playing";
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
    o["minSpeed"] = tenths(a.minSpeed);
    o["maxAccel"] = tenths(a.maxAccel);
    o["pauseMin"] = tenths(a.pauseMin);
    o["pauseMax"] = tenths(a.pauseMax);
  }
  out["sessionMinutes"] = settings.sessionMinutes;
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
    if (o["minSpeed"].is<float>()) a.minSpeed = o["minSpeed"];
    if (o["maxAccel"].is<float>()) a.maxAccel = o["maxAccel"];
    if (o["pauseMin"].is<float>()) a.pauseMin = o["pauseMin"];
    if (o["pauseMax"].is<float>()) a.pauseMax = o["pauseMax"];
  }
}

// The Session duration from a request body: false when absent, and
// `minutes` = 0 (out of range) when not a whole number of minutes.
bool readSessionMinutes(JsonObjectConst in, uint8_t& minutes) {
  JsonVariantConst value = in["sessionMinutes"];
  if (value.isNull()) return false;
  minutes = value.is<int>() && value.as<int>() >= 0 && value.as<int>() <= 255 ? value.as<int>() : 0;
  return true;
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

// Home Assistant's Native integration names itself on every request; any
// other caller counts as the Device interface.
constexpr const char* kClientHeader = "X-CatWandConnect-Client";
constexpr const char* kNativeClient = "home_assistant";

play_journal::Source requestSource(AsyncWebServerRequest* request) {
  return request->hasHeader(kClientHeader) && request->header(kClientHeader) == kNativeClient
             ? play_journal::Source::HomeAssistantNative
             : play_journal::Source::DeviceInterface;
}

// A clock that moved by more than this is worth a log line.
constexpr int64_t kClockLogDriftMs = 60000;

void sendJson(AsyncWebServerRequest* request, int code, const JsonDocument& doc) {
  String body;
  serializeJson(doc, body);
  request->send(code, "application/json", body);
}

}  // namespace

void DeviceApi::begin(AsyncWebServer& server, const cwc::HardwareConfig& hardware, const String& name,
                      const String& hostname, bool legacyHostname) {
  hardware_ = hardware;
  name_ = name;
  hostname_ = hostname;
  legacyHostname_ = legacyHostname;
  channel_ = update_channel::loadChannel(storage_, CWC_VERSION);
  {
    std::lock_guard<std::mutex> guard(robotLock_);
    pushedJournalId_ = play_.journal.lastId();  // only new events go out on /ws
  }

  server.on("/api/journal", HTTP_GET, [this](AsyncWebServerRequest* request) { getJournal(request); });
  auto* time = new AsyncCallbackJsonWebHandler(
      "/api/time", [this](AsyncWebServerRequest* request, JsonVariant& body) { postTime(request, body); });
  time->setMethod(HTTP_POST);
  server.addHandler(time);

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

  // Registered before /api/update (ota.cpp), which matches only that path.
  auto* available = new AsyncCallbackJsonWebHandler(
      "/api/update/available",
      [this](AsyncWebServerRequest* request, JsonVariant& body) { postAvailableUpdate(request, body); });
  available->setMethod(HTTP_POST);
  server.addHandler(available);

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
      inbox_.stowSource = requestSource(request);
      inbox_.move = false;
    }
    request->send(202, "application/json", "{}");
  });

  auto* position = new AsyncCallbackJsonWebHandler(
      "/api/position", [this](AsyncWebServerRequest* request, JsonVariant& body) { postPosition(request, body); });
  position->setMethod(HTTP_POST);
  server.addHandler(position);

  server.on("/api/calibration", HTTP_GET, [this](AsyncWebServerRequest* request) { getCalibration(request); });
  auto* calibrate = new AsyncCallbackJsonWebHandler(
      "/api/calibration",
      [this](AsyncWebServerRequest* request, JsonVariant& body) { postCalibration(request, body); });
  calibrate->setMethod(HTTP_POST);
  server.addHandler(calibrate);

  server.on("/api/speed-tuning", HTTP_GET, [this](AsyncWebServerRequest* request) { getSpeedTuning(request); });
  auto* tune = new AsyncCallbackJsonWebHandler(
      "/api/speed-tuning",
      [this](AsyncWebServerRequest* request, JsonVariant& body) { postSpeedTuning(request, body); });
  tune->setMethod(HTTP_POST);
  server.addHandler(tune);

  server.on("/api/play", HTTP_GET, [this](AsyncWebServerRequest* request) { getPlay(request); });
  auto* play = new AsyncCallbackJsonWebHandler(
      "/api/play", [this](AsyncWebServerRequest* request, JsonVariant& body) { postPlay(request, body); });
  play->setMethod(HTTP_POST);
  server.addHandler(play);

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
    doc["channel"] = update_channel::channelName(channel_);
  }
  sendJson(request, 200, doc);
}

String DeviceApi::settingsJson() {
  JsonDocument doc;
  {
    std::lock_guard<std::mutex> guard(robotLock_);
    writeSettings(doc.to<JsonObject>(), robot_.settings());
    doc["channel"] = update_channel::channelName(channel_);
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
  // The speed slider and the Session duration are part of play; everything
  // else needs the PIN.
  JsonObjectConst in = body.as<JsonObjectConst>();
  size_t playFields = in["playSpeed"].is<float>() + !in["sessionMinutes"].isNull();
  bool onlyPlay = in.size() > 0 && in.size() == playFields;
  if (!onlyPlay && !requirePin(pin_, request)) return;

  JsonDocument doc;
  cwc::SettingsCheck check;
  bool badChannel = false;
  {
    std::lock_guard<std::mutex> guard(robotLock_);
    uint8_t minutes = 0;
    bool hasMinutes = readSessionMinutes(in, minutes);
    // The Update channel is the robot's, not the Robot core's: saved apart.
    bool hasChannel = !in["channel"].isNull();
    update_channel::Channel channel = channel_;
    if (hasChannel && !update_channel::parseChannel(in["channel"] | "", channel)) badChannel = true;
    if (!badChannel && hasMinutes && (minutes < cwc::kMinSessionMinutes || minutes > cwc::kMaxSessionMinutes)) {
      check = {cwc::SettingsError::SessionDurationOutOfRange, cwc::Axis::Horizontal};
    }
    size_t others = (hasMinutes ? 1u : 0u) + (hasChannel ? 1u : 0u);
    if (!badChannel && check.ok() && in.size() > others) {
      cwc::RobotSettings settings = robot_.settings();
      readSettings(in, settings);
      check = robot_.applySettings(settings);
    }
    // Saved once the robot is idle, and only if it changed.
    if (!badChannel && check.ok() && hasMinutes) robot_.setSessionMinutes(minutes);
    if (!badChannel && check.ok() && hasChannel && channel != channel_) {
      update_channel::saveChannel(storage_, channel);
      channel_ = channel;
      logLine("[update] Update channel: %s", update_channel::channelName(channel));
    }
    if (!badChannel && check.ok()) {
      writeSettings(doc.to<JsonObject>(), robot_.settings());
      doc["channel"] = update_channel::channelName(channel_);
    }
  }

  if (badChannel) {
    request->send(400, "application/json", "{\"error\":\"invalid_channel\"}");
    return;
  }
  if (!check.ok()) {
    doc["error"] = cwc::settingsErrorName(check.error);
    if (check.error != cwc::SettingsError::PlaySpeedOutOfRange &&
        check.error != cwc::SettingsError::SessionDurationOutOfRange) {
      doc["axis"] = axisName(check.axis);
    }
    sendJson(request, 400, doc);
    return;
  }
  sendJson(request, 200, doc);
  newClient_ = true;  // push the new speed and duration to every open interface
}

void DeviceApi::writePlay(JsonObject out) {
  bool playing = robot_.activity() == cwc::Activity::Playing;
  out["active"] = playing;
  out["remaining"] = robot_.playRemainingS();
  out["minutes"] = robot_.settings().sessionMinutes;
}

void DeviceApi::getPlay(AsyncWebServerRequest* request) {
  JsonDocument doc;
  {
    std::lock_guard<std::mutex> guard(robotLock_);
    writePlay(doc.to<JsonObject>());
  }
  sendJson(request, 200, doc);
}

// No PIN: starting and stopping a Play session is play, like the joystick.
void DeviceApi::postPlay(AsyncWebServerRequest* request, JsonVariant& body) {
  const char* action = body["action"] | "";
  const char* error = nullptr;
  JsonDocument doc;
  {
    std::lock_guard<std::mutex> guard(robotLock_);
    play_.setSource(requestSource(request));
    if (strcmp(action, "start") == 0) {
      uint8_t minutes = robot_.settings().sessionMinutes;
      if (readSessionMinutes(body.as<JsonObjectConst>(), minutes) &&
          (minutes < cwc::kMinSessionMinutes || minutes > cwc::kMaxSessionMinutes)) {
        error = "session_duration_out_of_range";
      } else if (!robot_.startPlaySession(minutes)) {
        // The Robot core journals the refusal and its reason.
        error = robot_.settings().calibrated ? "busy" : "not_calibrated";
      }
    } else if (strcmp(action, "stop") == 0) {
      robot_.stopPlaySession();
    } else {
      error = "invalid_request";
    }
    if (!error) writePlay(doc.to<JsonObject>());
  }

  if (error) {
    doc["error"] = error;
    sendJson(request, strcmp(error, "not_calibrated") == 0 || strcmp(error, "busy") == 0 ? 409 : 400, doc);
    return;
  }
  sendJson(request, 200, doc);
  newClient_ = true;  // push the new activity to every open interface
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

void DeviceApi::writeSpeedTuning(JsonObject out) {
  bool tuning = robot_.activity() == cwc::Activity::TuningSpeed;
  out["active"] = tuning;
  if (!tuning) return;
  out["axis"] = axisName(robot_.tuningAxis());
  bool accel = robot_.tuningAccel();
  out["target"] = accel ? "accel" : "speed";
  out["bound"] = robot_.tuningBound() == cwc::SpeedBound::Maximum ? "max" : "min";
  out["speed"] = tenths(robot_.tuningSpeed());
  if (accel) out["accel"] = tenths(robot_.tuningAccelValue());
}

void DeviceApi::getSpeedTuning(AsyncWebServerRequest* request) {
  JsonDocument doc;
  {
    std::lock_guard<std::mutex> guard(robotLock_);
    writeSpeedTuning(doc.to<JsonObject>());
  }
  sendJson(request, 200, doc);
}

void DeviceApi::postSpeedTuning(AsyncWebServerRequest* request, JsonVariant& body) {
  if (!requirePin(pin_, request)) return;
  const char* action = body["action"] | "";
  JsonDocument doc;
  cwc::SettingsCheck check;
  bool known = true;
  {
    std::lock_guard<std::mutex> guard(robotLock_);
    play_.setSource(requestSource(request));  // starting one stops a Play session
    if (strcmp(action, "start") == 0) {
      cwc::Axis axis;
      const char* target = body["target"] | "speed";
      const char* bound = body["bound"] | "";
      bool max = strcmp(bound, "max") == 0;
      if (strcmp(target, "accel") == 0) {
        known = parseAxis(body["axis"] | "", axis) && robot_.startAccelTuning(axis);
      } else {
        known = strcmp(target, "speed") == 0 && parseAxis(body["axis"] | "", axis) &&
                (max || strcmp(bound, "min") == 0) &&
                robot_.startSpeedTuning(axis, max ? cwc::SpeedBound::Maximum : cwc::SpeedBound::Minimum);
      }
    } else if (strcmp(action, "faster") == 0) {
      robot_.adjustSpeedTuning(true);
    } else if (strcmp(action, "slower") == 0) {
      robot_.adjustSpeedTuning(false);
    } else if (strcmp(action, "keep") == 0) {
      check = robot_.keepSpeedTuning();
    } else if (strcmp(action, "cancel") == 0) {
      robot_.cancelSpeedTuning();
    } else {
      known = false;
    }
    if (known && check.ok()) writeSpeedTuning(doc.to<JsonObject>());
  }

  if (!known) {
    request->send(400, "application/json", "{\"error\":\"invalid_request\"}");
  } else if (!check.ok()) {
    doc["error"] = cwc::settingsErrorName(check.error);
    doc["axis"] = axisName(check.axis);
    sendJson(request, 400, doc);
  } else {
    sendJson(request, 200, doc);
    newClient_ = true;  // push the new activity and trial speed to every open interface
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
    play_.setSource(requestSource(request));  // starting it stops a Play session
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
    } else if (strcmp(action, "reverse") == 0) {
      cwc::Axis axis;
      if (parseAxis(body["axis"] | "", axis) && robot_.reverseAxis(axis)) {
        // The servo driver already flipped; save the wiring to match.
        cwc::ServoHardware& servo = hardware_.axis(axis);
        servo.inverted = !servo.inverted;
        cwc::saveHardware(storage_, hardware_, chip_);
      } else {
        known = false;
      }
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

// Manual control by position, like the MQTT position command. No PIN: moving
// the wand is play, like the joystick.
void DeviceApi::postPosition(AsyncWebServerRequest* request, JsonVariant& body) {
  const char* axisName = body["axis"] | "";
  cwc::Axis axis;
  if (strcmp(axisName, "horizontal") == 0) {
    axis = cwc::Axis::Horizontal;
  } else if (strcmp(axisName, "vertical") == 0) {
    axis = cwc::Axis::Vertical;
  } else {
    request->send(400, "application/json", "{\"error\":\"invalid_request\"}");
    return;
  }
  if (!body["degrees"].is<float>()) {
    request->send(400, "application/json", "{\"error\":\"invalid_request\"}");
    return;
  }
  // Into the servo range first, so any number ends up at the nearest Travel
  // limit rather than being ignored by the Robot core.
  float degrees = cwc::kServoRange.clamp(body["degrees"].as<float>());
  {
    std::lock_guard<std::mutex> guard(robotLock_);
    play_.setSource(requestSource(request));
    robot_.manualGoto(axis, degrees);
  }
  request->send(202, "application/json", "{}");
  newClient_ = true;  // push the new activity to every open interface
}

// --- Play session journal and Robot clock --------------------------------------

void DeviceApi::getJournal(AsyncWebServerRequest* request) {
  std::string body;
  {
    std::lock_guard<std::mutex> guard(robotLock_);
    body = play_.journal.json();
  }
  request->send(200, "application/json", body.c_str());
}

// No PIN: the Device interface sends the browser's time on every page load,
// the Native integration Home Assistant's when it connects (docs/adr/0013).
void DeviceApi::postTime(AsyncWebServerRequest* request, JsonVariant& body) {
  JsonVariantConst epoch = body["epochMs"];
  JsonVariantConst offset = body["offset"];
  JsonVariantConst zone = body["zone"];
  if (!epoch.is<double>() || !offset.is<int>() || (!zone.isNull() && !zone.is<const char*>())) {
    request->send(400, "application/json", "{\"error\":\"invalid_request\"}");
    return;
  }
  play_journal::Source source = requestSource(request);
  bool ok;
  bool wasSet;
  int64_t driftMs = 0;
  int today;
  {
    std::lock_guard<std::mutex> guard(robotLock_);
    wasSet = play_.clock.isSet();
    int64_t before = play_.clock.nowMs();
    ok = play_.clock.set(static_cast<int64_t>(epoch.as<double>()), offset.as<int>(),
                         zone.is<const char*>() ? zone.as<const char*>() : "");
    if (ok) {
      driftMs = play_.clock.nowMs() - before;
      play_.journal.clockSet();
    }
    today = play_.journal.sessionsToday();
  }
  if (!ok) {
    request->send(400, "application/json", "{\"error\":\"invalid_time\"}");
    return;
  }
  if (!wasSet || driftMs > kClockLogDriftMs || driftMs < -kClockLogDriftMs) {
    logLine("[clock] %s by %s, UTC%+d min", wasSet ? "corrected" : "set", play_journal::sourceName(source),
            offset.as<int>());
  }
  JsonDocument doc;
  doc["set"] = true;
  doc["sessionsToday"] = today;
  sendJson(request, 200, doc);
  newClient_ = true;  // Sessions today may have become known
}

uint32_t DeviceApi::journalLastId() {
  std::lock_guard<std::mutex> guard(robotLock_);
  return play_.journal.lastId();
}

std::vector<DeviceApi::JournalEvent> DeviceApi::journalSince(uint32_t id) {
  std::vector<JournalEvent> events;
  std::lock_guard<std::mutex> guard(robotLock_);
  for (size_t i = play_.journal.size(); i-- > 0;) {  // oldest first
    const play_journal::Event& e = play_.journal.newest(i);
    if (e.id <= id) continue;
    events.push_back({e.id, play_journal::whatName(e.what), play_journal::sourceName(e.source),
                      play_journal::reasonName(e.reason)});
  }
  return events;
}

int DeviceApi::sessionsToday() {
  std::lock_guard<std::mutex> guard(robotLock_);
  return play_.journal.sessionsToday();
}

String DeviceApi::journalReport() {
  std::lock_guard<std::mutex> guard(robotLock_);
  return play_.journal.reportText().c_str();
}

// Sends the journal's new events to every /ws client, oldest first.
void DeviceApi::pushJournal() {
  std::vector<std::string> messages;
  {
    std::lock_guard<std::mutex> guard(robotLock_);
    uint32_t last = play_.journal.lastId();
    if (last == pushedJournalId_) return;
    for (size_t i = play_.journal.size(); i-- > 0;) {
      const play_journal::Event& e = play_.journal.newest(i);
      if (e.id > pushedJournalId_) messages.push_back(play_.journal.eventJson(e));
    }
    pushedJournalId_ = last;
  }
  if (socket_.count() == 0) return;
  for (const std::string& message : messages) socket_.textAll(message.c_str());
  newClient_ = true;  // and the new Sessions today with the state
}

// --- Identity and info ------------------------------------------------------

void DeviceApi::getInfo(AsyncWebServerRequest* request) {
  JsonDocument doc;
  doc["name"] = name_;
  doc["hostname"] = hostname_ + ".local";
  // What it would answer to with the default name, for the rename preview (#195).
  doc["defaultHostname"] =
      String(identity::hostnameFor(identity::kDefaultName, deviceId().c_str(), legacyHostname_).c_str()) + ".local";
  doc["version"] = CWC_VERSION;
  doc["api"] = discovery::kApiVersion;
  doc["id"] = deviceId();
  {
    std::lock_guard<std::mutex> guard(robotLock_);
    doc["channel"] = update_channel::channelName(channel_);
  }
  doc["chipFamily"] = chipName(chip_);  // the key into the Update feed's `ota`
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
  // The address it will answer to after the restart.
  doc["hostname"] = String(identity::hostnameFor(name, deviceId().c_str(), legacyHostname_).c_str()) + ".local";
  doc["restart"] = true;
  sendJson(request, 200, doc);
  restartAt_ = millis() + kRestartDelayMs;  // mDNS, DHCP and the hotspot pick up the name at boot
}

// No PIN: it only changes what Home Assistant displays; the robot never
// downloads anything itself (docs/adr/0012).
void DeviceApi::postAvailableUpdate(AsyncWebServerRequest* request, JsonVariant& body) {
  JsonVariantConst version = body["version"];
  JsonVariantConst notes = body["notes"];
  if (!version.is<const char*>() || (!notes.isNull() && !notes.is<const char*>()) ||
      strlen(version.as<const char*>()) > 32 || (notes.is<const char*>() && strlen(notes.as<const char*>()) > 200)) {
    request->send(400, "application/json", "{\"error\":\"invalid_request\"}");
    return;
  }
  AvailableUpdate next{version.as<const char*>(), notes.is<const char*>() ? notes.as<const char*>() : ""};
  if (next.version.isEmpty()) next.notes = "";
  {
    std::lock_guard<std::mutex> guard(availableLock_);
    if (next.version != available_.version || next.notes != available_.notes) {
      available_ = next;
      ++availableGeneration_;
    }
  }
  JsonDocument doc;
  doc["version"] = next.version;
  doc["notes"] = next.notes;
  sendJson(request, 200, doc);
}

DeviceApi::AvailableUpdate DeviceApi::availableUpdate() {
  std::lock_guard<std::mutex> guard(availableLock_);
  return available_;
}

void DeviceApi::stopForUpdate() {
  std::lock_guard<std::mutex> guard(inboxLock_);
  inbox_.stop = true;
  inbox_.stopPlay = true;
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

void DeviceApi::onEvent(AsyncWebSocketClient* client, AwsEventType type, void* arg, uint8_t* data, size_t len) {
  switch (type) {
    case WS_EVT_CONNECT:
      newClient_ = true;  // make loop() send the state to the newcomer
      break;

    case WS_EVT_DISCONNECT: {
      // Dead-man rule: a vanished controller must not leave the wand moving.
      std::lock_guard<std::mutex> guard(inboxLock_);
      inbox_.stop = true;
      inbox_.move = false;
      for (size_t i = 0; i < nativeClients_.size(); ++i) {
        if (nativeClients_[i] == client->id()) {
          nativeClients_.erase(nativeClients_.begin() + i);
          break;
        }
      }
      break;
    }

    case WS_EVT_DATA: {
      auto* info = static_cast<AwsFrameInfo*>(arg);
      // Commands are small; only accept complete single-frame text messages.
      if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
        onMessage(client->id(), reinterpret_cast<const char*>(data), len);
      }
      break;
    }

    default:
      break;
  }
}

play_journal::Source DeviceApi::sourceOf(uint32_t client) {
  for (uint32_t id : nativeClients_) {
    if (id == client) return play_journal::Source::HomeAssistantNative;
  }
  return play_journal::Source::DeviceInterface;
}

void DeviceApi::onMessage(uint32_t client, const char* text, size_t len) {
  JsonDocument doc;
  if (deserializeJson(doc, text, len)) return;
  const char* type = doc["t"] | "";

  std::lock_guard<std::mutex> guard(inboxLock_);
  if (strcmp(type, "hello") == 0) {
    // The Native integration says who it is, for the Play session journal.
    bool native = strcmp(doc["client"] | "", kNativeClient) == 0;
    if (native && sourceOf(client) != play_journal::Source::HomeAssistantNative && nativeClients_.size() < 8) {
      nativeClients_.push_back(client);
    }
  } else if (strcmp(type, "move") == 0) {
    inbox_.move = true;
    inbox_.moveSource = sourceOf(client);
    inbox_.horizontal = doc["h"] | 0.0f;
    inbox_.vertical = doc["v"] | 0.0f;
  } else if (strcmp(type, "stop") == 0) {
    inbox_.stop = true;
    inbox_.move = false;
  } else if (strcmp(type, "stow") == 0) {
    inbox_.stow = true;
    inbox_.stowSource = sourceOf(client);
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
    play_.setSource(play_journal::Source::DeviceInterface);  // a firmware upload
    if (inbox.stopPlay) robot_.stopPlaySession();
    play_.setSource(inbox.stowSource);
    if (inbox.stow) robot_.stow();
    play_.setSource(inbox.moveSource);
    if (inbox.move) robot_.manualMove(inbox.horizontal, inbox.vertical);
    play_.clock.uptimeMs();  // keeps the Robot clock past the millis() wrap
    robot_.tick();
  }
  pushJournal();

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
    doc["sessionMinutes"] = robot_.settings().sessionMinutes;
    if (robot_.activity() == cwc::Activity::TuningSpeed) writeSpeedTuning(doc["tuning"].to<JsonObject>());
    if (robot_.activity() == cwc::Activity::Playing) doc["play"]["remaining"] = robot_.playRemainingS();
    int today = play_.journal.sessionsToday();
    if (today >= 0) doc["sessionsToday"] = today;
    else doc["sessionsToday"] = nullptr;
  }
  doc["name"] = name_;
  String state;
  serializeJson(doc, state);

  if (!force && state == lastState_) return;
  socket_.textAll(state);
  lastState_ = state;
  lastSentMs_ = millis();
}
