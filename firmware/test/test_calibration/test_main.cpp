#include <robot_core/robot.h>
#include <unity.h>

#include <cmath>

#include "../fakes.h"

using namespace cwc;
using namespace cwc::test;

namespace {

struct Rig {
  FakeMotorDriver motors;
  FakeClock clock;
  FakeStorage storage;
  Robot robot{motors, clock, storage};

  Rig() {
    robot.begin();
    run(10);
  }

  void run(uint32_t ms) {
    for (uint32_t t = 0; t < ms; t += 10) {
      clock.advance(10);
      robot.tick();
    }
  }

  // Nudges until the axis reaches `degrees`, the way the owner would.
  void nudgeTo(Axis axis, float degrees) {
    robot.nudge(axis, degrees - robot.position(axis));
    run(15000);
  }

  // Runs a whole Calibration up to the Confirm step.
  void calibrate(float hMin, float hMax, float vMin, float vMax, float hStow, float vStow) {
    robot.startCalibration();
    nudgeTo(Axis::Horizontal, hMin);
    TEST_ASSERT_TRUE(robot.markCalibration().ok());
    nudgeTo(Axis::Horizontal, hMax);
    TEST_ASSERT_TRUE(robot.markCalibration().ok());
    nudgeTo(Axis::Vertical, vMin);
    TEST_ASSERT_TRUE(robot.markCalibration().ok());
    nudgeTo(Axis::Vertical, vMax);
    TEST_ASSERT_TRUE(robot.markCalibration().ok());
    robot.nudge(Axis::Horizontal, hStow - robot.position(Axis::Horizontal));
    robot.nudge(Axis::Vertical, vStow - robot.position(Axis::Vertical));
    run(15000);
    TEST_ASSERT_TRUE(robot.markCalibration().ok());
    run(60000);  // sweep
  }
};

}  // namespace

void setUp() {}
void tearDown() {}

void test_steps_follow_the_wizard_order() {
  Rig rig;
  rig.robot.startCalibration();
  TEST_ASSERT_TRUE(rig.robot.calibrationStep() == CalibrationStep::HorizontalMin);
  TEST_ASSERT_TRUE(rig.robot.activity() == Activity::Calibrating);

  rig.nudgeTo(Axis::Horizontal, 30);
  rig.robot.markCalibration();
  TEST_ASSERT_TRUE(rig.robot.calibrationStep() == CalibrationStep::HorizontalMax);
  rig.nudgeTo(Axis::Horizontal, 150);
  rig.robot.markCalibration();
  TEST_ASSERT_TRUE(rig.robot.calibrationStep() == CalibrationStep::VerticalMin);
  rig.nudgeTo(Axis::Vertical, 40);
  rig.robot.markCalibration();
  TEST_ASSERT_TRUE(rig.robot.calibrationStep() == CalibrationStep::VerticalMax);
  rig.nudgeTo(Axis::Vertical, 140);
  rig.robot.markCalibration();
  TEST_ASSERT_TRUE(rig.robot.calibrationStep() == CalibrationStep::Stow);
  rig.robot.nudge(Axis::Horizontal, -50);
  rig.run(15000);
  rig.robot.markCalibration();
  TEST_ASSERT_TRUE(rig.robot.calibrationStep() == CalibrationStep::Sweep);
  rig.run(60000);
  TEST_ASSERT_TRUE(rig.robot.calibrationStep() == CalibrationStep::Confirm);
}

void test_calibration_can_reach_beyond_the_default_limits() {
  Rig rig;
  rig.robot.startCalibration();

  rig.nudgeTo(Axis::Horizontal, 20);

  TEST_ASSERT_EQUAL_FLOAT(20, rig.robot.position(Axis::Horizontal));
}

