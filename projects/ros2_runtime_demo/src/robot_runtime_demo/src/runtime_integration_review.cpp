#include <array>
#include <iostream>

namespace {

struct IntegrationRow {
    const char* module;
    const char* interface_name;
    const char* responsibility;
    const char* evidence;
};

constexpr std::array<IntegrationRow, 6> kIntegrationRows{{
    {
        "sensor_sim_node",
        "/robot/imu, /robot/joint_states",
        "publish typed sensor streams at 10Hz",
        "ros2 topic echo with best_effort + ros2 topic info --verbose",
    },
    {
        "runtime_node",
        "Topic subscriber + /runtime/query_status + /runtime/reset_fault",
        "maintain runtime state, latency, health, and fault reset",
        "query_status message contains state, runtime_error, latency, and health fields",
    },
    {
        "runtime_node",
        "/runtime/apply_event",
        "adapt external event names into guarded RuntimeStateMachine transitions",
        "SensorTimeout -> ResetFault -> RecoveryDone recovery chain",
    },
    {
        "runtime_node",
        "/runtime/execute_task",
        "run cancelable long tasks without blocking heartbeat or services",
        "Action feedback, result, rejection, and cancellation outputs",
    },
    {
        "heartbeat_monitor_node",
        "/runtime/heartbeat",
        "track last_seq, last_stamp_ms, age_ms, and status for every node",
        "[heartbeat_table] node=... status=OK",
    },
    {
        "watchdog_node",
        "/runtime/heartbeat + /runtime/apply_event",
        "turn stale heartbeat into timeout evidence and runtime Fault",
        "[watchdog] node=... status=OK/TIMEOUT",
    },
}};

struct EvidenceRow {
    const char* evidence;
    const char* proves;
};

constexpr std::array<EvidenceRow, 6> kEvidenceRows{{
    {
        "[ok] runtime state machine transitions verified",
        "pure C++ RuntimeStateMachine guards legal and illegal transitions",
    },
    {
        "[runtime_log] ... event=heartbeat ...",
        "runtime_node emits structured key-value logs for observability",
    },
    {
        "[heartbeat_table] node=runtime_node ... status=OK",
        "heartbeat subscription and node freshness tracking work",
    },
    {
        "[watchdog] node=runtime_node status=OK ...",
        "watchdog observes heartbeat age and can detect timeout state",
    },
    {
        "[perf] node=runtime_node ... max_callback_duration_ms=...",
        "performance metrics separate callback cost, latency, and action duration",
    },
    {
        "Result: success: true message: task completed",
        "ExecuteTask Action can finish a long-running task and return result",
    },
}};

void printIntegrationTopology() {
    std::cout << "[integration] phase 1 ROS2 runtime topology\n";
    for (const auto& row : kIntegrationRows) {
        std::cout << "  module=" << row.module
                  << " interface=\"" << row.interface_name << "\""
                  << " responsibility=\"" << row.responsibility << "\""
                  << " evidence=\"" << row.evidence << "\"\n";
    }
}

void printEvidenceChecklist() {
    std::cout << "[integration] phase 1 evidence checklist\n";
    for (const auto& row : kEvidenceRows) {
        std::cout << "  evidence=\"" << row.evidence << "\""
                  << " proves=\"" << row.proves << "\"\n";
    }
}

void printInterviewPitch() {
    std::cout << "[integration] phase 1 interview pitch\n";
    std::cout << "  I built a ROS2 robot runtime demo as a closed loop system: "
              << "typed sensor Topics feed runtime_node, Services expose query/reset/event injection, "
              << "Action handles cancelable long-running tasks, and heartbeat/watchdog/perf logs make the system observable.\n";
}

void printNextWeekQuestions() {
    std::cout << "[integration] next week QoS entry questions\n";
    std::cout << "  1. Why should high-rate sensor streams usually prefer best effort?\n";
    std::cout << "  2. Why should control commands and runtime events usually prefer reliable?\n";
    std::cout << "  3. How does queue depth change the tradeoff between message loss and latency?\n";
}

}  // namespace

int main() {
    printIntegrationTopology();
    printEvidenceChecklist();
    printInterviewPitch();
    printNextWeekQuestions();
    std::cout << "[ok] phase 1 ROS2 runtime integration review ready\n";
    return 0;
}
