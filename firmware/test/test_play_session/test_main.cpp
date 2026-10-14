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
  // Speed range 9–90 °/s, default Pause ranges.
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

  // Runs until the axis is at `degrees`, at most `ms`.
  bool runUntil(Axis axis, float degrees, uint32_t ms) {
    for (uint32_t t = 0; t < ms; t += 10) {
      run(10);
      if (robot.position(axis) == degrees) return true;
    }
    return false;
  }
};

}  // namespace

void setUp() {}
void tearDown() {}

void test_refused_before_calibration() {
  Rig rig(false);
  TEST_ASSERT_FALSE(rig.robot.startPlaySession(10));
  TEST_ASSERT_TRUE(rig.robot.activity() == Activity::Idle);
}

void test_moves_each_axis_to_a_drawn_target_at_a_drawn_speed() {
  Rig rig;
  // Horizontal: target at the minimum, fastest speed; Vertical: target at
  // the maximum, slowest speed.
  rig.random.script = {0, 1, 1, 0, 1};  // then a 3 s Horizontal pause
  TEST_ASSERT_TRUE(rig.robot.startPlaySession(10));
  TEST_ASSERT_TRUE(rig.robot.activity() == Activity::Playing);

  // 50° at up to 90 °/s takes well under 1.5 s; 30° at 9 °/s takes over 3 s.
  rig.run(1500);
  TEST_ASSERT_EQUAL_FLOAT(40, rig.robot.position(Axis::Horizontal));
  TEST_ASSERT_TRUE(rig.robot.position(Axis::Vertical) > 90);
  TEST_ASSERT_TRUE(rig.robot.position(Axis::Vertical) < 120);
  TEST_ASSERT_TRUE(rig.runUntil(Axis::Vertical, 120, 3000));
}

void test_stays_within_travel_limits_and_speed_range() {
  Rig rig;  // seeded draws from here on
  TEST_ASSERT_TRUE(rig.robot.startPlaySession(10));
  float previous[kAxisCount] = {rig.robot.position(Axis::Horizontal), rig.robot.position(Axis::Vertical)};
  TravelLimits seen[kAxisCount] = {{999, -999}, {999, -999}};
  for (int t = 0; t < 120000; t += 10) {
    rig.run(10);
    TEST_ASSERT_TRUE(rig.robot.activity() == Activity::Playing);
    for (Axis axis : kAxes) {
      size_t i = static_cast<size_t>(axis);
      float p = rig.robot.position(axis);
      float step = p > previous[i] ? p - previous[i] : previous[i] - p;
      TEST_ASSERT_TRUE(step <= 90 * 0.01f + 0.001f);  // never above the maximum speed
      previous[i] = p;
      if (p < seen[i].min) seen[i].min = p;
      if (p > seen[i].max) seen[i].max = p;
    }
  }
  TEST_ASSERT_TRUE(rig.random.draws > 20);
  TEST_ASSERT_TRUE(seen[0].min >= 40 && seen[0].max <= 140);
  TEST_ASSERT_TRUE(seen[1].min >= 60 && seen[1].max <= 120);
  TEST_ASSERT_TRUE(seen[0].max - seen[0].min > 50);  // the whole range is used
  TEST_ASSERT_TRUE(seen[1].max - seen[1].min > 30);
}

void test_the_axes_pause_independently() {
  Rig rig;
  RobotSettings s = rig.robot.settings();
  s.axis(Axis::Horizontal).pauseMin = 0.5f;
  s.axis(Axis::Horizontal).pauseMax = 3;
  s.axis(Axis::Vertical).pauseMin = 1;
  s.axis(Axis::Vertical).pauseMax = 5;
  TEST_ASSERT_TRUE(rig.robot.applySettings(s).ok());
  // Horizontal to 40 and Vertical to 120, both fast. Vertical arrives first
  // and pauses 1 s; Horizontal arrives next and pauses 3 s. Vertical then
  // heads for 60 while Horizontal still rests.
  rig.random.script = {0, 1, 1, 1, 0, 1, 0, 1};
  TEST_ASSERT_TRUE(rig.robot.startPlaySession(10));
  rig.run(1200);
  TEST_ASSERT_EQUAL_FLOAT(40, rig.robot.position(Axis::Horizontal));
  TEST_ASSERT_EQUAL_FLOAT(120, rig.robot.position(Axis::Vertical));
  rig.run(1300);
  TEST_ASSERT_EQUAL_FLOAT(40, rig.robot.position(Axis::Horizontal));
  TEST_ASSERT_EQUAL_FLOAT(60, rig.robot.position(Axis::Vertical));
  rig.run(1500);
  TEST_ASSERT_TRUE(rig.robot.position(Axis::Horizontal) > 40);
}

