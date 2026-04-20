from launch import LaunchDescription
from launch_ros.actions import Node


def generate_launch_description():
    return LaunchDescription([
        Node(
            package='esp32_bridge',
            executable='udp_bridge',
            name='udp_bridge'
        ),
        Node(
            package='esp32_bridge',
            executable='imu_subscriber',
            name='imu_subscriber'
        )

    ])