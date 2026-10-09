#include <robot_core/settings.h>
#include <unity.h>

#include "../fakes.h"

using namespace cwc;
using namespace cwc::test;

void setUp() {}
void tearDown() {}

namespace {

void assertRejected(const RobotSettings& settings, SettingsError error, Axis axis) {
  SettingsCheck check = validate(settings);
  TEST_ASSERT_EQUAL_STRING(settingsErrorName(error), settingsErrorName(check.error));
  TEST_ASSERT_TRUE(check.axis == axis);
}

}  // namespace

void test_defaults_are_valid() { TEST_ASSERT_TRUE(validate(defaultSettings()).ok()); }

void test_rejects_minimum_not_below_maximum() {
  RobotSettings s = defaultSettings();
  s.axis(Axis::Vertical).travel = {100, 100};
  assertRejected(s, SettingsError::TravelOrder, Axis::Vertical);
}

void test_rejects_limits_outside_servo_range() {
  RobotSettings s = defaultSettings();
  s.axis(Axis::Horizontal).travel = {-5, 120};
  assertRejected(s, SettingsError::TravelOutOfRange, Axis::Horizontal);

  s = defaultSettings();
  s.axis(Axis::Vertical).travel = {60, 181};
  assertRejected(s, SettingsError::TravelOutOfRange, Axis::Vertical);
}

void test_rejects_stow_outside_travel() {
  RobotSettings s = defaultSettings();
  s.axis(Axis::Horizontal).stow = 130;
  assertRejected(s, SettingsError::StowOutOfTravel, Axis::Horizontal);
}

void test_rejects_speeds_out_of_range() {
  RobotSettings s = defaultSettings();
  s.axis(Axis::Vertical).maxSpeed = kMinSpeed - 1;
  assertRejected(s, SettingsError::SpeedOutOfRange, Axis::Vertical);

  s = defaultSettings();
  s.axis(Axis::Horizontal).maxAccel = kMaxAccel + 1;
  assertRejected(s, SettingsError::AccelOutOfRange, Axis::Horizontal);

  s = defaultSettings();
  s.playSpeed = 0;
  TEST_ASSERT_TRUE(validate(s).error == SettingsError::PlaySpeedOutOfRange);
}

void test_rejects_non_numbers() {
  RobotSettings s = defaultSettings();
  s.axis(Axis::Horizontal).maxSpeed = NAN;
  TEST_ASSERT_FALSE(validate(s).ok());
}

void test_first_boot_loads_defaults() {
  FakeStorage storage;
  RobotSettings s = loadSettings(storage);
  TEST_ASSERT_EQUAL_FLOAT(defaultSettings().playSpeed, s.playSpeed);
  TEST_ASSERT_FALSE(s.calibrated);
}

void test_saved_settings_survive_a_reboot() {
  FakeStorage storage;
  RobotSettings s = defaultSettings();
  s.axis(Axis::Vertical).maxSpeed = 45;
  s.calibrated = true;

  TEST_ASSERT_TRUE(saveSettings(storage, s));
  RobotSettings loaded = loadSettings(storage);

  TEST_ASSERT_EQUAL_FLOAT(45, loaded.axis(Axis::Vertical).maxSpeed);
  TEST_ASSERT_TRUE(loaded.calibrated);
}

void test_unknown_version_falls_back_to_defaults() {
  FakeStorage storage;
  RobotSettings s = defaultSettings();
  s.axis(Axis::Vertical).maxSpeed = 45;
  saveSettings(storage, s);
  storage.blobs["robot"][4] = 99;  // version field, as if written by a future firmware

  TEST_ASSERT_EQUAL_FLOAT(defaultSettings().axis(Axis::Vertical).maxSpeed,
                          loadSettings(storage).axis(Axis::Vertical).maxSpeed);
}

void test_corrupted_record_falls_back_to_defaults() {
  FakeStorage storage;
  saveSettings(storage, defaultSettings());
  storage.blobs["robot"].resize(storage.blobs["robot"].size() - 3);

  TEST_ASSERT_TRUE(validate(loadSettings(storage)).ok());
}

void test_stored_invalid_values_fall_back_to_defaults() {
  FakeStorage storage;
  RobotSettings s = defaultSettings();
  s.axis(Axis::Horizontal).stow = 170;  // bypasses validation, as a corrupted write would
  saveSettings(storage, s);

  TEST_ASSERT_EQUAL_FLOAT(kDefaultStow, loadSettings(storage).axis(Axis::Horizontal).stow);
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_defaults_are_valid);
  RUN_TEST(test_rejects_minimum_not_below_maximum);
  RUN_TEST(test_rejects_limits_outside_servo_range);
  RUN_TEST(test_rejects_stow_outside_travel);
  RUN_TEST(test_rejects_speeds_out_of_range);
  RUN_TEST(test_rejects_non_numbers);
  RUN_TEST(test_first_boot_loads_defaults);
  RUN_TEST(test_saved_settings_survive_a_reboot);
  RUN_TEST(test_unknown_version_falls_back_to_defaults);
  RUN_TEST(test_corrupted_record_falls_back_to_defaults);
  RUN_TEST(test_stored_invalid_values_fall_back_to_defaults);
  return UNITY_END();
}
