#include <iostream>
#include "robot_arm.hpp"

int main() {
    RobotArm arm;

    Logger        logger;
    SafetyMonitor safety;
    Dashboard     dash;

    arm.subscribe(&logger);
    arm.subscribe(&safety);
    arm.subscribe(&dash);

    std::cout << "=== normal pick-and-place ===\n";
    for (auto ev : {ArmEvent::START, ArmEvent::ARRIVED,
                    ArmEvent::GRASPED, ArmEvent::RELEASE, ArmEvent::HOME}) {
        std::cout << "--- send event ---\n";
        arm.send(ev);
    }

    std::cout << "\n=== unsubscribe dashboard, then error recovery ===\n";
    arm.unsubscribe(&dash);
    arm.send(ArmEvent::START);
    std::cout << "--- ERROR ---\n";
    arm.send(ArmEvent::ERROR);

    std::cout << "\nfinal state: " << to_string(arm.state()) << "\n";
}
