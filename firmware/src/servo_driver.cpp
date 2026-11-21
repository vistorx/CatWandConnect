#include "servo_driver.h"

#include <Arduino.h>

#include "log.h"

namespace {

constexpr uint32_t kFrequencyHz = 50;
constexpr uint8_t kResolutionBits = 14;  // the highest every supported chip offers at 50 Hz
constexpr uint32_t kPeriodUs = 1000000 / kFrequencyHz;
constexpr uint32_t kMaxDuty = (1u << kResolutionBits) - 1;

uint8_t channel(cwc::Axis axis) { return static_cast<uint8_t>(axis); }

}  // namespace

// Arduino-ESP32 3.x addresses LEDC by pin; each axis keeps its own channel
// (0 and 1, as before), so both share one 50 Hz timer.
void PwmServoDriver::begin(const cwc::HardwareConfig& config) {
  config_ = config;
  for (cwc::Axis axis : cwc::kAxes) {
    const uint8_t pin = config_.axis(axis).pin;
    if (!ledcAttachChannel(pin, kFrequencyHz, kResolutionBits, channel(axis))) {
      logLine("[servo] PWM setup failed on pin %u", unsigned(pin));
      continue;
    }
    ledcWrite(pin, 0);  // no pulse until the Robot core commands a position
  }
}

void PwmServoDriver::moveTo(cwc::Axis axis, float degrees) {
  const cwc::ServoHardware& servo = config_.axis(axis);
  float fraction = constrain(degrees, 0.0f, 180.0f) / 180.0f;
  if (servo.inverted) fraction = 1 - fraction;
  float pulseUs = servo.minPulseUs + fraction * (servo.maxPulseUs - servo.minPulseUs);
  ledcWrite(servo.pin, static_cast<uint32_t>(pulseUs * kMaxDuty / kPeriodUs + 0.5f));
}

void PwmServoDriver::release(cwc::Axis axis) {
  // Without pulses the servo stops holding and rests.
  ledcWrite(config_.axis(axis).pin, 0);
}

void PwmServoDriver::reverse(cwc::Axis axis) {
  // Flips the direction from the next pulse on; the Robot core mirrors its
  // angles at the same time, so the same pulse keeps going out.
  cwc::ServoHardware& servo = config_.axis(axis);
  servo.inverted = !servo.inverted;
}
