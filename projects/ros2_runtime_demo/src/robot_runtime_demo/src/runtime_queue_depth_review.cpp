#include <array>
#include <iostream>

namespace {

struct QueueDepthExperimentRow {
    int depth;
    const char* expected_behavior;
    const char* latency_risk;
    const char* verification;
};

struct QueueDepthTopicRow {
    const char* topic;
    const char* current_qos;
    const char* depth_reason;
};

constexpr std::array<QueueDepthTopicRow, 3> kTopicRows{{
    {
        "/robot/imu",
        "best_effort + volatile + keep_last(5)",
        "high-rate IMU data should stay fresh instead of replaying old samples",
    },
    {
        "/robot/joint_states",
        "best_effort + volatile + keep_last(5)",
        "runtime checks need recent joint positions and a bounded stale-data window",
    },
    {
        "/runtime/heartbeat",
        "reliable + volatile + keep_last(3)",
        "watchdog liveness should use a short reliable queue plus age_ms freshness checks",
    },
}};

constexpr std::array<QueueDepthExperimentRow, 3> kExperimentRows{{
    {
        1,
        "only the newest sample survives when the subscriber falls behind",
        "low queueing latency, but intermediate samples are easiest to overwrite",
        "compare imu_latency_ms and joint_latency_ms against the baseline",
    },
    {
        5,
        "current sensor stream baseline with a small burst buffer",
        "balanced freshness and short scheduling jitter tolerance",
        "verify [qos] logs show history=keep_last depth=5",
    },
    {
        20,
        "slow subscribers can keep more historical samples",
        "stale samples may accumulate and latency_ms can grow under backlog",
        "simulate input_rate > processing_rate and watch latency fields",
    },
}};

void printConfiguredDepths() {
    std::cout << "[queue_depth] configured topic depth\n";
    for (const auto& row : kTopicRows) {
        std::cout << "  topic=" << row.topic
                  << " qos=\"" << row.current_qos << "\""
                  << " reason=\"" << row.depth_reason << "\"\n";
    }
}

void printExperimentMatrix() {
    std::cout << "[queue_depth] depth experiment matrix\n";
    for (const auto& row : kExperimentRows) {
        std::cout << "  depth=" << row.depth
                  << " expected=\"" << row.expected_behavior << "\""
                  << " latency_risk=\"" << row.latency_risk << "\""
                  << " verification=\"" << row.verification << "\"\n";
    }
}

void printBacklogNotes() {
    std::cout << "[queue_depth] backlog notes\n";
    std::cout << "  backlog_condition=\"input_rate > processing_rate\"\n";
    std::cout << "  depth_tradeoff=\"small depth drops stale samples; large depth preserves history but can increase latency\"\n";
    std::cout << "  latency_fields=\"imu_latency_ms, joint_latency_ms, max_callback_duration_ms, heartbeat age_ms\"\n";
    std::cout << "  control_note=\"control commands should use Service or Action acknowledgement instead of Topic queue backlog\"\n";
}

void printInterviewSummary() {
    std::cout << "[queue_depth] interview summary\n";
    std::cout << "  Queue depth is a latency-versus-history tradeoff. For high-rate robot sensors, "
              << "a bounded keep_last queue protects freshness; a large queue can make a slow subscriber process stale state.\n";
}

}  // namespace

int main() {
    printConfiguredDepths();
    printExperimentMatrix();
    printBacklogNotes();
    printInterviewSummary();
    std::cout << "[ok] queue depth review ready\n";
    return 0;
}
