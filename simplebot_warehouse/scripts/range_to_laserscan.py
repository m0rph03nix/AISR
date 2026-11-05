#!/usr/bin/env python3
import math

import rclpy
from rclpy.node import Node
from sensor_msgs.msg import LaserScan, Range

# Position de chaque capteur sur le robot : x, y, angle
SENSORS = {
    'left': (0.10, 0.07, math.radians(60)),
    'center': (0.10, 0.0, 0.0),
    'right': (0.10, -0.07, math.radians(-60)),
}
NB_RAYS = 181


class RangeToLaserScan(Node):

    def __init__(self):
        super().__init__('range_to_laserscan')
        self.distances = {}
        self.stamp = None
        for name in SENSORS:
            self.create_subscription(
                Range, f'/range_sensor_broadcaster_{name}/range',
                lambda msg, name=name: self.range_callback(msg, name), 10)
        self.scan_pub = self.create_publisher(LaserScan, '/scan', 10)
        self.create_timer(0.05, self.publish_scan)

    def range_callback(self, msg, name):
        self.distances[name] = msg.range
        self.stamp = msg.header.stamp

    def publish_scan(self):
        if self.stamp is None:
            return

        scan = LaserScan()
        scan.header.stamp = self.stamp
        scan.header.frame_id = 'base_footprint'
        scan.angle_min = -math.pi / 2
        scan.angle_max = math.pi / 2
        scan.angle_increment = math.pi / (NB_RAYS - 1)
        scan.range_min = 0.02
        scan.range_max = 0.30
        scan.ranges = [float('inf')] * NB_RAYS

        for name, distance in self.distances.items():
            if distance > 0.145:  # rien devant ce capteur
                continue
            x, y, angle = SENSORS[name]
            hit_x = x + distance * math.cos(angle)
            hit_y = y + distance * math.sin(angle)
            ray = round((math.atan2(hit_y, hit_x) - scan.angle_min) / scan.angle_increment)
            scan.ranges[ray] = math.hypot(hit_x, hit_y)

        self.scan_pub.publish(scan)


def main():
    rclpy.init()
    rclpy.spin(RangeToLaserScan())


if __name__ == '__main__':
    main()
