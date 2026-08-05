#!/usr/bin/env bash
# 测试 watchdog 超时检测：杀掉 sensor_sim_node，观察 watchdog 是否判定 TIMEOUT 并触发 Fault。
source /opt/ros/jazzy/setup.bash
source /mnt/d/Work/Code/job_/projects/ros2_runtime_demo/install/setup.bash

OUT=/tmp/launch_out.txt
rm -f "$OUT"

echo "=== 启动 launch ==="
ros2 launch robot_runtime_demo runtime_demo.launch.py > "$OUT" 2>&1 &
LAUNCH_PID=$!
sleep 6

echo "=== 当前节点进程 ==="
ps aux | grep -E 'sensor_sim|runtime_node|heartbeat|watchdog' | grep -v grep

echo "=== 杀掉 sensor_sim_node ==="
pkill -9 -f sensor_sim_node
echo "kill exit=$?"

sleep 7

echo "=== watchdog 输出（超时检测）==="
grep -E '\[watchdog\]' "$OUT" | tail -30

echo "=== 清理 ==="
kill -9 "$LAUNCH_PID" 2>/dev/null
pkill -9 -f runtime_node 2>/dev/null
pkill -9 -f heartbeat_monitor_node 2>/dev/null
pkill -9 -f watchdog_node 2>/dev/null
echo "DONE"
