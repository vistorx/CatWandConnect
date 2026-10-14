#include "robot_core/settings.h"

#include <cmath>

namespace cwc {

namespace {

constexpr const char* kKey = "robot";
constexpr uint32_t kMagic = 0x43574331;  // "CWC1"

// Bump when RobotSettings changes, and teach loadSettings() to migrate the
// previous record into the new one.
constexpr uint16_t kVersion = 3;

struct Record {
  uint32_t magic;
  uint16_t version;
  uint16_t size;
  RobotSettings settings;
};

// Version 1, written by firmware 0.1.0: no minimum speed.
struct AxisSettingsV1 {
  TravelLimits travel;
  float stow;
  float maxSpeed;
  float maxAccel;
};

struct RobotSettingsV1 {
  AxisSettingsV1 axes[kAxisCount];
  float playSpeed;
  bool calibrated;
};

struct RecordV1 {
  uint32_t magic;
  uint16_t version;
  uint16_t size;
  RobotSettingsV1 settings;
};

// Version 2, up to the Play session: no Pause range nor Session duration.
struct AxisSettingsV2 {
  TravelLimits travel;
  float stow;
  float maxSpeed;
  float maxAccel;
  float minSpeed;
};

struct RobotSettingsV2 {
  AxisSettingsV2 axes[kAxisCount];
  float playSpeed;
  bool calibrated;
};

struct RecordV2 {
  uint32_t magic;
  uint16_t version;
  uint16_t size;
  RobotSettingsV2 settings;
};

// Fills `out` from an older record, keeping the new fields at their defaults.
void migrateAxis(AxisSettings& out, TravelLimits travel, float stow, float maxSpeed, float maxAccel,
                 float minSpeed) {
  out.travel = travel;
  out.stow = stow;
  out.maxSpeed = maxSpeed;
  out.maxAccel = maxAccel;
  out.minSpeed = minSpeed;
}

bool readV2(Storage& storage, RobotSettings& out) {
  RecordV2 record;
  if (!storage.read(kKey, &record, sizeof(record))) return false;
  if (record.magic != kMagic || record.version != 2 || record.size != sizeof(RobotSettingsV2)) return false;
  out = defaultSettings();
  for (size_t i = 0; i < kAxisCount; ++i) {
    const AxisSettingsV2& old = record.settings.axes[i];
    migrateAxis(out.axes[i], old.travel, old.stow, old.maxSpeed, old.maxAccel, old.minSpeed);
  }
  out.playSpeed = record.settings.playSpeed;
  out.calibrated = record.settings.calibrated;
  return true;
}

bool readV1(Storage& storage, RobotSettings& out) {
  RecordV1 record;
  if (!storage.read(kKey, &record, sizeof(record))) return false;
  if (record.magic != kMagic || record.version != 1 || record.size != sizeof(RobotSettingsV1)) return false;
  out = defaultSettings();
  for (size_t i = 0; i < kAxisCount; ++i) {
    const AxisSettingsV1& old = record.settings.axes[i];
    migrateAxis(out.axes[i], old.travel, old.stow, old.maxSpeed, old.maxAccel, defaultMinSpeed(old.maxSpeed));
  }
  out.playSpeed = record.settings.playSpeed;
  out.calibrated = record.settings.calibrated;
  return true;
}

bool within(float value, float low, float high) { return std::isfinite(value) && value >= low && value <= high; }

}  // namespace

float defaultMinSpeed(float maxSpeed) {
  float speed = maxSpeed * 0.1f;
  return speed < kSlowestSpeed ? kSlowestSpeed : speed;
}

float manualSpeed(const AxisSettings& axis, float playSpeed) {
  float along = (playSpeed - kMinPlaySpeed) / (1 - kMinPlaySpeed);
  if (along < 0) along = 0;
  if (along > 1) along = 1;
  return axis.minSpeed + (axis.maxSpeed - axis.minSpeed) * along;
}

RobotSettings defaultSettings() {
  RobotSettings settings{};
  for (AxisSettings& axis : settings.axes) {
    axis.travel = kDefaultTravel;
    axis.stow = kDefaultStow;
    axis.maxSpeed = 90;
    axis.maxAccel = 360;
    axis.minSpeed = defaultMinSpeed(axis.maxSpeed);
  }
  for (AxisSettings& axis : settings.axes) {
    axis.pauseMin = 0;
    axis.pauseMax = 2;
  }
  settings.playSpeed = 0.6f;
  settings.calibrated = false;
  settings.sessionMinutes = 10;
  return settings;
}

SettingsCheck validate(const RobotSettings& settings) {
  for (Axis axis : kAxes) {
    const AxisSettings& a = settings.axis(axis);
    auto fail = [axis](SettingsError error) { return SettingsCheck{error, axis}; };

    if (!within(a.travel.min, kServoRange.min, kServoRange.max) ||
        !within(a.travel.max, kServoRange.min, kServoRange.max)) {
      return fail(SettingsError::TravelOutOfRange);
    }
    if (a.travel.min >= a.travel.max) return fail(SettingsError::TravelOrder);
    if (!within(a.stow, a.travel.min, a.travel.max)) return fail(SettingsError::StowOutOfTravel);
    if (!within(a.maxSpeed, kMinSpeed, kMaxSpeed)) return fail(SettingsError::SpeedOutOfRange);
    if (!within(a.minSpeed, kSlowestSpeed, kMaxSpeed) || a.minSpeed >= a.maxSpeed) {
      return fail(SettingsError::MinSpeedOutOfRange);
    }
    if (!within(a.maxAccel, kMinAccel, kMaxAccel)) return fail(SettingsError::AccelOutOfRange);
    if (!within(a.pauseMin, 0, kMaxPause) || !within(a.pauseMax, 0, kMaxPause)) {
      return fail(SettingsError::PauseOutOfRange);
    }
    if (a.pauseMin > a.pauseMax) return fail(SettingsError::PauseOrder);
  }
  if (!within(settings.playSpeed, kMinPlaySpeed, 1)) return {SettingsError::PlaySpeedOutOfRange, Axis::Horizontal};
  if (settings.sessionMinutes < kMinSessionMinutes || settings.sessionMinutes > kMaxSessionMinutes) {
    return {SettingsError::SessionDurationOutOfRange, Axis::Horizontal};
  }
  return {};
}

const char* settingsErrorName(SettingsError error) {
  switch (error) {
    case SettingsError::None: return "none";
    case SettingsError::TravelOrder: return "travel_order";
    case SettingsError::TravelOutOfRange: return "travel_out_of_range";
    case SettingsError::StowOutOfTravel: return "stow_out_of_travel";
    case SettingsError::SpeedOutOfRange: return "speed_out_of_range";
    case SettingsError::MinSpeedOutOfRange: return "min_speed_out_of_range";
    case SettingsError::AccelOutOfRange: return "accel_out_of_range";
    case SettingsError::PlaySpeedOutOfRange: return "play_speed_out_of_range";
    case SettingsError::PauseOutOfRange: return "pause_out_of_range";
    case SettingsError::PauseOrder: return "pause_order";
    case SettingsError::SessionDurationOutOfRange: return "session_duration_out_of_range";
  }
  return "unknown";
}

RobotSettings loadSettings(Storage& storage) {
  Record record;
  RobotSettings settings;
  if (storage.read(kKey, &record, sizeof(record))) {
    if (record.magic != kMagic || record.version != kVersion || record.size != sizeof(RobotSettings)) {
      return defaultSettings();
    }
    settings = record.settings;
  } else if (!readV2(storage, settings) && !readV1(storage, settings)) {
    return defaultSettings();
  }
  if (!validate(settings).ok()) return defaultSettings();
  return settings;
}

bool saveSettings(Storage& storage, const RobotSettings& settings) {
  Record record{kMagic, kVersion, sizeof(RobotSettings), settings};
  return storage.write(kKey, &record, sizeof(record));
}

}  // namespace cwc
