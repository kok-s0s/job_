#include <array>
#include <iostream>

namespace {

struct DataLoopRow {
    const char* phase;
    const char* tool_or_signal;
    const char* evidence;
};

constexpr std::array<DataLoopRow, 5> kRows{{
    {
        "capture",
        "ros2 bag record -o runtime_capture /robot/imu /robot/joint_states /runtime/heartbeat",
        "typed sensor data and heartbeat snapshots are stored as replayable evidence",
    },
    {
        "trace",
        "ROBOT_RUNTIME_SESSION_ID=session_YYYY_MM_DD",
        "runtime_log, heartbeat, query_status, perf, and bag notes share one experiment id",
    },
    {
        "replay",
        "ros2 bag info runtime_capture && ros2 bag play runtime_capture",
        "topic names, message counts, and timeline can be inspected after the live run",
    },
    {
        "diagnose",
        "query_status + [runtime_log] + [perf]",
        "state, runtime_error, severity, latency, callback duration, and heartbeat age explain the failure",
    },
    {
        "verify",
        "bash scripts/verify_fault_reproduction.sh",
        "SensorTimeout -> FAULT -> ResetFault -> RecoveryDone -> STANDBY stays repeatable",
    },
}};

void printDataLoop() {
    std::cout << "[data_loop] capture replay debug loop\n";
    for (const auto& row : kRows) {
        std::cout << "  phase=" << row.phase
                  << " tool=\"" << row.tool_or_signal << "\""
                  << " evidence=\"" << row.evidence << "\"\n";
    }
}

void printEvidenceChain() {
    std::cout << "[data_loop] week 6 evidence chain\n";
    std::cout << "  loop=\"capture -> replay -> diagnose -> fix -> verify\"\n";
    std::cout << "  record_topics=\"/robot/imu /robot/joint_states /runtime/heartbeat\"\n";
    std::cout << "  session_trace=\"ROBOT_RUNTIME_SESSION_ID ties logs, services, heartbeat, perf, and bags together\"\n";
    std::cout << "  fault_path=\"verify_fault_reproduction.sh proves the same failure and recovery path repeatedly\"\n";
}

void printInterviewSummary() {
    std::cout << "[data_loop] interview summary\n";
    std::cout << "  I treat rosbag2, session_id, structured runtime logs, query_status, perf metrics, "
              << "and deterministic fault scripts as one debugging loop: record the runtime scene, "
              << "replay or inspect it offline, diagnose with state and timing evidence, apply the fix, "
              << "then rerun the same verification script.\n";
}

}  // namespace

int main() {
    printDataLoop();
    printEvidenceChain();
    printInterviewSummary();
    std::cout << "[ok] data loop review ready\n";
    return 0;
}
