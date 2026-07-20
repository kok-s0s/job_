#include <chrono>
#include <memory>

#include "rclcpp/rclcpp.hpp"

using namespace std::chrono_literals;

class RuntimeNode : public rclcpp::Node {
public:
    RuntimeNode()
        : Node("runtime_node") {
        timer_ = create_wall_timer(1s, [this] {
            RCLCPP_INFO(get_logger(), "runtime_node heartbeat");
        });
    }

private:
    rclcpp::TimerBase::SharedPtr timer_;
};

int main(int argc, char** argv) {
    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<RuntimeNode>());
    rclcpp::shutdown();
    return 0;
}
