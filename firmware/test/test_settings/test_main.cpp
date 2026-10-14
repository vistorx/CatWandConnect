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

void test_default_minimum_speed_is_a_tenth_of_the_maximum() {
  RobotSettings s = defaultSettings();
  TEST_ASSERT_EQUAL_FLOAT(9, s.axis(Axis::Horizontal).minSpeed);  // maximum 90 °/s
  TEST_ASSERT_EQUAL_FLOAT(9, s.axis(Axis::Vertical).minSpeed);
}

void test_default_pause_ranges_and_session_duration() {
  RobotSettings s = defaultSettings();
  TEST_ASSERT_EQUAL_FLOAT(0.5f, s.axis(Axis::Horizontal).pauseMin);
  TEST_ASSERT_EQUAL_FLOAT(3, s.axis(Axis::Horizontal).pauseMax);
  TEST_ASSERT_EQUAL_FLOAT(1, s.axis(Axis::Vertical).pauseMin);
  TEST_ASSERT_EQUAL_FLOAT(5, s.axis(Axis::Vertical).pauseMax);
  TEST_ASSERT_EQUAL_UINT8(10, s.sessionMinutes);
}

void test_rejects_minimum_speed_not_below_maximum() {
  RobotSettings s = defaultSettings();
  s.axis(Axis::Vertical).minSpeed = s.axis(Axis::Vertical).maxSpeed;
  assertRejected(s, SettingsError::MinSpeedOutOfRange, Axis::Vertical);

  s = defaultSettings();
  s.axis(Axis::Horizontal).minSpeed = kSlowestSpeed - 0.5f;
  assertRejected(s, SettingsError::MinSpeedOutOfRange, Axis::Horizontal);
}

// The on-flash record written by firmware 0.1.0, before the minimum speed.
struct AxisV1 {
  TravelLimits travel;
  float stow, maxSpeed, maxAccel;
};
struct SettingsV1 {
  AxisV1 axes[2];
  float playSpeed;
  bool calibrated;
};
struct RecordV1 {
  uint32_t magic;
  uint16_t version;
  uint16_t size;
  SettingsV1 settings;
};

void test_settings_from_0_1_0_are_kept_with_default_minimum_speeds() {
  RecordV1 old{0x43574331, 1, sizeof(SettingsV1),
               {{{{30, 150}, 100, 120, 500}, {{40, 140}, 70, 60, 300}}, 0.4f, true}};
  FakeStorage storage;
  storage.write("robot", &old, sizeof(old));

  RobotSettings s = loadSettings(storage);

  TEST_ASSERT_TRUE(s.calibrated);
  TEST_ASSERT_EQUAL_FLOAT(0.4f, s.playSpeed);
  TEST_ASSERT_EQUAL_FLOAT(30, s.axis(Axis::Horizontal).travel.min);
  TEST_ASSERT_EQUAL_FLOAT(150, s.axis(Axis::Horizontal).travel.max);
  TEST_ASSERT_EQUAL_FLOAT(100, s.axis(Axis::Horizontal).stow);
  TEST_ASSERT_EQUAL_FLOAT(120, s.axis(Axis::Horizontal).maxSpeed);
  TEST_ASSERT_EQUAL_FLOAT(500, s.axis(Axis::Horizontal).maxAccel);
  TEST_ASSERT_EQUAL_FLOAT(12, s.axis(Axis::Horizontal).minSpeed);
  TEST_ASSERT_EQUAL_FLOAT(70, s.axis(Axis::Vertical).stow);
  TEST_ASSERT_EQUAL_FLOAT(60, s.axis(Axis::Vertical).maxSpeed);
  TEST_ASSERT_EQUAL_FLOAT(6, s.axis(Axis::Vertical).minSpeed);
}

// The on-flash record written before the Play session: no Pause range nor
// Session duration.
struct AxisV2 {
  TravelLimits travel;
  float stow, maxSpeed, maxAccel, minSpeed;
};
struct SettingsV2 {
  AxisV2 axes[2];
  float playSpeed;
  bool calibrated;
};
struct RecordV2 {
  uint32_t magic;
  uint16_t version;
  uint16_t size;
  SettingsV2 settings;
};

