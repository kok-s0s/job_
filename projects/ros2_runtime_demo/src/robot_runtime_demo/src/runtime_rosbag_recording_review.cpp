#include <array>
#include <iostream>

namespace {

struct RosbagTopicRow {
    const char* topic;
    const char* why_record;
    const char* replay_value;
};

constexpr std::array<RosbagTopicRow, 3> kRows{{
    {
        "/robot/imu",
        "captures high-rate motion evidence and sensor freshness",
        "lets runtime subscribers replay the same IMU sequence during debugging",
    },
    {
        "/robot/joint_states",
        "captures joint shape, names, positions, and update cadence",
        "reproduces joint_valid and latency observations without live hardware",
    },
    {
        "/runtime/heartbeat",
        "captures node liveness evidence and age_ms inputs for watchdog reasoning",
        "replays node health timeline beside sensor samples",
    },
}};

void printRecordingPlan() {
    std::cout << "[rosbag_recording] topic recording plan\n";
    for (const auto& row : kRows) {
        std::cout << "  topic=" << row.topic
                  << " why=\"" << row.why_record << "\""
                  << " replay_value=\"" << row.replay_value << "\"\n";
    }
}

void printCommandNotes() {
    std::cout << "[rosbag_recording] command notes\n";
    std::cout << "  record=\"ros2 bag record -o runtime_capture /robot/imu /robot/joint_states /runtime/heartbeat\"\n";
    std::cout << "  inspect=\"ros2 bag info runtime_capture\"\n";
    std::cout << "  replay=\"ros2 bag play runtime_capture\"\n";
    std::cout << "  verification=\"bag info must list all three topics and a positive message count\"\n";
}

void printInterviewSummary() {
    std::cout << "[rosbag_recording] interview summary\n";
    std::cout << "  rosbag2 turns a live robot runtime problem into replayable evidence. "
              << "I record sensor streams and heartbeat together so later debugging can correlate data freshness, "
              << "node liveness, and runtime state without needing the original live run.\n";
}

}  // namespace

int main() {
    printRecordingPlan();
    printCommandNotes();
    printInterviewSummary();
    std::cout << "[ok] rosbag recording review ready\n";
    return 0;
}
