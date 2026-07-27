#include <atomic>
#include <chrono>
#include <cstddef>
#include <iomanip>
#include <memory>
#include <sstream>
#include <string>
#include <thread>

#include "builtin_interfaces/msg/time.hpp"
#include "rclcpp/rclcpp.hpp"
#include "rclcpp_action/rclcpp_action.hpp"
#include "robot_runtime_demo/action/execute_task.hpp"
#include "sensor_msgs/msg/imu.hpp"
#include "sensor_msgs/msg/joint_state.hpp"
#include "std_srvs/srv/trigger.hpp"

using namespace std::chrono_literals;

class RuntimeNode : public rclcpp::Node {
public:
    using ExecuteTask = robot_runtime_demo::action::ExecuteTask;
    using GoalHandleExecuteTask = rclcpp_action::ServerGoalHandle<ExecuteTask>;

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
                processRuntimeEvent(RuntimeEvent::RESET_FAULT);
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
                response->success = runtime_state_.load() != RuntimeState::FAULT;
                response->message = buildStatusSummary();
                RCLCPP_INFO(get_logger(), "query_status service called: %s", response->message.c_str());
            });

        execute_task_server_ = rclcpp_action::create_server<ExecuteTask>(
            this,
            "runtime/execute_task",
            [this](
                const rclcpp_action::GoalUUID&,
                std::shared_ptr<const ExecuteTask::Goal> goal) {
                return handleGoal(goal);
            },
            [this](const std::shared_ptr<GoalHandleExecuteTask> goal_handle) {
                return handleCancel(goal_handle);
            },
            [this](const std::shared_ptr<GoalHandleExecuteTask> goal_handle) {
                handleAccepted(goal_handle);
            });

        heartbeat_timer_ = create_wall_timer(1s, [this] {
            updateRuntimeState();
            RCLCPP_INFO(
                get_logger(),
                "runtime status %s",
                buildStatusSummary().c_str());
        });
    }

    ~RuntimeNode() override {
        if (task_thread_.joinable()) {
            task_thread_.join();
        }
    }

