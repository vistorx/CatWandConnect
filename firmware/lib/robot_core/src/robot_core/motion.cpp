#include "robot_core/motion.h"

#include <cmath>

namespace cwc {

namespace {

float clampTo(float value, float low, float high) { return value < low ? low : value > high ? high : value; }

// Highest speed from which the axis can still stop within `distance`.
float stoppingSpeed(float distance, float maxAccel) {
  return distance <= 0 ? 0 : std::sqrt(2 * maxAccel * distance);
}

}  // namespace

void AxisMotion::reset(float position) {
  position_ = position;
  velocity_ = 0;
}

void AxisMotion::stepVelocity(float velocity, float dt, const TravelLimits& travel,
                              const MotionLimits& motion) {
  float desired = clampTo(velocity, -motion.maxSpeed, motion.maxSpeed);
  if (desired > 0) desired = std::fmin(desired, stoppingSpeed(travel.max - position_, motion.maxAccel));
  if (desired < 0) desired = std::fmax(desired, -stoppingSpeed(position_ - travel.min, motion.maxAccel));

  float maxChange = motion.maxAccel * dt;
  velocity_ += clampTo(desired - velocity_, -maxChange, maxChange);

  float previous = position_;
  position_ += velocity_ * dt;

  // Stop on a limit crossed during this step. Never snap an axis that started
  // outside the limits, as that would make the servo jump.
  if (previous <= travel.max && position_ > travel.max) {
    position_ = travel.max;
    velocity_ = 0;
  } else if (previous >= travel.min && position_ < travel.min) {
    position_ = travel.min;
    velocity_ = 0;
  }
}

void AxisMotion::stepToward(float target, float dt, const TravelLimits& travel, const MotionLimits& motion) {
  target = travel.clamp(target);
  float error = target - position_;
  if (error == 0 && velocity_ == 0) return;

  float speed = std::fmin(motion.maxSpeed, stoppingSpeed(std::fabs(error), motion.maxAccel));
  stepVelocity(error > 0 ? speed : -speed, dt, travel, motion);

  // Land on the target instead of oscillating around it.
  float remaining = target - position_;
  bool passed = (error > 0 && remaining <= 0) || (error < 0 && remaining >= 0);
  bool close = std::fabs(remaining) < 0.05f && std::fabs(velocity_) < motion.maxAccel * dt;
  if (passed || close) {
    position_ = target;
    velocity_ = 0;
  }
}

}  // namespace cwc
