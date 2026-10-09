#include "robot_core/robot.h"

namespace cwc {

namespace {

constexpr Axis kAxes[] = {Axis::Horizontal, Axis::Vertical};

// Longest step simulated in one tick, so a stalled loop never turns into a leap.
constexpr float kMaxStepSeconds = 0.1f;

float unit(float value) { return value < -1 ? -1 : value > 1 ? 1 : value; }

}  // namespace

Robot::Robot(MotorDriver& motors, Clock& clock, Storage& storage)
    : motors_(motors), clock_(clock), storage_(storage) {}

void Robot::begin() {
  lastTickMs_ = clock_.nowMs();
  for (Axis axis : kAxes) {
    state(axis).motion.reset(kDefaultStow);
    drive(axis);
  }
}

void Robot::tick() {
  uint32_t now = clock_.nowMs();
  float dt = (now - lastTickMs_) / 1000.0f;
  if (dt > kMaxStepSeconds) dt = kMaxStepSeconds;
  lastTickMs_ = now;
  if (dt <= 0) return;

  if (activity_ == Activity::Manual && now - lastManualMs_ > kManualTimeoutMs) {
    for (Axis axis : kAxes) state(axis).command = 0;
  }

  bool moving = false;
  for (Axis axis : kAxes) {
    AxisState& s = state(axis);
    if (activity_ == Activity::Manual) {
      s.motion.stepVelocity(s.command * kDefaultMotion.maxSpeed, dt, kDefaultTravel, kDefaultMotion);
    }
    drive(axis);
    moving = moving || s.command != 0 || !s.motion.atRest();
  }

  if (activity_ == Activity::Manual && !moving) activity_ = Activity::Idle;
}

void Robot::manualMove(float horizontal, float vertical) {
  state(Axis::Horizontal).command = unit(horizontal);
  state(Axis::Vertical).command = unit(vertical);
  activity_ = Activity::Manual;
  lastManualMs_ = clock_.nowMs();
}

void Robot::manualStop() {
  for (Axis axis : kAxes) state(axis).command = 0;
}

float Robot::position(Axis axis) const { return state(axis).motion.position(); }

TravelLimits Robot::travelLimits(Axis) const { return kDefaultTravel; }

void Robot::drive(Axis axis) {
  AxisState& s = state(axis);
  float position = s.motion.position();
  if (position == s.lastSent) return;
  motors_.moveTo(axis, position);
  s.lastSent = position;
}

}  // namespace cwc
