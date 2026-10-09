#pragma once

// Kinematics of one Axis: position and velocity in degrees, moved within
// Travel limits, a maximum speed and a maximum acceleration.

namespace cwc {

struct TravelLimits {
  float min;
  float max;

  float clamp(float degrees) const { return degrees < min ? min : degrees > max ? max : degrees; }
  bool contains(float degrees) const { return degrees >= min && degrees <= max; }
};

struct MotionLimits {
  float maxSpeed;  // °/s
  float maxAccel;  // °/s²
};

class AxisMotion {
 public:
  explicit AxisMotion(float position = 0) : position_(position) {}

  // Places the axis without moving it, e.g. at power-up.
  void reset(float position);

  // Advances `dt` seconds toward `velocity` (°/s). Brakes so it never crosses
  // a Travel limit; an axis already outside them may only move back inside.
  void stepVelocity(float velocity, float dt, const TravelLimits& travel, const MotionLimits& motion);

  // Advances `dt` seconds toward `target`, clamped to the Travel limits, and
  // lands on it exactly.
  void stepToward(float target, float dt, const TravelLimits& travel, const MotionLimits& motion);

  float position() const { return position_; }
  float velocity() const { return velocity_; }
  bool atRest() const { return velocity_ == 0; }

 private:
  float position_;
  float velocity_ = 0;
};

}  // namespace cwc