void test_settings_from_v2_are_kept_with_default_pause_ranges() {
  RecordV2 old{0x43574331, 2, sizeof(SettingsV2),
               {{{{30, 150}, 100, 120, 500, 20}, {{40, 140}, 70, 60, 300, 15}}, 0.4f, true}};
  FakeStorage storage;
  storage.write("robot", &old, sizeof(old));

  RobotSettings s = loadSettings(storage);

  TEST_ASSERT_TRUE(s.calibrated);
  TEST_ASSERT_EQUAL_FLOAT(0.4f, s.playSpeed);
  TEST_ASSERT_EQUAL_FLOAT(30, s.axis(Axis::Horizontal).travel.min);
  TEST_ASSERT_EQUAL_FLOAT(150, s.axis(Axis::Horizontal).travel.max);
  TEST_ASSERT_EQUAL_FLOAT(100, s.axis(Axis::Horizontal).stow);
  TEST_ASSERT_EQUAL_FLOAT(120, s.axis(Axis::Horizontal).maxSpeed);
  TEST_ASSERT_EQUAL_FLOAT(500, s.axis(Axis::Horizontal).maxAccel);
  TEST_ASSERT_EQUAL_FLOAT(20, s.axis(Axis::Horizontal).minSpeed);
  TEST_ASSERT_EQUAL_FLOAT(15, s.axis(Axis::Vertical).minSpeed);
  TEST_ASSERT_EQUAL_FLOAT(0.5f, s.axis(Axis::Horizontal).pauseMin);
  TEST_ASSERT_EQUAL_FLOAT(3, s.axis(Axis::Horizontal).pauseMax);
  TEST_ASSERT_EQUAL_FLOAT(1, s.axis(Axis::Vertical).pauseMin);
  TEST_ASSERT_EQUAL_FLOAT(5, s.axis(Axis::Vertical).pauseMax);
  TEST_ASSERT_EQUAL_UINT8(10, s.sessionMinutes);
}

void test_rejects_pause_ranges_out_of_range() {
  RobotSettings s = defaultSettings();
  s.axis(Axis::Vertical).pauseMin = -0.1f;
  assertRejected(s, SettingsError::PauseOutOfRange, Axis::Vertical);

  s = defaultSettings();
  s.axis(Axis::Horizontal).pauseMax = 60.5f;
  assertRejected(s, SettingsError::PauseOutOfRange, Axis::Horizontal);

  s = defaultSettings();
  s.axis(Axis::Horizontal).pauseMin = 4;
  s.axis(Axis::Horizontal).pauseMax = 3;
  assertRejected(s, SettingsError::PauseOrder, Axis::Horizontal);

  s = defaultSettings();
  s.axis(Axis::Vertical).pauseMin = 0;
  s.axis(Axis::Vertical).pauseMax = 0;  // non-stop
  TEST_ASSERT_TRUE(validate(s).ok());
  s.axis(Axis::Vertical).pauseMin = 60;
  s.axis(Axis::Vertical).pauseMax = 60;
  TEST_ASSERT_TRUE(validate(s).ok());
}

void test_rejects_a_session_duration_out_of_range() {
  RobotSettings s = defaultSettings();
  s.sessionMinutes = 0;
  TEST_ASSERT_TRUE(validate(s).error == SettingsError::SessionDurationOutOfRange);
  s.sessionMinutes = 61;
  TEST_ASSERT_TRUE(validate(s).error == SettingsError::SessionDurationOutOfRange);
  s.sessionMinutes = 60;
  TEST_ASSERT_TRUE(validate(s).ok());
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
  s.axis(Axis::Vertical).minSpeed = 15;
  s.axis(Axis::Vertical).pauseMin = 0;
  s.axis(Axis::Vertical).pauseMax = 12.5f;
  s.sessionMinutes = 42;
  s.calibrated = true;

  TEST_ASSERT_TRUE(saveSettings(storage, s));
  RobotSettings loaded = loadSettings(storage);

  TEST_ASSERT_EQUAL_FLOAT(45, loaded.axis(Axis::Vertical).maxSpeed);
  TEST_ASSERT_EQUAL_FLOAT(15, loaded.axis(Axis::Vertical).minSpeed);
  TEST_ASSERT_EQUAL_FLOAT(0, loaded.axis(Axis::Vertical).pauseMin);
  TEST_ASSERT_EQUAL_FLOAT(12.5f, loaded.axis(Axis::Vertical).pauseMax);
  TEST_ASSERT_EQUAL_UINT8(42, loaded.sessionMinutes);
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
  RUN_TEST(test_default_minimum_speed_is_a_tenth_of_the_maximum);
  RUN_TEST(test_rejects_minimum_speed_not_below_maximum);
  RUN_TEST(test_settings_from_0_1_0_are_kept_with_default_minimum_speeds);
  RUN_TEST(test_default_pause_ranges_and_session_duration);
  RUN_TEST(test_settings_from_v2_are_kept_with_default_pause_ranges);
  RUN_TEST(test_rejects_pause_ranges_out_of_range);
  RUN_TEST(test_rejects_a_session_duration_out_of_range);
  RUN_TEST(test_first_boot_loads_defaults);
  RUN_TEST(test_saved_settings_survive_a_reboot);
  RUN_TEST(test_unknown_version_falls_back_to_defaults);
  RUN_TEST(test_corrupted_record_falls_back_to_defaults);
  RUN_TEST(test_stored_invalid_values_fall_back_to_defaults);
  return UNITY_END();
}
