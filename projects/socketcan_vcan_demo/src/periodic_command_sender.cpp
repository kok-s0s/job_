#include <cmath>
#include <iostream>
#include <vector>

#include "can_frame.hpp"

namespace {

socketcan_demo::CanFrame makeCommandFrame(const int node_id, const double position_rad, const double velocity_rad_s) {
    socketcan_demo::CanFrame frame;
    frame.id = static_cast<std::uint32_t>(0x200 + node_id);
    frame.data.assign(8, 0);

    socketcan_demo::writeInt16Le(frame.data, 0, static_cast<std::int16_t>(std::lround(position_rad * 1000.0)));
    socketcan_demo::writeInt16Le(frame.data, 2, static_cast<std::int16_t>(std::lround(velocity_rad_s * 1000.0)));
    frame.data[4] = 0x01U;
    frame.data[5] = 0x00U;
    return frame;
}

}  // namespace

int main() {
    constexpr int hz = 50;
    constexpr int period_ms = 1000 / hz;
    constexpr int sample_count = 10;

    std::cout << "[command_sender] target_hz=" << hz
              << " period_ms=" << period_ms
              << " sample_count=" << sample_count << "\n";

    std::vector<socketcan_demo::CanFrame> frames;
    for (int i = 0; i < sample_count; ++i) {
        const auto t = static_cast<double>(i) / hz;
        frames.push_back(makeCommandFrame(1, 0.4 * std::sin(t), 0.4 * std::cos(t)));
    }

    for (std::size_t i = 0; i < frames.size(); ++i) {
        std::cout << "[command_tx] tick=" << i
                  << " t_ms=" << (i * period_ms)
                  << " cansend=\"cansend vcan0 " << socketcan_demo::formatCandumpLine(frames[i]) << "\"\n";
    }

    std::cout << "[ok] periodic command sender generated 50Hz control frames\n";
    return 0;
}
