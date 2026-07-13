#include <iostream>
#include <memory>
#include <string>
#include <utility>

class ResourceGuard {
public:
    explicit ResourceGuard(std::string name)
        : name_(std::move(name)), valid_(true) {
        std::cout << "open resource: " << name_ << '\n';
    }

    ~ResourceGuard() {
        closeIfNeeded();
    }

    ResourceGuard(const ResourceGuard&) = delete;
    ResourceGuard& operator=(const ResourceGuard&) = delete;

    ResourceGuard(ResourceGuard&& other) noexcept
        : name_(std::move(other.name_)), valid_(other.valid_) {
        other.valid_ = false;
        std::cout << "move construct resource owner\n";
    }

    ResourceGuard& operator=(ResourceGuard&& other) noexcept {
        if (this == &other) {
            return *this;
        }

        closeIfNeeded();
        name_ = std::move(other.name_);
        valid_ = other.valid_;
        other.valid_ = false;
        std::cout << "move assign resource owner\n";
        return *this;
    }

    void use() const {
        if (valid_) {
            std::cout << "use resource: " << name_ << '\n';
            return;
        }

        std::cout << "resource is moved-from, skip use\n";
    }

private:
    void closeIfNeeded() {
        if (valid_) {
            std::cout << "close resource: " << name_ << '\n';
            valid_ = false;
        }
    }

    std::string name_;
    bool valid_{false};
};

class SensorSession {
public:
    explicit SensorSession(std::string sensor_name)
        : sensor_name_(std::move(sensor_name)) {
        std::cout << "connect sensor: " << sensor_name_ << '\n';
    }

    ~SensorSession() {
        std::cout << "disconnect sensor: " << sensor_name_ << '\n';
    }

    SensorSession(const SensorSession&) = delete;
    SensorSession& operator=(const SensorSession&) = delete;

    void start() const {
        std::cout << "start sensor session: " << sensor_name_ << '\n';
    }

private:
    std::string sensor_name_;
};

void startSession(std::unique_ptr<SensorSession> session) {
    std::cout << "startSession takes unique ownership\n";
    session->start();
}

struct RobotNode;

struct RobotMonitor {
    explicit RobotMonitor(std::string name) : name(std::move(name)) {}

    void printNodeState() const {
        if (auto locked = node.lock()) {
            std::cout << "monitor " << name << " observes node still alive\n";
        } else {
            std::cout << "monitor " << name << " observes node expired\n";
        }
    }

    std::string name;
    std::weak_ptr<RobotNode> node;
};

struct RobotNode {
    explicit RobotNode(std::string name) : name(std::move(name)) {}

    ~RobotNode() {
        std::cout << "destroy robot node: " << name << '\n';
    }

    std::string name;
    std::shared_ptr<RobotMonitor> monitor;
};

void demonstrateResourceGuard() {
    std::cout << "\n[1] ResourceGuard ownership\n";

    ResourceGuard imu("imu-driver");
    imu.use();

    ResourceGuard owner = std::move(imu);
    imu.use();
    owner.use();

    ResourceGuard can("can-device");
    can = std::move(owner);
    owner.use();
    can.use();
}

void demonstrateUniquePtr() {
    std::cout << "\n[2] unique_ptr ownership transfer\n";

    auto session = std::make_unique<SensorSession>("force-torque-sensor");
    session->start();

    startSession(std::move(session));

    if (!session) {
        std::cout << "caller unique_ptr is empty after ownership transfer\n";
    }
}

void demonstrateSharedWeakPtr() {
    std::cout << "\n[3] shared_ptr with weak_ptr observer\n";

    auto node = std::make_shared<RobotNode>("runtime-node");
    auto monitor = std::make_shared<RobotMonitor>("watchdog");

    node->monitor = monitor;
    monitor->node = node;

    std::cout << "node use_count while shared by local variable: " << node.use_count() << '\n';
    monitor->printNodeState();

    node.reset();
    monitor->printNodeState();
}

int main() {
    demonstrateResourceGuard();
    demonstrateUniquePtr();
    demonstrateSharedWeakPtr();

    return 0;
}
