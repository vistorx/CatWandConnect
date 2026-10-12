#pragma once

#include "robot_core/motion.h"
#include "robot_core/ports.h"
#include "robot_core/settings.h"

namespace cwc {

enum class Activity : uint8_t {
  Idle,
  Manual,   // a person is moving the wand
  Stowing,  // moving to the Stow position, then releasing the servos
  Calibrating,
};

// Calibration walks the owner through these steps in order. The owner moves
// the axis with small nudges and marks the current position at each step,
// then watches a slow sweep of the whole envelope and confirms.
enum class CalibrationStep : uint8_t {
  None,           // not calibrating
  HorizontalMin,
  HorizontalMax,
  VerticalMin,
  VerticalMax,
  Stow,           // both axes nudged to the rest position
  Sweep,          // the robot sweeps the new envelope by itself
  Confirm,        // waiting for the owner to confirm nothing touched
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

  // Power-up: loads the settings, commands the remembered resting position
  // (clamped into the Travel limits) so the servos don't jump, then Stows at
  // the slowest speed. Without a valid remembered position, goes straight to
  // Stow.
  void begin();

  // Advances the robot by one control step. Call it often from the main loop.
  void tick();

  // Manual control with joystick deflections in [-1, 1] per Axis: the wand
  // moves at that fraction of its maximum speed. Repeat at least every
  // kManualTimeoutMs while the joystick is held. Ignored while stowing.
  void manualMove(float horizontal, float vertical);

  // Manual control by position, e.g. from Home Assistant: moves the axis to
  // `degrees` (clamped into the Travel limits) at the Manual control speed,
  // then stops. Ignored while stowing; the joystick or Stow cancel it.
  void manualGoto(Axis axis, float degrees);

  // Brings Manual control to a stop, e.g. when the joystick is released.
  void manualStop();

  // Stops everything else and moves the wand to the Stow position, where the
  // servos are released.
  void stow();

  // --- Calibration ---------------------------------------------------------
  // While calibrating, the axes may go anywhere in the servo range, slowly,
  // since the Travel limits are what is being set. Nothing is saved until
  // confirmCalibration(); Stow, Manual control or cancel keep the previous
  // settings.

  static constexpr float kCalibrationSpeed = 20;  // °/s

  void startCalibration();

  // Moves the axis of the current step by `degrees`. In the Stow step both
  // axes can be nudged. Ignored outside those steps.
  void nudge(Axis axis, float degrees);

  // Records the current position for the current step and moves to the next.
  // Rejected (the step stays) when the maximum is not above the minimum, or
  // the Stow position is outside the new Travel limits.
  SettingsCheck markCalibration();

  // In the Confirm step: saves the new Travel limits and Stow position, marks
  // the robot calibrated and Stows.
  void confirmCalibration();

  // Abandons Calibration, keeping the previous settings, and Stows.
  void cancelCalibration();

  CalibrationStep calibrationStep() const { return calibrationStep_; }

  // Travel limits and Stow position recorded so far.
  const RobotSettings& calibrationDraft() const { return draft_; }

  // Validates, stores and applies new settings as a whole. Invalid settings
  // are rejected and the current ones stay in effect.
  SettingsCheck applySettings(const RobotSettings& settings);

  // Forgets Calibration, e.g. after the servo direction or pulse range
  // changed: movement goes back to the conservative defaults.
  void invalidateCalibration();

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
    bool hasGoto = false;   // Manual control by position
    float gotoTarget = 0;
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
  void rememberRestingPosition();
  void stepCalibration(Axis axis, float dt);
  void enterCalibrationStep(CalibrationStep step);
  void endCalibration();

  MotorDriver& motors_;
  Clock& clock_;
  Storage& storage_;

  RobotSettings settings_ = defaultSettings();
  AxisState axes_[kAxisCount];
  Activity activity_ = Activity::Idle;
  bool gentleStow_ = false;  // power-up Stow, at kMinSpeed
  float savedRest_[kAxisCount] = {-1, -1};

  CalibrationStep calibrationStep_ = CalibrationStep::None;
  RobotSettings draft_ = defaultSettings();
  float nudgeTarget_[kAxisCount] = {};
  size_t sweepPoint_ = 0;
  uint32_t lastTickMs_ = 0;
  uint32_t lastManualMs_ = 0;
};

}  // namespace cwc
