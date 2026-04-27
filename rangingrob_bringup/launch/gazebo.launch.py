"""
Demarrage avec Gazebo Harmonic (Ubuntu 24.04 / ROS2 Jazzy).

Architecture :
  - Gazebo publie le capteur gpu_lidar sur /sharp_lidar_gz (topic gz)
  - ros_gz_bridge convertit /sharp_lidar_gz (gz.msgs.LaserScan) en /sharp_lidar (ROS)
  - GazeboSharpHardwareInterface souscrit a /sharp_lidar et alimente l'interface d'etat
  - Le controller_manager tourne en node ROS separe (meme pattern que mockup/hardware)

Prerequis :
    sudo apt install ros-jazzy-ros-gz ros-jazzy-gz-ros2-control
"""

import os
from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import IncludeLaunchDescription, RegisterEventHandler, SetEnvironmentVariable, TimerAction
from launch.event_handlers import OnProcessStart
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch.substitutions import Command, FindExecutable, PathJoinSubstitution
from launch_ros.actions import Node
from launch_ros.parameter_descriptions import ParameterValue
from launch_ros.substitutions import FindPackageShare


def generate_launch_description():
    pkg_bringup = get_package_share_directory("rangingrob_bringup")
    pkg_description = get_package_share_directory("rangingrob_description")
    # Gazebo convertit package:// en model:// et cherche dans GZ_SIM_RESOURCE_PATH.
    # Le parent du share du paquet contient le dossier rangingrob_description/mesh/.
    gz_resource_path = os.path.dirname(pkg_description)

    robot_description_content = Command([
        FindExecutable(name="xacro"),
        " ",
        PathJoinSubstitution([
            FindPackageShare("rangingrob_description"), "urdf", "robot.urdf.xacro"
        ]),
        " hardware_type:=gazebo",
    ])
    robot_description = {"robot_description": ParameterValue(robot_description_content, value_type=str)}

    controllers_yaml = os.path.join(pkg_bringup, "config", "controllers.yaml")
    world_sdf = os.path.join(pkg_bringup, "worlds", "sharp_test.sdf")

    # Demarrage de Gazebo Harmonic avec le monde de test (camera zoomee sur le capteur)
    gz_sim = IncludeLaunchDescription(
        PythonLaunchDescriptionSource([
            PathJoinSubstitution([
                FindPackageShare("ros_gz_sim"), "launch", "gz_sim.launch.py"
            ])
        ]),
        launch_arguments={"gz_args": ["-r ", world_sdf]}.items(),
    )

    robot_state_publisher = Node(
        package="robot_state_publisher",
        executable="robot_state_publisher",
        output="screen",
        parameters=[robot_description],
    )

    # Injection du robot dans Gazebo depuis le topic /robot_description
    spawn_robot = Node(
        package="ros_gz_sim",
        executable="create",
        arguments=["-topic", "/robot_description", "-name", "sharp_robot"],
        output="screen",
    )

    # Bridge unidirectionnel : Gazebo gz.msgs.LaserScan -> ROS sensor_msgs/LaserScan
    # La syntaxe "[" indique la direction Gazebo -> ROS
    gz_bridge = Node(
        package="ros_gz_bridge",
        executable="parameter_bridge",
        arguments=[
            "/sharp_lidar_gz@sensor_msgs/msg/LaserScan[gz.msgs.LaserScan"
        ],
        remappings=[("/sharp_lidar_gz", "/sharp_lidar")],
        output="screen",
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
        SetEnvironmentVariable("GZ_SIM_RESOURCE_PATH", gz_resource_path),
        # Sur ThinkPad Optimus, GLVND charge le vendeur EGL NVIDIA (10_nvidia.json,
        # priorite haute) qui echoue avec "driver (null)". On restreint EGL a Mesa
        # uniquement : Intel GPU via DRI2, fonctionne pour la GUI et les capteurs.
        SetEnvironmentVariable(
            "__EGL_VENDOR_LIBRARY_FILENAMES",
            "/usr/share/glvnd/egl_vendor.d/50_mesa.json",
        ),
        gz_sim,
        robot_state_publisher,
        # Laisser Gazebo demarrer avant de spawner le robot
        TimerAction(period=3.0, actions=[spawn_robot]),
        gz_bridge,
        controller_manager,
        spawn_after_cm,
        rviz,
    ])
