#include <chrono>
#include <cstddef>
#include <iostream>
#include <memory>
#include <string>
#include <thread>

#include "rclcpp/rclcpp.hpp"
#include "rclcpp_action/rclcpp_action.hpp"
#include "robot_runtime_demo/action/execute_task.hpp"

using namespace std::chrono_literals;

int main(int argc, char** argv) {
    rclcpp::init(argc, argv);
    auto node = rclcpp::Node::make_shared("action_cancel_test_client");
    using ExecuteTask = robot_runtime_demo::action::ExecuteTask;
    using GoalHandle = rclcpp_action::ClientGoalHandle<ExecuteTask>;

    auto client =
        rclcpp_action::create_client<ExecuteTask>(node, "/runtime/execute_task");
    if (!client->wait_for_action_server(5s)) {
        std::cerr << "execute_task action server not available" << std::endl;
        rclcpp::shutdown();
        return 1;
    }

    std::size_t feedback_count = 0;
    rclcpp_action::Client<ExecuteTask>::SendGoalOptions options;
    options.feedback_callback =
        [&feedback_count](
            GoalHandle::SharedPtr,
            const std::shared_ptr<const ExecuteTask::Feedback> feedback) {
            ++feedback_count;
            std::cout
                << "feedback current_step=" << feedback->current_step
                << " progress=" << feedback->progress << std::endl;
        };

    ExecuteTask::Goal goal;
    goal.target_steps = 30;
    auto goal_future = client->async_send_goal(goal, options);
    if (rclcpp::spin_until_future_complete(node, goal_future, 5s) !=
        rclcpp::FutureReturnCode::SUCCESS) {
        std::cerr << "timed out while sending goal" << std::endl;
        rclcpp::shutdown();
        return 1;
    }

    auto goal_handle = goal_future.get();
    if (!goal_handle) {
        std::cerr << "cancel-test goal was rejected" << std::endl;
        rclcpp::shutdown();
        return 1;
    }

    const auto feedback_deadline = std::chrono::steady_clock::now() + 700ms;
    while (std::chrono::steady_clock::now() < feedback_deadline) {
        rclcpp::spin_some(node);
        std::this_thread::sleep_for(20ms);
    }

    auto cancel_future = client->async_cancel_goal(goal_handle);
    if (rclcpp::spin_until_future_complete(node, cancel_future, 5s) !=
        rclcpp::FutureReturnCode::SUCCESS ||
        cancel_future.get()->goals_canceling.empty()) {
        std::cerr << "goal cancellation was not accepted" << std::endl;
        rclcpp::shutdown();
        return 1;
    }

    auto result_future = client->async_get_result(goal_handle);
    if (rclcpp::spin_until_future_complete(node, result_future, 5s) !=
        rclcpp::FutureReturnCode::SUCCESS) {
        std::cerr << "timed out while waiting for canceled result" << std::endl;
        rclcpp::shutdown();
        return 1;
    }

    const auto wrapped_result = result_future.get();
    const bool passed =
        wrapped_result.code == rclcpp_action::ResultCode::CANCELED &&
        !wrapped_result.result->success &&
        wrapped_result.result->message.find("task canceled at step") !=
            std::string::npos &&
        feedback_count > 0;

    std::cout
        << "cancel_result code=" << static_cast<int>(wrapped_result.code)
        << " success=" << wrapped_result.result->success
        << " message=\"" << wrapped_result.result->message << "\""
        << " feedback_count=" << feedback_count << std::endl;

    rclcpp::shutdown();
    return passed ? 0 : 1;
}
