#include <array>
#include <iostream>

namespace {

struct FaultReproductionRow {
    const char* step;
    const char* command_or_signal;
    const char* expected_evidence;
};

constexpr std::array<FaultReproductionRow, 5> kRows{{
    {
        "start",
        "ROBOT_RUNTIME_SESSION_ID=fault_repro_YYYYMMDD ros2 launch robot_runtime_demo runtime_demo.launch.py",
        "all nodes publish heartbeat with the same session_id",
    },
    {
        "baseline",
        "ros2 service call /runtime/query_status std_srvs/srv/Trigger {}",
        "state=STANDBY runtime_error=NONE",
    },
    {
        "inject",
        "ros2 service call /runtime/apply_event robot_runtime_demo/srv/ApplyRuntimeEvent {event: SensorTimeout}",
        "accepted=True transitioned=True current_state='FAULT'",
    },
    {
        "verify",
        "ros2 service call /runtime/query_status std_srvs/srv/Trigger {}",
        "success=False state=FAULT runtime_error=SENSOR_TIMEOUT runtime_severity=CRITICAL",
    },
    {
        "recover",
        "ResetFault then RecoveryDone",
        "state=STANDBY runtime_error=NONE",
    },
}};

void printFaultReproductionPlan() {
    std::cout << "[fault_reproduction] deterministic fault path\n";
    for (const auto& row : kRows) {
        std::cout << "  step=" << row.step
                  << " command=\"" << row.command_or_signal << "\""
                  << " expected=\"" << row.expected_evidence << "\"\n";
    }
}

void printEvidenceNotes() {
    std::cout << "[fault_reproduction] evidence notes\n";
    std::cout << "  failure_signal=\"SensorTimeout enters FAULT with runtime_error=SENSOR_TIMEOUT\"\n";
    std::cout << "  traceability=\"session_id ties service calls, runtime_log, heartbeat, and perf lines together\"\n";
    std::cout << "  recovery_chain=\"ResetFault -> RECOVERY, RecoveryDone -> STANDBY\"\n";
    std::cout << "  script=\"bash scripts/verify_fault_reproduction.sh\"\n";
}

void printInterviewSummary() {
    std::cout << "[fault_reproduction] interview summary\n";
    std::cout << "  I keep a deterministic fault reproduction script for the runtime state machine. "
              << "It fixes the session_id, triggers SensorTimeout through the public apply_event service, "
              << "checks FAULT metadata, then proves the recovery chain returns to STANDBY.\n";
}

}  // namespace

int main() {
    printFaultReproductionPlan();
    printEvidenceNotes();
    printInterviewSummary();
    std::cout << "[ok] fault reproduction review ready\n";
    return 0;
}
