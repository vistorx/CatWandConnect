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
  Robot robot{motors, clock, storage};

  Rig() { robot.begin(); }

  // Runs the control loop for `ms` milliseconds in 10 ms ticks.
  void run(uint32_t ms) {
    for (uint32_t t = 0; t < ms; t += 10) {
      clock.advance(10);
      robot.tick();
    }
  }

  // Holds the joystick for `ms`, refreshing it every 100 ms like the interface.
  void hold(float horizontal, float vertical, uint32_t ms) {
    for (uint32_t t = 0; t < ms; t += 100) {
      robot.manualMove(horizontal, vertical);
      run(100);
    }
  }
};

}  // namespace

void setUp() {}
void tearDown() {}

void test_begin_holds_both_axes_at_stow() {
  Rig rig;

  TEST_ASSERT_EQUAL(2, rig.motors.moves.size());
  TEST_ASSERT_EQUAL_FLOAT(kDefaultStow, rig.robot.position(Axis::Horizontal));
  TEST_ASSERT_EQUAL_FLOAT(kDefaultStow, rig.robot.position(Axis::Vertical));
}

void test_idle_robot_does_not_move() {
  Rig rig;
  rig.motors.moves.clear();

  rig.run(1000);

  TEST_ASSERT_EQUAL(0, rig.motors.moves.size());
}

void test_joystick_moves_the_axes() {
  Rig rig;

  rig.hold(1, -1, 300);

  TEST_ASSERT_TRUE(rig.robot.activity() == Activity::Manual);
  TEST_ASSERT_TRUE(rig.robot.position(Axis::Horizontal) > kDefaultStow);
  TEST_ASSERT_TRUE(rig.robot.position(Axis::Vertical) < kDefaultStow);
}

void test_manual_control_never_leaves_travel_limits() {
  Rig rig;

  rig.hold(1, -1, 3000);
  rig.hold(-1, 1, 3000);

  for (const auto& move : rig.motors.moves) {
    TravelLimits limits = rig.robot.travelLimits(move.axis);
    TEST_ASSERT_TRUE(move.degrees >= limits.min);
    TEST_ASSERT_TRUE(move.degrees <= limits.max);
  }
  TEST_ASSERT_FLOAT_WITHIN(0.5f, kDefaultTravel.min, rig.robot.position(Axis::Horizontal));
  TEST_ASSERT_FLOAT_WITHIN(0.5f, kDefaultTravel.max, rig.robot.position(Axis::Vertical));
}

void test_out_of_range_deflection_is_capped() {
  Rig full;
  Rig huge;

  full.hold(1, 0, 200);
  huge.hold(50, 0, 200);

  TEST_ASSERT_EQUAL_FLOAT(full.robot.position(Axis::Horizontal), huge.robot.position(Axis::Horizontal));
}

void test_release_stops_the_wand() {
  Rig rig;
  rig.hold(1, 0, 200);

  rig.robot.manualStop();
  rig.run(500);
  float stopped = rig.robot.position(Axis::Horizontal);
  rig.run(1000);

  TEST_ASSERT_EQUAL_FLOAT(stopped, rig.robot.position(Axis::Horizontal));
  TEST_ASSERT_TRUE(rig.robot.activity() == Activity::Idle);
}

void test_silent_controller_stops_the_wand() {
  Rig rig;
  rig.robot.manualMove(0.3f, 0);  // slow enough not to reach the limit before the timeout

  rig.run(Robot::kManualTimeoutMs + 500);
  float stopped = rig.robot.position(Axis::Horizontal);
  rig.run(2000);

  TEST_ASSERT_EQUAL_FLOAT(stopped, rig.robot.position(Axis::Horizontal));
  TEST_ASSERT_TRUE(stopped < kDefaultTravel.max);
  TEST_ASSERT_TRUE(rig.robot.activity() == Activity::Idle);
}

void test_stalled_loop_does_not_leap() {
  Rig rig;
  rig.robot.manualMove(1, 0);

  rig.clock.advance(5000);
  rig.robot.tick();

  // At most one capped step of acceleration from rest.
  TEST_ASSERT_TRUE(rig.robot.position(Axis::Horizontal) - kDefaultStow < 5);
}

void test_manual_speed_follows_settings() {
  Rig rig;
  RobotSettings s = rig.robot.settings();
  s.axis(Axis::Horizontal).maxSpeed = 20;
  s.playSpeed = 0.5f;
  TEST_ASSERT_TRUE(rig.robot.applySettings(s).ok());

  rig.hold(1, 0, 2000);

  // 10 °/s for 2 s, minus the time spent accelerating.
  float travelled = rig.robot.position(Axis::Horizontal) - kDefaultStow;
  TEST_ASSERT_TRUE(travelled > 18);
  TEST_ASSERT_TRUE(travelled <= 20.01f);
}

void test_invalid_settings_are_rejected_whole() {
  Rig rig;
  RobotSettings s = rig.robot.settings();
  s.axis(Axis::Horizontal).maxSpeed = 30;  // valid
  s.axis(Axis::Vertical).stow = 200;       // invalid

  SettingsCheck check = rig.robot.applySettings(s);

  TEST_ASSERT_TRUE(check.error == SettingsError::StowOutOfTravel);
  TEST_ASSERT_TRUE(check.axis == Axis::Vertical);
  TEST_ASSERT_EQUAL_FLOAT(defaultSettings().axis(Axis::Horizontal).maxSpeed,
                          rig.robot.settings().axis(Axis::Horizontal).maxSpeed);
  TEST_ASSERT_EQUAL(0, rig.storage.blobs.count("robot"));
}

void test_settings_are_restored_at_power_up() {
  FakeMotorDriver motors;
  FakeClock clock;
  FakeStorage storage;
  RobotSettings s = defaultSettings();
  s.axis(Axis::Vertical).maxAccel = 100;
  saveSettings(storage, s);

  Robot robot(motors, clock, storage);
  robot.begin();

  TEST_ASSERT_EQUAL_FLOAT(100, robot.settings().axis(Axis::Vertical).maxAccel);
}

void test_default_travel_applies_until_calibrated() {
  Rig rig;
  RobotSettings s = rig.robot.settings();
  s.axis(Axis::Horizontal).travel = {10, 170};
  rig.robot.applySettings(s);
  TEST_ASSERT_EQUAL_FLOAT(kDefaultTravel.max, rig.robot.travelLimits(Axis::Horizontal).max);

  s.calibrated = true;
  rig.robot.applySettings(s);
  TEST_ASSERT_EQUAL_FLOAT(170, rig.robot.travelLimits(Axis::Horizontal).max);
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_begin_holds_both_axes_at_stow);
  RUN_TEST(test_idle_robot_does_not_move);
  RUN_TEST(test_joystick_moves_the_axes);
  RUN_TEST(test_manual_control_never_leaves_travel_limits);
  RUN_TEST(test_out_of_range_deflection_is_capped);
  RUN_TEST(test_release_stops_the_wand);
  RUN_TEST(test_silent_controller_stops_the_wand);
  RUN_TEST(test_stalled_loop_does_not_leap);
  RUN_TEST(test_manual_speed_follows_settings);
  RUN_TEST(test_invalid_settings_are_rejected_whole);
  RUN_TEST(test_settings_are_restored_at_power_up);
  RUN_TEST(test_default_travel_applies_until_calibrated);
  return UNITY_END();
}
