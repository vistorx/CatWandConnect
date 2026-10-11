#pragma once

// How the servos are wired: GPIO, direction and pulse range per Axis, with
// the pin rules of each supported chip. Changing it takes a restart.

#include "robot_core/ports.h"

namespace cwc {

enum class Chip : uint8_t { Esp32, Esp32S3, Esp32C3 };

struct ServoHardware {
  uint8_t pin;
  bool inverted;         // swaps the direction: 0° becomes 180°
  uint16_t minPulseUs;   // pulse at 0°
  uint16_t maxPulseUs;   // pulse at 180°
};

struct HardwareConfig {
  ServoHardware axes[kAxisCount];

  ServoHardware& axis(Axis a) { return axes[static_cast<size_t>(a)]; }
  const ServoHardware& axis(Axis a) const { return axes[static_cast<size_t>(a)]; }
};

constexpr uint16_t kMinPulseUs = 300;
constexpr uint16_t kMaxPulseUs = 2700;
constexpr uint16_t kMinPulseSpanUs = 500;

HardwareConfig defaultHardware(Chip chip);

// GPIOs that may drive a servo: never flash, PSRAM, USB, the serial console,
// strapping or input-only pins.
struct PinList {
  const uint8_t* pins;
  size_t count;
};
PinList allowedPins(Chip chip);
bool pinAllowed(Chip chip, uint8_t pin);

enum class HardwareError : uint8_t { None, PinNotAllowed, PinDuplicate, PulseOutOfRange };

struct HardwareCheck {
  HardwareError error = HardwareError::None;
  Axis axis = Axis::Horizontal;

  bool ok() const { return error == HardwareError::None; }
};

HardwareCheck validateHardware(const HardwareConfig& config, Chip chip);
const char* hardwareErrorName(HardwareError error);

// True when the change mirrors or rescales the servo angles, so Travel limits
// found by Calibration no longer match the real positions.
bool invalidatesCalibration(const HardwareConfig& before, const HardwareConfig& after);

// Defaults when nothing valid for this chip is stored.
HardwareConfig loadHardware(Storage& storage, Chip chip);
bool saveHardware(Storage& storage, const HardwareConfig& config, Chip chip);

}  // namespace cwc
