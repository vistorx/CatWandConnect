#include "servo_driver.h"

#include <Arduino.h>

namespace {

constexpr uint32_t kFrequencyHz = 50;
constexpr uint8_t kResolutionBits = 14;  // the highest every supported chip offers at 50 Hz
constexpr uint32_t kPeriodUs = 1000000 / kFrequencyHz;
constexpr uint32_t kMaxDuty = (1u << kResolutionBits) - 1;

uint8_t channel(cwc::Axis axis) { return static_cast<uint8_t>(axis); }

}  // namespace

PwmServoDriver::PwmServoDriver(const ServoConfig& horizontal, const ServoConfig& vertical)
    : configs_{horizontal, vertical} {}

void PwmServoDriver::begin() {
  for (cwc::Axis axis : {cwc::Axis::Horizontal, cwc::Axis::Vertical}) {
    ledcSetup(channel(axis), kFrequencyHz, kResolutionBits);
    ledcWrite(channel(axis), 0);  // no pulse until the Robot core commands a position
    ledcAttachPin(configs_[channel(axis)].pin, channel(axis));
  }
}

void PwmServoDriver::moveTo(cwc::Axis axis, float degrees) {
  const ServoConfig& config = configs_[channel(axis)];
  float fraction = constrain(degrees, 0.0f, 180.0f) / 180.0f;
  float pulseUs = config.minPulseUs + fraction * (config.maxPulseUs - config.minPulseUs);
  ledcWrite(channel(axis), static_cast<uint32_t>(pulseUs * kMaxDuty / kPeriodUs + 0.5f));
}

void PwmServoDriver::release(cwc::Axis axis) {
  // Without pulses the servo stops holding and rests.
  ledcWrite(channel(axis), 0);
}
