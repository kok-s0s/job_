#include <algorithm>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>

#include "blocking_queue.hpp"

// ── terminal helpers ──────────────────────────────────────────────────────

static std::mutex g_out;

// Overwrite the current prompt line with msg, then reprint "> ".
static void emit(const std::string& msg) {
    std::lock_guard<std::mutex> lock(g_out);
    std::cout << "\r" << msg << "\033[K\n> " << std::flush;
}

static std::string ts() {
    auto t = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    std::ostringstream s;
    s << std::put_time(std::localtime(&t), "%H:%M:%S");
    return s.str();
}

// ── robot state machine ───────────────────────────────────────────────────

enum class State { IDLE, MOVING, GRASPING, HOLDING, RETURNING };

static std::string state_label(State s) {
    switch (s) {
        case State::IDLE:      return "\033[32mIDLE\033[0m";
        case State::MOVING:    return "\033[33mMOVING\033[0m";
        case State::GRASPING:  return "\033[33mGRASPING\033[0m";
        case State::HOLDING:   return "\033[32mHOLDING\033[0m";
        case State::RETURNING: return "\033[33mRETURNING\033[0m";
    }
    return "?";
}

struct Robot {
    std::mutex mtx;
    State      state   {State::IDLE};
    float      lidar   {1.50f};     // m — distance to target
    float      force   {0.00f};     // N — grip force
    bool       hinted  {false};     // whether the "next step" hint was shown
    bool       running {true};
    std::chrono::steady_clock::time_point since{std::chrono::steady_clock::now()};

    float elapsed() const {
        return std::chrono::duration<float>(
            std::chrono::steady_clock::now() - since).count();
    }
    void enter(State s) {
        state = s; hinted = false;
        since = std::chrono::steady_clock::now();
    }
};

// ── simulation thread: updates sensors + auto-advances certain states ─────

static void sim_loop(Robot& robot) {
    using namespace std::chrono_literals;
    int tick = 0;

    while (true) {
        std::this_thread::sleep_for(600ms);

        std::string out;
        {
            std::lock_guard<std::mutex> lock(robot.mtx);
            if (!robot.running) break;

            float t = robot.elapsed();

            switch (robot.state) {
                case State::MOVING:
                    // LiDAR closes in on the target over 5 seconds
                    robot.lidar = std::max(0.50f, 1.50f - t * 0.20f);
                    if (t > 5.0f && !robot.hinted) {
                        robot.hinted = true;
                        out = "\033[36m[>>]\033[0m Target reached. Type \033[1mgrasp\033[0m.";
                    }
                    break;

                case State::GRASPING:
                    // Grip force builds up over 2 seconds → auto-transition to HOLDING
                    robot.force = std::min(15.0f, t * 7.5f);
                    if (t > 2.0f) {
                        robot.enter(State::HOLDING);
                        robot.force = 15.0f;
                        out = "\033[36m[>>]\033[0m Object secured (force=15.0N). Type \033[1mrelease\033[0m.";
                    }
                    break;

                case State::RETURNING:
                    // LiDAR opens back to 1.5m → auto-transition to IDLE
                    robot.force = 0.0f;
                    robot.lidar = std::min(1.50f, 0.50f + t * 0.20f);
                    if (t > 5.0f) {
                        robot.enter(State::IDLE);
                        robot.lidar = 1.50f;
                        out = "\033[36m[>>]\033[0m Home reached. Ready. Type \033[1mstart\033[0m.";
                    }
                    break;

                default:
                    break;
            }

            // Sensor readout every ~3 s
            if (++tick % 5 == 0 && out.empty()) {
                std::ostringstream ss;
                ss << "[" << ts() << "] "
                   << state_label(robot.state) << "  "
                   << std::fixed << std::setprecision(2)
                   << "lidar=" << robot.lidar << "m  "
                   << "force=" << robot.force << "N";
                out = ss.str();
            }
        }

        if (!out.empty()) emit(out);
    }
}

// ── command handler ───────────────────────────────────────────────────────

// Returns false when the user asks to quit.
static bool handle(const std::string& cmd, Robot& robot) {
    std::string out;
    bool quit = false;

    {
        std::lock_guard<std::mutex> lock(robot.mtx);

        auto try_transition = [&](State from, State to, const std::string& msg) {
            if (robot.state != from) {
                out = "\033[31m[!]\033[0m Can't do that in state " + state_label(robot.state);
            } else {
                robot.enter(to);
                out = "[" + ts() + "] " + msg;
            }
        };

        if (cmd == "start") {
            try_transition(State::IDLE, State::MOVING,
                           "Arm moving to target… (sensor feed started)");

        } else if (cmd == "grasp") {
            try_transition(State::MOVING, State::GRASPING,
                           "Closing gripper…");

        } else if (cmd == "release") {
            try_transition(State::HOLDING, State::RETURNING,
                           "Releasing object, arm returning home…");

        } else if (cmd == "status") {
            std::ostringstream ss;
            ss << "[" << ts() << "] "
               << state_label(robot.state) << "  "
               << std::fixed << std::setprecision(2)
               << "lidar=" << robot.lidar << "m  "
               << "force=" << robot.force << "N";
            out = ss.str();

        } else if (cmd == "quit" || cmd == "exit") {
            robot.running = false;
            quit = true;

        } else if (!cmd.empty()) {
            out = "\033[33m[?]\033[0m Commands: start  grasp  release  status  quit";
        }
    }

    if (!out.empty()) emit(out);
    return !quit;
}

// ── main ──────────────────────────────────────────────────────────────────

int main() {
    std::cout << "\033[2J\033[H";   // clear screen
    std::cout <<
        "┌─────────────────────────────────────────────┐\n"
        "│  RoboMon — 机械臂仿真控制台                 │\n"
        "│  commands: start  grasp  release  status  quit │\n"
        "└─────────────────────────────────────────────┘\n"
        "> " << std::flush;

    Robot robot;
    std::thread sim(sim_loop, std::ref(robot));

    std::string line;
    while (std::getline(std::cin, line)) {
        // trim whitespace
        auto b = line.find_first_not_of(" \t");
        auto e = line.find_last_not_of(" \t");
        std::string cmd = (b == std::string::npos) ? "" : line.substr(b, e - b + 1);

        if (!handle(cmd, robot)) break;

        std::lock_guard<std::mutex> lock(g_out);
        std::cout << "> " << std::flush;
    }

    {
        std::lock_guard<std::mutex> lock(robot.mtx);
        robot.running = false;
    }
    sim.join();
    std::cout << "\nBye.\n";
}
