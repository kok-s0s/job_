#pragma once

#include <array>
#include <cmath>
#include <stdexcept>
#include <string>

namespace tiny_robot_inference {

struct InferenceResult {
    float score = 0.0F;
    std::string status;
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
