#pragma once

#include "robot_core/motion.h"
#include "robot_core/ports.h"

namespace cwc {

enum class Activity : uint8_t { Idle, Manual };

// Owns all safety-critical behaviour of CatWandConnect: Travel limits, speed,
// Stow priority, power-up and Calibration. Every command source (Device
// interface, Home Assistant, ...) goes through it.
class Robot {
 public:
  // Manual control stops when no command refreshes it for this long.
  static constexpr uint32_t kManualTimeoutMs = 500;

  // Conservative envelope used until Calibration has been completed.
  static constexpr TravelLimits kDefaultTravel{60, 120};
  static constexpr float kDefaultStow = 90;
  static constexpr MotionLimits kDefaultMotion{90, 360};

  Robot(MotorDriver& motors, Clock& clock, Storage& storage);

  // Puts the wand at its Stow position and holds it.
  void begin();

  // Advances the robot by one control step. Call it often from the main loop.
  void tick();

  // Manual control with joystick deflections in [-1, 1] per Axis: the wand
  // moves at that fraction of its maximum speed. Repeat at least every
  // kManualTimeoutMs while the joystick is held.
  void manualMove(float horizontal, float vertical);

  // Brings Manual control to a stop, e.g. when the joystick is released.
  void manualStop();

  Activity activity() const { return activity_; }
  float position(Axis axis) const;
  TravelLimits travelLimits(Axis axis) const;

 private:
  struct AxisState {
    AxisMotion motion;
    float command = 0;      // joystick deflection, Manual control only
    float lastSent = -1;    // last position sent to the motor driver
  };

  AxisState& state(Axis axis) { return axes_[static_cast<size_t>(axis)]; }
  const AxisState& state(Axis axis) const { return axes_[static_cast<size_t>(axis)]; }
  void drive(Axis axis);

  MotorDriver& motors_;
  Clock& clock_;
  Storage& storage_;

  AxisState axes_[kAxisCount];
  Activity activity_ = Activity::Idle;
  uint32_t lastTickMs_ = 0;
  uint32_t lastManualMs_ = 0;
};

}  // namespace cwc
