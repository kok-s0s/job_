#include <array>
#include <iomanip>
#include <iostream>
#include <string>

#include "can_frame.hpp"

namespace {

void printFrame(const std::string& line) {
    const auto frame = socketcan_demo::parseCandumpLine(line);
    std::cout << "[can_frame] raw=" << line
              << " id=0x" << std::hex << std::uppercase << frame.id << std::dec
              << " dlc=" << frame.data.size()
              << " data=\"" << socketcan_demo::formatBytes(frame.data) << "\"\n";
}

void printSocketCanCommands() {
    constexpr std::array<const char*, 4> commands{{
        "sudo modprobe vcan",
        "sudo ip link add dev vcan0 type vcan",
        "sudo ip link set up vcan0",
        "candump vcan0",
    }};

    std::cout << "[socketcan] basic command checklist\n";
    for (const auto* command : commands) {
        std::cout << "  command=\"" << command << "\"\n";
    }
}

}  // namespace

int main() {
    try {
        printSocketCanCommands();
        printFrame("123#1122334455667788");
        printFrame("321#AABBCCDD");
        std::cout << "[can_note] classic_can=\"11-bit id + dlc 0..8 + payload bytes\"\n";
        std::cout << "[ok] CAN frame basics verified\n";
    } catch (const std::exception& ex) {
        std::cerr << "[error] " << ex.what() << "\n";
        return 1;
    }
    return 0;
}
