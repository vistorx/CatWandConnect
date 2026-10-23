// What the Robot core reports about Play sessions to its PlayObserver: the
// events the Play session journal records.

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
  FakePlayObserver observer;
  Robot robot{motors, clock, storage, random};

  explicit Rig(bool calibrated = true) {
    RobotSettings s = defaultSettings();
    s.calibrated = calibrated;
    saveSettings(storage, s);
    robot.setPlayObserver(&observer);
    robot.begin();
    run(15000);
  }

  void run(uint32_t ms) {
    for (uint32_t t = 0; t < ms; t += 10) {
      clock.advance(10);
      robot.tick();
    }
  }
};

void assertReports(const Rig& rig, std::initializer_list<FakePlayObserver::Report> expected) {
  TEST_ASSERT_EQUAL(expected.size(), rig.observer.reports.size());
  size_t i = 0;
  for (const auto& report : expected) {
    TEST_ASSERT_EQUAL(static_cast<int>(report.event), static_cast<int>(rig.observer.reports[i].event));
    TEST_ASSERT_EQUAL(static_cast<int>(report.cause), static_cast<int>(rig.observer.reports[i].cause));
    ++i;
  }
}

}  // namespace

void setUp() {}
void tearDown() {}

void test_power_up_reports_nothing() {
  Rig rig;
  assertReports(rig, {});
}

void test_a_session_that_runs_its_duration_starts_then_ends() {
  Rig rig;
  TEST_ASSERT_TRUE(rig.robot.startPlaySession(1));
  assertReports(rig, {{PlayEvent::Started, PlayCause::None}});
  rig.run(60000);
  assertReports(rig, {{PlayEvent::Started, PlayCause::None}, {PlayEvent::Ended, PlayCause::None}});
  rig.run(15000);  // the Stow that follows is not another event
  TEST_ASSERT_EQUAL(2, rig.observer.reports.size());
}

void test_stop_stow_and_manual_control_stop_it() {
  Rig rig;
  rig.robot.startPlaySession(10);
  rig.robot.stopPlaySession();
  rig.run(15000);
  rig.robot.startPlaySession(10);
  rig.robot.stow();
  rig.run(15000);
  rig.robot.startPlaySession(10);
  rig.robot.manualMove(1, 0);
  rig.robot.manualStop();
  rig.run(2000);
  rig.robot.startPlaySession(10);
  rig.robot.manualGoto(Axis::Vertical, 100);
  assertReports(rig, {{PlayEvent::Started, PlayCause::None},
                      {PlayEvent::Stopped, PlayCause::Stop},
                      {PlayEvent::Started, PlayCause::None},
                      {PlayEvent::Stopped, PlayCause::Stow},
                      {PlayEvent::Started, PlayCause::None},
                      {PlayEvent::Stopped, PlayCause::ManualControl},
                      {PlayEvent::Started, PlayCause::None},
                      {PlayEvent::Stopped, PlayCause::ManualControl}});
}

void test_calibration_and_speed_tuning_stop_it() {
  Rig rig;
  rig.robot.startPlaySession(10);
  rig.robot.startCalibration();
  rig.robot.cancelCalibration();
  rig.run(15000);
  rig.robot.startPlaySession(10);
  TEST_ASSERT_TRUE(rig.robot.startSpeedTuning(Axis::Horizontal, SpeedBound::Maximum));
  rig.robot.cancelSpeedTuning();
  rig.run(15000);
  rig.robot.startPlaySession(10);
  TEST_ASSERT_TRUE(rig.robot.startAccelTuning(Axis::Vertical));
  assertReports(rig, {{PlayEvent::Started, PlayCause::None},
                      {PlayEvent::Stopped, PlayCause::Calibration},
                      {PlayEvent::Started, PlayCause::None},
                      {PlayEvent::Stopped, PlayCause::SpeedTuning},
                      {PlayEvent::Started, PlayCause::None},
                      {PlayEvent::Stopped, PlayCause::SpeedTuning}});
}

void test_commands_outside_a_session_report_nothing() {
  Rig rig;
  rig.robot.stopPlaySession();
  rig.robot.stow();
  rig.run(15000);
  rig.robot.manualMove(1, 0);
  rig.robot.manualStop();
  rig.run(2000);
  assertReports(rig, {});
}

void test_refused_before_calibration() {
  Rig rig(false);
  TEST_ASSERT_FALSE(rig.robot.startPlaySession(10));
  assertReports(rig, {{PlayEvent::Refused, PlayCause::NotCalibrated}});
}

void test_refused_while_busy() {
  Rig rig;
  rig.robot.stow();
  TEST_ASSERT_FALSE(rig.robot.startPlaySession(10));
  rig.run(15000);
  rig.robot.startCalibration();
  TEST_ASSERT_FALSE(rig.robot.startPlaySession(10));
  rig.robot.cancelCalibration();
  rig.run(15000);
  rig.robot.startSpeedTuning(Axis::Horizontal, SpeedBound::Minimum);
  TEST_ASSERT_FALSE(rig.robot.startPlaySession(10));
  assertReports(rig, {{PlayEvent::Refused, PlayCause::Busy},
                      {PlayEvent::Refused, PlayCause::Busy},
                      {PlayEvent::Refused, PlayCause::Busy}});
}

// A duration out of range is a bad request, not a refusal of play.
void test_a_duration_out_of_range_reports_nothing() {
  Rig rig;
  TEST_ASSERT_FALSE(rig.robot.startPlaySession(0));
  TEST_ASSERT_FALSE(rig.robot.startPlaySession(61));
  assertReports(rig, {});
}

// Starting again replaces the running session: one more start, no stop.
void test_starting_again_reports_a_new_start() {
  Rig rig;
  rig.robot.startPlaySession(10);
  rig.run(1000);
  rig.robot.startPlaySession(5);
  assertReports(rig, {{PlayEvent::Started, PlayCause::None}, {PlayEvent::Started, PlayCause::None}});
}

void test_works_without_an_observer() {
  FakeMotorDriver motors;
  FakeClock clock;
  FakeStorage storage;
  FakeRandom random;
  RobotSettings s = defaultSettings();
  s.calibrated = true;
  saveSettings(storage, s);
  Robot robot{motors, clock, storage, random};
  robot.begin();
  for (int t = 0; t < 15000; t += 10) {
    clock.advance(10);
    robot.tick();
  }
  TEST_ASSERT_TRUE(robot.startPlaySession(1));
  robot.stow();
  TEST_ASSERT_TRUE(robot.activity() == Activity::Stowing);
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_power_up_reports_nothing);
  RUN_TEST(test_a_session_that_runs_its_duration_starts_then_ends);
  RUN_TEST(test_stop_stow_and_manual_control_stop_it);
  RUN_TEST(test_calibration_and_speed_tuning_stop_it);
  RUN_TEST(test_commands_outside_a_session_report_nothing);
  RUN_TEST(test_refused_before_calibration);
  RUN_TEST(test_refused_while_busy);
  RUN_TEST(test_a_duration_out_of_range_reports_nothing);
  RUN_TEST(test_starting_again_reports_a_new_start);
  RUN_TEST(test_works_without_an_observer);
  return UNITY_END();
}
