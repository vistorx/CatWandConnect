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
  TuningSpeed,  // sweeping one axis while the owner adjusts its speed
  Playing,      // a Play session: each axis moves and pauses on its own
};

// Which end of an axis's Speed range Speed tuning adjusts.
enum class SpeedBound : uint8_t { Minimum, Maximum };

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

  Robot(MotorDriver& motors, Clock& clock, Storage& storage, Random& random);

  // Power-up: loads the settings, commands the remembered resting position
  // (clamped into the Travel limits) so the servos don't jump, then Stows at
  // the slowest speed. Without a valid remembered position, goes straight to
  // Stow.
  void begin();

  // Advances the robot by one control step. Call it often from the main loop.
  void tick();

  // Manual control with joystick deflections in [-1, 1] per Axis: the wand
  // moves at that fraction of the speed slider's speed (manualSpeed()). Repeat at least every
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

  // On the axis's minimum or maximum step: reverses the axis's direction
  // without moving the wand. Every angle of that axis is mirrored
  // (a -> 180 - a): position, draft, saved settings and resting position;
  // Calibration is kept, and the axis starts over at its minimum step. The
  // caller saves the wiring's new direction. False when not offered.
  bool reverseAxis(Axis axis);

  CalibrationStep calibrationStep() const { return calibrationStep_; }

  // Travel limits and Stow position recorded so far.
  const RobotSettings& calibrationDraft() const { return draft_; }

  // --- Speed tuning ----------------------------------------------------------
  // The robot sweeps one axis between its Travel limits at a trial speed while
  // the other axis holds still; the owner makes it faster or slower, then
  // keeps the speed as the axis's minimum or maximum. Nothing is saved until
  // keepSpeedTuning(); Stow, Manual control or cancel keep the previous value.

  // Starts from the axis's current value of `bound`. Refused before
  // Calibration (the sweep needs safe Travel limits) and while stowing or
  // calibrating.
  bool startSpeedTuning(Axis axis, SpeedBound bound);

  // One step faster or slower: 2 °/s below 30 °/s, 5 °/s up to 90 °/s,
  // 10 °/s above, within the range allowed for the bound. In Acceleration
  // tuning, one step snappier or softer: 20 °/s² below 200 °/s², 50 °/s² up
  // to 500 °/s², 100 °/s² above, within kMinAccel..kMaxAccel.
  void adjustSpeedTuning(bool faster);

  // Saves the trial speed through the settings validation, then Stows.
  // Keeping a maximum at or below the minimum resets the minimum to its
  // default for that maximum; a minimum not below the maximum is refused and
  // the tuning goes on.
  SettingsCheck keepSpeedTuning();

  // Abandons Speed tuning, keeping the previous value, and Stows.
  void cancelSpeedTuning();

  Axis tuningAxis() const { return tuningAxis_; }
  SpeedBound tuningBound() const { return tuningBound_; }
  float tuningSpeed() const { return tuningSpeed_; }

  // --- Acceleration tuning -----------------------------------------------------
  // The Speed tuning variant for an axis's maximum acceleration: the axis
  // makes short back-and-forth moves across the middle half of its Travel
  // limits, each starting from a stop and run at the axis's maximum speed, so
  // the owner feels the start and the stop. adjustSpeedTuning() makes it
  // snappier or softer, keepSpeedTuning() saves it as maxAccel, and cancel,
  // Stow or Manual control keep the previous value, as for Speed tuning.

  // Starts from the axis's current maxAccel. Refused when startSpeedTuning()
  // would be.
  bool startAccelTuning(Axis axis);

  // True while the running Speed tuning adjusts the acceleration.
  bool tuningAccel() const { return tuningAccel_; }
  float tuningAccelValue() const { return tuningAccelValue_; }

  // --- Play session ------------------------------------------------------------
  // Each Axis loops on its own, never synchronised with the other: move to a
  // random position within its Travel limits at a random speed within its
  // Speed range, pause for a random time within its Pause range, repeat.
  // Time up, stopPlaySession() and Stow end it with Stow; Manual control ends
  // it at once, without Stow. It never resumes and is not kept across restarts.

  // Starts a session of `minutes` (from kMinSessionMinutes to
  // kMaxSessionMinutes), from zero even if one is running. Refused before
  // Calibration and while stowing, calibrating or tuning a speed.
  bool startPlaySession(uint8_t minutes);

  // Who to tell about Play session events (PlayEvent): starts, refusals
  // (busy or not calibrated; a duration out of range is not one), the end of
  // the Session duration and every early stop with its cause. Starting again
  // while playing is one more start. nullptr: nobody.
  void setPlayObserver(PlayObserver* observer) { observer_ = observer; }

  // Sets the Session duration (kMinSessionMinutes to kMaxSessionMinutes)
  // without starting a session. Like startPlaySession(), it is written to
  // flash only once the robot is idle, and only when it changed.
  bool setSessionMinutes(uint8_t minutes);

  // Ends the session with Stow. Ignored when no session runs.
  void stopPlaySession();

  // Seconds left in the session, rounded up; 0 when no session runs.
  uint32_t playRemainingS() const;

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
    // Play session
    bool pausing = false;
    float playTarget = 0;
    float playMoveSpeed = 0;
    uint32_t pauseEndMs = 0;
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
  void stepSpeedTuning(Axis axis, float dt);
  void stepPlay(Axis axis, float dt, uint32_t now);
  void drawMove(Axis axis);
  float draw(float low, float high);
  void save();
  void report(PlayEvent event, PlayCause cause);
  void leavePlay(PlayCause cause);  // reports Stopped when a session runs
  void stowNow();

  MotorDriver& motors_;
  Clock& clock_;
  Storage& storage_;
  Random& random_;
  PlayObserver* observer_ = nullptr;

  RobotSettings settings_ = defaultSettings();
  AxisState axes_[kAxisCount];
  Activity activity_ = Activity::Idle;
  bool gentleStow_ = false;  // power-up Stow, at kMinSpeed
  float savedRest_[kAxisCount] = {-1, -1};

  CalibrationStep calibrationStep_ = CalibrationStep::None;
  RobotSettings draft_ = defaultSettings();
  float nudgeTarget_[kAxisCount] = {};
  size_t sweepPoint_ = 0;

  Axis tuningAxis_ = Axis::Horizontal;
  SpeedBound tuningBound_ = SpeedBound::Maximum;
  float tuningSpeed_ = 0;
  bool tuningTowardMax_ = true;
  bool tuningAccel_ = false;
  float tuningAccelValue_ = 0;
  float tuningHoldS_ = 0;  // Acceleration tuning: pause left at each end
  uint32_t playEndMs_ = 0;
  uint8_t savedSessionMinutes_ = 0;  // as last written to flash
  uint32_t lastTickMs_ = 0;
  uint32_t lastManualMs_ = 0;
};

}  // namespace cwc
