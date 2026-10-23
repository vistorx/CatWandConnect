#pragma once

// The Device API (docs/api.md): WebSocket /ws for Manual control and live
// state, REST /api/settings for the Robot core's settings.
//
// HTTP and WebSocket events arrive on the network task while the Robot core
// ticks on the main loop, so every access to the robot holds robotLock_.
// Manual control commands go through an inbox drained by loop().
// The Play session journal (PlayLog) shares the robot lock: the Robot core
// reports to it from inside its calls, and each call first tells it the
// source of the command: Home Assistant's Native integration says who it is
// (header X-CatWandConnect-Client, /ws "hello"), anything else counts as the
// Device interface.

#include <ArduinoJson.h>
#include <auth/pin_guard.h>
#include <ESPAsyncWebServer.h>
#include <robot_core/hardware.h>
#include <robot_core/robot.h>
#include <update_channel/update_channel.h>

#include <atomic>
#include <mutex>
#include <vector>

#include "play_log.h"

class DeviceApi {
 public:
  DeviceApi(cwc::Robot& robot, cwc::Storage& storage, cwc::Chip chip, auth::PinGuard& pin, PlayLog& play)
      : robot_(robot), storage_(storage), chip_(chip), pin_(pin), play_(play) {}

  // `hardware` is the wiring the servo driver was started with; `name` the
  // robot's name, `hostname` the one it answers to (without ".local") and
  // `legacyHostname` whether it keeps plain "catwandconnect" (#195).
  void begin(AsyncWebServer& server, const cwc::HardwareConfig& hardware, const String& name,
             const String& hostname, bool legacyHostname);

  // Runs `f(robot)` under the robot lock, for other tasks such as MQTT.
  template <class F>
  auto withRobot(F&& f) {
    std::lock_guard<std::mutex> guard(robotLock_);
    return f(robot_);
  }

  // The same, for a command from `source`: Play session events it causes are
  // journaled with it.
  template <class F>
  auto withRobotFrom(play_journal::Source source, F&& f) {
    std::lock_guard<std::mutex> guard(robotLock_);
    play_.setSource(source);
    return f(robot_);
  }

  // A Play session journal event, with its names, for MQTT.
  struct JournalEvent {
    uint32_t id;
    const char* what;
    const char* source;
    const char* reason;  // nullptr: none
  };
  // The journal's newest event id (0: none) and the events after `id`, oldest first.
  uint32_t journalLastId();
  std::vector<JournalEvent> journalSince(uint32_t id);
  // Play sessions started today, -1 while the robot has no clock.
  int sessionsToday();
  // The journal's section of the Diagnostic report.
  String journalReport();

  // The settings and the saved wiring as GET /api/settings and /api/hardware
  // return them, for the Diagnostic report.
  String settingsJson();
  String hardwareJson();

  // The newest firmware the Device interface found in the Update feed
  // (POST /api/update/available), kept in RAM for the MQTT Firmware entity.
  struct AvailableUpdate {
    String version;  // empty when none is known
    String notes;
  };
  AvailableUpdate availableUpdate();
  // Goes up each time the available update changes, so MQTT republishes it.
  uint32_t availableUpdateGeneration() const { return availableGeneration_; }

  // Stops Manual control and any Play session before a firmware update;
  // safe from any task.
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
    bool stopPlay = false;
    play_journal::Source moveSource = play_journal::Source::DeviceInterface;
    play_journal::Source stowSource = play_journal::Source::DeviceInterface;
  };

  void onEvent(AsyncWebSocketClient* client, AwsEventType type, void* arg, uint8_t* data, size_t len);
  void onMessage(uint32_t client, const char* text, size_t len);
  void broadcastState(bool force);
  void pushJournal();
  play_journal::Source sourceOf(uint32_t client);  // caller holds inboxLock_

  void getJournal(AsyncWebServerRequest* request);
  void postTime(AsyncWebServerRequest* request, JsonVariant& body);

  void getSettings(AsyncWebServerRequest* request);
  void putSettings(AsyncWebServerRequest* request, JsonVariant& body);
  void getCalibration(AsyncWebServerRequest* request);
  void postCalibration(AsyncWebServerRequest* request, JsonVariant& body);
  void writeCalibration(JsonObject out);  // caller holds robotLock_
  void getSpeedTuning(AsyncWebServerRequest* request);
  void postSpeedTuning(AsyncWebServerRequest* request, JsonVariant& body);
  void writeSpeedTuning(JsonObject out);  // caller holds robotLock_
  void getPlay(AsyncWebServerRequest* request);
  void postPlay(AsyncWebServerRequest* request, JsonVariant& body);
  void writePlay(JsonObject out);  // caller holds robotLock_
  void getHardware(AsyncWebServerRequest* request);
  void putHardware(AsyncWebServerRequest* request, JsonVariant& body);
  void writeHardware(JsonObject out, const cwc::HardwareConfig& hardware);
  void postPosition(AsyncWebServerRequest* request, JsonVariant& body);
  void getInfo(AsyncWebServerRequest* request);
  void putDevice(AsyncWebServerRequest* request, JsonVariant& body);
  void getAuth(AsyncWebServerRequest* request);
  void unlock(AsyncWebServerRequest* request, JsonVariant& body);
  void putPin(AsyncWebServerRequest* request, JsonVariant& body);
  void postAvailableUpdate(AsyncWebServerRequest* request, JsonVariant& body);

  cwc::Robot& robot_;
  cwc::Storage& storage_;
  cwc::Chip chip_;
  auth::PinGuard& pin_;  // only touched from the network task
  PlayLog& play_;        // under robotLock_
  uint32_t pushedJournalId_ = 0;  // newest journal event sent on /ws
  std::mutex robotLock_;
  cwc::HardwareConfig hardware_{};       // as saved, applied after restart
  std::atomic<uint32_t> restartAt_{0};
  String name_;
  String hostname_;
  bool legacyHostname_ = false;
  update_channel::Channel channel_ = update_channel::Channel::Stable;  // under robotLock_, like storage_
  AsyncWebSocket socket_{"/ws"};

  std::mutex availableLock_;
  AvailableUpdate available_;
  std::atomic<uint32_t> availableGeneration_{0};

  std::mutex inboxLock_;
  Inbox inbox_;
  std::vector<uint32_t> nativeClients_;  // /ws clients that said hello as Home Assistant

  std::atomic<bool> newClient_{false};
  uint32_t lastCheckMs_ = 0;
  uint32_t lastSentMs_ = 0;
  uint32_t lastCleanupMs_ = 0;
  String lastState_;
};
