#pragma once

#include "rclcpp/rclcpp.hpp"

namespace robot_runtime_demo {

inline rclcpp::QoS sensorStreamQos() {
    return rclcpp::QoS(rclcpp::KeepLast(5)).best_effort().durability_volatile();
}

inline rclcpp::QoS heartbeatQos() {
    return rclcpp::QoS(rclcpp::KeepLast(3)).reliable().durability_volatile();
}

inline constexpr const char* sensorStreamQosSummary() {
    return "reliability=best_effort durability=volatile history=keep_last depth=5";
}

inline constexpr const char* heartbeatQosSummary() {
    return "reliability=reliable durability=volatile history=keep_last depth=3";
}

}  // namespace robot_runtime_demo
