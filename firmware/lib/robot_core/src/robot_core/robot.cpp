#include "robot_core/robot.h"

namespace cwc {

namespace {

// Longest step simulated in one tick, so a stalled loop never turns into a leap.
constexpr float kMaxStepSeconds = 0.1f;

float unit(float value) { return value < -1 ? -1 : value > 1 ? 1 : value; }

}  // namespace

Robot::Robot(MotorDriver& motors, Clock& clock, Storage& storage)
    : motors_(motors), clock_(clock), storage_(storage) {}

void Robot::begin() {
  settings_ = loadSettings(storage_);
  lastTickMs_ = clock_.nowMs();
  for (Axis axis : kAxes) {
    state(axis).motion.reset(stowTarget(axis));
    drive(axis);
  }
  stow();
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
    MotionLimits limits = motionLimits(axis);
    switch (activity_) {
      case Activity::Manual:
        s.motion.stepVelocity(s.command * limits.maxSpeed * settings_.playSpeed, dt, travelLimits(axis), limits);
        break;
      case Activity::Stowing:
        s.motion.stepToward(stowTarget(axis), dt, travelLimits(axis), limits);
        break;
      case Activity::Idle:
        break;
    }
    if (activity_ != Activity::Idle) drive(axis);
    moving = moving || s.command != 0 || !s.motion.atRest();
  }

  if (activity_ == Activity::Manual && !moving) activity_ = Activity::Idle;
  if (activity_ == Activity::Stowing && atStow()) {
    for (Axis axis : kAxes) release(axis);
    activity_ = Activity::Idle;
  }
}

void Robot::manualMove(float horizontal, float vertical) {
  if (activity_ == Activity::Stowing) return;
  state(Axis::Horizontal).command = unit(horizontal);
  state(Axis::Vertical).command = unit(vertical);
  activity_ = Activity::Manual;
  lastManualMs_ = clock_.nowMs();
}

void Robot::manualStop() {
  for (Axis axis : kAxes) state(axis).command = 0;
}

void Robot::stow() {
  manualStop();
  activity_ = Activity::Stowing;
}

float Robot::position(Axis axis) const { return state(axis).motion.position(); }

SettingsCheck Robot::applySettings(const RobotSettings& settings) {
  SettingsCheck check = validate(settings);
  if (!check.ok()) return check;
  settings_ = settings;
  saveSettings(storage_, settings_);
  return check;
}

TravelLimits Robot::travelLimits(Axis axis) const {
  return settings_.calibrated ? settings_.axis(axis).travel : kDefaultTravel;
}

MotionLimits Robot::motionLimits(Axis axis) const {
  const AxisSettings& a = settings_.axis(axis);
  return {a.maxSpeed, a.maxAccel};
}

float Robot::stowTarget(Axis axis) const { return travelLimits(axis).clamp(settings_.axis(axis).stow); }

bool Robot::atStow() const {
  for (Axis axis : kAxes) {
    const AxisMotion& motion = state(axis).motion;
    if (!motion.atRest() || motion.position() != stowTarget(axis)) return false;
  }
  return true;
}

void Robot::drive(Axis axis) {
  AxisState& s = state(axis);
  float position = s.motion.position();
  if (s.held && position == s.lastSent) return;
  motors_.moveTo(axis, position);
  s.lastSent = position;
  s.held = true;
}

void Robot::release(Axis axis) {
  AxisState& s = state(axis);
  if (!s.held) return;
  motors_.release(axis);
  s.held = false;
}

}  // namespace cwc
