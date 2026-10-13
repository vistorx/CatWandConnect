#pragma once

// Hobby PWM servos (MG996R on the Mk2) driven by the ESP32 LEDC peripheral,
// wired as described by a HardwareConfig.

#include <robot_core/hardware.h>
#include <robot_core/ports.h>

class PwmServoDriver : public cwc::MotorDriver {
 public:
  // Takes the wiring and starts the PWM channels, without sending pulses.
  void begin(const cwc::HardwareConfig& config);

  void moveTo(cwc::Axis axis, float degrees) override;
  void release(cwc::Axis axis) override;
  void reverse(cwc::Axis axis) override;

 private:
  cwc::HardwareConfig config_{};
};
