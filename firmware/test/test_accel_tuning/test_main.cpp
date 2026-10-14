// Acceleration tuning: the Speed tuning variant that adjusts an axis's
// maximum acceleration with short back-and-forth moves at its maximum speed.
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
  // maximum speeds 90 °/s, acceleration 360 °/s².
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

  // Fastest speed (°/s) over `ms`, and the time (ms) the first move took to
  // reach it from a stop.
  void measure(Axis axis, uint32_t ms, float& topSpeed, uint32_t& rampMs) {
    topSpeed = 0;
    rampMs = 0;
    float previous = robot.position(axis);
    uint32_t started = 0;
    bool moving = false, ramped = false;
    for (uint32_t t = 0; t < ms; t += 10) {
      run(10);
      float v = (robot.position(axis) - previous) / 0.01f;
      previous = robot.position(axis);
      if (v < 0) v = -v;
      if (v > topSpeed) topSpeed = v;
      if (v == 0) {
        moving = false;
      } else if (!moving) {
        moving = true;
        started = t;
      }
      if (!ramped && moving && v >= 89 && started > 0) {
        rampMs = t - started;
        ramped = true;
      }
    }
  }

  // Longest time (ms) the axis stays still over `ms`.
  uint32_t longestStop(Axis axis, uint32_t ms) {
    uint32_t longest = 0, still = 0;
    float previous = robot.position(axis);
    for (uint32_t t = 0; t < ms; t += 10) {
      run(10);
      still = robot.position(axis) == previous ? still + 10 : 0;
      if (still > longest) longest = still;
      previous = robot.position(axis);
    }
    return longest;
  }
};

}  // namespace

void setUp() {}
void tearDown() {}

void test_not_offered_before_calibration() {
  Rig rig(false);
  TEST_ASSERT_FALSE(rig.robot.startAccelTuning(Axis::Horizontal));
  TEST_ASSERT_TRUE(rig.robot.activity() == Activity::Idle);
}

void test_not_offered_while_stowing() {
  Rig rig;
  rig.robot.manualGoto(Axis::Horizontal, 130);
  rig.run(2000);
  rig.robot.stow();
  TEST_ASSERT_FALSE(rig.robot.startAccelTuning(Axis::Horizontal));
}

void test_starts_from_the_current_value() {
  Rig rig;
  TEST_ASSERT_TRUE(rig.robot.startAccelTuning(Axis::Vertical));
  TEST_ASSERT_TRUE(rig.robot.activity() == Activity::TuningSpeed);
  TEST_ASSERT_TRUE(rig.robot.tuningAccel());
  TEST_ASSERT_TRUE(rig.robot.tuningAxis() == Axis::Vertical);
  TEST_ASSERT_EQUAL_FLOAT(360, rig.robot.tuningAccelValue());
  TEST_ASSERT_EQUAL_FLOAT(90, rig.robot.tuningSpeed());  // the axis's maximum speed
}

void test_speed_tuning_is_not_acceleration_tuning() {
  Rig rig;
  rig.robot.startAccelTuning(Axis::Horizontal);
  rig.robot.startSpeedTuning(Axis::Horizontal, SpeedBound::Maximum);
  TEST_ASSERT_FALSE(rig.robot.tuningAccel());
}

void test_moves_back_and_forth_within_the_travel_limits_and_holds_the_other() {
  Rig rig;
  rig.robot.startAccelTuning(Axis::Horizontal);

  TravelLimits seen = rig.watch(Axis::Horizontal, 6000);
  TravelLimits other = rig.watch(Axis::Vertical, 2000);

  // Short moves across the middle half of the Travel limits (40–140).
  TEST_ASSERT_EQUAL_FLOAT(65, seen.min);
  TEST_ASSERT_EQUAL_FLOAT(115, seen.max);
  TEST_ASSERT_EQUAL_FLOAT(90, other.min);
  TEST_ASSERT_EQUAL_FLOAT(90, other.max);
  TEST_ASSERT_TRUE(rig.robot.held(Axis::Vertical));
}

void test_each_move_starts_from_a_stop() {
  Rig rig;
  rig.robot.startAccelTuning(Axis::Horizontal);
  TEST_ASSERT_TRUE(rig.longestStop(Axis::Horizontal, 6000) >= 400);
}

void test_moves_reach_the_maximum_speed_at_the_trial_acceleration() {
  Rig rig;
  rig.robot.startAccelTuning(Axis::Horizontal);
  float speed;
  uint32_t ramp;
  rig.measure(Axis::Horizontal, 6000, speed, ramp);
  TEST_ASSERT_FLOAT_WITHIN(1, 90, speed);
  TEST_ASSERT_UINT32_WITHIN(30, 240, ramp);  // 90 °/s at 360 °/s²: 0.25 s

  for (int i = 0; i < 10; ++i) rig.robot.adjustSpeedTuning(true);  // snappier
  TEST_ASSERT_TRUE(rig.robot.tuningAccelValue() > 1000);
  rig.measure(Axis::Horizontal, 6000, speed, ramp);
  TEST_ASSERT_FLOAT_WITHIN(1, 90, speed);
  TEST_ASSERT_TRUE(ramp <= 100);
}

