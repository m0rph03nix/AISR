"""Lance le poste de chargement.

hardware_type:=mock (défaut) ou real
backend:=ik (défaut) ou moveit
"""

import os

from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, GroupAction, IncludeLaunchDescription
from launch.conditions import IfCondition
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch.substitutions import (Command, EqualsSubstitution, FindExecutable,
                                  LaunchConfiguration, PathJoinSubstitution)
from launch_ros.actions import Node, PushRosNamespace, SetRemap
from launch_ros.parameter_descriptions import ParameterValue
from launch_ros.substitutions import FindPackageShare


def generate_launch_description():
    controllers = os.path.join(
        get_package_share_directory('scara_station'), 'config', 'controllers.yaml')
    hardware_type = LaunchConfiguration('hardware_type')

    robot_description = Command([
        FindExecutable(name='xacro'), ' ',
        PathJoinSubstitution([
            FindPackageShare('scara_station'), 'urdf', 'scara_station.urdf.xacro'
        ]),
        ' hardware_type:=', hardware_type,
        ' dynamixel_port:=', LaunchConfiguration('dynamixel_port'),
    ])

    # Tout le bras est dans /scara, avec ses propres TF : il ne se mélange pas à simplebot
    tf_remap = [('/tf', 'tf'), ('/tf_static', 'tf_static')]

    arm = [
        Node(
            package='robot_state_publisher',
            executable='robot_state_publisher',
            namespace='scara',
            parameters=[{
                'robot_description': ParameterValue(robot_description, value_type=str)
            }],
            remappings=tf_remap,
        ),
        Node(
            package='controller_manager',
            executable='ros2_control_node',
            namespace='scara',
            parameters=[controllers],
        ),
        Node(
            package='controller_manager',
            executable='spawner',
            namespace='scara',
            arguments=['joint_state_broadcaster', 'scara_cpe_group_controller',
                       '--param-file', controllers],
        ),
        Node(
            package='scara_station',
            executable='magnet_node.py',
            namespace='scara',
            parameters=[{
                'mode': hardware_type,
                'serial_port': LaunchConfiguration('magnet_port'),
            }],
            remappings=tf_remap,
        ),
    ]

    move_group = GroupAction(
        condition=IfCondition(EqualsSubstitution(LaunchConfiguration('backend'), 'moveit')),
        actions=[
            PushRosNamespace('scara'),
            SetRemap('/tf', 'tf'),
            SetRemap('/tf_static', 'tf_static'),
            IncludeLaunchDescription(PythonLaunchDescriptionSource(
                os.path.join(get_package_share_directory('scara_station'),
                             'launch', 'move_group.launch.py'))),
        ],
    )

    station = Node(
        package='scara_station',
        executable='station_node.py',
        parameters=[{'backend': LaunchConfiguration('backend')}],
        output='screen',
    )

    return LaunchDescription([
        DeclareLaunchArgument('hardware_type', default_value='mock'),
        DeclareLaunchArgument('backend', default_value='ik'),
        DeclareLaunchArgument('dynamixel_port', default_value='/dev/ttyUSB1'),
        DeclareLaunchArgument('magnet_port', default_value='/dev/ttyACM1'),
        *arm,
        move_group,
        station,
    ])
