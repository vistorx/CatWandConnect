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

// Speed tuning's step for the next change from `speed`: finer at low speeds.
float tuningStep(float speed, bool faster) {
  if (faster ? speed < 30 : speed <= 30) return 2;
  if (faster ? speed < 90 : speed <= 90) return 5;
  return 10;
}

// Acceleration tuning's step for the next change from `accel`.
float accelStep(float accel, bool faster) {
  if (faster ? accel < 200 : accel <= 200) return 20;
  if (faster ? accel < 500 : accel <= 500) return 50;
  return 100;
}

// Acceleration tuning pauses this long at each end, so every move starts from a stop.
constexpr float kAccelTuningHoldS = 0.6f;

}  // namespace

Robot::Robot(MotorDriver& motors, Clock& clock, Storage& storage, Random& random)
    : motors_(motors), clock_(clock), storage_(storage), random_(random) {}

void Robot::begin() {
  settings_ = loadSettings(storage_);
  savedSessionMinutes_ = settings_.sessionMinutes;
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

  if (activity_ == Activity::Playing && static_cast<int32_t>(now - playEndMs_) >= 0) stow();
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
          MotionLimits gotoLimits{manualSpeed(settings_.axis(axis), settings_.playSpeed), limits.maxAccel};
          s.motion.stepToward(s.gotoTarget, dt, travelLimits(axis), gotoLimits);
          if (s.motion.atRest() && s.motion.position() == travelLimits(axis).clamp(s.gotoTarget)) s.hasGoto = false;
        } else {
          s.motion.stepVelocity(s.command * manualSpeed(settings_.axis(axis), settings_.playSpeed), dt,
                                travelLimits(axis), limits);
        }
        break;
      case Activity::Stowing:
        if (gentleStow_) limits.maxSpeed = kMinSpeed;
        s.motion.stepToward(stowTarget(axis), dt, travelLimits(axis), limits);
        break;
      case Activity::Calibrating:
        stepCalibration(axis, dt);
        break;
      case Activity::TuningSpeed:
        stepSpeedTuning(axis, dt);
        break;
      case Activity::Playing:
        stepPlay(axis, dt, now);
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
  // The Session duration changes from play, not from Settings: write it
  // once the robot rests, to spare the flash.
  if (activity_ == Activity::Idle && settings_.sessionMinutes != savedSessionMinutes_) save();
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

bool Robot::startSpeedTuning(Axis axis, SpeedBound bound) {
  if (!settings_.calibrated || activity_ == Activity::Stowing || activity_ == Activity::Calibrating) return false;
  manualStop();
  const AxisSettings& a = settings_.axis(axis);
  tuningAxis_ = axis;
  tuningBound_ = bound;
  tuningSpeed_ = bound == SpeedBound::Maximum ? a.maxSpeed : a.minSpeed;
  tuningTowardMax_ = true;
  tuningAccel_ = false;
  activity_ = Activity::TuningSpeed;
  return true;
}

bool Robot::startAccelTuning(Axis axis) {
  if (!startSpeedTuning(axis, SpeedBound::Maximum)) return false;
  tuningAccel_ = true;
  tuningAccelValue_ = settings_.axis(axis).maxAccel;
  tuningHoldS_ = 0;
  return true;
}

void Robot::adjustSpeedTuning(bool faster) {
  if (activity_ != Activity::TuningSpeed) return;
  if (tuningAccel_) {
    float accel = tuningAccelValue_ + (faster ? 1 : -1) * accelStep(tuningAccelValue_, faster);
    tuningAccelValue_ = accel < kMinAccel ? kMinAccel : accel > kMaxAccel ? kMaxAccel : accel;
    return;
  }
  float step = tuningStep(tuningSpeed_, faster);
  float speed = tuningSpeed_ + (faster ? step : -step);
  float lowest = tuningBound_ == SpeedBound::Maximum ? kMinSpeed : kSlowestSpeed;
  tuningSpeed_ = speed < lowest ? lowest : speed > kMaxSpeed ? kMaxSpeed : speed;
}

SettingsCheck Robot::keepSpeedTuning() {
  if (activity_ != Activity::TuningSpeed) return {};
  RobotSettings kept = settings_;
  AxisSettings& a = kept.axis(tuningAxis_);
  if (tuningAccel_) {
    a.maxAccel = tuningAccelValue_;
  } else if (tuningBound_ == SpeedBound::Maximum) {
    a.maxSpeed = tuningSpeed_;
    if (a.minSpeed >= a.maxSpeed) a.minSpeed = defaultMinSpeed(a.maxSpeed);
  } else {
    a.minSpeed = tuningSpeed_;
  }
  SettingsCheck check = applySettings(kept);
  if (check.ok()) stow();
  return check;
}

