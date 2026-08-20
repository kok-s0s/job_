#include <array>
#include <iostream>

namespace {

struct SessionTraceRow {
    const char* surface;
    const char* evidence;
    const char* purpose;
};

constexpr std::array<SessionTraceRow, 4> kRows{{
    {
        "[runtime_log]",
        "session_id=<id> appears beside node, level, event, state, and runtime_error",
        "correlate structured runtime logs from one experiment",
    },
    {
        "/runtime/heartbeat",
        "heartbeat payload includes session_id=<id>",
        "join sensor, runtime, monitor, and watchdog liveness records",
    },
    {
        "/runtime/query_status",
        "service response begins with session_id=<id>",
        "make CLI snapshots traceable to the same run",
    },
    {
        "[perf]",
        "performance summary includes session_id=<id>",
        "connect latency and callback-duration evidence to one capture",
    },
}};

void printSessionTraceTable() {
    std::cout << "[session_trace] runtime session trace surfaces\n";
    for (const auto& row : kRows) {
        std::cout << "  surface=" << row.surface
                  << " evidence=\"" << row.evidence << "\""
                  << " purpose=\"" << row.purpose << "\"\n";
    }
}

void printCommandNotes() {
    std::cout << "[session_trace] command notes\n";
    std::cout << "  set_session=\"export ROBOT_RUNTIME_SESSION_ID=session_2026_08_19\"\n";
    std::cout << "  launch=\"ros2 launch robot_runtime_demo runtime_demo.launch.py\"\n";
    std::cout << "  query=\"ros2 service call /runtime/query_status std_srvs/srv/Trigger {}\"\n";
    std::cout << "  verify=\"grep session_id in [runtime_log], [heartbeat], [perf], and query_status response\"\n";
}

void printInterviewSummary() {
    std::cout << "[session_trace] interview summary\n";
    std::cout << "  A session id turns distributed ROS2 logs into one traceable experiment. "
              << "When sensor data, heartbeat, runtime state, service snapshots, and perf metrics share the same session_id, "
              << "I can correlate one failure across nodes and later match it with rosbag2 evidence.\n";
}

}  // namespace

int main() {
    printSessionTraceTable();
    printCommandNotes();
    printInterviewSummary();
    std::cout << "[ok] session trace review ready\n";
    return 0;
}
