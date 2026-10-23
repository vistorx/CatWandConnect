#pragma once

// The Play session journal and the Robot clock (CONTEXT.md, docs/adr/0013).
// Pure: the uptime comes through the Clock port, the events are kept through
// the Storage port. The firmware records what the Robot core reports
// (PlayObserver) with the source of the command.

#include <robot_core/ports.h>

#include <cstddef>
#include <cstdint>
#include <string>

namespace play_journal {

// Stored as numbers: never renumber, only add.
enum class What : uint8_t { Started = 0, Ended = 1, Stopped = 2, Refused = 3 };

enum class Source : uint8_t {
  DeviceInterface = 0,
  HomeAssistantMqtt = 1,
  HomeAssistantNative = 2,
  PawTap = 3,    // reserved for Play on request (#240)
  Schedule = 4,  // reserved for the schedule (#176)
};

enum class Reason : uint8_t {
  None = 0,
  // Refused
  Busy = 1,
  NotCalibrated = 2,
  RestTime = 3,      // reserved for the guards of automatic starts (#176, #240)
  DailyLimit = 4,
  QuietHours = 5,
  AutomaticOff = 6,
  // Stopped
  Stop = 7,
  Stow = 8,
  ManualControl = 9,
  Calibration = 10,
  SpeedTuning = 11,
};

// Names used by the API, MQTT and Home Assistant ("started", "device_interface",
// "not_calibrated"…); "unknown" for a value this firmware doesn't know, and
// nullptr for Reason::None.
const char* whatName(What what);
const char* sourceName(Source source);
const char* reasonName(Reason reason);

What whatOf(cwc::PlayEvent event);
Reason reasonOf(cwc::PlayCause cause);

// The Robot clock: the time the Device interface (browser) or the Native
// integration (Home Assistant) gave the robot, run on from its uptime. Unset
// after every boot; never from NTP (the robot makes no outbound connection).
class RobotClock {
 public:
  explicit RobotClock(cwc::Clock& uptime) : uptime_(uptime) {}

  // Milliseconds since boot, past the 32-bit counter's wrap after 49.7 days.
  // Call it at least once in each wrap period (the main loop does).
  uint64_t uptimeMs();

  // `epochMs` since 1970 UTC, `offsetMinutes` east of UTC now (UTC+02:00 is
  // 120), `zone` the IANA name if known ("Europe/Paris", kept for later).
  // False, nothing changed, for a time outside 2024–2100, an offset outside
  // UTC-12:00..UTC+14:00 or a zone that isn't a short IANA-like name.
  bool set(int64_t epochMs, int offsetMinutes, const std::string& zone);

  bool isSet() const { return set_; }
  int64_t nowMs();  // since 1970 UTC; 0 while unset
  int offsetMinutes() const { return offsetMinutes_; }
  const std::string& zone() const { return zone_; }

 private:
  cwc::Clock& uptime_;
  uint32_t last32_ = 0;
  uint64_t high_ = 0;
  bool set_ = false;
  int64_t epochAtSetMs_ = 0;
  uint64_t uptimeAtSetMs_ = 0;
  int offsetMinutes_ = 0;
  std::string zone_;
};

struct Event {
  uint32_t id = 0;  // one more for each event, kept across restarts
  What what = What::Started;
  Source source = Source::DeviceInterface;
  Reason reason = Reason::None;
  uint32_t epochS = 0;   // when, in seconds since 1970 UTC; 0 = not known
  uint32_t uptimeS = 0;  // seconds since boot when recorded
  bool thisBoot = false; // recorded since the last restart (not stored)
};

// The last kCapacity Play session events, oldest dropped first, saved after
// each change. Events recorded before the Robot clock is set get their time
// once it is (clockSet()); undated events from before a restart stay undated.
class Journal {
 public:
  static constexpr size_t kCapacity = 50;

  Journal(cwc::Storage& storage, RobotClock& clock) : storage_(storage), clock_(clock) {}

  // Loads the saved events: all from earlier boots.
  void begin();

  const Event& record(What what, Source source, Reason reason = Reason::None);

  // Call after each RobotClock::set(): dates this boot's undated events.
  void clockSet();

  size_t size() const { return count_; }
  const Event& newest(size_t index) const;  // 0 = the newest
  uint32_t lastId() const { return count_ == 0 ? 0 : newest(0).id; }

  // Seconds since the event; -1 when not known.
  int64_t ageS(const Event& event);

  // Play sessions started since local midnight (offset of the Robot clock);
  // -1 while the clock is unset.
  int sessionsToday();

  // GET /api/journal: {"clock":{...},"sessionsToday":n|null,"events":[newest first]}.
  std::string json();
  // The /ws message for a new event: {"t":"journal","event":{...}}.
  std::string eventJson(const Event& event);

  // The Diagnostic report's section: clock, sessions today, events newest
  // first in UTC. No personal data: the zone name is left out.
  std::string reportText();

 private:
  void save();

  cwc::Storage& storage_;
  RobotClock& clock_;
  Event events_[kCapacity];  // ring, oldest at first_
  size_t first_ = 0;
  size_t count_ = 0;
  uint32_t nextId_ = 1;
};

}  // namespace play_journal