void Robot::cancelSpeedTuning() {
  if (activity_ != Activity::TuningSpeed) return;
  stow();
}

void Robot::stepSpeedTuning(Axis axis, float dt) {
  if (axis != tuningAxis_) return;  // the other axis holds its position
  AxisMotion& motion = state(axis).motion;
  TravelLimits travel = travelLimits(axis);
  if (tuningAccel_) {
    if (tuningHoldS_ > 0) {
      tuningHoldS_ -= dt;
      return;
    }
    float quarter = (travel.max - travel.min) / 4;
    float target = tuningTowardMax_ ? travel.max - quarter : travel.min + quarter;
    motion.stepToward(target, dt, travel, {tuningSpeed_, tuningAccelValue_});
    if (motion.atRest() && motion.position() == target) {
      tuningTowardMax_ = !tuningTowardMax_;
      tuningHoldS_ = kAccelTuningHoldS;
    }
    return;
  }
  float target = tuningTowardMax_ ? travel.max : travel.min;
  motion.stepToward(target, dt, travel, {tuningSpeed_, settings_.axis(axis).maxAccel});
  if (motion.atRest() && motion.position() == target) tuningTowardMax_ = !tuningTowardMax_;
}

bool Robot::startPlaySession(uint8_t minutes) {
  if (minutes < kMinSessionMinutes || minutes > kMaxSessionMinutes) return false;
  if (!settings_.calibrated || activity_ == Activity::Stowing || activity_ == Activity::Calibrating ||
      activity_ == Activity::TuningSpeed) {
    return false;
  }
  manualStop();
  settings_.sessionMinutes = minutes;
  for (Axis axis : kAxes) drawMove(axis);
  playEndMs_ = clock_.nowMs() + minutes * 60000u;
  activity_ = Activity::Playing;
  return true;
}

bool Robot::setSessionMinutes(uint8_t minutes) {
  if (minutes < kMinSessionMinutes || minutes > kMaxSessionMinutes) return false;
  settings_.sessionMinutes = minutes;
  return true;
}

void Robot::stopPlaySession() {
  if (activity_ == Activity::Playing) stow();
}

uint32_t Robot::playRemainingS() const {
  if (activity_ != Activity::Playing) return 0;
  int32_t left = static_cast<int32_t>(playEndMs_ - clock_.nowMs());
  return left <= 0 ? 0 : (static_cast<uint32_t>(left) + 999) / 1000;
}

float Robot::draw(float low, float high) {
  return low + (high - low) * static_cast<float>(random_.next() / 4294967295.0);
}

void Robot::drawMove(Axis axis) {
  AxisState& s = state(axis);
  const AxisSettings& a = settings_.axis(axis);
  TravelLimits travel = travelLimits(axis);
  s.playTarget = travel.clamp(draw(travel.min, travel.max));
  s.playMoveSpeed = draw(a.minSpeed, a.maxSpeed);
  s.pausing = false;
}

void Robot::stepPlay(Axis axis, float dt, uint32_t now) {
  AxisState& s = state(axis);
  if (s.pausing) {
    if (static_cast<int32_t>(now - s.pauseEndMs) < 0) return;
    drawMove(axis);
  }
  s.motion.stepToward(s.playTarget, dt, travelLimits(axis), {s.playMoveSpeed, settings_.axis(axis).maxAccel});
  if (s.motion.atRest() && s.motion.position() == s.playTarget) {
    const AxisSettings& a = settings_.axis(axis);
    s.pauseEndMs = now + static_cast<uint32_t>(draw(a.pauseMin, a.pauseMax) * 1000 + 0.5f);
    s.pausing = true;
  }
}

float Robot::position(Axis axis) const { return state(axis).motion.position(); }

SettingsCheck Robot::applySettings(const RobotSettings& settings) {
  SettingsCheck check = validate(settings);
  if (!check.ok()) return check;
  settings_ = settings;
  save();
  return check;
}

void Robot::save() {
  if (saveSettings(storage_, settings_)) savedSessionMinutes_ = settings_.sessionMinutes;
}

void Robot::invalidateCalibration() {
  if (!settings_.calibrated) return;
  settings_.calibrated = false;
  save();
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