void test_nudges_stay_within_the_servo_range_and_are_slow() {
  Rig rig;
  rig.robot.startCalibration();
  rig.robot.nudge(Axis::Horizontal, -500);

  float previous = rig.robot.position(Axis::Horizontal);
  for (int i = 0; i < 1000; ++i) {
    rig.run(10);
    float now = rig.robot.position(Axis::Horizontal);
    TEST_ASSERT_TRUE(std::fabs(now - previous) <= Robot::kCalibrationSpeed * 0.01f + 0.001f);
    previous = now;
  }
  TEST_ASSERT_EQUAL_FLOAT(kServoRange.min, rig.robot.position(Axis::Horizontal));
}

void test_only_the_current_axis_can_be_nudged() {
  Rig rig;
  rig.robot.startCalibration();

  rig.robot.nudge(Axis::Vertical, 20);
  rig.run(3000);

  TEST_ASSERT_EQUAL_FLOAT(kDefaultStow, rig.robot.position(Axis::Vertical));
}

void test_maximum_must_be_above_minimum() {
  Rig rig;
  rig.robot.startCalibration();
  rig.nudgeTo(Axis::Horizontal, 100);
  rig.robot.markCalibration();

  rig.nudgeTo(Axis::Horizontal, 80);
  SettingsCheck check = rig.robot.markCalibration();

  TEST_ASSERT_TRUE(check.error == SettingsError::TravelOrder);
  TEST_ASSERT_TRUE(rig.robot.calibrationStep() == CalibrationStep::HorizontalMax);
}

void test_stow_must_be_within_the_new_limits() {
  Rig rig;
  rig.robot.startCalibration();
  rig.nudgeTo(Axis::Horizontal, 40);
  rig.robot.markCalibration();
  rig.nudgeTo(Axis::Horizontal, 140);
  rig.robot.markCalibration();
  rig.nudgeTo(Axis::Vertical, 50);
  rig.robot.markCalibration();
  rig.nudgeTo(Axis::Vertical, 130);
  rig.robot.markCalibration();

  rig.nudgeTo(Axis::Vertical, 160);
  SettingsCheck check = rig.robot.markCalibration();

  TEST_ASSERT_TRUE(check.error == SettingsError::StowOutOfTravel);
  TEST_ASSERT_TRUE(check.axis == Axis::Vertical);
  TEST_ASSERT_TRUE(rig.robot.calibrationStep() == CalibrationStep::Stow);
}

void test_sweep_stays_within_the_new_envelope_and_visits_its_corners() {
  Rig rig;
  rig.robot.startCalibration();
  rig.nudgeTo(Axis::Horizontal, 40);
  rig.robot.markCalibration();
  rig.nudgeTo(Axis::Horizontal, 140);
  rig.robot.markCalibration();
  rig.nudgeTo(Axis::Vertical, 50);
  rig.robot.markCalibration();
  rig.nudgeTo(Axis::Vertical, 130);
  rig.robot.markCalibration();
  rig.nudgeTo(Axis::Horizontal, 90);
  rig.robot.markCalibration();
  rig.motors.moves.clear();

  rig.run(60000);

  float lowest = 180, highest = 0;
  for (const auto& move : rig.motors.moves) {
    if (move.axis != Axis::Horizontal) continue;
    lowest = std::fmin(lowest, move.degrees);
    highest = std::fmax(highest, move.degrees);
  }
  TEST_ASSERT_EQUAL_FLOAT(40, lowest);
  TEST_ASSERT_EQUAL_FLOAT(140, highest);
  TEST_ASSERT_EQUAL_FLOAT(90, rig.robot.position(Axis::Horizontal));  // ends at Stow
}

void test_confirm_saves_and_applies_the_new_envelope() {
  Rig rig;
  rig.calibrate(30, 150, 40, 140, 100, 60);

  rig.robot.confirmCalibration();
  rig.run(1000);

  TEST_ASSERT_TRUE(rig.robot.settings().calibrated);
  TEST_ASSERT_EQUAL_FLOAT(30, rig.robot.travelLimits(Axis::Horizontal).min);
  TEST_ASSERT_EQUAL_FLOAT(140, rig.robot.travelLimits(Axis::Vertical).max);
  TEST_ASSERT_EQUAL_FLOAT(100, rig.robot.settings().axis(Axis::Horizontal).stow);
  TEST_ASSERT_TRUE(loadSettings(rig.storage).calibrated);
  TEST_ASSERT_TRUE(rig.robot.activity() == Activity::Idle);
  TEST_ASSERT_FALSE(rig.robot.held(Axis::Horizontal));  // rests at the new Stow
}

