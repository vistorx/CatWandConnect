#pragma once

// What CatWandConnect publishes to and accepts from Home Assistant over MQTT
// (ADR 0005). Pure: builds topics and payloads, parses commands; the firmware
// does the networking.

#include <robot_core/ports.h>

#include <string>
#include <vector>

namespace home_assistant {

struct Device {
  std::string name;      // the robot's name
  std::string hostname;  // e.g. "catwandconnect-salon"
  std::string mac;       // "24:6F:28:00:00:01", the stable identity
  std::string version;   // firmware version
  std::string url;       // the Device interface, e.g. "http://192.168.1.20/"
};

// Topics live under "catwandconnect/<hostname>".
struct Topics {
  explicit Topics(const Device& device);

  std::string base;
  std::string availability() const { return base + "/status"; }  // "online" / "offline"
  std::string state() const { return base + "/state"; }          // JSON, see statePayload()
  std::string commands() const { return base + "/+/set"; }       // subscribe pattern
  std::string axisCommands() const { return base + "/+/+/set"; }
  std::string update() const { return base + "/update"; }        // JSON, see updatePayload()
};

constexpr const char* kOnline = "online";
constexpr const char* kOffline = "offline";
constexpr const char* kDefaultPrefix = "homeassistant";

struct Message {
  std::string topic;
  std::string payload;
};

// Retained discovery messages: a Stow button, a position and a maximum speed
// number per Axis, an activity sensor, a Play session switch, a Session
// duration number and a Firmware update entity, all grouped under one device.
std::vector<Message> discovery(const Device& device, const std::string& prefix = kDefaultPrefix);

struct State {
  float position[cwc::kAxisCount];
  float maxSpeed[cwc::kAxisCount];
  const char* activity;  // "idle", "manual", "stowing", "calibrating", "tuning_speed", "playing"
  int sessionMinutes;    // the Session duration
};
// The Play session switch reads ON exactly while the activity is "playing",
// whatever started the session.
std::string statePayload(const State& state);

// What the Firmware update entity shows. `latest` is the newest version the
// Device interface found in the Update feed (empty when unknown), `notes` its
// release notes URL. The robot never downloads anything itself (ADR 0012), so
// the entity has no install command.
struct UpdateInfo {
  std::string installed;
  std::string latest;
  std::string notes;
};
// A `latest` not newer than `installed` reads as up to date.
std::string updatePayload(const UpdateInfo& info);

enum class CommandKind { None, Stow, Position, MaxSpeed, StartPlay, StopPlay, SessionDuration };

struct Command {
  CommandKind kind = CommandKind::None;
  cwc::Axis axis = cwc::Axis::Horizontal;
  float value = 0;
};

// Unknown topics and unreadable values give CommandKind::None. Ranges are
// enforced by the Robot core, not here.
Command parseCommand(const Topics& topics, const std::string& topic, const std::string& payload);

}  // namespace home_assistant