private:
    enum class RuntimeState {
        IDLE,
        STANDBY,
        RUNNING,
        FAULT,
        RECOVERY,
    };

    enum class RuntimeEvent {
        SENSOR_HEALTHY,
        SENSOR_TIMEOUT,
        JOINT_INVALID,
        START_TASK,
        TASK_SUCCEEDED,
        TASK_CANCELED,
        TASK_FAILED,
        RESET_FAULT,
        RECOVERY_DONE,
    };

    enum class ErrorCode {
        NONE,
        SENSOR_TIMEOUT,
        JOINT_STATE_INVALID,
        TASK_FAILED,
        RECOVERY_FAILED,
    };

    enum class TaskState {
        IDLE,
        RUNNING,
        COMPLETED,
        CANCELED,
        FAILED,
    };

    double latencyMs(
        const builtin_interfaces::msg::Time& stamp,
        const rclcpp::Time& received_at) const {
        const rclcpp::Time sent_at(stamp);
        return (received_at - sent_at).seconds() * 1000.0;
    }

    void updateRuntimeState() {
        if (imu_count_ == 0 || joint_count_ == 0) {
            runtime_state_.store(RuntimeState::IDLE);
            return;
        }

        const auto current_time = now();
        const bool imu_timeout = (current_time - last_imu_time_).seconds() > 1.0;
        const bool joint_timeout = (current_time - last_joint_time_).seconds() > 1.0;

        if (imu_timeout || joint_timeout) {
            processRuntimeEvent(RuntimeEvent::SENSOR_TIMEOUT);
            return;
        }

        if (!latest_joint_valid_) {
            processRuntimeEvent(RuntimeEvent::JOINT_INVALID);
            return;
        }

        processRuntimeEvent(
            runtime_state_.load() == RuntimeState::RECOVERY
                ? RuntimeEvent::RECOVERY_DONE
                : RuntimeEvent::SENSOR_HEALTHY);
    }

    void processRuntimeEvent(RuntimeEvent event) {
        const auto current = runtime_state_.load();
        RuntimeState next = current;
        ErrorCode next_error = error_code_.load();

        switch (current) {
            case RuntimeState::IDLE:
                if (event == RuntimeEvent::SENSOR_HEALTHY) {
                    next = RuntimeState::STANDBY;
                    next_error = ErrorCode::NONE;
                } else if (event == RuntimeEvent::SENSOR_TIMEOUT) {
                    next = RuntimeState::FAULT;
                    next_error = ErrorCode::SENSOR_TIMEOUT;
                } else if (event == RuntimeEvent::JOINT_INVALID) {
                    next = RuntimeState::FAULT;
                    next_error = ErrorCode::JOINT_STATE_INVALID;
                }
                break;

            case RuntimeState::STANDBY:
                if (event == RuntimeEvent::START_TASK) {
                    next = RuntimeState::RUNNING;
                    next_error = ErrorCode::NONE;
                } else if (event == RuntimeEvent::SENSOR_TIMEOUT) {
                    next = RuntimeState::FAULT;
                    next_error = ErrorCode::SENSOR_TIMEOUT;
                } else if (event == RuntimeEvent::JOINT_INVALID) {
                    next = RuntimeState::FAULT;
                    next_error = ErrorCode::JOINT_STATE_INVALID;
                }
                break;

            case RuntimeState::RUNNING:
                if (event == RuntimeEvent::TASK_SUCCEEDED ||
                    event == RuntimeEvent::TASK_CANCELED) {
                    next = RuntimeState::STANDBY;
                    next_error = ErrorCode::NONE;
                } else if (event == RuntimeEvent::TASK_FAILED) {
                    next = RuntimeState::FAULT;
                    next_error = ErrorCode::TASK_FAILED;
                } else if (event == RuntimeEvent::SENSOR_TIMEOUT) {
                    next = RuntimeState::FAULT;
                    next_error = ErrorCode::SENSOR_TIMEOUT;
                } else if (event == RuntimeEvent::JOINT_INVALID) {
                    next = RuntimeState::FAULT;
                    next_error = ErrorCode::JOINT_STATE_INVALID;
                }
                break;

            case RuntimeState::FAULT:
                if (event == RuntimeEvent::RESET_FAULT) {
                    next = RuntimeState::RECOVERY;
                    next_error = ErrorCode::NONE;
                }
                break;

            case RuntimeState::RECOVERY:
                if (event == RuntimeEvent::RECOVERY_DONE) {
                    next = RuntimeState::STANDBY;
                    next_error = ErrorCode::NONE;
                } else if (event == RuntimeEvent::SENSOR_TIMEOUT) {
                    next = RuntimeState::FAULT;
                    next_error = ErrorCode::RECOVERY_FAILED;
                } else if (event == RuntimeEvent::JOINT_INVALID) {
                    next = RuntimeState::FAULT;
                    next_error = ErrorCode::JOINT_STATE_INVALID;
                }
                break;
        }

        runtime_state_.store(next);
        error_code_.store(next_error);

        if (next != current) {
            RCLCPP_INFO(
                get_logger(),
                "runtime transition %s --%s--> %s error=%s",
                runtimeStateName(current),
                runtimeEventName(event),
                runtimeStateName(next),
                errorCodeName(next_error));
        }
    }

    rclcpp_action::GoalResponse handleGoal(
        const std::shared_ptr<const ExecuteTask::Goal> goal) {
        updateRuntimeState();
        if (goal->target_steps <= 0) {
            RCLCPP_WARN(
                get_logger(),
                "rejecting execute_task goal: target_steps=%d must be positive",
                goal->target_steps);
            return rclcpp_action::GoalResponse::REJECT;
        }

        if (runtime_state_.load() != RuntimeState::STANDBY) {
            RCLCPP_WARN(
                get_logger(),
                "rejecting execute_task goal: runtime_state=%s is not STANDBY",
                runtimeStateName(runtime_state_.load()));
            return rclcpp_action::GoalResponse::REJECT;
        }

        bool expected = false;
        if (!task_active_.compare_exchange_strong(expected, true)) {
            RCLCPP_WARN(get_logger(), "rejecting execute_task goal: another task is active");
            return rclcpp_action::GoalResponse::REJECT;
        }

        task_state_.store(TaskState::RUNNING);
        task_current_step_.store(0);
        processRuntimeEvent(RuntimeEvent::START_TASK);
        RCLCPP_INFO(
            get_logger(),
            "accepted execute_task goal target_steps=%d",
            goal->target_steps);
        return rclcpp_action::GoalResponse::ACCEPT_AND_EXECUTE;
    }

    rclcpp_action::CancelResponse handleCancel(
        const std::shared_ptr<GoalHandleExecuteTask>) {
        RCLCPP_INFO(get_logger(), "received execute_task cancel request");
        return rclcpp_action::CancelResponse::ACCEPT;
    }

    void handleAccepted(const std::shared_ptr<GoalHandleExecuteTask> goal_handle) {
        if (task_thread_.joinable()) {
            task_thread_.join();
        }
        task_thread_ = std::thread([this, goal_handle] {
            executeTask(goal_handle);
        });
    }

    void executeTask(const std::shared_ptr<GoalHandleExecuteTask> goal_handle) {
        const auto goal = goal_handle->get_goal();
        auto feedback = std::make_shared<ExecuteTask::Feedback>();
        auto result = std::make_shared<ExecuteTask::Result>();

        for (int32_t step = 1; step <= goal->target_steps; ++step) {
            if (goal_handle->is_canceling()) {
                task_current_step_.store(step - 1);
                task_state_.store(TaskState::CANCELED);
                result->success = false;
                result->message =
                    "task canceled at step " + std::to_string(step - 1);
                goal_handle->canceled(result);
                task_active_.store(false);
                processRuntimeEvent(RuntimeEvent::TASK_CANCELED);
                RCLCPP_WARN(get_logger(), "%s", result->message.c_str());
                return;
            }

            if (!rclcpp::ok()) {
                task_state_.store(TaskState::FAILED);
                result->success = false;
                result->message = "task aborted because ROS2 is shutting down";
                goal_handle->abort(result);
                task_active_.store(false);
                processRuntimeEvent(RuntimeEvent::TASK_FAILED);
                return;
            }

            std::this_thread::sleep_for(200ms);
            task_current_step_.store(step);
            feedback->current_step = step;
            feedback->progress =
                static_cast<float>(step) / static_cast<float>(goal->target_steps);
            goal_handle->publish_feedback(feedback);
            RCLCPP_INFO(
                get_logger(),
                "execute_task feedback step=%d/%d progress=%.2f",
                step,
                goal->target_steps,
                feedback->progress);
        }

        task_state_.store(TaskState::COMPLETED);
        result->success = true;
        result->message = "task completed";
        goal_handle->succeed(result);
        task_active_.store(false);
        processRuntimeEvent(RuntimeEvent::TASK_SUCCEEDED);
        RCLCPP_INFO(get_logger(), "execute_task completed");
    }

    static const char* runtimeStateName(RuntimeState state) {
        switch (state) {
            case RuntimeState::IDLE:
                return "IDLE";
            case RuntimeState::STANDBY:
                return "STANDBY";
            case RuntimeState::RUNNING:
                return "RUNNING";
            case RuntimeState::FAULT:
                return "FAULT";
            case RuntimeState::RECOVERY:
                return "RECOVERY";
        }
        return "UNKNOWN";
    }

    static const char* runtimeEventName(RuntimeEvent event) {
        switch (event) {
            case RuntimeEvent::SENSOR_HEALTHY:
                return "SensorHealthy";
            case RuntimeEvent::SENSOR_TIMEOUT:
                return "SensorTimeout";
            case RuntimeEvent::JOINT_INVALID:
                return "JointInvalid";
            case RuntimeEvent::START_TASK:
                return "StartTask";
            case RuntimeEvent::TASK_SUCCEEDED:
                return "TaskSucceeded";
            case RuntimeEvent::TASK_CANCELED:
                return "TaskCanceled";
            case RuntimeEvent::TASK_FAILED:
                return "TaskFailed";
            case RuntimeEvent::RESET_FAULT:
                return "ResetFault";
            case RuntimeEvent::RECOVERY_DONE:
                return "RecoveryDone";
        }
        return "UnknownEvent";
    }

    static const char* errorCodeName(ErrorCode error_code) {
        switch (error_code) {
            case ErrorCode::NONE:
                return "NONE";
            case ErrorCode::SENSOR_TIMEOUT:
                return "SENSOR_TIMEOUT";
            case ErrorCode::JOINT_STATE_INVALID:
                return "JOINT_STATE_INVALID";
            case ErrorCode::TASK_FAILED:
                return "TASK_FAILED";
            case ErrorCode::RECOVERY_FAILED:
                return "RECOVERY_FAILED";
        }
        return "UNKNOWN";
    }

    static const char* taskStateName(TaskState state) {
        switch (state) {
            case TaskState::IDLE:
                return "IDLE";
            case TaskState::RUNNING:
                return "RUNNING";
            case TaskState::COMPLETED:
                return "COMPLETED";
            case TaskState::CANCELED:
                return "CANCELED";
            case TaskState::FAILED:
                return "FAILED";
        }
        return "UNKNOWN";
    }

    std::string buildStatusSummary() const {
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(2)
            << "state=" << runtimeStateName(runtime_state_.load())
            << " runtime_error=" << errorCodeName(error_code_.load())
            << " imu_count=" << imu_count_
            << " joint_count=" << joint_count_
            << " latest_accel_z=" << latest_accel_z_
            << " latest_joint_count=" << latest_joint_count_
            << " imu_latency_ms=" << latest_imu_latency_ms_
            << " joint_latency_ms=" << latest_joint_latency_ms_
            << " joint_valid=" << static_cast<int>(latest_joint_valid_)
            << " task_state=" << taskStateName(task_state_.load())
            << " task_step=" << task_current_step_.load();
        return oss.str();
    }

    rclcpp::Subscription<sensor_msgs::msg::Imu>::SharedPtr imu_sub_;
    rclcpp::Subscription<sensor_msgs::msg::JointState>::SharedPtr joint_sub_;
    rclcpp::Service<std_srvs::srv::Trigger>::SharedPtr reset_service_;
    rclcpp::Service<std_srvs::srv::Trigger>::SharedPtr query_status_service_;
    rclcpp_action::Server<ExecuteTask>::SharedPtr execute_task_server_;
    rclcpp::TimerBase::SharedPtr heartbeat_timer_;
    std::thread task_thread_;
    std::atomic<bool> task_active_{false};
    std::atomic<TaskState> task_state_{TaskState::IDLE};
    std::atomic<int32_t> task_current_step_{0};
    std::atomic<RuntimeState> runtime_state_{RuntimeState::IDLE};
    std::atomic<ErrorCode> error_code_{ErrorCode::NONE};
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
