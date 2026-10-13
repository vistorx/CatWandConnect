#include "robot_core/robot.h"

namespace cwc {

namespace {

// Longest step simulated in one tick, so a stalled loop never turns into a leap.
constexpr float kMaxStepSeconds = 0.1f;

constexpr const char* kRestKey = "rest";
constexpr uint32_t kRestMagic = 0x52455354;  // "REST"

// Where the wand last came to rest, so power-up starts there.
struct RestRecord {
  uint32_t magic;
  float position[kAxisCount];
};

float unit(float value) { return value < -1 ? -1 : value > 1 ? 1 : value; }

constexpr size_t kSweepPoints = 6;

// The sweep visits the four corners of the new envelope, then the Stow position.
float sweepTarget(const RobotSettings& draft, Axis axis, size_t point) {
  const AxisSettings& a = draft.axis(axis);
  if (point >= kSweepPoints - 1) return a.stow;
  // Corners in order: (min,min) (max,min) (max,max) (min,max) (min,min)
  static constexpr bool kHorizontalMax[] = {false, true, true, false, false};
  static constexpr bool kVerticalMax[] = {false, false, true, true, false};
  bool max = axis == Axis::Horizontal ? kHorizontalMax[point] : kVerticalMax[point];
  return max ? a.travel.max : a.travel.min;
}

bool nudgeable(CalibrationStep step, Axis axis) {
  switch (step) {
    case CalibrationStep::HorizontalMin:
    case CalibrationStep::HorizontalMax:
      return axis == Axis::Horizontal;
    case CalibrationStep::VerticalMin:
    case CalibrationStep::VerticalMax:
      return axis == Axis::Vertical;
    case CalibrationStep::Stow:
      return true;
    default:
      return false;
  }
}

}  // namespace

Robot::Robot(MotorDriver& motors, Clock& clock, Storage& storage)
    : motors_(motors), clock_(clock), storage_(storage) {}

void Robot::begin() {
  settings_ = loadSettings(storage_);
  lastTickMs_ = clock_.nowMs();

  RestRecord rest{};
  bool remembered = storage_.read(kRestKey, &rest, sizeof(rest)) && rest.magic == kRestMagic;
  for (Axis axis : kAxes) {
    float position = rest.position[static_cast<size_t>(axis)];
    remembered = remembered && kServoRange.contains(position);  // also rejects NaN
  }

  for (Axis axis : kAxes) {
    float start = remembered ? rest.position[static_cast<size_t>(axis)] : stowTarget(axis);
    start = travelLimits(axis).clamp(start);
    state(axis).motion.reset(start);
    savedRest_[static_cast<size_t>(axis)] = remembered ? rest.position[static_cast<size_t>(axis)] : -1;
    drive(axis);
  }
  stow();
  gentleStow_ = true;
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
        if (s.hasGoto) {
          MotionLimits gotoLimits{limits.maxSpeed * settings_.playSpeed, limits.maxAccel};
          s.motion.stepToward(s.gotoTarget, dt, travelLimits(axis), gotoLimits);
          if (s.motion.atRest() && s.motion.position() == travelLimits(axis).clamp(s.gotoTarget)) s.hasGoto = false;
        } else {
          s.motion.stepVelocity(s.command * limits.maxSpeed * settings_.playSpeed, dt, travelLimits(axis), limits);
        }
        break;
      case Activity::Stowing:
        if (gentleStow_) limits.maxSpeed = kMinSpeed;
        s.motion.stepToward(stowTarget(axis), dt, travelLimits(axis), limits);
        break;
      case Activity::Calibrating:
        stepCalibration(axis, dt);
        break;
      case Activity::Idle:
        break;
    }
    if (activity_ != Activity::Idle) drive(axis);
    moving = moving || s.command != 0 || s.hasGoto || !s.motion.atRest();
  }

  if (activity_ == Activity::Manual && !moving) {
    activity_ = Activity::Idle;
    rememberRestingPosition();
  }
  if (calibrationStep_ == CalibrationStep::Sweep) {
    bool reached = true;
    for (Axis axis : kAxes) {
      const AxisMotion& motion = state(axis).motion;
      reached = reached && motion.atRest() && motion.position() == sweepTarget(draft_, axis, sweepPoint_);
    }
    if (reached && ++sweepPoint_ == kSweepPoints) calibrationStep_ = CalibrationStep::Confirm;
  }
  if (activity_ == Activity::Stowing && atStow()) {
    for (Axis axis : kAxes) release(axis);
    activity_ = Activity::Idle;
    gentleStow_ = false;
    rememberRestingPosition();
  }
}

