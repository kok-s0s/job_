#pragma once

#include <array>
#include <cmath>
#include <cstddef>
#include <stdexcept>
#include <string>

namespace tiny_robot_inference {

struct InferenceResult {
    float score = 0.0F;
    std::string status;
};

class InferenceStats {
public:
    void observeSuccess(double duration_ms) {
        ++success_count_;
        total_ms_ += duration_ms;
        if (duration_ms > max_ms_) {
            max_ms_ = duration_ms;
        }
    }

    void observeFailure() {
        ++failure_count_;
    }

    std::size_t successCount() const {
        return success_count_;
    }

    std::size_t failureCount() const {
        return failure_count_;
    }

    double averageMs() const {
        return success_count_ == 0 ? 0.0 : total_ms_ / static_cast<double>(success_count_);
    }

    double maxMs() const {
        return max_ms_;
    }

private:
    std::size_t success_count_ = 0;
    std::size_t failure_count_ = 0;
    double total_ms_ = 0.0;
    double max_ms_ = 0.0;
};

inline float computeScore(const std::array<float, 3>& robot_features) {
    return robot_features[0] * 0.2F + robot_features[1] * -0.1F + robot_features[2] * 0.7F + 0.05F;
}

inline InferenceResult run(const std::array<float, 3>& robot_features) {
    for (const auto value : robot_features) {
        if (!std::isfinite(value)) {
            throw std::invalid_argument("robot_features contains a non-finite value");
        }
    }

    const auto score = computeScore(robot_features);
    return {score, score >= 0.5F ? "WARN" : "OK"};
}

}  // namespace tiny_robot_inference
