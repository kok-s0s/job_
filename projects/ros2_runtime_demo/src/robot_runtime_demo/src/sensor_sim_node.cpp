#include <chrono>
#include <cmath>
#include <cstddef>
#include <memory>
#include <sstream>

#include "rclcpp/rclcpp.hpp"
#include "runtime_qos.hpp"
#include "runtime_session.hpp"
#include "sensor_msgs/msg/imu.hpp"
#include "sensor_msgs/msg/joint_state.hpp"
#include "std_msgs/msg/string.hpp"

using namespace std::chrono_literals;

class SensorSimNode : public rclcpp::Node {
public:
    SensorSimNode()
        : Node("sensor_sim_node") {
        imu_pub_ = create_publisher<sensor_msgs::msg::Imu>(
            "/robot/imu",
            robot_runtime_demo::sensorStreamQos());
        joint_pub_ = create_publisher<sensor_msgs::msg::JointState>(
            "/robot/joint_states",
            robot_runtime_demo::sensorStreamQos());
        heartbeat_pub_ = create_publisher<std_msgs::msg::String>(
            "/runtime/heartbeat",
            robot_runtime_demo::heartbeatQos());
        RCLCPP_INFO(
            get_logger(),
            "[session] node=sensor_sim_node session_id=%s",
            session_id_.c_str());
        RCLCPP_INFO(
            get_logger(),
            "[qos] topic=/robot/imu role=sensor_stream %s",
            robot_runtime_demo::sensorStreamQosSummary());
        RCLCPP_INFO(
            get_logger(),
            "[qos] topic=/robot/joint_states role=sensor_stream %s",
            robot_runtime_demo::sensorStreamQosSummary());
        RCLCPP_INFO(
            get_logger(),
            "[qos] topic=/runtime/heartbeat role=heartbeat %s",
            robot_runtime_demo::heartbeatQosSummary());
        timer_ = create_wall_timer(100ms, [this] {
            publishSensors();
        });
        heartbeat_timer_ = create_wall_timer(1s, [this] {
            publishHeartbeat();
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

    void publishHeartbeat() {
        std_msgs::msg::String heartbeat;
        std::ostringstream payload;
        payload << "node=sensor_sim_node"
                << " seq=" << ++heartbeat_sequence_
                << " stamp_ms=" << now().nanoseconds() / 1000000
                << " session_id=" << session_id_
                << " status=OK"
                << " message=\"sensor publisher alive\"";
        heartbeat.data = payload.str();
        heartbeat_pub_->publish(heartbeat);
        RCLCPP_INFO(get_logger(), "[heartbeat] %s", heartbeat.data.c_str());
    }

    rclcpp::Publisher<sensor_msgs::msg::Imu>::SharedPtr imu_pub_;
    rclcpp::Publisher<sensor_msgs::msg::JointState>::SharedPtr joint_pub_;
    rclcpp::Publisher<std_msgs::msg::String>::SharedPtr heartbeat_pub_;
    rclcpp::TimerBase::SharedPtr timer_;
    rclcpp::TimerBase::SharedPtr heartbeat_timer_;
    std::size_t sequence_ = 0;
    std::size_t heartbeat_sequence_ = 0;
    const std::string session_id_ = robot_runtime_demo::runtimeSessionId();
};

int main(int argc, char** argv) {
    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<SensorSimNode>());
    rclcpp::shutdown();
    return 0;
}