void Robot::manualMove(float horizontal, float vertical) {
  if (activity_ == Activity::Stowing) return;
  endCalibration();
  for (Axis axis : kAxes) state(axis).hasGoto = false;
  state(Axis::Horizontal).command = unit(horizontal);
  state(Axis::Vertical).command = unit(vertical);
  activity_ = Activity::Manual;
  lastManualMs_ = clock_.nowMs();
}

void Robot::manualGoto(Axis axis, float degrees) {
  if (activity_ == Activity::Stowing || !kServoRange.contains(degrees)) return;
  if (activity_ != Activity::Manual) {
    endCalibration();
    for (Axis a : kAxes) state(a).command = 0;
  }
  AxisState& s = state(axis);
  s.command = 0;
  s.hasGoto = true;
  s.gotoTarget = degrees;
  activity_ = Activity::Manual;
  lastManualMs_ = clock_.nowMs();
}

void Robot::manualStop() {
  for (Axis axis : kAxes) {
    state(axis).command = 0;
    state(axis).hasGoto = false;
  }
}

void Robot::stow() {
  manualStop();
  endCalibration();
  activity_ = Activity::Stowing;
  gentleStow_ = false;
}

void Robot::startCalibration() {
  if (activity_ == Activity::Stowing) return;
  manualStop();
  draft_ = settings_;
  for (Axis axis : kAxes) {
    draft_.axis(axis).travel = travelLimits(axis);
    nudgeTarget_[static_cast<size_t>(axis)] = position(axis);
  }
  enterCalibrationStep(CalibrationStep::HorizontalMin);
  activity_ = Activity::Calibrating;
}

// Moves the step's axes to the value the step is about to set, so the owner
// starts from the current limits (known safe) wherever the wand was left.
void Robot::enterCalibrationStep(CalibrationStep step) {
  calibrationStep_ = step;
  const AxisSettings& horizontal = draft_.axis(Axis::Horizontal);
  const AxisSettings& vertical = draft_.axis(Axis::Vertical);
  float* h = &nudgeTarget_[static_cast<size_t>(Axis::Horizontal)];
  float* v = &nudgeTarget_[static_cast<size_t>(Axis::Vertical)];
  switch (step) {
    case CalibrationStep::HorizontalMin: *h = horizontal.travel.min; break;
    case CalibrationStep::HorizontalMax: *h = horizontal.travel.max; break;
    case CalibrationStep::VerticalMin: *v = vertical.travel.min; break;
    case CalibrationStep::VerticalMax: *v = vertical.travel.max; break;
    case CalibrationStep::Stow:
      *h = horizontal.travel.clamp(horizontal.stow);
      *v = vertical.travel.clamp(vertical.stow);
      break;
    default: break;
  }
}

bool Robot::reverseAxis(Axis axis) {
  if (activity_ != Activity::Calibrating) return false;
  bool horizontal = calibrationStep_ == CalibrationStep::HorizontalMin ||
                    calibrationStep_ == CalibrationStep::HorizontalMax;
  bool vertical = calibrationStep_ == CalibrationStep::VerticalMin ||
                  calibrationStep_ == CalibrationStep::VerticalMax;
  if (!(axis == Axis::Horizontal ? horizontal : vertical)) return false;

  auto mirror = [](float degrees) { return kServoRange.max - degrees; };
  auto mirrorAxis = [&](AxisSettings& a) {
    TravelLimits travel{mirror(a.travel.max), mirror(a.travel.min)};
    a.travel = travel;
    a.stow = mirror(a.stow);
  };

  motors_.reverse(axis);
  AxisState& s = state(axis);
  float position = mirror(s.motion.position());
  s.motion.reset(position);
  s.lastSent = position;  // same pulse as before: nothing to send
  size_t i = static_cast<size_t>(axis);
  nudgeTarget_[i] = position;

  mirrorAxis(draft_.axis(axis));
  RobotSettings saved = settings_;
  mirrorAxis(saved.axis(axis));
  applySettings(saved);

  if (savedRest_[i] >= 0) {
    RestRecord rest{kRestMagic, {}};
    for (size_t j = 0; j < kAxisCount; ++j) rest.position[j] = savedRest_[j];
    rest.position[i] = mirror(savedRest_[i]);
    if (storage_.write(kRestKey, &rest, sizeof(rest))) savedRest_[i] = rest.position[i];
  }

  enterCalibrationStep(axis == Axis::Horizontal ? CalibrationStep::HorizontalMin : CalibrationStep::VerticalMin);
  return true;
}

