#include <chrono>
#include <cstddef>
#include <map>
#include <sstream>
#include <string>

#include "rclcpp/rclcpp.hpp"
#include "std_msgs/msg/string.hpp"

using namespace std::chrono_literals;

namespace {

std::map<std::string, std::string> parseKeyValuePayload(const std::string& payload) {
    std::map<std::string, std::string> values;
    std::istringstream stream(payload);
    std::string token;
    while (stream >> token) {
        const auto equal_pos = token.find('=');
        if (equal_pos == std::string::npos) {
            continue;
        }
        values[token.substr(0, equal_pos)] = token.substr(equal_pos + 1);
    }
    return values;
}

}  // namespace

class HeartbeatMonitorNode : public rclcpp::Node {
public:
    HeartbeatMonitorNode()
        : Node("heartbeat_monitor_node") {
        heartbeat_pub_ = create_publisher<std_msgs::msg::String>("/runtime/heartbeat", 10);
        heartbeat_sub_ = create_subscription<std_msgs::msg::String>(
            "/runtime/heartbeat",
            10,
            [this](const std_msgs::msg::String::SharedPtr msg) {
                handleHeartbeat(msg->data);
            });
        publish_timer_ = create_wall_timer(1s, [this] {
            publishHeartbeat();
        });
        report_timer_ = create_wall_timer(1s, [this] {
            reportHeartbeatTable();
        });
    }

private:
    struct HeartbeatRecord {
        std::size_t seq = 0;
        int64_t stamp_ms = 0;
        std::string status = "UNKNOWN";
    };

    void handleHeartbeat(const std::string& payload) {
        const auto values = parseKeyValuePayload(payload);
        const auto node_it = values.find("node");
        if (node_it == values.end()) {
            RCLCPP_WARN(get_logger(), "heartbeat without node field: %s", payload.c_str());
            return;
        }

        HeartbeatRecord& record = heartbeat_table_[node_it->second];
        if (const auto seq_it = values.find("seq"); seq_it != values.end()) {
            record.seq = static_cast<std::size_t>(std::stoull(seq_it->second));
        }
        if (const auto stamp_it = values.find("stamp_ms"); stamp_it != values.end()) {
            record.stamp_ms = std::stoll(stamp_it->second);
        }
        if (const auto status_it = values.find("status"); status_it != values.end()) {
            record.status = status_it->second;
        }

        RCLCPP_INFO(get_logger(), "[heartbeat_rx] %s", payload.c_str());
    }

    void publishHeartbeat() {
        std_msgs::msg::String heartbeat;
        std::ostringstream payload;
        payload << "node=heartbeat_monitor_node"
                << " seq=" << ++heartbeat_sequence_
                << " stamp_ms=" << nowMs()
                << " status=OK"
                << " message=\"heartbeat monitor alive\"";
        heartbeat.data = payload.str();
        heartbeat_pub_->publish(heartbeat);
        RCLCPP_INFO(get_logger(), "[heartbeat] %s", heartbeat.data.c_str());
    }

    void reportHeartbeatTable() {
        const auto current_ms = nowMs();
        for (const auto& [node, record] : heartbeat_table_) {
            const auto age_ms = current_ms - record.stamp_ms;
            RCLCPP_INFO(
                get_logger(),
                "[heartbeat_table] node=%s last_seq=%zu last_stamp_ms=%ld age_ms=%ld status=%s",
                node.c_str(),
                record.seq,
                record.stamp_ms,
                age_ms,
                record.status.c_str());
        }
    }

    int64_t nowMs() const {
        return now().nanoseconds() / 1000000;
    }

    rclcpp::Publisher<std_msgs::msg::String>::SharedPtr heartbeat_pub_;
    rclcpp::Subscription<std_msgs::msg::String>::SharedPtr heartbeat_sub_;
    rclcpp::TimerBase::SharedPtr publish_timer_;
    rclcpp::TimerBase::SharedPtr report_timer_;
    std::map<std::string, HeartbeatRecord> heartbeat_table_;
    std::size_t heartbeat_sequence_ = 0;
};

int main(int argc, char** argv) {
    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<HeartbeatMonitorNode>());
    rclcpp::shutdown();
    return 0;
}
