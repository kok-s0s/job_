#include <chrono>
#include <cstddef>
#include <iomanip>
#include <memory>
#include <sstream>
#include <string>

#include "builtin_interfaces/msg/time.hpp"
#include "rclcpp/rclcpp.hpp"
#include "sensor_msgs/msg/imu.hpp"
#include "sensor_msgs/msg/joint_state.hpp"
#include "std_srvs/srv/trigger.hpp"

using namespace std::chrono_literals;

class RuntimeNode : public rclcpp::Node {
public:
    RuntimeNode()
        : Node("runtime_node") {
        imu_sub_ = create_subscription<sensor_msgs::msg::Imu>(
            "/robot/imu",
            10,
            [this](const sensor_msgs::msg::Imu::SharedPtr msg) {
                ++imu_count_;
                last_imu_time_ = now();
                latest_imu_latency_ms_ = latencyMs(msg->header.stamp, last_imu_time_);
                latest_accel_z_ = msg->linear_acceleration.z;
                RCLCPP_INFO(
                    get_logger(),
                    "received imu count=%zu frame=%s accel_z=%.2f latency_ms=%.2f",
                    imu_count_,
                    msg->header.frame_id.c_str(),
                    latest_accel_z_,
                    latest_imu_latency_ms_);
            });

        joint_sub_ = create_subscription<sensor_msgs::msg::JointState>(
            "/robot/joint_states",
            10,
            [this](const sensor_msgs::msg::JointState::SharedPtr msg) {
                ++joint_count_;
                last_joint_time_ = now();
                latest_joint_latency_ms_ = latencyMs(msg->header.stamp, last_joint_time_);
                latest_joint_count_ = msg->name.size();
                latest_joint_valid_ =
                    !msg->name.empty() &&
                    msg->position.size() == msg->name.size() &&
                    msg->velocity.size() == msg->name.size();
                if (!latest_joint_valid_) {
                    RCLCPP_WARN(
                        get_logger(),
                        "invalid joint_states shape names=%zu positions=%zu velocities=%zu",
                        msg->name.size(),
                        msg->position.size(),
                        msg->velocity.size());
                }
                RCLCPP_INFO(
                    get_logger(),
                    "received joint_states count=%zu joints=%zu latency_ms=%.2f valid=%d",
                    joint_count_,
                    latest_joint_count_,
                    latest_joint_latency_ms_,
                    latest_joint_valid_);
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

        query_status_service_ = create_service<std_srvs::srv::Trigger>(
            "runtime/query_status",
            [this](
                const std::shared_ptr<std_srvs::srv::Trigger::Request>,
                std::shared_ptr<std_srvs::srv::Trigger::Response> response) {
                updateRuntimeState();
                response->success = runtime_state_ == "RUNNING";
                response->message = buildStatusSummary();
                RCLCPP_INFO(get_logger(), "query_status service called: %s", response->message.c_str());
            });

        heartbeat_timer_ = create_wall_timer(1s, [this] {
            updateRuntimeState();
            RCLCPP_INFO(
                get_logger(),
                "runtime status %s",
                buildStatusSummary().c_str());
        });
    }

private:
    double latencyMs(
        const builtin_interfaces::msg::Time& stamp,
        const rclcpp::Time& received_at) const {
        const rclcpp::Time sent_at(stamp);
        return (received_at - sent_at).seconds() * 1000.0;
    }

    void updateRuntimeState() {
        if (imu_count_ == 0 || joint_count_ == 0) {
            runtime_state_ = "BOOTING";
            return;
        }

        const auto current_time = now();
        const bool imu_timeout = (current_time - last_imu_time_).seconds() > 1.0;
        const bool joint_timeout = (current_time - last_joint_time_).seconds() > 1.0;

        runtime_state_ = (!imu_timeout && !joint_timeout && latest_joint_valid_)
            ? "RUNNING"
            : "FAULT";
    }

    std::string buildStatusSummary() const {
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(2)
            << "state=" << runtime_state_
            << " imu_count=" << imu_count_
            << " joint_count=" << joint_count_
            << " latest_accel_z=" << latest_accel_z_
            << " latest_joint_count=" << latest_joint_count_
            << " imu_latency_ms=" << latest_imu_latency_ms_
            << " joint_latency_ms=" << latest_joint_latency_ms_
            << " joint_valid=" << static_cast<int>(latest_joint_valid_);
        return oss.str();
    }

    rclcpp::Subscription<sensor_msgs::msg::Imu>::SharedPtr imu_sub_;
    rclcpp::Subscription<sensor_msgs::msg::JointState>::SharedPtr joint_sub_;
    rclcpp::Service<std_srvs::srv::Trigger>::SharedPtr reset_service_;
    rclcpp::Service<std_srvs::srv::Trigger>::SharedPtr query_status_service_;
    rclcpp::TimerBase::SharedPtr heartbeat_timer_;
    std::string runtime_state_ = "BOOTING";
    std::size_t imu_count_ = 0;
    std::size_t joint_count_ = 0;
    std::size_t latest_joint_count_ = 0;
    double latest_accel_z_ = 0.0;
    rclcpp::Time last_imu_time_;
    rclcpp::Time last_joint_time_;
    double latest_imu_latency_ms_ = 0.0;
    double latest_joint_latency_ms_ = 0.0;
    bool latest_joint_valid_ = false;
};

int main(int argc, char** argv) {
    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<RuntimeNode>());
    rclcpp::shutdown();
    return 0;
}
