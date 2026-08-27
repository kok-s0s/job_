#pragma once

#include <array>
#include <cmath>
#include <stdexcept>
#include <string>

namespace robot_runtime_demo {

struct InferenceResult {
    float score = 0.0F;
    std::string status;
};

inline InferenceResult runTinyRobotScore(const std::array<float, 3>& features) {
    for (const auto value : features) {
        if (!std::isfinite(value)) {
            throw std::invalid_argument("inference feature is not finite");
        }
    }

    const auto score = features[0] * 0.2F + features[1] * -0.1F + features[2] * 0.7F + 0.05F;
    return {score, score >= 0.5F ? "WARN" : "OK"};
}

}  // namespace robot_runtime_demo
