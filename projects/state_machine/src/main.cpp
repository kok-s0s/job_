#include <iostream>
#include "robot_arm.hpp"

int main() {
    RobotArm arm;

    std::cout << "=== normal pick-and-place sequence ===\n";
    arm.send(ArmEvent::START,   "START — move to target");
    arm.send(ArmEvent::ARRIVED, "ARRIVED — at target");
    arm.send(ArmEvent::GRASPED, "GRASPED — force sensor OK");
    arm.send(ArmEvent::RELEASE, "RELEASE — drop command");
    arm.send(ArmEvent::HOME,    "HOME — back at origin");

    std::cout << "\n=== invalid event (ignored) ===\n";
    arm.send(ArmEvent::ARRIVED, "ARRIVED (arm is IDLE, nothing to ignore)");

    std::cout << "\n=== error recovery (emergency stop) ===\n";
    arm.send(ArmEvent::START,  "START — move to target");
    arm.send(ArmEvent::ERROR,  "ERROR — joint fault detected");

    std::cout << "\nfinal state: " << to_string(arm.state()) << "\n";
}
