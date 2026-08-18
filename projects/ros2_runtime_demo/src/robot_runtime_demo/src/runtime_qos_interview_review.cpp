#include <array>
#include <iostream>

namespace {

struct QosDecisionRow {
    const char* channel;
    const char* qos_choice;
    const char* engineering_reason;
    const char* interview_phrase;
};

constexpr std::array<QosDecisionRow, 4> kRows{{
    {
        "/robot/imu, /robot/joint_states",
        "best_effort + volatile + keep_last(5)",
        "high-rate sensor streams need fresh state more than old sample replay",
        "sensor streams prefer freshness over retransmission",
    },
    {
        "/runtime/heartbeat",
        "reliable + volatile + keep_last(3)",
        "watchdog liveness should tolerate small scheduling jitter without building a stale queue",
        "heartbeat is low-rate liveness evidence, checked again with age_ms",
    },
    {
        "/runtime/query_status, /runtime/reset_fault, /runtime/apply_event",
        "Service request/response",
        "control callers need an explicit success, failure, rejection, and state transition result",
        "control commands require deterministic acknowledgement",
    },
    {
        "/runtime/execute_task",
        "Action goal + feedback + result + cancel",
        "long-running work must expose acceptance, progress, completion, rejection, and cancellation",
        "actions are reliable task contracts, not fire-and-forget messages",
    },
}};

void printDecisionTable() {
    std::cout << "[qos_interview] week 5 decision table\n";
    for (const auto& row : kRows) {
        std::cout << "  channel=" << row.channel
                  << " qos=\"" << row.qos_choice << "\""
                  << " reason=\"" << row.engineering_reason << "\""
                  << " phrase=\"" << row.interview_phrase << "\"\n";
    }
}

void printTradeoffNotes() {
    std::cout << "[qos_interview] tradeoff notes\n";
    std::cout << "  reliability=\"best effort is acceptable for replaceable sensor samples; Service and Action are used for commands that need explicit outcomes\"\n";
    std::cout << "  durability=\"volatile is enough because this runtime cares about live state, not replaying old samples to late joiners\"\n";
    std::cout << "  depth=\"small keep_last queues protect freshness; large queues can increase stale-data latency under backlog\"\n";
    std::cout << "  observability=\"query_status, [qos] logs, topic info, and review programs give repeatable evidence\"\n";
}

void printInterviewSummary() {
    std::cout << "[qos_interview] interview summary\n";
    std::cout << "  My QoS rule is semantic first: high-rate robot sensors prioritize fresh samples, "
              << "heartbeats balance reliability with bounded freshness, and control paths use Service or Action "
              << "so success, failure, transition, result, and cancellation are observable.\n";
}

}  // namespace

int main() {
    printDecisionTable();
    printTradeoffNotes();
    printInterviewSummary();
    std::cout << "[ok] QoS interview review ready\n";
    return 0;
}