void test_manual_control_uses_the_new_limits_after_calibration() {
  Rig rig;
  rig.calibrate(30, 150, 40, 140, 100, 60);
  rig.robot.confirmCalibration();
  rig.run(1000);

  for (int i = 0; i < 100; ++i) {
    rig.robot.manualMove(1, 0);
    rig.run(100);
  }

  TEST_ASSERT_FLOAT_WITHIN(0.5f, 150, rig.robot.position(Axis::Horizontal));
}

void test_cancel_keeps_the_previous_settings_and_stows() {
  Rig rig;
  rig.calibrate(30, 150, 40, 140, 100, 60);

  rig.robot.cancelCalibration();
  rig.run(20000);

  TEST_ASSERT_FALSE(rig.robot.settings().calibrated);
  TEST_ASSERT_EQUAL_FLOAT(kDefaultTravel.min, rig.robot.travelLimits(Axis::Horizontal).min);
  TEST_ASSERT_EQUAL_FLOAT(kDefaultStow, rig.robot.position(Axis::Horizontal));
  TEST_ASSERT_TRUE(rig.robot.calibrationStep() == CalibrationStep::None);
  TEST_ASSERT_EQUAL(0, rig.storage.blobs.count("robot"));
}

void test_stow_and_manual_control_abandon_calibration() {
  Rig rig;
  rig.robot.startCalibration();
  rig.robot.stow();
  TEST_ASSERT_TRUE(rig.robot.calibrationStep() == CalibrationStep::None);

  rig.run(100);
  rig.robot.startCalibration();
  rig.robot.manualMove(1, 0);
  TEST_ASSERT_TRUE(rig.robot.calibrationStep() == CalibrationStep::None);
  TEST_ASSERT_TRUE(rig.robot.activity() == Activity::Manual);
}

void test_recalibration_starts_from_the_current_limits() {
  Rig rig;
  rig.calibrate(30, 150, 40, 140, 100, 60);
  rig.robot.confirmCalibration();
  rig.run(1000);

  rig.robot.startCalibration();

  TEST_ASSERT_EQUAL_FLOAT(30, rig.robot.calibrationDraft().axis(Axis::Horizontal).travel.min);
}

void test_confirm_outside_the_confirm_step_does_nothing() {
  Rig rig;
  rig.robot.startCalibration();

  rig.robot.confirmCalibration();

  TEST_ASSERT_FALSE(rig.robot.settings().calibrated);
  TEST_ASSERT_TRUE(rig.robot.calibrationStep() == CalibrationStep::HorizontalMin);
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_steps_follow_the_wizard_order);
  RUN_TEST(test_calibration_can_reach_beyond_the_default_limits);
  RUN_TEST(test_nudges_stay_within_the_servo_range_and_are_slow);
  RUN_TEST(test_only_the_current_axis_can_be_nudged);
  RUN_TEST(test_maximum_must_be_above_minimum);
  RUN_TEST(test_stow_must_be_within_the_new_limits);
  RUN_TEST(test_sweep_stays_within_the_new_envelope_and_visits_its_corners);
  RUN_TEST(test_confirm_saves_and_applies_the_new_envelope);
  RUN_TEST(test_manual_control_uses_the_new_limits_after_calibration);
  RUN_TEST(test_cancel_keeps_the_previous_settings_and_stows);
  RUN_TEST(test_stow_and_manual_control_abandon_calibration);
  RUN_TEST(test_recalibration_starts_from_the_current_limits);
  RUN_TEST(test_confirm_outside_the_confirm_step_does_nothing);
  return UNITY_END();
}
