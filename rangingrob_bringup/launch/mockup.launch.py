"""
Demarrage avec hardware simule (mock_components/GenericSystem).
Aucun materiel ni simulateur requis.

Visualisation de la courbe :
    ros2 run rqt_plot rqt_plot   (puis saisir /range_sensor_broadcaster/range/range)
"""

import os
from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import RegisterEventHandler
from launch.event_handlers import OnProcessStart
from launch.substitutions import Command, FindExecutable, PathJoinSubstitution
from launch_ros.actions import Node
from launch_ros.parameter_descriptions import ParameterValue
from launch_ros.substitutions import FindPackageShare


def generate_launch_description():
    pkg_bringup = get_package_share_directory("rangingrob_bringup")

    robot_description_content = Command([
        FindExecutable(name="xacro"),
        " ",
        PathJoinSubstitution([
            FindPackageShare("rangingrob_description"), "urdf", "robot.urdf.xacro"
        ]),
        " hardware_type:=mockup",
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

    # ForwardCommandController : ecrit dans la command interface sharp_sensor/range
    # (rendue disponible par mock_sensor_commands=true dans le URDF mockup).
    # Utilisation : ros2 topic pub --once /forward_command_controller/commands \
    #               std_msgs/msg/Float64MultiArray "data: [0.08]"
    forward_command_spawner = Node(
        package="controller_manager",
        executable="spawner",
        arguments=["forward_command_controller", "--param-file", controllers_yaml],
        output="screen",
    )

    spawn_after_cm = RegisterEventHandler(
        event_handler=OnProcessStart(
            target_action=controller_manager,
            on_start=[range_broadcaster_spawner, forward_command_spawner],
        )
    )

    rviz = Node(
        package="rviz2",
        executable="rviz2",
        arguments=["-d", os.path.join(pkg_bringup, "rviz", "sharp_sensor.rviz")],
        output="screen",
    )

    return LaunchDescription([
        robot_state_publisher,
        controller_manager,
        spawn_after_cm,
        rviz,
    ])
