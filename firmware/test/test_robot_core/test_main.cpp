#include <robot_core/robot.h>
#include <unity.h>

#include "fakes.h"

using namespace cwc;
using namespace cwc::test;

void setUp() {}
void tearDown() {}

void test_robot_ticks_without_touching_the_motors() {
  FakeMotorDriver motors;
  FakeClock clock;
  FakeStorage storage;
  Robot robot(motors, clock, storage);

  for (int i = 0; i < 10; ++i) {
    robot.tick();
    clock.advance(20);
  }

  TEST_ASSERT_EQUAL(0, motors.moves.size());
  TEST_ASSERT_EQUAL(0, motors.releases.size());
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_robot_ticks_without_touching_the_motors);
  return UNITY_END();
}
