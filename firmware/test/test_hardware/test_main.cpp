#include <robot_core/hardware.h>
#include <robot_core/robot.h>
#include <unity.h>

#include "../fakes.h"

using namespace cwc;
using namespace cwc::test;

void setUp() {}
void tearDown() {}

void test_defaults_are_valid_on_every_chip() {
  for (Chip chip : {Chip::Esp32, Chip::Esp32S3, Chip::Esp32C3}) {
    TEST_ASSERT_TRUE(validateHardware(defaultHardware(chip), chip).ok());
  }
}

void test_esp32_defaults_match_the_proof_of_concept_wiring() {
  HardwareConfig hw = defaultHardware(Chip::Esp32);
  TEST_ASSERT_EQUAL(18, hw.axis(Axis::Horizontal).pin);
  TEST_ASSERT_EQUAL(19, hw.axis(Axis::Vertical).pin);
}

void test_rejects_flash_strapping_input_only_and_usb_pins() {
  TEST_ASSERT_FALSE(pinAllowed(Chip::Esp32, 6));    // flash
  TEST_ASSERT_FALSE(pinAllowed(Chip::Esp32, 0));    // strapping
  TEST_ASSERT_FALSE(pinAllowed(Chip::Esp32, 34));   // input-only
  TEST_ASSERT_FALSE(pinAllowed(Chip::Esp32, 1));    // serial console
  TEST_ASSERT_FALSE(pinAllowed(Chip::Esp32S3, 19)); // USB
  TEST_ASSERT_FALSE(pinAllowed(Chip::Esp32S3, 35)); // octal PSRAM
  TEST_ASSERT_FALSE(pinAllowed(Chip::Esp32C3, 9));  // strapping
  TEST_ASSERT_FALSE(pinAllowed(Chip::Esp32C3, 18)); // USB
  TEST_ASSERT_TRUE(pinAllowed(Chip::Esp32, 25));
  TEST_ASSERT_TRUE(pinAllowed(Chip::Esp32S3, 18));
  TEST_ASSERT_TRUE(pinAllowed(Chip::Esp32C3, 4));
}

void test_rejects_a_pin_the_chip_cannot_use() {
  HardwareConfig hw = defaultHardware(Chip::Esp32C3);
  hw.axis(Axis::Vertical).pin = 19;

  HardwareCheck check = validateHardware(hw, Chip::Esp32C3);

  TEST_ASSERT_TRUE(check.error == HardwareError::PinNotAllowed);
  TEST_ASSERT_TRUE(check.axis == Axis::Vertical);
}

void test_rejects_both_servos_on_one_pin() {
  HardwareConfig hw = defaultHardware(Chip::Esp32);
  hw.axis(Axis::Vertical).pin = 18;

  TEST_ASSERT_TRUE(validateHardware(hw, Chip::Esp32).error == HardwareError::PinDuplicate);
}

void test_rejects_bad_pulse_ranges() {
  HardwareConfig hw = defaultHardware(Chip::Esp32);
  hw.axis(Axis::Horizontal).minPulseUs = 100;
  TEST_ASSERT_TRUE(validateHardware(hw, Chip::Esp32).error == HardwareError::PulseOutOfRange);

  hw = defaultHardware(Chip::Esp32);
  hw.axis(Axis::Vertical).maxPulseUs = 900;  // below min + span
  TEST_ASSERT_TRUE(validateHardware(hw, Chip::Esp32).error == HardwareError::PulseOutOfRange);
}

void test_inversion_or_pulse_change_invalidates_calibration_but_pins_do_not() {
  HardwareConfig before = defaultHardware(Chip::Esp32);

  HardwareConfig pins = before;
  pins.axis(Axis::Horizontal).pin = 25;
  TEST_ASSERT_FALSE(invalidatesCalibration(before, pins));

  HardwareConfig inverted = before;
  inverted.axis(Axis::Vertical).inverted = true;
  TEST_ASSERT_TRUE(invalidatesCalibration(before, inverted));

  HardwareConfig pulse = before;
  pulse.axis(Axis::Horizontal).maxPulseUs = 2400;
  TEST_ASSERT_TRUE(invalidatesCalibration(before, pulse));
}

void test_saved_hardware_survives_a_reboot() {
  FakeStorage storage;
  HardwareConfig hw = defaultHardware(Chip::Esp32);
  hw.axis(Axis::Horizontal).pin = 25;
  hw.axis(Axis::Horizontal).inverted = true;

  saveHardware(storage, hw, Chip::Esp32);
  HardwareConfig loaded = loadHardware(storage, Chip::Esp32);

  TEST_ASSERT_EQUAL(25, loaded.axis(Axis::Horizontal).pin);
  TEST_ASSERT_TRUE(loaded.axis(Axis::Horizontal).inverted);
}

void test_hardware_from_another_chip_falls_back_to_defaults() {
  FakeStorage storage;
  HardwareConfig hw = defaultHardware(Chip::Esp32);
  hw.axis(Axis::Horizontal).pin = 25;
  saveHardware(storage, hw, Chip::Esp32);

  HardwareConfig loaded = loadHardware(storage, Chip::Esp32C3);

  TEST_ASSERT_EQUAL(4, loaded.axis(Axis::Horizontal).pin);
}

void test_invalidating_calibration_restores_default_limits() {
  FakeMotorDriver motors;
  FakeClock clock;
  FakeStorage storage;
  FakeRandom random;
  Robot robot(motors, clock, storage, random);
  robot.begin();
  RobotSettings s = robot.settings();
  s.axis(Axis::Horizontal).travel = {20, 160};
  s.calibrated = true;
  saveSettings(storage, s);
  robot.begin();
  TEST_ASSERT_EQUAL_FLOAT(160, robot.travelLimits(Axis::Horizontal).max);

  robot.invalidateCalibration();

  TEST_ASSERT_EQUAL_FLOAT(kDefaultTravel.max, robot.travelLimits(Axis::Horizontal).max);
  TEST_ASSERT_FALSE(loadSettings(storage).calibrated);
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_defaults_are_valid_on_every_chip);
  RUN_TEST(test_esp32_defaults_match_the_proof_of_concept_wiring);
  RUN_TEST(test_rejects_flash_strapping_input_only_and_usb_pins);
  RUN_TEST(test_rejects_a_pin_the_chip_cannot_use);
  RUN_TEST(test_rejects_both_servos_on_one_pin);
  RUN_TEST(test_rejects_bad_pulse_ranges);
  RUN_TEST(test_inversion_or_pulse_change_invalidates_calibration_but_pins_do_not);
  RUN_TEST(test_saved_hardware_survives_a_reboot);
  RUN_TEST(test_hardware_from_another_chip_falls_back_to_defaults);
  RUN_TEST(test_invalidating_calibration_restores_default_limits);
  return UNITY_END();
}
