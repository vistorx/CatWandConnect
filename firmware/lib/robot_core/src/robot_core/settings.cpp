#include "robot_core/settings.h"

#include <cmath>

namespace cwc {

namespace {

constexpr const char* kKey = "robot";
constexpr uint32_t kMagic = 0x43574331;  // "CWC1"

// Bump when RobotSettings changes, and teach loadSettings() to migrate the
// previous record into the new one.
constexpr uint16_t kVersion = 1;

struct Record {
  uint32_t magic;
  uint16_t version;
  uint16_t size;
  RobotSettings settings;
};

bool within(float value, float low, float high) { return std::isfinite(value) && value >= low && value <= high; }

}  // namespace

RobotSettings defaultSettings() {
  RobotSettings settings{};
  for (AxisSettings& axis : settings.axes) {
    axis.travel = kDefaultTravel;
    axis.stow = kDefaultStow;
    axis.maxSpeed = 90;
    axis.maxAccel = 360;
  }
  settings.playSpeed = 0.6f;
  settings.calibrated = false;
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
    if (!within(a.maxAccel, kMinAccel, kMaxAccel)) return fail(SettingsError::AccelOutOfRange);
  }
  if (!within(settings.playSpeed, kMinPlaySpeed, 1)) return {SettingsError::PlaySpeedOutOfRange, Axis::Horizontal};
  return {};
}

const char* settingsErrorName(SettingsError error) {
  switch (error) {
    case SettingsError::None: return "none";
    case SettingsError::TravelOrder: return "travel_order";
    case SettingsError::TravelOutOfRange: return "travel_out_of_range";
    case SettingsError::StowOutOfTravel: return "stow_out_of_travel";
    case SettingsError::SpeedOutOfRange: return "speed_out_of_range";
    case SettingsError::AccelOutOfRange: return "accel_out_of_range";
    case SettingsError::PlaySpeedOutOfRange: return "play_speed_out_of_range";
  }
  return "unknown";
}

RobotSettings loadSettings(Storage& storage) {
  Record record;
  if (!storage.read(kKey, &record, sizeof(record))) return defaultSettings();
  if (record.magic != kMagic || record.version != kVersion || record.size != sizeof(RobotSettings)) {
    return defaultSettings();
  }
  if (!validate(record.settings).ok()) return defaultSettings();
  return record.settings;
}

bool saveSettings(Storage& storage, const RobotSettings& settings) {
  Record record{kMagic, kVersion, sizeof(RobotSettings), settings};
  return storage.write(kKey, &record, sizeof(record));
}

}  // namespace cwc
