"""Fonctions de haut niveau du simplebot, pour écrire une mission."""
import math
import time

import rclpy
from geometry_msgs.msg import PoseStamped, TwistStamped
from nav2_simple_commander.robot_navigator import BasicNavigator, TaskResult
from rclpy.time import Time
from sensor_msgs.msg import Range
from std_srvs.srv import Trigger
from tf2_ros import Buffer, TransformException, TransformListener

STATIONS = {
    'poste': (0.76, 0.38),
    'zone 1': (1.14, 0.76),
    'zone 2': (0.38, 0.76),
    'base': (0.0, 0.0),
}


class Robot(BasicNavigator):

    def __init__(self):
        super().__init__()
        self.front_distance = 1.0
        self.create_subscription(
            Range, '/range_sensor_broadcaster_center/range', self.range_callback, 10)
        self.cmd_pub = self.create_publisher(
            TwistStamped, '/diff_drive_controller/cmd_vel', 10)
        self.tf_buffer = Buffer()
        self.tf_listener = TransformListener(self.tf_buffer, self)
        self.station = self.create_client(Trigger, '/station/load_coin')
        self.station.wait_for_service()

    def range_callback(self, msg):
        self.front_distance = msg.range

    def go_to(self, name):
        """Va à une station. Renvoie True si le robot est arrivé."""
        pose = PoseStamped()
        pose.header.frame_id = 'map'
        pose.header.stamp = self.get_clock().now().to_msg()
        pose.pose.position.x, pose.pose.position.y = STATIONS[name]
        pose.pose.orientation.w = 1.0
        self.goToPose(pose)
        while not self.isTaskComplete():
            pass
        return self.getResult() == TaskResult.SUCCEEDED

    def send_speed(self, linear, angular):
        msg = TwistStamped()
        msg.header.stamp = self.get_clock().now().to_msg()
        msg.header.frame_id = 'base_link'
        msg.twist.linear.x = linear
        msg.twist.angular.z = angular
        self.cmd_pub.publish(msg)

    def turn_to_north(self):
        while True:
            rclpy.spin_once(self, timeout_sec=0.05)
            try:
                tf = self.tf_buffer.lookup_transform('map', 'base_footprint', Time())
            except TransformException:
                continue
            q = tf.transform.rotation
            heading = math.atan2(2 * (q.w * q.z + q.x * q.y), 1 - 2 * (q.y * q.y + q.z * q.z))
            error = math.atan2(math.sin(math.pi / 2 - heading), math.cos(math.pi / 2 - heading))
            if abs(error) < 0.03:
                break
            self.send_speed(0.0, math.copysign(0.3, error))
        self.send_speed(0.0, 0.0)

    def dock(self):
        """Se place sous le bras : avance jusqu'à 6 cm du fond de l'alcôve."""
        self.turn_to_north()
        while self.front_distance > 0.06:
            rclpy.spin_once(self, timeout_sec=0.05)
            self.send_speed(0.03, 0.0)
        self.send_speed(0.0, 0.0)

    def leave_dock(self):
        """Sort en marche arrière : un demi-tour dans l'alcôve ferait perdre AMCL."""
        start = time.monotonic()
        while time.monotonic() - start < 6.0:
            rclpy.spin_once(self, timeout_sec=0.05)
            self.send_speed(-0.06, 0.0)
        self.send_speed(0.0, 0.0)

    def load_coin(self):
        """Demande une pièce au poste. Renvoie True si elle est chargée."""
        future = self.station.call_async(Trigger.Request())
        rclpy.spin_until_future_complete(self, future)
        self.info(future.result().message)
        return future.result().success
