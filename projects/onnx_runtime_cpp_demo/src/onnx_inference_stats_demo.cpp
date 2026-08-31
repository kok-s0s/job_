#include "tiny_robot_inference.hpp"

#include <array>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <limits>
#include <stdexcept>

namespace {

double elapsedMs(std::chrono::steady_clock::time_point start, std::chrono::steady_clock::time_point end) {
    return std::chrono::duration<double, std::milli>(end - start).count();
}

}  // namespace

int main() {
    tiny_robot_inference::InferenceStats stats;

    const std::array<std::array<float, 3>, 3> samples{{
        {0.5F, -1.0F, 0.2F},
        {0.1F, 0.2F, 0.0F},
        {2.0F, 0.0F, 0.1F},
    }};

    for (const auto& sample : samples) {
        const auto start = std::chrono::steady_clock::now();
        const auto result = tiny_robot_inference::run(sample);
        const auto end = std::chrono::steady_clock::now();
        stats.observeSuccess(elapsedMs(start, end));

        std::cout << std::fixed << std::setprecision(3)
                  << "[inference_sample] score=" << result.score << " status=" << result.status << "\n";
    }

    try {
        (void)tiny_robot_inference::run({std::numeric_limits<float>::quiet_NaN(), 0.0F, 0.0F});
    } catch (const std::invalid_argument&) {
        stats.observeFailure();
    }

    std::cout << std::fixed << std::setprecision(3)
              << "[inference_stats] count=" << stats.successCount()
              << " avg_ms=" << stats.averageMs()
              << " max_ms=" << stats.maxMs()
              << " failures=" << stats.failureCount() << "\n";
    std::cout << "[ok] inference timing stats ready\n";
    return 0;
}
