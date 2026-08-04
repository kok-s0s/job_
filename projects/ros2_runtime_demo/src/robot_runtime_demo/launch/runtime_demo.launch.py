from launch import LaunchDescription
from launch_ros.actions import Node


def generate_launch_description():
    return LaunchDescription([
        Node(
            package='robot_runtime_demo',
            executable='sensor_sim_node',
            name='sensor_sim_node',
            output='screen',
        ),
        Node(
            package='robot_runtime_demo',
            executable='runtime_node',
            name='runtime_node',
            output='screen',
        ),
        Node(
            package='robot_runtime_demo',
            executable='heartbeat_monitor_node',
            name='heartbeat_monitor_node',
            output='screen',
        ),
    ])
