#include <iostream>
#include <stdexcept>
#include <string>

#include "can_frame.hpp"

namespace {

enum class RuntimeState { Standby, Fault };

const char* toString(const RuntimeState state) {
    switch (state) {
        case RuntimeState::Standby:
            return "STANDBY";
        case RuntimeState::Fault:
            return "FAULT";
    }
    return "UNKNOWN";
}

struct CanRuntimeMonitor {
    RuntimeState state = RuntimeState::Standby;
    int last_rx_ms = -1;
    std::string error = "NONE";
};

bool statusHasFault(const socketcan_demo::CanFrame& frame) {
    if (frame.data.size() < 5) {
        throw std::runtime_error("status frame is too short");
    }
    return (frame.data[4] & 0x01U) != 0;
}

void observeStatus(CanRuntimeMonitor& monitor, const int now_ms, const std::string& raw) {
    const auto frame = socketcan_demo::parseCandumpLine(raw);
    monitor.last_rx_ms = now_ms;
    if (statusHasFault(frame)) {
        monitor.state = RuntimeState::Fault;
        monitor.error = "CAN_ACTUATOR_FAULT";
    }
    std::cout << "[can_bridge] now_ms=" << now_ms
              << " raw=" << raw
              << " state=" << toString(monitor.state)
              << " error=" << monitor.error << "\n";
}

void checkTimeout(CanRuntimeMonitor& monitor, const int now_ms, const int timeout_ms) {
    const auto age_ms = monitor.last_rx_ms < 0 ? now_ms : now_ms - monitor.last_rx_ms;
    if (age_ms > timeout_ms) {
        monitor.state = RuntimeState::Fault;
        monitor.error = "CAN_TIMEOUT";
    }
    std::cout << "[can_watchdog] now_ms=" << now_ms
              << " age_ms=" << age_ms
              << " timeout_ms=" << timeout_ms
              << " state=" << toString(monitor.state)
              << " error=" << monitor.error << "\n";
}

}  // namespace

int main() {
    try {
        constexpr int timeout_ms = 100;

        CanRuntimeMonitor timeout_case;
        observeStatus(timeout_case, 0, "181#E8032C0100000000");
        checkTimeout(timeout_case, 150, timeout_ms);

        CanRuntimeMonitor fault_case;
        observeStatus(fault_case, 0, "182#18FC38FF01070000");

        if (timeout_case.error != "CAN_TIMEOUT" || fault_case.error != "CAN_ACTUATOR_FAULT") {
            std::cerr << "[error] expected CAN timeout and fault paths\n";
            return 1;
        }

        std::cout << "[ok] CAN timeout and actuator fault map to runtime Fault\n";
    } catch (const std::exception& ex) {
        std::cerr << "[error] " << ex.what() << "\n";
        return 1;
    }
    return 0;
}