void test_a_zero_pause_chains_moves() {
  Rig rig;
  RobotSettings s = rig.robot.settings();
  s.axis(Axis::Horizontal).pauseMin = 0;
  s.axis(Axis::Horizontal).pauseMax = 0;
  TEST_ASSERT_TRUE(rig.robot.applySettings(s).ok());
  // Horizontal to 40; Vertical stays at 90 and pauses 5 s; Horizontal's
  // 0 s pause, then straight on to 140.
  rig.random.script = {0, 1, 0.5, 1, 1, 0, 1, 1};
  TEST_ASSERT_TRUE(rig.robot.startPlaySession(10));
  TEST_ASSERT_TRUE(rig.runUntil(Axis::Horizontal, 40, 1500));
  rig.run(50);
  TEST_ASSERT_TRUE(rig.robot.position(Axis::Horizontal) > 40);
  TEST_ASSERT_TRUE(rig.runUntil(Axis::Horizontal, 140, 2000));
}

void test_ends_with_stow_when_time_is_up() {
  Rig rig;
  TEST_ASSERT_TRUE(rig.robot.startPlaySession(1));
  TEST_ASSERT_EQUAL_UINT32(60, rig.robot.playRemainingS());
  rig.run(30000);
  TEST_ASSERT_EQUAL_UINT32(30, rig.robot.playRemainingS());
  rig.run(29990);
  TEST_ASSERT_TRUE(rig.robot.activity() == Activity::Playing);
  rig.run(20);
  TEST_ASSERT_TRUE(rig.robot.activity() == Activity::Stowing);
  TEST_ASSERT_EQUAL_UINT32(0, rig.robot.playRemainingS());
  rig.run(5000);
  TEST_ASSERT_TRUE(rig.robot.activity() == Activity::Idle);
  TEST_ASSERT_EQUAL_FLOAT(90, rig.robot.position(Axis::Horizontal));
  TEST_ASSERT_EQUAL_FLOAT(90, rig.robot.position(Axis::Vertical));
  TEST_ASSERT_FALSE(rig.robot.held(Axis::Horizontal));
}

void test_stop_ends_it_with_stow_without_finishing_the_move() {
  Rig rig;
  rig.random.script = {0, 0, 1, 0};  // slow moves to 40 and 120
  TEST_ASSERT_TRUE(rig.robot.startPlaySession(10));
  rig.run(1000);
  float h = rig.robot.position(Axis::Horizontal);
  TEST_ASSERT_TRUE(h < 90 && h > 40);
  rig.robot.stopPlaySession();
  TEST_ASSERT_TRUE(rig.robot.activity() == Activity::Stowing);
  rig.run(200);
  TEST_ASSERT_TRUE(rig.robot.position(Axis::Horizontal) > h - 1);  // turned back, not on to 40
  rig.run(5000);
  TEST_ASSERT_TRUE(rig.robot.activity() == Activity::Idle);
  TEST_ASSERT_EQUAL_FLOAT(90, rig.robot.position(Axis::Horizontal));
}

void test_stow_ends_it() {
  Rig rig;
  TEST_ASSERT_TRUE(rig.robot.startPlaySession(10));
  rig.run(1000);
  rig.robot.stow();
  TEST_ASSERT_TRUE(rig.robot.activity() == Activity::Stowing);
  TEST_ASSERT_EQUAL_UINT32(0, rig.robot.playRemainingS());
  rig.run(5000);
  TEST_ASSERT_TRUE(rig.robot.activity() == Activity::Idle);
  rig.run(60000);
  TEST_ASSERT_TRUE(rig.robot.activity() == Activity::Idle);  // never resumes
}

void test_the_joystick_ends_it_without_stow() {
  Rig rig;
  TEST_ASSERT_TRUE(rig.robot.startPlaySession(10));
  rig.run(1000);
  rig.robot.manualMove(0, 0.5f);
  TEST_ASSERT_TRUE(rig.robot.activity() == Activity::Manual);
  TEST_ASSERT_EQUAL_UINT32(0, rig.robot.playRemainingS());
  rig.robot.manualStop();
  rig.run(2000);
  TEST_ASSERT_TRUE(rig.robot.activity() == Activity::Idle);  // stopped where it was, no Stow
  TEST_ASSERT_TRUE(rig.robot.held(Axis::Horizontal));
  rig.run(60000);
  TEST_ASSERT_TRUE(rig.robot.activity() == Activity::Idle);
}

void test_manual_goto_ends_it_without_stow() {
  Rig rig;
  TEST_ASSERT_TRUE(rig.robot.startPlaySession(10));
  rig.run(500);
  rig.robot.manualGoto(Axis::Horizontal, 100);
  TEST_ASSERT_TRUE(rig.robot.activity() == Activity::Manual);
  rig.run(5000);
  TEST_ASSERT_TRUE(rig.robot.activity() == Activity::Idle);
  TEST_ASSERT_EQUAL_FLOAT(100, rig.robot.position(Axis::Horizontal));
}

void test_refused_while_stowing_calibrating_or_tuning_a_speed() {
  Rig rig;
  rig.robot.stow();
  TEST_ASSERT_FALSE(rig.robot.startPlaySession(10));
  TEST_ASSERT_TRUE(rig.robot.activity() == Activity::Stowing);
  rig.run(5000);

  rig.robot.startCalibration();
  TEST_ASSERT_FALSE(rig.robot.startPlaySession(10));
  TEST_ASSERT_TRUE(rig.robot.activity() == Activity::Calibrating);
  rig.robot.cancelCalibration();
  rig.run(10000);

  TEST_ASSERT_TRUE(rig.robot.startSpeedTuning(Axis::Vertical, SpeedBound::Maximum));
  TEST_ASSERT_FALSE(rig.robot.startPlaySession(10));
  TEST_ASSERT_TRUE(rig.robot.activity() == Activity::TuningSpeed);
}

