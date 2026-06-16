#pragma once

#include <iostream>
#include <string>
#include "observer.hpp"

// ── domain types ──────────────────────────────────────────────────────────

enum class ArmState {
    IDLE, MOVING, GRASPING, HOLDING, RETURNING,
};

enum class ArmEvent {
    START, ARRIVED, GRASPED, RELEASE, HOME, ERROR,
};

struct StateChange {
    ArmState from;
    ArmState to;
    ArmEvent trigger;
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

// ── concrete observers ────────────────────────────────────────────────────

class Logger : public Observer<StateChange> {
public:
    void on_event(const StateChange& e) override {
        std::cout << "  [Logger]  " << to_string(e.from)
                  << " → " << to_string(e.to) << "\n";
    }
};

class SafetyMonitor : public Observer<StateChange> {
public:
    void on_event(const StateChange& e) override {
        if (e.trigger == ArmEvent::ERROR)
            std::cout << "  [Safety]  *** FAULT DETECTED — halting arm ***\n";
    }
};

class Dashboard : public Observer<StateChange> {
public:
    void on_event(const StateChange& e) override {
        std::cout << "  [Dash]    status → " << to_string(e.to) << "\n";
    }
};

// ── RobotArm: Subject that fires StateChange events ───────────────────────

class RobotArm : public Subject<StateChange> {
public:
    // Try to apply an event. Returns false if the transition is invalid.
    bool send(ArmEvent ev) {
        ArmState next;
        if (!transition(state_, ev, next)) return false;

        notify(StateChange{state_, next, ev});
        state_ = next;
        return true;
    }

    ArmState state() const { return state_; }

private:
    ArmState state_{ArmState::IDLE};

    static bool transition(ArmState s, ArmEvent ev, ArmState& out) {
        using S = ArmState; using E = ArmEvent;
        // clang-format off
        if (s == S::IDLE      && ev == E::START)   { out = S::MOVING;    return true; }
        if (s == S::MOVING    && ev == E::ARRIVED)  { out = S::GRASPING;  return true; }
        if (s == S::GRASPING  && ev == E::GRASPED)  { out = S::HOLDING;   return true; }
        if (s == S::HOLDING   && ev == E::RELEASE)  { out = S::RETURNING; return true; }
        if (s == S::RETURNING && ev == E::HOME)      { out = S::IDLE;      return true; }
        // ERROR from any active state
        if (ev == E::ERROR && s != S::IDLE)          { out = S::IDLE;      return true; }
        // clang-format on
        return false;
    }
};
