#include "robot_core/hardware.h"

namespace cwc {

namespace {

// ESP32: no flash (6-11), PSRAM on WROVER (16, 17), serial (1, 3),
// strapping (0, 2, 5, 12, 15) or input-only (34-39) pins.
constexpr uint8_t kEsp32Pins[] = {4, 13, 14, 18, 19, 21, 22, 23, 25, 26, 27, 32, 33};
// ESP32-S3: no strapping (0, 3, 45, 46), USB (19, 20), flash and octal PSRAM
// (26-37) or serial (43, 44) pins.
constexpr uint8_t kEsp32S3Pins[] = {1, 2, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14,
                                    15, 16, 17, 18, 21, 38, 39, 40, 41, 42, 47, 48};
// ESP32-C3: no strapping (2, 8, 9), flash (11-17), USB (18, 19) or serial
// (20, 21) pins.
constexpr uint8_t kEsp32C3Pins[] = {0, 1, 3, 4, 5, 6, 7, 10};

constexpr const char* kKey = "hw";
constexpr uint32_t kMagic = 0x48574331;  // "HWC1"
constexpr uint16_t kVersion = 1;

struct Record {
  uint32_t magic;
  uint16_t version;
  uint8_t chip;
  uint8_t size;
  HardwareConfig config;
};

}  // namespace

HardwareConfig defaultHardware(Chip chip) {
  // Pulse ranges from the ESPHome proof of concept (2-11 % and 3-12 % of 20 ms).
  // GPIO 18 and 19 carry USB on the S3 and C3.
  bool esp32 = chip == Chip::Esp32;
  return {{
      {static_cast<uint8_t>(esp32 ? 18 : 4), false, 400, 2200},
      {static_cast<uint8_t>(esp32 ? 19 : 5), false, 600, 2400},
  }};
}

PinList allowedPins(Chip chip) {
  switch (chip) {
    case Chip::Esp32S3: return {kEsp32S3Pins, sizeof(kEsp32S3Pins)};
    case Chip::Esp32C3: return {kEsp32C3Pins, sizeof(kEsp32C3Pins)};
    case Chip::Esp32: break;
  }
  return {kEsp32Pins, sizeof(kEsp32Pins)};
}

bool pinAllowed(Chip chip, uint8_t pin) {
  PinList list = allowedPins(chip);
  for (size_t i = 0; i < list.count; ++i) {
    if (list.pins[i] == pin) return true;
  }
  return false;
}

HardwareCheck validateHardware(const HardwareConfig& config, Chip chip) {
  for (Axis axis : kAxes) {
    const ServoHardware& servo = config.axis(axis);
    if (!pinAllowed(chip, servo.pin)) return {HardwareError::PinNotAllowed, axis};
    if (servo.minPulseUs < kMinPulseUs || servo.maxPulseUs > kMaxPulseUs ||
        servo.minPulseUs + kMinPulseSpanUs > servo.maxPulseUs) {
      return {HardwareError::PulseOutOfRange, axis};
    }
  }
  if (config.axis(Axis::Horizontal).pin == config.axis(Axis::Vertical).pin) {
    return {HardwareError::PinDuplicate, Axis::Vertical};
  }
  return {};
}

const char* hardwareErrorName(HardwareError error) {
  switch (error) {
    case HardwareError::None: return "none";
    case HardwareError::PinNotAllowed: return "pin_not_allowed";
    case HardwareError::PinDuplicate: return "pin_duplicate";
    case HardwareError::PulseOutOfRange: return "pulse_out_of_range";
  }
  return "unknown";
}

bool invalidatesCalibration(const HardwareConfig& before, const HardwareConfig& after) {
  for (Axis axis : kAxes) {
    const ServoHardware& a = before.axis(axis);
    const ServoHardware& b = after.axis(axis);
    if (a.inverted != b.inverted || a.minPulseUs != b.minPulseUs || a.maxPulseUs != b.maxPulseUs) return true;
  }
  return false;
}

HardwareConfig loadHardware(Storage& storage, Chip chip) {
  Record record;
  if (!storage.read(kKey, &record, sizeof(record)) || record.magic != kMagic || record.version != kVersion ||
      record.chip != static_cast<uint8_t>(chip) || record.size != sizeof(HardwareConfig) ||
      !validateHardware(record.config, chip).ok()) {
    return defaultHardware(chip);
  }
  return record.config;
}

bool saveHardware(Storage& storage, const HardwareConfig& config, Chip chip) {
  // Recording the chip makes a config from another chip fall back to defaults.
  Record record{kMagic, kVersion, static_cast<uint8_t>(chip), sizeof(HardwareConfig), config};
  return storage.write(kKey, &record, sizeof(record));
}

}  // namespace cwc
