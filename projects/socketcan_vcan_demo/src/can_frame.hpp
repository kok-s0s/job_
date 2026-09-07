#pragma once

#include <cstdint>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace socketcan_demo {

struct CanFrame {
    std::uint32_t id = 0;
    std::vector<std::uint8_t> data;
};

inline std::uint32_t parseHexId(const std::string& text) {
    std::uint32_t value = 0;
    std::istringstream in(text);
    in >> std::hex >> value;
    if (!in || value > 0x7FFU) {
        throw std::runtime_error("expected standard 11-bit CAN id");
    }
    return value;
}

inline std::uint8_t parseHexByte(const std::string& text) {
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

inline CanFrame parseCandumpLine(const std::string& line) {
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

inline std::string formatBytes(const std::vector<std::uint8_t>& bytes) {
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

inline std::string formatCandumpLine(const CanFrame& frame) {
    std::ostringstream out;
    out << std::hex << std::uppercase << std::setfill('0') << std::setw(3) << frame.id << "#";
    for (const auto byte : frame.data) {
        out << std::setw(2) << static_cast<int>(byte);
    }
    return out.str();
}

inline std::int16_t readInt16Le(const std::vector<std::uint8_t>& data, const std::size_t offset) {
    if (offset + 1 >= data.size()) {
        throw std::runtime_error("not enough CAN payload bytes for int16");
    }
    const auto raw = static_cast<std::uint16_t>(data[offset]) |
                     static_cast<std::uint16_t>(data[offset + 1] << 8U);
    return static_cast<std::int16_t>(raw);
}

inline void writeInt16Le(std::vector<std::uint8_t>& data, const std::size_t offset, const std::int16_t value) {
    if (offset + 1 >= data.size()) {
        throw std::runtime_error("not enough CAN payload bytes for int16");
    }
    const auto raw = static_cast<std::uint16_t>(value);
    data[offset] = static_cast<std::uint8_t>(raw & 0xFFU);
    data[offset + 1] = static_cast<std::uint8_t>((raw >> 8U) & 0xFFU);
}

}  // namespace socketcan_demo
