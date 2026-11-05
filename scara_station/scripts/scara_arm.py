#!/usr/bin/env python3
"""Commande du bras et de l'électroaimant."""
import rclpy
from control_msgs.action import FollowJointTrajectory
from geometry_msgs.msg import Pose
from moveit_msgs.action import MoveGroup
from moveit_msgs.msg import BoundingVolume, Constraints, PositionConstraint
from rclpy.action import ActionClient
from rclpy.node import Node
from shape_msgs.msg import SolidPrimitive
from std_srvs.srv import SetBool
from trajectory_msgs.msg import JointTrajectoryPoint


class ScaraArm(Node):

    def __init__(self):
        super().__init__('scara_arm')
        self.trajectory_client = ActionClient(
            self, FollowJointTrajectory,
            '/scara/scara_cpe_group_controller/follow_joint_trajectory')
        self.moveit_client = ActionClient(self, MoveGroup, '/scara/move_action')
        self.magnet_client = self.create_client(SetBool, '/scara/magnet')
        self.trajectory_client.wait_for_server()
        self.magnet_client.wait_for_service()

    def wait_for(self, future):
        rclpy.spin_until_future_complete(self, future)
        return future.result()

    def move(self, q1, q2):
        """Envoie directement les angles au contrôleur."""
        point = JointTrajectoryPoint()
        point.positions = [q1, q2]
        point.time_from_start.sec = 2

        goal = FollowJointTrajectory.Goal()
        goal.trajectory.joint_names = ['shoulder_1_joint', 'shoulder_2_joint']
        goal.trajectory.points = [point]

        handle = self.wait_for(self.trajectory_client.send_goal_async(goal))
        self.wait_for(handle.get_result_async())

    def move_with_moveit(self, x, y):
        """Demande à MoveIt d'amener l'aimant à moins de 5 mm du point."""
        center = Pose()
        center.position.x = x
        center.position.y = y
        center.position.z = 0.01
        center.orientation.w = 1.0

        region = BoundingVolume()
        region.primitives = [SolidPrimitive(type=SolidPrimitive.SPHERE, dimensions=[0.005])]
        region.primitive_poses = [center]

        target = PositionConstraint(link_name='end_link', constraint_region=region, weight=1.0)
        target.header.frame_id = 'world'

        goal = MoveGroup.Goal()
        goal.request.group_name = 'scara_cpe_group'
        goal.request.goal_constraints = [Constraints(position_constraints=[target])]
        goal.request.num_planning_attempts = 5
        goal.request.allowed_planning_time = 5.0
        goal.request.max_velocity_scaling_factor = 1.0
        goal.request.max_acceleration_scaling_factor = 1.0

        self.moveit_client.wait_for_server()
        handle = self.wait_for(self.moveit_client.send_goal_async(goal))
        self.wait_for(handle.get_result_async())

    def magnet(self, enabled):
        """Active ou coupe l'aimant. Renvoie True si une pièce est sous l'aimant."""
        response = self.wait_for(self.magnet_client.call_async(SetBool.Request(data=enabled)))
        self.get_logger().info(response.message)
        return response.success
