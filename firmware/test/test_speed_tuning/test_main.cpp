#include <robot_core/robot.h>
#include <unity.h>

#include "../fakes.h"

using namespace cwc;
using namespace cwc::test;

namespace {

struct Rig {
  FakeMotorDriver motors;
  FakeClock clock;
  FakeStorage storage;
  FakeRandom random;
  Robot robot{motors, clock, storage, random};

  // A calibrated robot: Horizontal 40–140, Vertical 60–120, Stow 90/90,
  // maximum speeds 90 °/s, minimum speeds 9 °/s.
  explicit Rig(bool calibrated = true) {
    RobotSettings s = defaultSettings();
    s.axis(Axis::Horizontal).travel = {40, 140};
    s.calibrated = calibrated;
    saveSettings(storage, s);
    robot.begin();
    run(15000);
  }

  void run(uint32_t ms) {
    for (uint32_t t = 0; t < ms; t += 10) {
      clock.advance(10);
      robot.tick();
    }
  }

  // Runs `ms` and returns the lowest and highest positions the axis reached.
  TravelLimits watch(Axis axis, uint32_t ms) {
    TravelLimits seen{robot.position(axis), robot.position(axis)};
    for (uint32_t t = 0; t < ms; t += 10) {
      run(10);
      float p = robot.position(axis);
      if (p < seen.min) seen.min = p;
      if (p > seen.max) seen.max = p;
    }
    return seen;
  }

  // Largest distance the axis covers in one 10 ms tick over `ms`.
  float fastestStep(Axis axis, uint32_t ms) {
    float fastest = 0;
    float previous = robot.position(axis);
    for (uint32_t t = 0; t < ms; t += 10) {
      run(10);
      float step = robot.position(axis) - previous;
      if (step < 0) step = -step;
      if (step > fastest) fastest = step;
      previous = robot.position(axis);
    }
    return fastest;
  }
};

}  // namespace

void setUp() {}
void tearDown() {}

void test_not_offered_before_calibration() {
  Rig rig(false);
  TEST_ASSERT_FALSE(rig.robot.startSpeedTuning(Axis::Horizontal, SpeedBound::Maximum));
  TEST_ASSERT_TRUE(rig.robot.activity() == Activity::Idle);
}

void test_starts_from_the_current_value() {
  Rig rig;
  TEST_ASSERT_TRUE(rig.robot.startSpeedTuning(Axis::Horizontal, SpeedBound::Maximum));
  TEST_ASSERT_TRUE(rig.robot.activity() == Activity::TuningSpeed);
  TEST_ASSERT_TRUE(rig.robot.tuningAxis() == Axis::Horizontal);
  TEST_ASSERT_TRUE(rig.robot.tuningBound() == SpeedBound::Maximum);
  TEST_ASSERT_EQUAL_FLOAT(90, rig.robot.tuningSpeed());

  Rig other;
  other.robot.startSpeedTuning(Axis::Vertical, SpeedBound::Minimum);
  TEST_ASSERT_EQUAL_FLOAT(9, other.robot.tuningSpeed());
}

void test_sweeps_the_axis_between_its_travel_limits_and_holds_the_other() {
  Rig rig;
  rig.robot.startSpeedTuning(Axis::Horizontal, SpeedBound::Maximum);

  TravelLimits seen = rig.watch(Axis::Horizontal, 10000);
  TravelLimits other = rig.watch(Axis::Vertical, 3000);

  TEST_ASSERT_EQUAL_FLOAT(40, seen.min);
  TEST_ASSERT_EQUAL_FLOAT(140, seen.max);
  TEST_ASSERT_EQUAL_FLOAT(90, other.min);
  TEST_ASSERT_EQUAL_FLOAT(90, other.max);
  TEST_ASSERT_TRUE(rig.robot.held(Axis::Vertical));
}

