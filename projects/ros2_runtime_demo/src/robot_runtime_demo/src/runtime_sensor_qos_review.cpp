#include <array>
#include <iostream>

namespace {

struct SensorQosRow {
    const char* topic;
    const char* role;
    const char* qos;
    const char* reason;
};

constexpr std::array<SensorQosRow, 3> kRows{{
    {
        "/robot/imu",
        "high_rate_sensor_stream",
        "best_effort + volatile + keep_last(5)",
        "newest IMU sample is more useful than retransmitting old frames",
    },
    {
        "/robot/joint_states",
        "high_rate_sensor_stream",
        "best_effort + volatile + keep_last(5)",
        "monitoring needs fresh joint state and should not build a stale queue",
    },
    {
        "/runtime/heartbeat",
        "node_liveness_signal",
        "reliable + volatile + keep_last(3)",
        "watchdog decisions should not be based on casually dropped heartbeat samples",
    },
}};

void printSensorQosTable() {
    std::cout << "[sensor_qos] configured topic QoS\n";
    for (const auto& row : kRows) {
        std::cout << "  topic=" << row.topic
                  << " role=" << row.role
                  << " qos=\"" << row.qos << "\""
                  << " reason=\"" << row.reason << "\"\n";
    }
}

void printCompatibilityNotes() {
    std::cout << "[sensor_qos] compatibility notes\n";
    std::cout << "  best_effort_echo=\"ros2 topic echo /robot/imu --once --qos-reliability best_effort\"\n";
    std::cout << "  reliable_mismatch=\"a reliable subscriber may not match a best_effort publisher\"\n";
    std::cout << "  topic_info_fields=\"Reliability: BEST_EFFORT, Durability: VOLATILE\"\n";
    std::cout << "  depth_note=\"current ROS2 CLI may show History (Depth): UNKNOWN, so depth is verified by code and [qos] logs\"\n";
}

void printInterviewSummary() {
    std::cout << "[sensor_qos] interview summary\n";
    std::cout << "  Sensor Topics prefer freshness over retransmission, so /robot/imu and /robot/joint_states use best effort. "
              << "Control paths and task results still need deterministic success/failure semantics, so they should remain reliable.\n";
}

}  // namespace

int main() {
    printSensorQosTable();
    printCompatibilityNotes();
    printInterviewSummary();
    std::cout << "[ok] sensor best-effort QoS review ready\n";
    return 0;
}
