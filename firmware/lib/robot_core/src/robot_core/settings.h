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
  // Bottom of the Speed range: Manual control's speed at the lowest speed
  // slider setting. Strictly below maxSpeed.
  float minSpeed;       // °/s
  // Pause range: how long the axis rests between two moves of a Play
  // session, drawn uniformly in [pauseMin, pauseMax]. 0 means non-stop.
  float pauseMin;       // s
  float pauseMax;       // s, at least pauseMin
};

struct RobotSettings {
  AxisSettings axes[kAxisCount];
  // The main screen's speed slider, from kMinPlaySpeed to 1: Manual control
  // runs at minSpeed at the bottom and maxSpeed at the top (manualSpeed()).
  float playSpeed;
  // True once Calibration has been completed; until then movement stays
  // within kDefaultTravel whatever `travel` says.
  bool calibrated;
  // Session duration: how long a Play session lasts, in minutes.
  uint8_t sessionMinutes;

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
constexpr float kSlowestSpeed = 1;  // °/s, lowest allowed minimum speed
constexpr float kMaxPause = 60;     // s, longest Pause range bound
constexpr uint8_t kMinSessionMinutes = 1;
constexpr uint8_t kMaxSessionMinutes = 60;

// Default minimum speed for a maximum speed: a tenth of it, as the speed
// slider's bottom was before the minimum speed existed.
float defaultMinSpeed(float maxSpeed);

// Manual control's speed (°/s) for an axis at the given speed slider setting.
float manualSpeed(const AxisSettings& axis, float playSpeed);

RobotSettings defaultSettings();

enum class SettingsError : uint8_t {
  None,
  TravelOrder,        // minimum not below maximum
  TravelOutOfRange,   // limit outside the servo range
  StowOutOfTravel,    // Stow outside the Travel limits
  SpeedOutOfRange,
  MinSpeedOutOfRange,  // below kSlowestSpeed or not below the maximum speed
  AccelOutOfRange,
  PlaySpeedOutOfRange,
  PauseOutOfRange,            // a Pause range bound outside 0–60 s
  PauseOrder,                 // Pause range minimum above its maximum
  SessionDurationOutOfRange,  // outside 1–60 min
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
