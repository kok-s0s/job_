#include <chrono>
#include <memory>
#include <string>

#include "rclcpp/rclcpp.hpp"
#include "std_msgs/msg/string.hpp"

using namespace std::chrono_literals;

class SensorSimNode : public rclcpp::Node {
public:
    SensorSimNode()
        : Node("sensor_sim_node") {
        publisher_ = create_publisher<std_msgs::msg::String>("robot/sensor_state", 10);
        timer_ = create_wall_timer(500ms, [this] {
            publish_sensor_state();
        });
    }

private:
    void publish_sensor_state() {
        ++sequence_;
        const double temperature = 36.0 + static_cast<double>(sequence_ % 8) * 0.4;
        const bool fault = sequence_ % 12 == 0;

        std_msgs::msg::String msg;
        msg.data =
            "seq=" + std::to_string(sequence_) +
            " temperature=" + std::to_string(temperature) +
            " joint_position=" + std::to_string(0.1 * static_cast<double>(sequence_ % 20)) +
            " fault=" + (fault ? "true" : "false");

        publisher_->publish(msg);
        RCLCPP_INFO(get_logger(), "published sensor_state: '%s'", msg.data.c_str());
    }

    rclcpp::Publisher<std_msgs::msg::String>::SharedPtr publisher_;
    rclcpp::TimerBase::SharedPtr timer_;
    std::size_t sequence_ = 0;
};

int main(int argc, char** argv) {
    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<SensorSimNode>());
    rclcpp::shutdown();
    return 0;
}