void test_steps_are_finer_at_low_accelerations() {
  Rig rig;
  rig.robot.startAccelTuning(Axis::Horizontal);  // 360
  rig.robot.adjustSpeedTuning(true);
  TEST_ASSERT_EQUAL_FLOAT(410, rig.robot.tuningAccelValue());
  rig.robot.adjustSpeedTuning(true);
  rig.robot.adjustSpeedTuning(true);
  TEST_ASSERT_EQUAL_FLOAT(510, rig.robot.tuningAccelValue());
  rig.robot.adjustSpeedTuning(true);
  TEST_ASSERT_EQUAL_FLOAT(610, rig.robot.tuningAccelValue());

  Rig soft;
  soft.robot.startAccelTuning(Axis::Horizontal);  // 360
  for (int i = 0; i < 4; ++i) soft.robot.adjustSpeedTuning(false);  // 310, 260, 210, 160
  TEST_ASSERT_EQUAL_FLOAT(160, soft.robot.tuningAccelValue());
  soft.robot.adjustSpeedTuning(false);
  TEST_ASSERT_EQUAL_FLOAT(140, soft.robot.tuningAccelValue());
}

void test_trial_acceleration_stays_within_bounds() {
  Rig rig;
  rig.robot.startAccelTuning(Axis::Horizontal);
  for (int i = 0; i < 100; ++i) rig.robot.adjustSpeedTuning(false);
  TEST_ASSERT_EQUAL_FLOAT(kMinAccel, rig.robot.tuningAccelValue());
  for (int i = 0; i < 100; ++i) rig.robot.adjustSpeedTuning(true);
  TEST_ASSERT_EQUAL_FLOAT(kMaxAccel, rig.robot.tuningAccelValue());
}

void test_nothing_is_saved_until_kept() {
  Rig rig;
  int writes = rig.storage.writes;
  rig.robot.startAccelTuning(Axis::Horizontal);
  rig.robot.adjustSpeedTuning(true);
  rig.run(2000);
  TEST_ASSERT_EQUAL_FLOAT(360, rig.robot.settings().axis(Axis::Horizontal).maxAccel);
  TEST_ASSERT_EQUAL(writes, rig.storage.writes);
}

void test_keep_saves_the_acceleration_only_and_stows() {
  Rig rig;
  rig.robot.startAccelTuning(Axis::Vertical);
  rig.robot.adjustSpeedTuning(false);  // 310

  TEST_ASSERT_TRUE(rig.robot.keepSpeedTuning().ok());

  const AxisSettings& v = rig.robot.settings().axis(Axis::Vertical);
  TEST_ASSERT_EQUAL_FLOAT(310, v.maxAccel);
  TEST_ASSERT_EQUAL_FLOAT(90, v.maxSpeed);
  TEST_ASSERT_EQUAL_FLOAT(9, v.minSpeed);
  TEST_ASSERT_EQUAL_FLOAT(360, rig.robot.settings().axis(Axis::Horizontal).maxAccel);
  TEST_ASSERT_EQUAL_FLOAT(310, loadSettings(rig.storage).axis(Axis::Vertical).maxAccel);
  TEST_ASSERT_TRUE(rig.robot.activity() == Activity::Stowing);
}

void test_cancel_stow_and_joystick_keep_the_previous_value() {
  Rig rig;
  rig.robot.startAccelTuning(Axis::Horizontal);
  rig.robot.adjustSpeedTuning(true);
  rig.run(1000);
  rig.robot.cancelSpeedTuning();
  TEST_ASSERT_TRUE(rig.robot.activity() == Activity::Stowing);
  rig.run(15000);
  TEST_ASSERT_EQUAL_FLOAT(90, rig.robot.position(Axis::Horizontal));

  rig.robot.startAccelTuning(Axis::Horizontal);
  rig.robot.adjustSpeedTuning(true);
  rig.robot.stow();
  rig.run(15000);
  rig.robot.keepSpeedTuning();

  rig.robot.startAccelTuning(Axis::Horizontal);
  rig.robot.adjustSpeedTuning(true);
  rig.robot.manualMove(0.5f, 0);
  TEST_ASSERT_TRUE(rig.robot.activity() == Activity::Manual);
  rig.robot.keepSpeedTuning();

  TEST_ASSERT_EQUAL_FLOAT(360, rig.robot.settings().axis(Axis::Horizontal).maxAccel);
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_not_offered_before_calibration);
  RUN_TEST(test_not_offered_while_stowing);
  RUN_TEST(test_starts_from_the_current_value);
  RUN_TEST(test_speed_tuning_is_not_acceleration_tuning);
  RUN_TEST(test_moves_back_and_forth_within_the_travel_limits_and_holds_the_other);
  RUN_TEST(test_each_move_starts_from_a_stop);
  RUN_TEST(test_moves_reach_the_maximum_speed_at_the_trial_acceleration);
  RUN_TEST(test_steps_are_finer_at_low_accelerations);
  RUN_TEST(test_trial_acceleration_stays_within_bounds);
  RUN_TEST(test_nothing_is_saved_until_kept);
  RUN_TEST(test_keep_saves_the_acceleration_only_and_stows);
  RUN_TEST(test_cancel_stow_and_joystick_keep_the_previous_value);
  return UNITY_END();
}
