#pragma once

#include <iostream>
#include <string>
#include "state_machine.hpp"

enum class ArmState {
    IDLE,       // waiting for command
    MOVING,     // arm travelling to target position
    GRASPING,   // gripper closing
    HOLDING,    // object secured
    RETURNING,  // arm travelling back to home
};

enum class ArmEvent {
    START,    // operator sends move command
    ARRIVED,  // position sensor confirms target reached
    GRASPED,  // force sensor confirms grip
    RELEASE,  // operator sends release command
    HOME,     // position sensor confirms home reached
    ERROR,    // any fault signal
};

inline std::string to_string(ArmState s) {
    switch (s) {
        case ArmState::IDLE:      return "IDLE";
        case ArmState::MOVING:    return "MOVING";
        case ArmState::GRASPING:  return "GRASPING";
        case ArmState::HOLDING:   return "HOLDING";
        case ArmState::RETURNING: return "RETURNING";
    }
    return "?";
}

class RobotArm {
public:
    RobotArm() : sm_(ArmState::IDLE) {
        // ── lifecycle callbacks ───────────────────────────────────────────
        sm_.on_enter(ArmState::IDLE,      [] { log("waiting for command"); });
        sm_.on_enter(ArmState::MOVING,    [] { log("moving to target position..."); });
        sm_.on_enter(ArmState::GRASPING,  [] { log("closing gripper..."); });
        sm_.on_enter(ArmState::HOLDING,   [] { log("object secured"); });
        sm_.on_enter(ArmState::RETURNING, [] { log("returning to home position..."); });

        sm_.on_exit(ArmState::MOVING,    [] { log("(motion stopped)"); });
        sm_.on_exit(ArmState::RETURNING, [] { log("(motion stopped)"); });

        // ── normal transitions ────────────────────────────────────────────
        sm_.add(ArmState::IDLE,      ArmEvent::START,   ArmState::MOVING);
        sm_.add(ArmState::MOVING,    ArmEvent::ARRIVED, ArmState::GRASPING);
        sm_.add(ArmState::GRASPING,  ArmEvent::GRASPED, ArmState::HOLDING);
        sm_.add(ArmState::HOLDING,   ArmEvent::RELEASE, ArmState::RETURNING);
        sm_.add(ArmState::RETURNING, ArmEvent::HOME,    ArmState::IDLE);

        // ── ERROR from any active state → emergency stop → IDLE ──────────
        for (auto s : {ArmState::MOVING, ArmState::GRASPING,
                        ArmState::HOLDING, ArmState::RETURNING}) {
            sm_.add(s, ArmEvent::ERROR, ArmState::IDLE,
                    [] { log("!!! EMERGENCY STOP !!!"); });
        }
    }

    // Send an event. Returns false if ignored (no matching transition).
    bool send(ArmEvent ev, const std::string& label = "") {
        if (!label.empty())
            std::cout << "\nevent: " << label << "\n";
        bool fired = sm_.process(ev);
        if (!fired)
            std::cout << "  [ignored] no transition from "
                      << to_string(sm_.current()) << " for this event\n";
        return fired;
    }

    ArmState state() const { return sm_.current(); }

private:
    StateMachine<ArmState, ArmEvent> sm_;

    static void log(const std::string& msg) {
        std::cout << "  [arm] " << msg << "\n";
    }
};
