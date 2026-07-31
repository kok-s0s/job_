#include "runtime_state_machine.hpp"

#include <array>
#include <iostream>

namespace {

struct TransitionRow {
    RuntimeState state;
    RuntimeEvent event;
    RuntimeState next_state;
    RuntimeError error;
};

constexpr std::array<TransitionRow, 9> kCoreTransitions{{
    {RuntimeState::Idle, RuntimeEvent::SensorHealthy, RuntimeState::Standby, RuntimeError::None},
    {RuntimeState::Idle, RuntimeEvent::SensorTimeout, RuntimeState::Fault, RuntimeError::SensorTimeout},
    {RuntimeState::Standby, RuntimeEvent::StartTask, RuntimeState::Running, RuntimeError::None},
    {RuntimeState::Standby, RuntimeEvent::JointInvalid, RuntimeState::Fault, RuntimeError::JointStateInvalid},
    {RuntimeState::Running, RuntimeEvent::TaskSucceeded, RuntimeState::Standby, RuntimeError::None},
    {RuntimeState::Running, RuntimeEvent::TaskCanceled, RuntimeState::Standby, RuntimeError::None},
    {RuntimeState::Running, RuntimeEvent::TaskFailed, RuntimeState::Fault, RuntimeError::TaskFailed},
    {RuntimeState::Fault, RuntimeEvent::ResetFault, RuntimeState::Recovery, RuntimeError::None},
    {RuntimeState::Recovery, RuntimeEvent::RecoveryDone, RuntimeState::Standby, RuntimeError::None},
}};

void printTransitionTable() {
    std::cout << "[review] core transition table\n";
    for (const auto& row : kCoreTransitions) {
        std::cout << "  " << stateName(row.state)
                  << " --" << eventName(row.event) << "--> "
                  << stateName(row.next_state)
                  << " error=" << errorName(row.error) << '\n';
    }
}

void printFaultMetadata() {
    std::cout << "[review] fault metadata\n";
    constexpr std::array<RuntimeError, 5> errors{{
        RuntimeError::None,
        RuntimeError::SensorTimeout,
        RuntimeError::JointStateInvalid,
        RuntimeError::TaskFailed,
        RuntimeError::RecoveryFailed,
    }};
    for (const auto error : errors) {
        const auto info = errorInfo(error);
        std::cout << "  " << errorName(error)
                  << " severity=" << severityName(info.severity)
                  << " recoverable=" << static_cast<int>(info.recoverable)
                  << " reason=" << info.reason
                  << " recovery_hint=" << info.recovery_hint << '\n';
    }
}

void printInterviewPoints() {
    std::cout << "[review] interview points\n";
    std::cout << "  1. Keep RuntimeStateMachine as pure C++ and keep ROS2 callbacks as event adapters.\n";
    std::cout << "  2. External callers send events, not target states, so invalid transitions stay guarded.\n";
    std::cout << "  3. Fault recovery is explicit: ResetFault enters RECOVERY, RecoveryDone returns to STANDBY.\n";
    std::cout << "  4. RuntimeErrorInfo gives CLI, scripts, logs, and UI the same fault meaning.\n";
    std::cout << "  5. Service handles query/reset/apply_event, while Action handles long-running cancelable tasks.\n";
}

}  // namespace

int main() {
    printTransitionTable();
    printFaultMetadata();
    printInterviewPoints();
    std::cout << "[ok] week 3 runtime state machine review ready\n";
    return 0;
}
