#include "robot_core/robot.h"

namespace cwc {

Robot::Robot(MotorDriver& motors, Clock& clock, Storage& storage)
    : motors_(motors), clock_(clock), storage_(storage) {}

void Robot::tick() {}

}  // namespace cwc
