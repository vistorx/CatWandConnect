#pragma once

#include "robot_core/ports.h"

namespace cwc {

// Owns all safety-critical behaviour of CatWandConnect: Travel limits, speed,
// Stow priority, power-up and Calibration. Behaviour arrives ticket by ticket
// (see issue #1); for now it only wires up its collaborators.
class Robot {
 public:
  Robot(MotorDriver& motors, Clock& clock, Storage& storage);

  // Advances the robot by one control step. Call it often from the main loop.
  void tick();

 private:
  MotorDriver& motors_;
  Clock& clock_;
  Storage& storage_;
};

}  // namespace cwc
