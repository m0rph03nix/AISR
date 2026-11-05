"""Ouvre RViz avec le panneau MoveIt, sur le bras du namespace /scara."""

import os

from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import GroupAction
from launch_ros.actions import PushRosNamespace, SetRemap
from moveit_configs_utils import MoveItConfigsBuilder
from moveit_configs_utils.launches import generate_moveit_rviz_launch


def generate_launch_description():
    kinematics = os.path.join(
        get_package_share_directory('scara_station'), 'config', 'moveit_kinematics.yaml')
    config = MoveItConfigsBuilder('scara_cpe', package_name='scara_cpe_moveit2') \
        .robot_description_kinematics(file_path=kinematics).to_moveit_configs()

    return LaunchDescription([
        GroupAction([
            PushRosNamespace('scara'),
            SetRemap('/tf', 'tf'),
            SetRemap('/tf_static', 'tf_static'),
            *generate_moveit_rviz_launch(config).entities,
        ]),
    ])
