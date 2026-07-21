#include <chrono>
#include <cstddef>
#include <memory>
#include <string>

#include "rclcpp/rclcpp.hpp"
#include "std_msgs/msg/string.hpp"
#include "std_srvs/srv/trigger.hpp"

using namespace std::chrono_literals;

class RuntimeNode : public rclcpp::Node {
public:
    RuntimeNode()
        : Node("runtime_node") {
        sensor_sub_ = create_subscription<std_msgs::msg::String>(
            "robot/sensor_state",
            10,
            [this](const std_msgs::msg::String::SharedPtr msg) {
                latest_sensor_state_ = msg->data;
                ++received_count_;

                if (msg->data.find("fault=true") != std::string::npos) {
                    runtime_state_ = "FAULT";
                } else {
                    runtime_state_ = "RUNNING";
                }

                RCLCPP_INFO(
                    get_logger(),
                    "received sensor_state: '%s', runtime_state=%s",
                    latest_sensor_state_.c_str(),
                    runtime_state_.c_str());
            });

        reset_service_ = create_service<std_srvs::srv::Trigger>(
            "runtime/reset_fault",
            [this](
                const std::shared_ptr<std_srvs::srv::Trigger::Request>,
                std::shared_ptr<std_srvs::srv::Trigger::Response> response) {
                runtime_state_ = "RUNNING";
                response->success = true;
                response->message = "runtime fault state cleared";
                RCLCPP_WARN(get_logger(), "reset_fault service called");
            });

        heartbeat_timer_ = create_wall_timer(1s, [this] {
            RCLCPP_INFO(
                get_logger(),
                "runtime status=%s received=%zu latest='%s'",
                runtime_state_.c_str(),
                received_count_,
                latest_sensor_state_.c_str());
        });
    }

private:
    rclcpp::Subscription<std_msgs::msg::String>::SharedPtr sensor_sub_;
    rclcpp::Service<std_srvs::srv::Trigger>::SharedPtr reset_service_;
    rclcpp::TimerBase::SharedPtr heartbeat_timer_;
    std::string runtime_state_ = "BOOTING";
    std::string latest_sensor_state_ = "<none>";
    std::size_t received_count_ = 0;
};

int main(int argc, char** argv) {
    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<RuntimeNode>());
    rclcpp::shutdown();
    return 0;
}