void test_sweep_runs_at_the_trial_speed() {
  Rig rig;
  rig.robot.startSpeedTuning(Axis::Horizontal, SpeedBound::Minimum);  // 9 °/s
  // 9 °/s: at most 0.09° per 10 ms tick.
  TEST_ASSERT_TRUE(rig.fastestStep(Axis::Horizontal, 3000) <= 0.0901f);

  for (int i = 0; i < 5; ++i) rig.robot.adjustSpeedTuning(true);  // 9 → 19 °/s in 2 °/s steps
  TEST_ASSERT_EQUAL_FLOAT(19, rig.robot.tuningSpeed());
  float faster = rig.fastestStep(Axis::Horizontal, 8000);
  TEST_ASSERT_TRUE(faster > 0.15f);
  TEST_ASSERT_TRUE(faster <= rig.robot.tuningSpeed() / 100 + 0.0001f);
}

void test_steps_are_finer_at_low_speeds() {
  Rig rig;
  rig.robot.startSpeedTuning(Axis::Horizontal, SpeedBound::Maximum);  // 90
  rig.robot.adjustSpeedTuning(true);
  TEST_ASSERT_EQUAL_FLOAT(100, rig.robot.tuningSpeed());
  rig.robot.adjustSpeedTuning(false);
  TEST_ASSERT_EQUAL_FLOAT(90, rig.robot.tuningSpeed());
  rig.robot.adjustSpeedTuning(false);
  TEST_ASSERT_EQUAL_FLOAT(85, rig.robot.tuningSpeed());

  Rig slow;
  slow.robot.startSpeedTuning(Axis::Horizontal, SpeedBound::Minimum);  // 9
  slow.robot.adjustSpeedTuning(true);
  TEST_ASSERT_EQUAL_FLOAT(11, slow.robot.tuningSpeed());
}

void test_trial_speed_stays_within_the_allowed_range() {
  Rig rig;
  rig.robot.startSpeedTuning(Axis::Horizontal, SpeedBound::Minimum);
  for (int i = 0; i < 20; ++i) rig.robot.adjustSpeedTuning(false);
  TEST_ASSERT_EQUAL_FLOAT(kSlowestSpeed, rig.robot.tuningSpeed());

  Rig fast;
  fast.robot.startSpeedTuning(Axis::Horizontal, SpeedBound::Maximum);
  for (int i = 0; i < 100; ++i) fast.robot.adjustSpeedTuning(true);
  TEST_ASSERT_EQUAL_FLOAT(kMaxSpeed, fast.robot.tuningSpeed());
  for (int i = 0; i < 100; ++i) fast.robot.adjustSpeedTuning(false);
  TEST_ASSERT_EQUAL_FLOAT(kMinSpeed, fast.robot.tuningSpeed());
}

void test_nothing_is_saved_until_kept() {
  Rig rig;
  int writes = rig.storage.writes;
  rig.robot.startSpeedTuning(Axis::Horizontal, SpeedBound::Maximum);
  rig.robot.adjustSpeedTuning(true);
  rig.run(2000);
  TEST_ASSERT_EQUAL_FLOAT(90, rig.robot.settings().axis(Axis::Horizontal).maxSpeed);
  TEST_ASSERT_EQUAL(writes, rig.storage.writes);
}

void test_keep_saves_the_speed_and_stows() {
  Rig rig;
  rig.robot.startSpeedTuning(Axis::Vertical, SpeedBound::Maximum);
  rig.robot.adjustSpeedTuning(true);
  rig.robot.adjustSpeedTuning(true);

  TEST_ASSERT_TRUE(rig.robot.keepSpeedTuning().ok());

  TEST_ASSERT_EQUAL_FLOAT(110, rig.robot.settings().axis(Axis::Vertical).maxSpeed);
  TEST_ASSERT_EQUAL_FLOAT(110, loadSettings(rig.storage).axis(Axis::Vertical).maxSpeed);
  TEST_ASSERT_TRUE(rig.robot.activity() == Activity::Stowing);
}

void test_keeping_a_maximum_below_the_minimum_lowers_the_minimum() {
  Rig rig;
  RobotSettings s = rig.robot.settings();
  s.axis(Axis::Horizontal).minSpeed = 40;
  TEST_ASSERT_TRUE(rig.robot.applySettings(s).ok());
  rig.robot.startSpeedTuning(Axis::Horizontal, SpeedBound::Maximum);
  for (int i = 0; i < 12; ++i) rig.robot.adjustSpeedTuning(false);  // down to 30

  TEST_ASSERT_TRUE(rig.robot.keepSpeedTuning().ok());

  const AxisSettings& a = rig.robot.settings().axis(Axis::Horizontal);
  TEST_ASSERT_EQUAL_FLOAT(30, a.maxSpeed);
  TEST_ASSERT_EQUAL_FLOAT(3, a.minSpeed);
}

