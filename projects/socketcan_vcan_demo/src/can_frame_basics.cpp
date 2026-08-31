#include <array>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

struct CanFrame {
    std::uint32_t id = 0;
    std::vector<std::uint8_t> data;
};

std::uint32_t parseHexId(const std::string& text) {
    std::uint32_t value = 0;
    std::istringstream in(text);
    in >> std::hex >> value;
    if (!in || value > 0x7FFU) {
        throw std::runtime_error("expected standard 11-bit CAN id");
    }
    return value;
}

std::uint8_t parseHexByte(const std::string& text) {
    if (text.size() != 2) {
        throw std::runtime_error("CAN data byte must contain two hex characters");
    }
    unsigned int value = 0;
    std::istringstream in(text);
    in >> std::hex >> value;
    if (!in || value > 0xFFU) {
        throw std::runtime_error("invalid CAN data byte");
    }
    return static_cast<std::uint8_t>(value);
}

CanFrame parseCandumpLine(const std::string& line) {
    const auto hash = line.find('#');
    if (hash == std::string::npos) {
        throw std::runtime_error("expected candump payload like 123#11223344");
    }

    CanFrame frame;
    frame.id = parseHexId(line.substr(0, hash));

    const auto payload = line.substr(hash + 1);
    if (payload.size() % 2 != 0 || payload.size() > 16) {
        throw std::runtime_error("classic CAN payload must contain 0 to 8 bytes");
    }

    for (std::size_t i = 0; i < payload.size(); i += 2) {
        frame.data.push_back(parseHexByte(payload.substr(i, 2)));
    }
    return frame;
}

std::string formatBytes(const std::vector<std::uint8_t>& bytes) {
    std::ostringstream out;
    out << std::hex << std::uppercase << std::setfill('0');
    for (std::size_t i = 0; i < bytes.size(); ++i) {
        if (i != 0) {
            out << " ";
        }
        out << std::setw(2) << static_cast<int>(bytes[i]);
    }
    return out.str();
}

void printFrame(const std::string& line) {
    const auto frame = parseCandumpLine(line);
    std::cout << "[can_frame] raw=" << line
              << " id=0x" << std::hex << std::uppercase << frame.id << std::dec
              << " dlc=" << frame.data.size()
              << " data=\"" << formatBytes(frame.data) << "\"\n";
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
