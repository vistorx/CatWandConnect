#pragma once

// What CatWandConnect publishes to and accepts from Home Assistant over MQTT
// (ADR 0005). Pure: builds topics and payloads, parses commands; the firmware
// does the networking.

#include <robot_core/ports.h>

#include <cstdint>
#include <string>
#include <vector>

namespace home_assistant {

struct Device {
  std::string name;      // the robot's name
  std::string hostname;  // e.g. "catwandconnect-salon"
  std::string mac;       // "24:6F:28:00:00:01", the stable identity
  std::string version;   // firmware version
  std::string url;       // the Device interface, e.g. "http://192.168.1.20/"
  // Readable entity ids start with this, hyphens as underscores; the
  // hostname when empty. "catwandconnect" for a default-named robot whose
  // hostname has a Device ID suffix (#195), so its ids stay documented.
  std::string entityPrefix = "";
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
  // The Play session time left in whole seconds, see playRemainingPayload().
  std::string playRemaining() const { return base + "/play_session/remaining"; }
  std::string playEvent() const { return base + "/play_session/event"; }  // JSON, see eventPayload()
};

constexpr const char* kOnline = "online";
constexpr const char* kOffline = "offline";
constexpr const char* kDefaultPrefix = "homeassistant";
// Home Assistant's birth message: "online" on this topic once it (re)starts.
constexpr const char* kHomeAssistantStatus = "homeassistant/status";
bool homeAssistantOnline(const std::string& topic, const std::string& payload);

struct Message {
  std::string topic;
  std::string payload;
};

// Retained discovery messages: a Stow button, a position and a maximum speed
// number per Axis, an activity sensor, a Play session switch, a Session
// duration number, a Firmware update entity, a Play session end sensor, a
// Play session event entity and a Sessions today sensor, all grouped under
// one device.
std::vector<Message> discovery(const Device& device, const std::string& prefix = kDefaultPrefix);

struct State {
  float position[cwc::kAxisCount];
  float maxSpeed[cwc::kAxisCount];
  const char* activity;  // "idle", "manual", "stowing", "calibrating", "tuning_speed", "playing"
  int sessionMinutes;    // the Session duration
  int sessionsToday = -1;  // Play sessions started today; -1 while the robot has no clock
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

// The Play session end sensor. The robot has no wall clock, so it sends the
// time left in whole seconds ("0" when no session runs) and the sensor's
// template adds it to Home Assistant's clock.
std::string playRemainingPayload(uint32_t remainingS);

// Decides when the time left is worth sending: when a session starts or ends,
// when its end moves (a new session during one), and after force(). Counting
// down is not news, so it is not sent every second.
class PlayRemainingReporter {
 public:
  // `remainingS` is the Play session time left (0 when none runs), `nowMs`
  // the robot's clock. True when it should be published now.
  bool due(uint32_t remainingS, uint32_t nowMs);
  // Publish at the next due(): after a reconnection or a Home Assistant restart.
  void force() { known_ = false; }

 private:
  bool known_ = false;
  bool playing_ = false;
  uint32_t endMs_ = 0;  // on the robot's clock
};

// One Play session journal event for the event entity, published not
// retained: `what` "started" / "ended" / "stopped" / "refused" becomes the
// event type; `source` and `reason` (nullptr: none) its attributes.
std::string eventPayload(const char* what, const char* source, const char* reason);

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
