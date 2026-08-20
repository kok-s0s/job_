#pragma once

#include <cstdlib>
#include <string>

namespace robot_runtime_demo {

inline std::string runtimeSessionId() {
    const char* value = std::getenv("ROBOT_RUNTIME_SESSION_ID");
    if (value != nullptr && value[0] != '\0') {
        return value;
    }
    return "local_session";
}

}  // namespace robot_runtime_demo