void test_refuses_a_duration_out_of_range() {
  Rig rig;
  TEST_ASSERT_FALSE(rig.robot.startPlaySession(0));
  TEST_ASSERT_FALSE(rig.robot.startPlaySession(61));
  TEST_ASSERT_TRUE(rig.robot.activity() == Activity::Idle);
  TEST_ASSERT_TRUE(rig.robot.startPlaySession(60));
  TEST_ASSERT_EQUAL_UINT32(3600, rig.robot.playRemainingS());
}

void test_starting_again_starts_from_zero() {
  Rig rig;
  TEST_ASSERT_TRUE(rig.robot.startPlaySession(2));
  rig.run(50000);
  TEST_ASSERT_EQUAL_UINT32(70, rig.robot.playRemainingS());
  TEST_ASSERT_TRUE(rig.robot.startPlaySession(5));
  TEST_ASSERT_EQUAL_UINT32(300, rig.robot.playRemainingS());
}

void test_does_not_survive_a_restart() {
  Rig rig;
  TEST_ASSERT_TRUE(rig.robot.startPlaySession(10));
  rig.run(3000);

  FakeRandom random;
  Robot restarted(rig.motors, rig.clock, rig.storage, random);
  restarted.begin();
  TEST_ASSERT_TRUE(restarted.activity() == Activity::Stowing);
  for (int t = 0; t < 20000; t += 10) {
    rig.clock.advance(10);
    restarted.tick();
  }
  TEST_ASSERT_TRUE(restarted.activity() == Activity::Idle);
  TEST_ASSERT_EQUAL_UINT32(0, restarted.playRemainingS());
  TEST_ASSERT_EQUAL_INT(0, random.draws);
}

void test_remembers_the_session_duration_once_idle() {
  Rig rig;
  TEST_ASSERT_EQUAL_UINT8(10, rig.robot.settings().sessionMinutes);
  int writes = rig.storage.writes;

  TEST_ASSERT_TRUE(rig.robot.startPlaySession(25));
  TEST_ASSERT_EQUAL_UINT8(25, rig.robot.settings().sessionMinutes);
  rig.run(5000);
  TEST_ASSERT_EQUAL_INT(writes, rig.storage.writes);  // not while playing
  rig.robot.stopPlaySession();
  rig.run(10000);
  TEST_ASSERT_EQUAL_UINT8(25, loadSettings(rig.storage).sessionMinutes);

  writes = rig.storage.writes;
  TEST_ASSERT_TRUE(rig.robot.startPlaySession(25));
  rig.robot.stopPlaySession();
  rig.run(10000);
  int afterSameDuration = rig.storage.writes;
  TEST_ASSERT_EQUAL_UINT8(25, loadSettings(rig.storage).sessionMinutes);
  TEST_ASSERT_TRUE(afterSameDuration - writes <= 1);  // at most the resting position
}

void test_sets_the_session_duration_without_playing() {
  Rig rig;
  TEST_ASSERT_FALSE(rig.robot.setSessionMinutes(0));
  TEST_ASSERT_FALSE(rig.robot.setSessionMinutes(61));
  TEST_ASSERT_EQUAL_UINT8(10, rig.robot.settings().sessionMinutes);

  TEST_ASSERT_TRUE(rig.robot.setSessionMinutes(15));
  TEST_ASSERT_EQUAL_UINT8(15, rig.robot.settings().sessionMinutes);
  rig.run(10);
  TEST_ASSERT_EQUAL_UINT8(15, loadSettings(rig.storage).sessionMinutes);
  TEST_ASSERT_TRUE(rig.robot.activity() == Activity::Idle);
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_refused_before_calibration);
  RUN_TEST(test_moves_each_axis_to_a_drawn_target_at_a_drawn_speed);
  RUN_TEST(test_stays_within_travel_limits_and_speed_range);
  RUN_TEST(test_the_axes_pause_independently);
  RUN_TEST(test_a_zero_pause_chains_moves);
  RUN_TEST(test_ends_with_stow_when_time_is_up);
  RUN_TEST(test_stop_ends_it_with_stow_without_finishing_the_move);
  RUN_TEST(test_stow_ends_it);
  RUN_TEST(test_the_joystick_ends_it_without_stow);
  RUN_TEST(test_manual_goto_ends_it_without_stow);
  RUN_TEST(test_refused_while_stowing_calibrating_or_tuning_a_speed);
  RUN_TEST(test_refuses_a_duration_out_of_range);
  RUN_TEST(test_starting_again_starts_from_zero);
  RUN_TEST(test_does_not_survive_a_restart);
  RUN_TEST(test_remembers_the_session_duration_once_idle);
  RUN_TEST(test_sets_the_session_duration_without_playing);
  return UNITY_END();
}
