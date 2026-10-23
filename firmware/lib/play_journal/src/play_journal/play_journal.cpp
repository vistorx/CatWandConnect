#include "play_journal/play_journal.h"

#include <ArduinoJson.h>

#include <cstdio>

namespace play_journal {

namespace {

// Stored under kKey in the settings storage: a header {kRecordVersion, count,
// 0, 0, next id (u32 LE)} then kCapacity entries of kEntrySize bytes, oldest
// first: id, epochS, uptimeS (u32 LE each), what, source, reason, 0.
constexpr const char* kKey = "journal";
constexpr uint8_t kRecordVersion = 1;
constexpr size_t kHeaderSize = 8;
constexpr size_t kEntrySize = 16;
constexpr size_t kRecordSize = kHeaderSize + Journal::kCapacity * kEntrySize;

// 2024-01-01 and 2100-01-01 UTC: a clock outside is a browser's mistake.
constexpr int64_t kEarliestMs = 1704067200000LL;
constexpr int64_t kLatestMs = 4102444800000LL;
constexpr int kMinOffset = -12 * 60;
constexpr int kMaxOffset = 14 * 60;
constexpr size_t kMaxZoneLength = 64;

void putU32(uint8_t* at, uint32_t value) {
  for (int i = 0; i < 4; ++i) at[i] = static_cast<uint8_t>(value >> (8 * i));
}

uint32_t getU32(const uint8_t* at) {
  uint32_t value = 0;
  for (int i = 0; i < 4; ++i) value |= static_cast<uint32_t>(at[i]) << (8 * i);
  return value;
}

bool zoneChar(char c) {
  return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '/' || c == '_' ||
         c == '+' || c == '-';
}

// Days since 1970-01-01 -> civil date (Howard Hinnant's algorithm).
void civilFromDays(int64_t days, int& year, unsigned& month, unsigned& day) {
  days += 719468;
  int64_t era = (days >= 0 ? days : days - 146096) / 146097;
  unsigned doe = static_cast<unsigned>(days - era * 146097);
  unsigned yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
  int64_t y = static_cast<int64_t>(yoe) + era * 400;
  unsigned doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
  unsigned mp = (5 * doy + 2) / 153;
  day = doy - (153 * mp + 2) / 5 + 1;
  month = mp < 10 ? mp + 3 : mp - 9;
  year = static_cast<int>(y + (month <= 2));
}

std::string utcText(uint32_t epochS) {
  int year;
  unsigned month, day;
  civilFromDays(epochS / 86400, year, month, day);
  unsigned rest = epochS % 86400;
  char text[32];
  std::snprintf(text, sizeof text, "%04d-%02u-%02u %02u:%02u:%02u UTC", year, month, day, rest / 3600, rest % 3600 / 60,
                rest % 60);
  return text;
}

std::string ageText(int64_t seconds) {
  char text[32];
  if (seconds < 60) std::snprintf(text, sizeof text, "%lld s ago", static_cast<long long>(seconds));
  else if (seconds < 3600) std::snprintf(text, sizeof text, "%lld min ago", static_cast<long long>(seconds / 60));
  else std::snprintf(text, sizeof text, "%lld h %lld min ago", static_cast<long long>(seconds / 3600),
                     static_cast<long long>(seconds % 3600 / 60));
  return text;
}

int64_t localDay(int64_t epochS, int offsetMinutes) {
  int64_t local = epochS + offsetMinutes * 60;
  return local >= 0 ? local / 86400 : (local - 86399) / 86400;
}

}  // namespace

const char* whatName(What what) {
  switch (what) {
    case What::Started: return "started";
    case What::Ended: return "ended";
    case What::Stopped: return "stopped";
    case What::Refused: return "refused";
  }
  return "unknown";
}

const char* sourceName(Source source) {
  switch (source) {
    case Source::DeviceInterface: return "device_interface";
    case Source::HomeAssistantMqtt: return "home_assistant_mqtt";
    case Source::HomeAssistantNative: return "home_assistant_native";
    case Source::PawTap: return "paw_tap";
    case Source::Schedule: return "schedule";
  }
  return "unknown";
}

const char* reasonName(Reason reason) {
  switch (reason) {
    case Reason::None: return nullptr;
    case Reason::Busy: return "busy";
    case Reason::NotCalibrated: return "not_calibrated";
    case Reason::RestTime: return "rest_time";
    case Reason::DailyLimit: return "daily_limit";
    case Reason::QuietHours: return "quiet_hours";
    case Reason::AutomaticOff: return "automatic_off";
    case Reason::Stop: return "stop";
    case Reason::Stow: return "stow";
    case Reason::ManualControl: return "manual_control";
    case Reason::Calibration: return "calibration";
    case Reason::SpeedTuning: return "speed_tuning";
  }
  return "unknown";
}

What whatOf(cwc::PlayEvent event) {
  switch (event) {
    case cwc::PlayEvent::Started: return What::Started;
    case cwc::PlayEvent::Ended: return What::Ended;
    case cwc::PlayEvent::Stopped: return What::Stopped;
    case cwc::PlayEvent::Refused: return What::Refused;
  }
  return What::Started;
}

Reason reasonOf(cwc::PlayCause cause) {
  switch (cause) {
    case cwc::PlayCause::None: return Reason::None;
    case cwc::PlayCause::Stop: return Reason::Stop;
    case cwc::PlayCause::Stow: return Reason::Stow;
    case cwc::PlayCause::ManualControl: return Reason::ManualControl;
    case cwc::PlayCause::Calibration: return Reason::Calibration;
    case cwc::PlayCause::SpeedTuning: return Reason::SpeedTuning;
    case cwc::PlayCause::Busy: return Reason::Busy;
    case cwc::PlayCause::NotCalibrated: return Reason::NotCalibrated;
  }
  return Reason::None;
}

// --- Robot clock --------------------------------------------------------------

uint64_t RobotClock::uptimeMs() {
  uint32_t now = uptime_.nowMs();
  if (now < last32_) high_ += 0x100000000ull;
  last32_ = now;
  return high_ + now;
}

bool RobotClock::set(int64_t epochMs, int offsetMinutes, const std::string& zone) {
  if (epochMs < kEarliestMs || epochMs >= kLatestMs) return false;
  if (offsetMinutes < kMinOffset || offsetMinutes > kMaxOffset) return false;
  if (zone.size() > kMaxZoneLength) return false;
  for (char c : zone) {
    if (!zoneChar(c)) return false;
  }
  epochAtSetMs_ = epochMs;
  uptimeAtSetMs_ = uptimeMs();
  offsetMinutes_ = offsetMinutes;
  zone_ = zone;
  set_ = true;
  return true;
}

int64_t RobotClock::nowMs() {
  if (!set_) return 0;
  return epochAtSetMs_ + static_cast<int64_t>(uptimeMs() - uptimeAtSetMs_);
}

// --- Journal ------------------------------------------------------------------

void Journal::begin() {
  first_ = 0;
  count_ = 0;
  nextId_ = 1;
  uint8_t record[kRecordSize];
  if (!storage_.read(kKey, record, sizeof record) || record[0] != kRecordVersion) return;
  size_t count = record[1] <= kCapacity ? record[1] : kCapacity;
  for (size_t i = 0; i < count; ++i) {
    const uint8_t* at = record + kHeaderSize + i * kEntrySize;
    Event& e = events_[i];
    e.id = getU32(at);
    e.epochS = getU32(at + 4);
    e.uptimeS = getU32(at + 8);
    e.what = static_cast<What>(at[12]);
    e.source = static_cast<Source>(at[13]);
    e.reason = static_cast<Reason>(at[14]);
    e.thisBoot = false;
  }
  count_ = count;
  nextId_ = getU32(record + 4);
  if (count_ > 0 && nextId_ <= newest(0).id) nextId_ = newest(0).id + 1;
}

void Journal::save() {
  uint8_t record[kRecordSize] = {};
  record[0] = kRecordVersion;
  record[1] = static_cast<uint8_t>(count_);
  putU32(record + 4, nextId_);
  for (size_t i = 0; i < count_; ++i) {
    const Event& e = events_[(first_ + i) % kCapacity];
    uint8_t* at = record + kHeaderSize + i * kEntrySize;
    putU32(at, e.id);
    putU32(at + 4, e.epochS);
    putU32(at + 8, e.uptimeS);
    at[12] = static_cast<uint8_t>(e.what);
    at[13] = static_cast<uint8_t>(e.source);
    at[14] = static_cast<uint8_t>(e.reason);
  }
  storage_.write(kKey, record, sizeof record);
}

const Event& Journal::record(What what, Source source, Reason reason) {
  size_t slot;
  if (count_ < kCapacity) {
    slot = (first_ + count_) % kCapacity;
    ++count_;
  } else {
    slot = first_;
    first_ = (first_ + 1) % kCapacity;
  }
  Event& e = events_[slot];
  e.id = nextId_++;
  e.what = what;
  e.source = source;
  e.reason = reason;
  e.uptimeS = static_cast<uint32_t>(clock_.uptimeMs() / 1000);
  e.epochS = clock_.isSet() ? static_cast<uint32_t>(clock_.nowMs() / 1000) : 0;
  e.thisBoot = true;
  save();
  return e;
}

void Journal::clockSet() {
  if (!clock_.isSet()) return;
  int64_t nowS = clock_.nowMs() / 1000;
  uint64_t uptimeS = clock_.uptimeMs() / 1000;
  bool changed = false;
  for (size_t i = 0; i < count_; ++i) {
    Event& e = events_[(first_ + i) % kCapacity];
    if (!e.thisBoot || e.epochS != 0) continue;
    e.epochS = static_cast<uint32_t>(nowS - static_cast<int64_t>(uptimeS - e.uptimeS));
    changed = true;
  }
  if (changed) save();
}

const Event& Journal::newest(size_t index) const { return events_[(first_ + count_ - 1 - index) % kCapacity]; }

int64_t Journal::ageS(const Event& event) {
  if (event.thisBoot) return static_cast<int64_t>(clock_.uptimeMs() / 1000) - event.uptimeS;
  if (event.epochS != 0 && clock_.isSet()) return clock_.nowMs() / 1000 - event.epochS;
  return -1;
}

int Journal::sessionsToday() {
  if (!clock_.isSet()) return -1;
  int offset = clock_.offsetMinutes();
  int64_t today = localDay(clock_.nowMs() / 1000, offset);
  int count = 0;
  for (size_t i = 0; i < count_; ++i) {
    const Event& e = events_[(first_ + i) % kCapacity];
    if (e.what == What::Started && e.epochS != 0 && localDay(e.epochS, offset) == today) ++count;
  }
  return count;
}

namespace {

void writeEvent(JsonObject out, const Event& e, int64_t age) {
  out["id"] = e.id;
  out["what"] = whatName(e.what);
  out["source"] = sourceName(e.source);
  const char* reason = reasonName(e.reason);
  if (reason) out["reason"] = reason;
  else out["reason"] = nullptr;
  if (e.epochS != 0) out["at"] = e.epochS;
  else out["at"] = nullptr;
  if (age >= 0) out["ago"] = age;
  else out["ago"] = nullptr;
}

}  // namespace

std::string Journal::json() {
  JsonDocument doc;
  JsonObject clock = doc["clock"].to<JsonObject>();
  clock["set"] = clock_.isSet();
  if (clock_.isSet()) {
    clock["offset"] = clock_.offsetMinutes();
    clock["zone"] = clock_.zone();
  } else {
    clock["offset"] = nullptr;
    clock["zone"] = nullptr;
  }
  int today = sessionsToday();
  if (today >= 0) doc["sessionsToday"] = today;
  else doc["sessionsToday"] = nullptr;
  JsonArray events = doc["events"].to<JsonArray>();
  for (size_t i = 0; i < count_; ++i) {
    const Event& e = newest(i);
    writeEvent(events.add<JsonObject>(), e, ageS(e));
  }
  std::string out;
  serializeJson(doc, out);
  return out;
}

std::string Journal::eventJson(const Event& event) {
  JsonDocument doc;
  doc["t"] = "journal";
  writeEvent(doc["event"].to<JsonObject>(), event, ageS(event));
  std::string out;
  serializeJson(doc, out);
  return out;
}

std::string Journal::reportText() {
  std::string out = "Robot clock: ";
  if (clock_.isSet()) {
    int offset = clock_.offsetMinutes();
    int magnitude = offset < 0 ? -offset : offset;
    char text[32];
    std::snprintf(text, sizeof text, "set, UTC%c%02d:%02d", offset < 0 ? '-' : '+', magnitude / 60, magnitude % 60);
    out += text;
  } else {
    out += "not set";
  }
  out += "\nSessions today: ";
  int today = sessionsToday();
  out += today >= 0 ? std::to_string(today) : "unknown";
  out += '\n';
  if (count_ == 0) out += "No events yet.\n";
  for (size_t i = 0; i < count_; ++i) {
    const Event& e = newest(i);
    int64_t age = ageS(e);
    if (e.epochS != 0) out += utcText(e.epochS);
    else if (age >= 0) out += ageText(age) + ", before the clock was set";
    else out += "before a restart, time unknown";
    out += ": ";
    out += whatName(e.what);
    out += ", ";
    out += sourceName(e.source);
    if (const char* reason = reasonName(e.reason)) {
      out += ", ";
      out += reason;
    }
    out += '\n';
  }
  return out;
}

}  // namespace play_journal