void test_keeping_a_minimum_not_below_the_maximum_is_refused() {
  Rig rig;
  RobotSettings s = rig.robot.settings();
  s.axis(Axis::Horizontal).maxSpeed = 20;
  TEST_ASSERT_TRUE(rig.robot.applySettings(s).ok());
  rig.robot.startSpeedTuning(Axis::Horizontal, SpeedBound::Minimum);  // 9
  for (int i = 0; i < 6; ++i) rig.robot.adjustSpeedTuning(true);       // 21

  SettingsCheck check = rig.robot.keepSpeedTuning();

  TEST_ASSERT_TRUE(check.error == SettingsError::MinSpeedOutOfRange);
  TEST_ASSERT_TRUE(rig.robot.activity() == Activity::TuningSpeed);
  TEST_ASSERT_EQUAL_FLOAT(9, rig.robot.settings().axis(Axis::Horizontal).minSpeed);
}

void test_cancel_keeps_the_previous_value_and_stows() {
  Rig rig;
  rig.robot.startSpeedTuning(Axis::Horizontal, SpeedBound::Maximum);
  rig.robot.adjustSpeedTuning(true);
  rig.run(1000);

  rig.robot.cancelSpeedTuning();

  TEST_ASSERT_EQUAL_FLOAT(90, rig.robot.settings().axis(Axis::Horizontal).maxSpeed);
  TEST_ASSERT_TRUE(rig.robot.activity() == Activity::Stowing);
  rig.run(15000);
  TEST_ASSERT_EQUAL_FLOAT(90, rig.robot.position(Axis::Horizontal));
}

void test_stow_and_joystick_end_it_without_saving() {
  Rig rig;
  rig.robot.startSpeedTuning(Axis::Horizontal, SpeedBound::Maximum);
  rig.robot.adjustSpeedTuning(true);
  rig.robot.stow();
  TEST_ASSERT_TRUE(rig.robot.activity() == Activity::Stowing);
  rig.run(15000);
  rig.robot.keepSpeedTuning();
  TEST_ASSERT_EQUAL_FLOAT(90, rig.robot.settings().axis(Axis::Horizontal).maxSpeed);

  rig.robot.startSpeedTuning(Axis::Horizontal, SpeedBound::Maximum);
  rig.robot.adjustSpeedTuning(true);
  rig.robot.manualMove(0.5f, 0);
  TEST_ASSERT_TRUE(rig.robot.activity() == Activity::Manual);
  rig.robot.keepSpeedTuning();
  TEST_ASSERT_EQUAL_FLOAT(90, rig.robot.settings().axis(Axis::Horizontal).maxSpeed);
}

void test_not_offered_while_stowing() {
  Rig rig;
  rig.robot.manualGoto(Axis::Horizontal, 130);
  rig.run(2000);
  rig.robot.stow();
  TEST_ASSERT_FALSE(rig.robot.startSpeedTuning(Axis::Horizontal, SpeedBound::Maximum));
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_not_offered_before_calibration);
  RUN_TEST(test_starts_from_the_current_value);
  RUN_TEST(test_sweeps_the_axis_between_its_travel_limits_and_holds_the_other);
  RUN_TEST(test_sweep_runs_at_the_trial_speed);
  RUN_TEST(test_steps_are_finer_at_low_speeds);
  RUN_TEST(test_trial_speed_stays_within_the_allowed_range);
  RUN_TEST(test_nothing_is_saved_until_kept);
  RUN_TEST(test_keep_saves_the_speed_and_stows);
  RUN_TEST(test_keeping_a_maximum_below_the_minimum_lowers_the_minimum);
  RUN_TEST(test_keeping_a_minimum_not_below_the_maximum_is_refused);
  RUN_TEST(test_cancel_keeps_the_previous_value_and_stows);
  RUN_TEST(test_stow_and_joystick_end_it_without_saving);
  RUN_TEST(test_not_offered_while_stowing);
  return UNITY_END();
}
