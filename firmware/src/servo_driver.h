#pragma once

// Hobby PWM servos (MG996R on the Mk2) driven by the ESP32 LEDC peripheral.

#include <robot_core/ports.h>

struct ServoConfig {
  uint8_t pin;
  uint16_t minPulseUs;  // pulse at 0°
  uint16_t maxPulseUs;  // pulse at 180°
};

class PwmServoDriver : public cwc::MotorDriver {
 public:
  PwmServoDriver(const ServoConfig& horizontal, const ServoConfig& vertical);

  void begin();
  void moveTo(cwc::Axis axis, float degrees) override;
  void release(cwc::Axis axis) override;

 private:
  ServoConfig configs_[cwc::kAxisCount];
};
