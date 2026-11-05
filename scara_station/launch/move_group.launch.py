"""Lance move_group avec le solveur pick_ik de ce paquet."""

import os

from ament_index_python.packages import get_package_share_directory
from moveit_configs_utils import MoveItConfigsBuilder
from moveit_configs_utils.launches import generate_move_group_launch


def generate_launch_description():
    kinematics = os.path.join(
        get_package_share_directory('scara_station'), 'config', 'moveit_kinematics.yaml')
    config = MoveItConfigsBuilder('scara_cpe', package_name='scara_cpe_moveit2') \
        .robot_description_kinematics(file_path=kinematics).to_moveit_configs()
    return generate_move_group_launch(config)
