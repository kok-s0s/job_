#include "runtime_inference.hpp"
#include "runtime_qos.hpp"
#include "runtime_session.hpp"

#include <array>
#include <chrono>
#include <cmath>
#include <iomanip>
#include <memory>
#include <sstream>
#include <string>

#include "rclcpp/rclcpp.hpp"
#include "sensor_msgs/msg/imu.hpp"
#include "sensor_msgs/msg/joint_state.hpp"
#include "std_msgs/msg/string.hpp"

class InferenceNode final : public rclcpp::Node {
public:
    InferenceNode()
        : Node("inference_node"),
          session_id_(robot_runtime_demo::runtimeSessionId()) {
        imu_subscription_ = create_subscription<sensor_msgs::msg::Imu>(
            "/robot/imu",
            robot_runtime_demo::sensorStreamQos(),
            [this](const sensor_msgs::msg::Imu::SharedPtr message) {
                latest_imu_ax_ = static_cast<float>(message->linear_acceleration.x);
                has_imu_ = true;
                publishInferenceIfReady();
            });

        joint_subscription_ = create_subscription<sensor_msgs::msg::JointState>(
            "/robot/joint_states",
            robot_runtime_demo::sensorStreamQos(),
            [this](const sensor_msgs::msg::JointState::SharedPtr message) {
                latest_joint_0_ = message->position.empty() ? 0.0F : static_cast<float>(message->position.front());
                latest_joint_count_ = message->position.size();
                has_joint_ = true;
                publishInferenceIfReady();
            });

        inference_publisher_ = create_publisher<std_msgs::msg::String>("/runtime/inference_score", rclcpp::QoS(10));

        RCLCPP_INFO(get_logger(), "[session] node=inference_node session_id=%s", session_id_.c_str());
        RCLCPP_INFO(
            get_logger(),
            "[qos] topic=/runtime/inference_score role=inference_result reliability=reliable durability=volatile history=keep_last depth=10");
    }

private:
    void publishInferenceIfReady() {
        if (!has_imu_ || !has_joint_) {
            return;
        }

        std::array<float, 3> features{
            std::fabs(latest_imu_ax_),
            std::fabs(latest_joint_0_),
            latest_joint_count_ >= 3 ? 0.0F : 1.0F,
        };

        robot_runtime_demo::InferenceResult result;
        double duration_ms = 0.0;
        try {
            const auto start = std::chrono::steady_clock::now();
            result = robot_runtime_demo::runTinyRobotScore(features);
            const auto end = std::chrono::steady_clock::now();
            duration_ms = std::chrono::duration<double, std::milli>(end - start).count();
            observeSuccess(duration_ms);
        } catch (const std::exception& ex) {
            ++failure_count_;
            RCLCPP_ERROR(get_logger(), "[inference_error] node=inference_node session_id=%s message=\"%s\"", session_id_.c_str(), ex.what());
            return;
        }

        std_msgs::msg::String message;
        message.data = buildPayload(features, result, duration_ms);
        inference_publisher_->publish(message);

        RCLCPP_INFO(
            get_logger(),
            "[inference] node=inference_node model=tiny_robot_score session_id=%s score=%.3f status=%s features=[%.3f,%.3f,%.3f] duration_ms=%.3f avg_ms=%.3f max_ms=%.3f failures=%zu",
            session_id_.c_str(),
            result.score,
            result.status.c_str(),
            features[0],
            features[1],
            features[2],
            duration_ms,
            averageMs(),
            max_ms_,
            failure_count_);
    }

    std::string buildPayload(
        const std::array<float, 3>& features,
        const robot_runtime_demo::InferenceResult& result,
        double duration_ms) const {
        std::ostringstream out;
        out << std::fixed << std::setprecision(3);
        out << "model=tiny_robot_score"
            << " session_id=" << session_id_
            << " score=" << result.score
            << " status=" << result.status
            << " duration_ms=" << duration_ms
            << " avg_ms=" << averageMs()
            << " max_ms=" << max_ms_
            << " failures=" << failure_count_
            << " features=[" << features[0] << "," << features[1] << "," << features[2] << "]";
        return out.str();
    }

    void observeSuccess(double duration_ms) {
        ++success_count_;
        total_ms_ += duration_ms;
        if (duration_ms > max_ms_) {
            max_ms_ = duration_ms;
        }
    }

    double averageMs() const {
        return success_count_ == 0 ? 0.0 : total_ms_ / static_cast<double>(success_count_);
    }

    std::string session_id_;
    bool has_imu_ = false;
    bool has_joint_ = false;
    float latest_imu_ax_ = 0.0F;
    float latest_joint_0_ = 0.0F;
    std::size_t latest_joint_count_ = 0;
    std::size_t success_count_ = 0;
    std::size_t failure_count_ = 0;
    double total_ms_ = 0.0;
    double max_ms_ = 0.0;
    rclcpp::Subscription<sensor_msgs::msg::Imu>::SharedPtr imu_subscription_;
    rclcpp::Subscription<sensor_msgs::msg::JointState>::SharedPtr joint_subscription_;
    rclcpp::Publisher<std_msgs::msg::String>::SharedPtr inference_publisher_;
};

int main(int argc, char** argv) {
    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<InferenceNode>());
    rclcpp::shutdown();
    return 0;
}
