#pragma once

// The Robot core's persistent settings: Travel limits, Stow position and
// motion limits per Axis. Stored as a versioned record so they survive
// firmware updates.

#include "robot_core/motion.h"
#include "robot_core/ports.h"

namespace cwc {

struct AxisSettings {
  TravelLimits travel;  // degrees
  float stow;           // degrees, within `travel`
  float maxSpeed;       // °/s
  float maxAccel;       // °/s²
};

struct RobotSettings {
  AxisSettings axes[kAxisCount];
  // Fraction of maxSpeed used by Manual control: the main screen's speed slider.
  float playSpeed;
  // True once Calibration has been completed; until then movement stays
  // within kDefaultTravel whatever `travel` says.
  bool calibrated;

  AxisSettings& axis(Axis a) { return axes[static_cast<size_t>(a)]; }
  const AxisSettings& axis(Axis a) const { return axes[static_cast<size_t>(a)]; }
};

// Range a servo can reach; Travel limits must stay inside it.
constexpr TravelLimits kServoRange{0, 180};
// Conservative envelope used until Calibration has been completed.
constexpr TravelLimits kDefaultTravel{60, 120};
constexpr float kDefaultStow = 90;

constexpr float kMinSpeed = 10;  // °/s, also the gentle speed used at power-up
constexpr float kMaxSpeed = 360;
constexpr float kMinAccel = 20;  // °/s²
constexpr float kMaxAccel = 2000;
constexpr float kMinPlaySpeed = 0.1f;

RobotSettings defaultSettings();

enum class SettingsError : uint8_t {
  None,
  TravelOrder,        // minimum not below maximum
  TravelOutOfRange,   // limit outside the servo range
  StowOutOfTravel,    // Stow outside the Travel limits
  SpeedOutOfRange,
  AccelOutOfRange,
  PlaySpeedOutOfRange,
};

struct SettingsCheck {
  SettingsError error = SettingsError::None;
  Axis axis = Axis::Horizontal;  // the offending Axis, for per-axis errors

  bool ok() const { return error == SettingsError::None; }
};

SettingsCheck validate(const RobotSettings& settings);

const char* settingsErrorName(SettingsError error);

// Reads the stored settings. Returns defaults when nothing valid is stored,
// e.g. on first boot, after a corrupted write or from an unknown version.
RobotSettings loadSettings(Storage& storage);

bool saveSettings(Storage& storage, const RobotSettings& settings);

}  // namespace cwc
