#include "runtime_state_machine.hpp"

#include <iostream>
#include <string>

namespace {

bool expectTransition(
    RuntimeState initial,
    RuntimeEvent event,
    RuntimeState expected_state,
    RuntimeError expected_error) {
    RuntimeStateMachine machine(initial);
    const bool transitioned = machine.process(event);
    if (!transitioned ||
        machine.state() != expected_state ||
        machine.error() != expected_error) {
        std::cerr << "[fail] expected "
                  << stateName(initial) << " --" << eventName(event) << "--> "
                  << stateName(expected_state) << " error=" << errorName(expected_error)
                  << ", got state=" << stateName(machine.state())
                  << " error=" << errorName(machine.error())
                  << " transitioned=" << transitioned << '\n';
        return false;
    }
    std::cout << "[case] "
              << stateName(initial) << " --" << eventName(event) << "--> "
              << stateName(expected_state) << " error=" << errorName(expected_error)
              << '\n';
    return true;
}

bool expectIgnored(RuntimeState initial, RuntimeEvent event) {
    RuntimeStateMachine machine(initial);
    const auto before_state = machine.state();
    const auto before_error = machine.error();
    const bool transitioned = machine.process(event);
    if (transitioned ||
        machine.state() != before_state ||
        machine.error() != before_error) {
        std::cerr << "[fail] expected ignored event "
                  << stateName(before_state) << " --" << eventName(event)
                  << "--> unchanged, got state=" << stateName(machine.state())
                  << " error=" << errorName(machine.error())
                  << " transitioned=" << transitioned << '\n';
        return false;
    }
    std::cout << "[case] "
              << stateName(before_state) << " ignored " << eventName(event)
              << " error=" << errorName(before_error) << '\n';
    return true;
}

bool expectFaultDoesNotAutoRecover() {
    RuntimeStateMachine machine;
    if (!machine.process(RuntimeEvent::SensorTimeout)) {
        std::cerr << "[fail] failed to enter Fault for guard test\n";
        return false;
    }

    const auto before_state = machine.state();
    const auto before_error = machine.error();
    const bool transitioned = machine.process(RuntimeEvent::SensorHealthy);
    if (transitioned ||
        machine.state() != before_state ||
        machine.error() != before_error) {
        std::cerr << "[fail] Fault auto recovered without ResetFault, got state="
                  << stateName(machine.state())
                  << " error=" << errorName(machine.error())
                  << " transitioned=" << transitioned << '\n';
        return false;
    }

    std::cout << "[case] "
              << stateName(before_state) << " ignored "
              << eventName(RuntimeEvent::SensorHealthy)
              << " error=" << errorName(before_error) << '\n';
    return true;
}

bool expectErrorInfo(
    RuntimeError error,
    RuntimeSeverity expected_severity,
    bool expected_recoverable,
    const std::string& expected_hint) {
    const auto info = errorInfo(error);
    if (info.severity != expected_severity ||
        info.recoverable != expected_recoverable ||
        expected_hint != info.recovery_hint) {
        std::cerr << "[fail] expected error info for "
                  << errorName(error)
                  << " severity=" << severityName(expected_severity)
                  << " recoverable=" << expected_recoverable
                  << " hint=" << expected_hint
                  << ", got severity=" << severityName(info.severity)
                  << " recoverable=" << info.recoverable
                  << " hint=" << info.recovery_hint << '\n';
        return false;
    }
    std::cout << "[case] "
              << errorName(error)
              << " severity=" << severityName(info.severity)
              << " recoverable=" << info.recoverable
              << " recovery_hint=" << info.recovery_hint << '\n';
    return true;
}

}  // namespace

int main() {
    bool ok = true;
    ok &= expectTransition(
        RuntimeState::Idle,
        RuntimeEvent::SensorHealthy,
        RuntimeState::Standby,
        RuntimeError::None);
    ok &= expectTransition(
        RuntimeState::Idle,
        RuntimeEvent::SensorTimeout,
        RuntimeState::Fault,
        RuntimeError::SensorTimeout);
    ok &= expectTransition(
        RuntimeState::Standby,
        RuntimeEvent::StartTask,
        RuntimeState::Running,
        RuntimeError::None);
    ok &= expectTransition(
        RuntimeState::Standby,
        RuntimeEvent::JointInvalid,
        RuntimeState::Fault,
        RuntimeError::JointStateInvalid);
    ok &= expectTransition(
        RuntimeState::Running,
        RuntimeEvent::TaskSucceeded,
        RuntimeState::Standby,
        RuntimeError::None);
    ok &= expectTransition(
        RuntimeState::Running,
        RuntimeEvent::TaskCanceled,
        RuntimeState::Standby,
        RuntimeError::None);
    ok &= expectTransition(
        RuntimeState::Running,
        RuntimeEvent::TaskFailed,
        RuntimeState::Fault,
        RuntimeError::TaskFailed);
    ok &= expectTransition(
        RuntimeState::Fault,
        RuntimeEvent::ResetFault,
        RuntimeState::Recovery,
        RuntimeError::None);
    ok &= expectTransition(
        RuntimeState::Recovery,
        RuntimeEvent::RecoveryDone,
        RuntimeState::Standby,
        RuntimeError::None);
    ok &= expectFaultDoesNotAutoRecover();
    ok &= expectIgnored(
        RuntimeState::Standby,
        RuntimeEvent::TaskSucceeded);
    ok &= expectErrorInfo(
        RuntimeError::SensorTimeout,
        RuntimeSeverity::Critical,
        true,
        "check sensor heartbeat then reset fault");
    ok &= expectErrorInfo(
        RuntimeError::JointStateInvalid,
        RuntimeSeverity::Critical,
        false,
        "inspect joint state publisher shape");
    ok &= expectErrorInfo(
        RuntimeError::TaskFailed,
        RuntimeSeverity::Warning,
        true,
        "inspect task result then reset fault");

    if (!ok) {
        return 1;
    }

    std::cout << "[ok] runtime state machine transitions verified\n";
    return 0;
}
