#include "tiny_robot_inference.hpp"

#include <array>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <stdexcept>

namespace {

void requireClose(float actual, float expected) {
    if (std::fabs(actual - expected) > 0.0001F) {
        throw std::runtime_error("unexpected inference score");
    }
}

}  // namespace

int main() {
    try {
        constexpr std::array<float, 3> kFeatures{0.5F, -1.0F, 0.2F};
        const auto result = tiny_robot_inference::run(kFeatures);
        requireClose(result.score, 0.39F);

        std::cout << std::fixed << std::setprecision(3);
        std::cout << "[inference] model=tiny_robot_score input=[0.500,-1.000,0.200]"
                  << " score=" << result.score << " status=" << result.status << "\n";
        std::cout << "[ok] fixed input inference stable\n";
    } catch (const std::exception& ex) {
        std::cerr << "[error] " << ex.what() << "\n";
        return 1;
    }
    return 0;
}
