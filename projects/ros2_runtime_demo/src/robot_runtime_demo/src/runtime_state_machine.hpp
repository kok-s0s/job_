#pragma once

enum class RuntimeState {
    Idle,
    Standby,
    Running,
    Fault,
    Recovery,
};

enum class RuntimeEvent {
    SensorHealthy,
    SensorTimeout,
    JointInvalid,
    StartTask,
    TaskSucceeded,
    TaskCanceled,
    TaskFailed,
    ResetFault,
    RecoveryDone,
};

enum class RuntimeError {
    None,
    SensorTimeout,
    JointStateInvalid,
    TaskFailed,
    RecoveryFailed,
};

enum class RuntimeSeverity {
    Info,
    Warning,
    Critical,
};

struct RuntimeErrorInfo {
    RuntimeSeverity severity;
    bool recoverable;
    const char* reason;
    const char* recovery_hint;
};

class RuntimeStateMachine {
public:
    explicit RuntimeStateMachine(RuntimeState initial_state = RuntimeState::Idle)
        : state_(initial_state) {}

    RuntimeState state() const {
        return state_;
    }

    RuntimeError error() const {
        return error_;
    }

    void reset(RuntimeState state = RuntimeState::Idle) {
        state_ = state;
        error_ = RuntimeError::None;
    }

    bool process(RuntimeEvent event) {
        const auto current = state_;
        auto next = current;
        auto next_error = error_;

        switch (current) {
            case RuntimeState::Idle:
                if (event == RuntimeEvent::SensorHealthy) {
                    next = RuntimeState::Standby;
                    next_error = RuntimeError::None;
                } else if (event == RuntimeEvent::SensorTimeout) {
                    next = RuntimeState::Fault;
                    next_error = RuntimeError::SensorTimeout;
                } else if (event == RuntimeEvent::JointInvalid) {
                    next = RuntimeState::Fault;
                    next_error = RuntimeError::JointStateInvalid;
                }
                break;

            case RuntimeState::Standby:
                if (event == RuntimeEvent::StartTask) {
                    next = RuntimeState::Running;
                    next_error = RuntimeError::None;
                } else if (event == RuntimeEvent::SensorTimeout) {
                    next = RuntimeState::Fault;
                    next_error = RuntimeError::SensorTimeout;
                } else if (event == RuntimeEvent::JointInvalid) {
                    next = RuntimeState::Fault;
                    next_error = RuntimeError::JointStateInvalid;
                }
                break;

            case RuntimeState::Running:
                if (event == RuntimeEvent::TaskSucceeded ||
                    event == RuntimeEvent::TaskCanceled) {
                    next = RuntimeState::Standby;
                    next_error = RuntimeError::None;
                } else if (event == RuntimeEvent::TaskFailed) {
                    next = RuntimeState::Fault;
                    next_error = RuntimeError::TaskFailed;
                } else if (event == RuntimeEvent::SensorTimeout) {
                    next = RuntimeState::Fault;
                    next_error = RuntimeError::SensorTimeout;
                } else if (event == RuntimeEvent::JointInvalid) {
                    next = RuntimeState::Fault;
                    next_error = RuntimeError::JointStateInvalid;
                }
                break;

            case RuntimeState::Fault:
                if (event == RuntimeEvent::ResetFault) {
                    next = RuntimeState::Recovery;
                    next_error = RuntimeError::None;
                }
                break;

            case RuntimeState::Recovery:
                if (event == RuntimeEvent::RecoveryDone) {
                    next = RuntimeState::Standby;
                    next_error = RuntimeError::None;
                } else if (event == RuntimeEvent::SensorTimeout) {
                    next = RuntimeState::Fault;
                    next_error = RuntimeError::RecoveryFailed;
                } else if (event == RuntimeEvent::JointInvalid) {
                    next = RuntimeState::Fault;
                    next_error = RuntimeError::JointStateInvalid;
                }
                break;
        }

        if (next == current && next_error == error_) {
            return false;
        }

        state_ = next;
        error_ = next_error;
        return next != current;
    }

private:
    RuntimeState state_;
    RuntimeError error_{RuntimeError::None};
};

inline const char* stateName(RuntimeState state) {
    switch (state) {
        case RuntimeState::Idle:
            return "IDLE";
        case RuntimeState::Standby:
            return "STANDBY";
        case RuntimeState::Running:
            return "RUNNING";
        case RuntimeState::Fault:
            return "FAULT";
        case RuntimeState::Recovery:
            return "RECOVERY";
    }
    return "UNKNOWN";
}

inline const char* eventName(RuntimeEvent event) {
    switch (event) {
        case RuntimeEvent::SensorHealthy:
            return "SensorHealthy";
        case RuntimeEvent::SensorTimeout:
            return "SensorTimeout";
        case RuntimeEvent::JointInvalid:
            return "JointInvalid";
        case RuntimeEvent::StartTask:
            return "StartTask";
        case RuntimeEvent::TaskSucceeded:
            return "TaskSucceeded";
        case RuntimeEvent::TaskCanceled:
            return "TaskCanceled";
        case RuntimeEvent::TaskFailed:
            return "TaskFailed";
        case RuntimeEvent::ResetFault:
            return "ResetFault";
        case RuntimeEvent::RecoveryDone:
            return "RecoveryDone";
    }
    return "UnknownEvent";
}

inline const char* errorName(RuntimeError error) {
    switch (error) {
        case RuntimeError::None:
            return "NONE";
        case RuntimeError::SensorTimeout:
            return "SENSOR_TIMEOUT";
        case RuntimeError::JointStateInvalid:
            return "JOINT_STATE_INVALID";
        case RuntimeError::TaskFailed:
            return "TASK_FAILED";
        case RuntimeError::RecoveryFailed:
            return "RECOVERY_FAILED";
    }
    return "UNKNOWN";
}

inline const char* severityName(RuntimeSeverity severity) {
    switch (severity) {
        case RuntimeSeverity::Info:
            return "INFO";
        case RuntimeSeverity::Warning:
            return "WARNING";
        case RuntimeSeverity::Critical:
            return "CRITICAL";
    }
    return "UNKNOWN";
}

inline RuntimeErrorInfo errorInfo(RuntimeError error) {
    switch (error) {
        case RuntimeError::None:
            return RuntimeErrorInfo{
                RuntimeSeverity::Info,
                true,
                "runtime healthy",
                "no action required",
            };
        case RuntimeError::SensorTimeout:
            return RuntimeErrorInfo{
                RuntimeSeverity::Critical,
                true,
                "sensor heartbeat timeout",
                "check sensor heartbeat then reset fault",
            };
        case RuntimeError::JointStateInvalid:
            return RuntimeErrorInfo{
                RuntimeSeverity::Critical,
                false,
                "joint state shape invalid",
                "inspect joint state publisher shape",
            };
        case RuntimeError::TaskFailed:
            return RuntimeErrorInfo{
                RuntimeSeverity::Warning,
                true,
                "runtime task failed",
                "inspect task result then reset fault",
            };
        case RuntimeError::RecoveryFailed:
            return RuntimeErrorInfo{
                RuntimeSeverity::Critical,
                false,
                "recovery path failed",
                "stop runtime and inspect recovery path",
            };
    }
    return RuntimeErrorInfo{
        RuntimeSeverity::Critical,
        false,
        "unknown runtime error",
        "stop runtime and inspect logs",
    };
}
