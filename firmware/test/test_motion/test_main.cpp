#include <robot_core/motion.h>
#include <unity.h>

#include <cmath>

using namespace cwc;

namespace {

const TravelLimits kTravel{60, 120};
const MotionLimits kMotion{90, 360};
constexpr float kDt = 0.01f;

}  // namespace

void setUp() {}
void tearDown() {}

void test_velocity_never_exceeds_max_speed() {
  AxisMotion axis(60);
  for (int i = 0; i < 50; ++i) {
    axis.stepVelocity(1000, kDt, kTravel, kMotion);
    TEST_ASSERT_TRUE(axis.velocity() <= kMotion.maxSpeed);
  }
}

void test_acceleration_is_limited() {
  AxisMotion axis(90);
  float previous = 0;
  for (int i = 0; i < 20; ++i) {
    axis.stepVelocity(90, kDt, kTravel, kMotion);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, kMotion.maxAccel * kDt, axis.velocity() - previous);
    previous = axis.velocity();
  }
}

void test_pushing_toward_a_limit_stops_exactly_on_it() {
  AxisMotion axis(90);
  for (int i = 0; i < 500; ++i) {
    axis.stepVelocity(90, kDt, kTravel, kMotion);
    TEST_ASSERT_TRUE(axis.position() <= kTravel.max);
  }
  TEST_ASSERT_FLOAT_WITHIN(0.5f, kTravel.max, axis.position());
  TEST_ASSERT_EQUAL_FLOAT(0, axis.velocity());
}

void test_brakes_before_the_lower_limit() {
  AxisMotion axis(90);
  float lowest = 90;
  for (int i = 0; i < 500; ++i) {
    axis.stepVelocity(-90, kDt, kTravel, kMotion);
    lowest = std::fmin(lowest, axis.position());
  }
  TEST_ASSERT_TRUE(lowest >= kTravel.min);
}

void test_axis_outside_limits_does_not_jump_and_can_only_come_back() {
  AxisMotion axis(130);

  axis.stepVelocity(90, kDt, kTravel, kMotion);
  TEST_ASSERT_EQUAL_FLOAT(130, axis.position());

  axis.stepVelocity(-90, kDt, kTravel, kMotion);
  TEST_ASSERT_TRUE(axis.position() < 130);
  TEST_ASSERT_TRUE(axis.position() > 129);
}

void test_zero_velocity_decelerates_to_a_stop() {
  AxisMotion axis(70);
  for (int i = 0; i < 30; ++i) axis.stepVelocity(90, kDt, kTravel, kMotion);
  for (int i = 0; i < 100; ++i) axis.stepVelocity(0, kDt, kTravel, kMotion);
  TEST_ASSERT_EQUAL_FLOAT(0, axis.velocity());
}

void test_step_toward_lands_exactly_on_target() {
  AxisMotion axis(60);
  for (int i = 0; i < 300; ++i) axis.stepToward(100, kDt, kTravel, kMotion);
  TEST_ASSERT_EQUAL_FLOAT(100, axis.position());
  TEST_ASSERT_TRUE(axis.atRest());
}

void test_step_toward_clamps_target_to_limits() {
  AxisMotion axis(90);
  for (int i = 0; i < 300; ++i) {
    axis.stepToward(170, kDt, kTravel, kMotion);
    TEST_ASSERT_TRUE(axis.position() <= kTravel.max);
  }
  TEST_ASSERT_EQUAL_FLOAT(kTravel.max, axis.position());
}

void test_step_toward_never_overshoots() {
  AxisMotion axis(60);
  for (int i = 0; i < 300; ++i) {
    axis.stepToward(75, kDt, kTravel, kMotion);
    TEST_ASSERT_TRUE(axis.position() <= 75);
  }
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_velocity_never_exceeds_max_speed);
  RUN_TEST(test_acceleration_is_limited);
  RUN_TEST(test_pushing_toward_a_limit_stops_exactly_on_it);
  RUN_TEST(test_brakes_before_the_lower_limit);
  RUN_TEST(test_axis_outside_limits_does_not_jump_and_can_only_come_back);
  RUN_TEST(test_zero_velocity_decelerates_to_a_stop);
  RUN_TEST(test_step_toward_lands_exactly_on_target);
  RUN_TEST(test_step_toward_clamps_target_to_limits);
  RUN_TEST(test_step_toward_never_overshoots);
  return UNITY_END();
}
