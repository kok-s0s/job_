#include <chrono>
#include <cstddef>
#include <map>
#include <sstream>
#include <string>

#include "rclcpp/rclcpp.hpp"
#include "robot_runtime_demo/srv/apply_runtime_event.hpp"
#include "std_msgs/msg/string.hpp"

using namespace std::chrono_literals;

namespace {

std::map<std::string, std::string>
parseKeyValuePayload(const std::string &payload) {
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

} // namespace

class WatchdogNode : public rclcpp::Node {
public:
  using ApplyRuntimeEvent = robot_runtime_demo::srv::ApplyRuntimeEvent;

  WatchdogNode() : Node("watchdog_node") {
    // 订阅所有节点发布到 /runtime/heartbeat 的心跳。
    heartbeat_sub_ = create_subscription<std_msgs::msg::String>(
        "/runtime/heartbeat", 10,
        [this](const std_msgs::msg::String::SharedPtr msg) {
          handleHeartbeat(msg->data);
        });

    // 周期检查每个节点最后一次心跳是否超时。
    check_timer_ = create_wall_timer(1s, [this] { checkTimeouts(); });

    // 通过 /runtime/apply_event 服务触发 runtime_node 进入 Fault。
    apply_event_client_ =
        create_client<ApplyRuntimeEvent>("runtime/apply_event");
  }

private:
  struct HeartbeatRecord {
    std::size_t seq = 0;
    int64_t stamp_ms = 0;
    std::string status = "UNKNOWN";
    bool seen = false; // 是否收到过该节点的心跳
  };

  void handleHeartbeat(const std::string &payload) {
    const auto values = parseKeyValuePayload(payload);
    const auto node_it = values.find("node");
    if (node_it == values.end()) {
      RCLCPP_WARN(get_logger(), "heartbeat without node field: %s",
                  payload.c_str());
      return;
    }

    HeartbeatRecord &record = heartbeat_table_[node_it->second];
    record.seen = true;
    if (const auto seq_it = values.find("seq"); seq_it != values.end()) {
      record.seq = static_cast<std::size_t>(std::stoull(seq_it->second));
    }
    if (const auto stamp_it = values.find("stamp_ms");
        stamp_it != values.end()) {
      record.stamp_ms = std::stoll(stamp_it->second);
    }
    if (const auto status_it = values.find("status");
        status_it != values.end()) {
      record.status = status_it->second;
    }

    RCLCPP_INFO(get_logger(), "[watchdog_rx] %s", payload.c_str());
  }

  void checkTimeouts() {
    const auto current_ms = nowMs();
    bool any_timeout = false;

    for (const auto &[node, record] : heartbeat_table_) {
      // 启动宽限期：还没收到过心跳的节点不判超时，避免刚启动时误报。
      if (!record.seen) {
        RCLCPP_INFO(get_logger(),
                    "[watchdog] node=%s status=UNKNOWN reason=no_heartbeat_yet",
                    node.c_str());
        continue;
      }

      const auto age_ms = current_ms - record.stamp_ms;
      if (age_ms > timeout_ms_) {
        any_timeout = true;
        RCLCPP_WARN(get_logger(),
                    "[watchdog] node=%s status=TIMEOUT age_ms=%ld last_seq=%zu",
                    node.c_str(), age_ms, record.seq);
      } else {
        RCLCPP_INFO(get_logger(),
                    "[watchdog] node=%s status=OK age_ms=%ld last_seq=%zu",
                    node.c_str(), age_ms, record.seq);
      }
    }

    // 任一关键节点超时，触发 runtime_node 进入 Fault。
    if (any_timeout && !fault_triggered_) {
      triggerFault();
    }
  }

  void triggerFault() {
    if (!apply_event_client_->service_is_ready()) {
      RCLCPP_WARN(
          get_logger(),
          "[watchdog] apply_event service not ready, skip fault trigger");
      return;
    }

    auto request = std::make_shared<ApplyRuntimeEvent::Request>();
    request->event = "SensorTimeout";

    auto future = apply_event_client_->async_send_request(
        request,
        [this](rclcpp::Client<ApplyRuntimeEvent>::SharedFuture response) {
          const auto result = response.get();
          fault_triggered_ = true;
          RCLCPP_WARN(get_logger(),
                      "[watchdog] fault triggered accepted=%d transitioned=%d "
                      "current_state=%s error=%s",
                      static_cast<int>(result->accepted),
                      static_cast<int>(result->transitioned),
                      result->current_state.c_str(),
                      result->runtime_error.c_str());
        });
    (void)future;
  }

  int64_t nowMs() const { return now().nanoseconds() / 1000000; }

  rclcpp::Subscription<std_msgs::msg::String>::SharedPtr heartbeat_sub_;
  rclcpp::TimerBase::SharedPtr check_timer_;
  rclcpp::Client<ApplyRuntimeEvent>::SharedPtr apply_event_client_;
  std::map<std::string, HeartbeatRecord> heartbeat_table_;
  bool fault_triggered_ = false;
  // 心跳超时阈值：3 秒无心跳判定为 TIMEOUT。
  static constexpr int64_t timeout_ms_ = 3000;
};

int main(int argc, char **argv) {
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<WatchdogNode>());
  rclcpp::shutdown();
  return 0;
}
