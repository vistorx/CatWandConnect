#include <robot_core/robot.h>
#include <unity.h>

#include <cmath>
#include <cstring>

#include "../fakes.h"

using namespace cwc;
using namespace cwc::test;

namespace {

struct Rig {
  FakeMotorDriver motors;
  FakeClock clock;
  FakeStorage storage;
  Robot robot{motors, clock, storage};

  void run(uint32_t ms) {
    for (uint32_t t = 0; t < ms; t += 10) {
      clock.advance(10);
      robot.tick();
    }
  }

  void hold(float horizontal, float vertical, uint32_t ms) {
    for (uint32_t t = 0; t < ms; t += 100) {
      robot.manualMove(horizontal, vertical);
      run(100);
    }
  }

  // Stores a resting position the way a previous run would have.
  void remember(float horizontal, float vertical) {
    struct {
      uint32_t magic;
      float position[2];
    } rest{0x52455354, {horizontal, vertical}};
    storage.write("rest", &rest, sizeof(rest));
  }

  float firstMove(Axis axis) {
    for (const auto& move : motors.moves) {
      if (move.axis == axis) return move.degrees;
    }
    return NAN;
  }
};

}  // namespace

void setUp() {}
void tearDown() {}

void test_starts_at_the_remembered_position() {
  Rig rig;
  rig.remember(110, 70);

  rig.robot.begin();

  TEST_ASSERT_EQUAL_FLOAT(110, rig.firstMove(Axis::Horizontal));
  TEST_ASSERT_EQUAL_FLOAT(70, rig.firstMove(Axis::Vertical));
}

void test_then_stows_at_the_slowest_speed() {
  Rig rig;
  rig.remember(110, 70);
  rig.robot.begin();

  float previous = rig.robot.position(Axis::Horizontal);
  for (int i = 0; i < 400; ++i) {
    rig.run(10);
    float now = rig.robot.position(Axis::Horizontal);
    TEST_ASSERT_TRUE(std::fabs(now - previous) <= kMinSpeed * 0.01f + 0.001f);
    previous = now;
  }
  TEST_ASSERT_EQUAL_FLOAT(kDefaultStow, rig.robot.position(Axis::Horizontal));
  TEST_ASSERT_EQUAL_FLOAT(kDefaultStow, rig.robot.position(Axis::Vertical));
  TEST_ASSERT_EQUAL(2, rig.motors.releases.size());
}

void test_remembered_position_is_clamped_into_travel_limits() {
  Rig rig;
  rig.remember(150, 20);

  rig.robot.begin();

  TEST_ASSERT_EQUAL_FLOAT(kDefaultTravel.max, rig.firstMove(Axis::Horizontal));
  TEST_ASSERT_EQUAL_FLOAT(kDefaultTravel.min, rig.firstMove(Axis::Vertical));
}

void test_without_remembered_position_goes_straight_to_stow() {
  Rig rig;

  rig.robot.begin();
  rig.run(10);

  TEST_ASSERT_EQUAL_FLOAT(kDefaultStow, rig.firstMove(Axis::Horizontal));
  TEST_ASSERT_EQUAL(2, rig.motors.moves.size());
  TEST_ASSERT_EQUAL(2, rig.motors.releases.size());
}

void test_garbage_remembered_position_goes_straight_to_stow() {
  Rig rig;
  rig.remember(NAN, 90);

  rig.robot.begin();

  TEST_ASSERT_EQUAL_FLOAT(kDefaultStow, rig.firstMove(Axis::Horizontal));
  TEST_ASSERT_EQUAL_FLOAT(kDefaultStow, rig.firstMove(Axis::Vertical));
}

void test_resting_position_is_saved_only_when_idle() {
  Rig rig;
  rig.robot.begin();
  rig.run(100);
  int before = rig.storage.writes;

  rig.robot.manualMove(1, 0);
  rig.hold(1, 0, 400);
  TEST_ASSERT_EQUAL(before, rig.storage.writes);  // no writes while moving

  rig.robot.manualStop();
  rig.run(1000);
  TEST_ASSERT_EQUAL(before + 1, rig.storage.writes);

  rig.run(5000);
  TEST_ASSERT_EQUAL(before + 1, rig.storage.writes);  // nothing new to save
}

void test_next_power_up_starts_where_the_wand_rested() {
  Rig first;
  first.robot.begin();
  first.run(100);
  first.hold(1, 0, 300);
  first.robot.manualStop();
  first.run(1000);
  float rested = first.robot.position(Axis::Horizontal);

  Rig second;
  second.storage.blobs = first.storage.blobs;
  second.robot.begin();

  TEST_ASSERT_EQUAL_FLOAT(rested, second.firstMove(Axis::Horizontal));
}

void test_manual_control_cannot_interrupt_power_up_stow() {
  Rig rig;
  rig.remember(110, 90);
  rig.robot.begin();

  rig.hold(1, 0, 500);
  rig.run(5000);

  TEST_ASSERT_EQUAL_FLOAT(kDefaultStow, rig.robot.position(Axis::Horizontal));
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_starts_at_the_remembered_position);
  RUN_TEST(test_then_stows_at_the_slowest_speed);
  RUN_TEST(test_remembered_position_is_clamped_into_travel_limits);
  RUN_TEST(test_without_remembered_position_goes_straight_to_stow);
  RUN_TEST(test_garbage_remembered_position_goes_straight_to_stow);
  RUN_TEST(test_resting_position_is_saved_only_when_idle);
  RUN_TEST(test_next_power_up_starts_where_the_wand_rested);
  RUN_TEST(test_manual_control_cannot_interrupt_power_up_stow);
  return UNITY_END();
}
