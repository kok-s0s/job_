#include <iostream>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <variant>
#include <vector>

struct ImuSample {
    int seq{};
    double accel_x{};
    double accel_y{};
    double accel_z{};
};

std::optional<ImuSample> readImu(int seq) {
    if (seq % 4 == 0) {
        return std::nullopt;
    }

    return ImuSample{seq, 0.1 * seq, 0.2 * seq, 9.8};
}

struct StartTask {
    int task_id{};
};

struct StopTask {
    std::string reason;
};

struct FaultEvent {
    int code{};
    std::string message;
};

using RobotEvent = std::variant<StartTask, StopTask, FaultEvent>;

struct EventStats {
    int start{};
    int stop{};
    int fault{};
};

void handleEvent(const RobotEvent& event, EventStats& stats) {
    std::visit([&](const auto& value) {
        using T = std::decay_t<decltype(value)>;
        if constexpr (std::is_same_v<T, StartTask>) {
            ++stats.start;
        } else if constexpr (std::is_same_v<T, StopTask>) {
            ++stats.stop;
        } else if constexpr (std::is_same_v<T, FaultEvent>) {
            ++stats.fault;
        }
    }, event);
}

std::string_view commandName(std::string_view line) {
    const std::size_t first_non_space = line.find_first_not_of(' ');
    if (first_non_space == std::string_view::npos) {
        return {};
    }

    line.remove_prefix(first_non_space);
    const std::size_t pos = line.find(' ');
    if (pos == std::string_view::npos) {
        return line;
    }
    return line.substr(0, pos);
}

int main() {
    int imu_ok = 0;
    int imu_missing = 0;
    double last_accel_z = 0.0;

    for (int seq = 0; seq < 10; ++seq) {
        const auto sample = readImu(seq);
        if (sample) {
            ++imu_ok;
            last_accel_z = sample->accel_z;
        } else {
            ++imu_missing;
        }
    }

    EventStats stats;
    const std::vector<RobotEvent> events{
        StartTask{1001},
        StartTask{1002},
        StopTask{"operator request"},
        FaultEvent{42, "imu timeout"},
    };

    for (const auto& event : events) {
        handleEvent(event, stats);
    }

    const std::string move = "MOVE joint_1 1.57";
    const std::string stop = "STOP emergency";
    const std::string status = "STATUS";
    const std::string spaced = "   RESET fault";

    const auto cmd_move = commandName(move);
    const auto cmd_stop = commandName(stop);
    const auto cmd_status = commandName(status);
    const auto cmd_spaced = commandName(spaced);

    const bool pass = imu_ok == 7 &&
                      imu_missing == 3 &&
                      last_accel_z == 9.8 &&
                      stats.start == 2 &&
                      stats.stop == 1 &&
                      stats.fault == 1 &&
                      cmd_move == "MOVE" &&
                      cmd_stop == "STOP" &&
                      cmd_status == "STATUS" &&
                      cmd_spaced == "RESET";

    std::cout << "imu_ok: " << imu_ok << '\n';
    std::cout << "imu_missing: " << imu_missing << '\n';
    std::cout << "last_accel_z: " << last_accel_z << '\n';
    std::cout << "events_start: " << stats.start << '\n';
    std::cout << "events_stop: " << stats.stop << '\n';
    std::cout << "events_fault: " << stats.fault << '\n';
    std::cout << "cmd_move: " << cmd_move << '\n';
    std::cout << "cmd_stop: " << cmd_stop << '\n';
    std::cout << "cmd_status: " << cmd_status << '\n';
    std::cout << "cmd_spaced: " << cmd_spaced << '\n';
    std::cout << "result: " << (pass ? "pass" : "fail") << '\n';

    return pass ? 0 : 1;
}