void Robot::nudge(Axis axis, float degrees) {
  if (activity_ != Activity::Calibrating || !nudgeable(calibrationStep_, axis)) return;
  float& target = nudgeTarget_[static_cast<size_t>(axis)];
  target = kServoRange.clamp(target + degrees);
}

SettingsCheck Robot::markCalibration() {
  if (activity_ != Activity::Calibrating) return {};
  float h = position(Axis::Horizontal);
  float v = position(Axis::Vertical);
  AxisSettings& horizontal = draft_.axis(Axis::Horizontal);
  AxisSettings& vertical = draft_.axis(Axis::Vertical);

  switch (calibrationStep_) {
    case CalibrationStep::HorizontalMin:
      horizontal.travel.min = h;
      enterCalibrationStep(CalibrationStep::HorizontalMax);
      break;
    case CalibrationStep::HorizontalMax:
      if (h <= horizontal.travel.min) return {SettingsError::TravelOrder, Axis::Horizontal};
      horizontal.travel.max = h;
      enterCalibrationStep(CalibrationStep::VerticalMin);
      break;
    case CalibrationStep::VerticalMin:
      vertical.travel.min = v;
      enterCalibrationStep(CalibrationStep::VerticalMax);
      break;
    case CalibrationStep::VerticalMax:
      if (v <= vertical.travel.min) return {SettingsError::TravelOrder, Axis::Vertical};
      vertical.travel.max = v;
      enterCalibrationStep(CalibrationStep::Stow);
      break;
    case CalibrationStep::Stow:
      if (!horizontal.travel.contains(h)) return {SettingsError::StowOutOfTravel, Axis::Horizontal};
      if (!vertical.travel.contains(v)) return {SettingsError::StowOutOfTravel, Axis::Vertical};
      horizontal.stow = h;
      vertical.stow = v;
      sweepPoint_ = 0;
      calibrationStep_ = CalibrationStep::Sweep;
      break;
    default:
      break;
  }
  return {};
}

void Robot::confirmCalibration() {
  if (calibrationStep_ != CalibrationStep::Confirm) return;
  RobotSettings confirmed = draft_;
  confirmed.calibrated = true;
  applySettings(confirmed);  // validated: an invalid draft leaves the old settings
  stow();
}

void Robot::cancelCalibration() {
  if (activity_ != Activity::Calibrating) return;
  stow();
}

void Robot::endCalibration() { calibrationStep_ = CalibrationStep::None; }

void Robot::stepCalibration(Axis axis, float dt) {
  MotionLimits limits = motionLimits(axis);
  if (limits.maxSpeed > kCalibrationSpeed) limits.maxSpeed = kCalibrationSpeed;
  float target = calibrationStep_ == CalibrationStep::Sweep || calibrationStep_ == CalibrationStep::Confirm
                     ? sweepTarget(draft_, axis, sweepPoint_)
                     : nudgeTarget_[static_cast<size_t>(axis)];
  state(axis).motion.stepToward(target, dt, kServoRange, limits);
}

float Robot::position(Axis axis) const { return state(axis).motion.position(); }

SettingsCheck Robot::applySettings(const RobotSettings& settings) {
  SettingsCheck check = validate(settings);
  if (!check.ok()) return check;
  settings_ = settings;
  saveSettings(storage_, settings_);
  return check;
}

void Robot::invalidateCalibration() {
  if (!settings_.calibrated) return;
  settings_.calibrated = false;
  saveSettings(storage_, settings_);
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

// Called only when the robot becomes idle, and writes only when the position
// changed, to spare the flash.
void Robot::rememberRestingPosition() {
  RestRecord rest{kRestMagic, {}};
  bool changed = false;
  for (Axis axis : kAxes) {
    size_t i = static_cast<size_t>(axis);
    rest.position[i] = position(axis);
    changed = changed || rest.position[i] != savedRest_[i];
  }
  if (!changed) return;
  if (storage_.write(kRestKey, &rest, sizeof(rest))) {
    for (size_t i = 0; i < kAxisCount; ++i) savedRest_[i] = rest.position[i];
  }
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
