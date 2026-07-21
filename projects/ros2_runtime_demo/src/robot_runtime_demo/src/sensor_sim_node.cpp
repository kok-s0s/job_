#include <chrono>
#include <cmath>
#include <cstddef>
#include <memory>

#include "rclcpp/rclcpp.hpp"
#include "sensor_msgs/msg/imu.hpp"
#include "sensor_msgs/msg/joint_state.hpp"

using namespace std::chrono_literals;

class SensorSimNode : public rclcpp::Node {
public:
    SensorSimNode()
        : Node("sensor_sim_node") {
        imu_pub_ = create_publisher<sensor_msgs::msg::Imu>("/robot/imu", 10);
        joint_pub_ = create_publisher<sensor_msgs::msg::JointState>("/robot/joint_states", 10);
        timer_ = create_wall_timer(100ms, [this] {
            publishSensors();
        });
    }

private:
    void publishSensors() {
        ++sequence_;
        const auto stamp = now();
        const double t = static_cast<double>(sequence_) * 0.1;

        sensor_msgs::msg::Imu imu;
        imu.header.stamp = stamp;
        imu.header.frame_id = "imu_link";
        imu.orientation.w = 1.0;
        imu.angular_velocity.z = 0.05 * std::sin(t);
        imu.linear_acceleration.x = 0.1 * std::sin(t);
        imu.linear_acceleration.y = 0.1 * std::cos(t);
        imu.linear_acceleration.z = 9.8;

        sensor_msgs::msg::JointState joints;
        joints.header.stamp = stamp;
        joints.name = {"joint_1", "joint_2", "joint_3"};
        joints.position = {
            0.5 * std::sin(t),
            0.25 * std::cos(t),
            0.1 * std::sin(0.5 * t),
        };
        joints.velocity = {
            0.5 * std::cos(t),
            -0.25 * std::sin(t),
            0.05 * std::cos(0.5 * t),
        };

        imu_pub_->publish(imu);
        joint_pub_->publish(joints);

        RCLCPP_INFO(
            get_logger(),
            "published sensors seq=%zu imu_ax=%.3f imu_az=%.1f joint_1=%.3f",
            sequence_,
            imu.linear_acceleration.x,
            imu.linear_acceleration.z,
            joints.position.front());
    }

    rclcpp::Publisher<sensor_msgs::msg::Imu>::SharedPtr imu_pub_;
    rclcpp::Publisher<sensor_msgs::msg::JointState>::SharedPtr joint_pub_;
    rclcpp::TimerBase::SharedPtr timer_;
    std::size_t sequence_ = 0;
};

int main(int argc, char** argv) {
    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<SensorSimNode>());
    rclcpp::shutdown();
    return 0;
}
