#pragma once

#include "robot_core/motion.h"
#include "robot_core/ports.h"
#include "robot_core/settings.h"

namespace cwc {

enum class Activity : uint8_t {
  Idle,
  Manual,   // a person is moving the wand
  Stowing,  // moving to the Stow position, then releasing the servos
};

// Owns all safety-critical behaviour of CatWandConnect: Travel limits, speed,
// Stow priority, power-up and Calibration. Every command source (Device
// interface, Home Assistant, ...) goes through it.
//
// Priority: Stow beats everything and cannot be interrupted; Manual control
// cancels any other activity. Nothing resumes on its own.
// Servos hold their position while in use and are released only once the
// wand rests at the Stow position.
class Robot {
 public:
  // Manual control stops when no command refreshes it for this long.
  static constexpr uint32_t kManualTimeoutMs = 500;

  Robot(MotorDriver& motors, Clock& clock, Storage& storage);

  // Loads the settings and puts the wand at its Stow position.
  void begin();

  // Advances the robot by one control step. Call it often from the main loop.
  void tick();

  // Manual control with joystick deflections in [-1, 1] per Axis: the wand
  // moves at that fraction of its maximum speed. Repeat at least every
  // kManualTimeoutMs while the joystick is held. Ignored while stowing.
  void manualMove(float horizontal, float vertical);

  // Brings Manual control to a stop, e.g. when the joystick is released.
  void manualStop();

  // Stops everything else and moves the wand to the Stow position, where the
  // servos are released.
  void stow();

  // Validates, stores and applies new settings as a whole. Invalid settings
  // are rejected and the current ones stay in effect.
  SettingsCheck applySettings(const RobotSettings& settings);

  const RobotSettings& settings() const { return settings_; }
  Activity activity() const { return activity_; }
  float position(Axis axis) const;
  bool held(Axis axis) const { return state(axis).held; }

  // The Travel limits in effect: the conservative defaults until Calibration
  // has been completed.
  TravelLimits travelLimits(Axis axis) const;

 private:
  struct AxisState {
    AxisMotion motion;
    float command = 0;      // joystick deflection, Manual control only
    float lastSent = -1;    // last position sent to the motor driver
    bool held = false;      // the servo is being driven
  };

  AxisState& state(Axis axis) { return axes_[static_cast<size_t>(axis)]; }
  const AxisState& state(Axis axis) const { return axes_[static_cast<size_t>(axis)]; }
  void drive(Axis axis);
  void release(Axis axis);
  MotionLimits motionLimits(Axis axis) const;
  float stowTarget(Axis axis) const;
  bool atStow() const;

  MotorDriver& motors_;
  Clock& clock_;
  Storage& storage_;

  RobotSettings settings_ = defaultSettings();
  AxisState axes_[kAxisCount];
  Activity activity_ = Activity::Idle;
  uint32_t lastTickMs_ = 0;
  uint32_t lastManualMs_ = 0;
};

}  // namespace cwc
