#include <array>
#include <iostream>

namespace {

struct CommandReliabilityRow {
    const char* interface_name;
    const char* kind;
    const char* reliable_semantics;
    const char* verification;
};

constexpr std::array<CommandReliabilityRow, 4> kRows{{
    {
        "/runtime/query_status",
        "Service",
        "caller needs a request/response snapshot of state, error, severity, and recovery hint",
        "success=True and message contains state=..., runtime_error=..., runtime_recoverable=...",
    },
    {
        "/runtime/reset_fault",
        "Service",
        "fault reset must not be a silent fire-and-forget command",
        "success=True and message=\"runtime fault state cleared\"",
    },
    {
        "/runtime/apply_event",
        "Service",
        "state-machine event injection must report whether the event was understood and whether state changed",
        "accepted, transitioned, previous_state, current_state, and runtime_error fields",
    },
    {
        "/runtime/execute_task",
        "Action",
        "long-running task commands need goal acceptance, progress, final result, rejection, and cancellation",
        "Goal accepted, feedback, result, invalid-goal rejection, and cancel_result",
    },
}};

void printCommandTable() {
    std::cout << "[command_reliability] reliable control command table\n";
    for (const auto& row : kRows) {
        std::cout << "  interface=" << row.interface_name
                  << " kind=" << row.kind
                  << " semantics=\"" << row.reliable_semantics << "\""
                  << " verification=\"" << row.verification << "\"\n";
    }
}

void printStateEventNotes() {
    std::cout << "[command_reliability] apply_event semantics\n";
    std::cout << "  accepted=true means the event name is known and the request was understood\n";
    std::cout << "  transitioned=true means the current state allowed an actual RuntimeStateMachine transition\n";
    std::cout << "  unknown_event=\"NoSuchEvent returns accepted=false with an explicit error message\"\n";
}

void printInterviewSummary() {
    std::cout << "[command_reliability] interview summary\n";
    std::cout << "  Sensor streams prefer freshness, but control commands require deterministic acknowledgement. "
              << "Services and Actions make success, failure, rejection, state transition, and cancellation observable.\n";
}

}  // namespace

int main() {
    printCommandTable();
    printStateEventNotes();
    printInterviewSummary();
    std::cout << "[ok] command reliability review ready\n";
    return 0;
}
