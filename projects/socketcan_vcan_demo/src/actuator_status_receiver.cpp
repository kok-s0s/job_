#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>

#include "can_frame.hpp"

namespace {

struct ActuatorStatus {
    int node_id = 0;
    double position_rad = 0.0;
    double velocity_rad_s = 0.0;
    bool fault = false;
    int fault_code = 0;
};

ActuatorStatus decodeStatus(const socketcan_demo::CanFrame& frame) {
    if (frame.id < 0x180U || frame.id > 0x18FU) {
        throw std::runtime_error("expected actuator status frame id 0x180..0x18F");
    }
    if (frame.data.size() != 8) {
        throw std::runtime_error("actuator status payload must be 8 bytes");
    }

    ActuatorStatus status;
    status.node_id = static_cast<int>(frame.id - 0x180U);
    status.position_rad = socketcan_demo::readInt16Le(frame.data, 0) / 1000.0;
    status.velocity_rad_s = socketcan_demo::readInt16Le(frame.data, 2) / 1000.0;
    status.fault = (frame.data[4] & 0x01U) != 0;
    status.fault_code = frame.data[5];
    return status;
}

void printStatus(const std::string& raw) {
    const auto frame = socketcan_demo::parseCandumpLine(raw);
    const auto status = decodeStatus(frame);

    std::cout << std::fixed << std::setprecision(3)
              << "[actuator_status] raw=" << raw
              << " node_id=" << status.node_id
              << " position_rad=" << status.position_rad
              << " velocity_rad_s=" << status.velocity_rad_s
              << " fault=" << (status.fault ? 1 : 0)
              << " fault_code=" << status.fault_code << "\n";
}

}  // namespace

int main() {
    try {
        printStatus("181#E8032C0100000000");
        printStatus("182#18FC38FF01070000");
        std::cout << "[ok] actuator status receiver decoded position velocity and fault\n";
    } catch (const std::exception& ex) {
        std::cerr << "[error] " << ex.what() << "\n";
        return 1;
    }
    return 0;
}
