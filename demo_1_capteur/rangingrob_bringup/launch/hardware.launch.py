"""
Demarrage avec l'ESP32 TTGO T-Display reel (Sharp GP2Y0A51SK0F).

Prerequis :
    sudo usermod -aG dialout $USER   (puis se reconnecter)
    Arduino programme avec arduino/sharp_sensor/sharp_sensor.ino

Argument :
    serial_port  -- port serie de l'ESP32 (defaut : /dev/ttyUSB0)

Exemple :
    ros2 launch rangingrob_bringup hardware.launch.py serial_port:=/dev/ttyACM0
"""

import os
from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, RegisterEventHandler
from launch.event_handlers import OnProcessStart
from launch.substitutions import Command, FindExecutable, LaunchConfiguration, PathJoinSubstitution
from launch_ros.actions import Node
from launch_ros.parameter_descriptions import ParameterValue
from launch_ros.substitutions import FindPackageShare


def generate_launch_description():
    pkg_bringup = get_package_share_directory("rangingrob_bringup")

    serial_port_arg = DeclareLaunchArgument(
        "serial_port",
        default_value="/dev/ttyUSB0",
        description="Port serie de l'ESP32 TTGO T-Display",
    )
    serial_port = LaunchConfiguration("serial_port")

    robot_description_content = Command([
        FindExecutable(name="xacro"),
        " ",
        PathJoinSubstitution([
            FindPackageShare("rangingrob_description"), "urdf", "robot.urdf.xacro"
        ]),
        " hardware_type:=hardware",
        " serial_port:=", serial_port,
    ])
    robot_description = {"robot_description": ParameterValue(robot_description_content, value_type=str)}

    controllers_yaml = os.path.join(pkg_bringup, "config", "controllers.yaml")

    robot_state_publisher = Node(
        package="robot_state_publisher",
        executable="robot_state_publisher",
        output="screen",
        parameters=[robot_description],
    )

    controller_manager = Node(
        package="controller_manager",
        executable="ros2_control_node",
        parameters=[robot_description, controllers_yaml],
        output="screen",
    )

    range_broadcaster_spawner = Node(
        package="controller_manager",
        executable="spawner",
        arguments=["range_sensor_broadcaster", "--param-file", controllers_yaml],
        output="screen",
    )

    spawn_after_cm = RegisterEventHandler(
        event_handler=OnProcessStart(
            target_action=controller_manager,
            on_start=[range_broadcaster_spawner],
        )
    )

    rviz = Node(
        package="rviz2",
        executable="rviz2",
        arguments=["-d", os.path.join(pkg_bringup, "rviz", "sharp_sensor.rviz")],
        output="screen",
    )

    return LaunchDescription([
        serial_port_arg,
        robot_state_publisher,
        controller_manager,
        spawn_after_cm,
        rviz,
    ])
