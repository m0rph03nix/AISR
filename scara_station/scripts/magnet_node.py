#!/usr/bin/env python3
"""Électroaimant du bras.

Mode real : carte série à 9600 bauds, on écrit 1 ou 0, on lit 1 si une pièce est détectée.
Mode mock : des pièces virtuelles sont posées sur les points A, B, C et D.
"""
import math

import rclpy
import serial
from rclpy.node import Node
from rclpy.time import Time
from std_msgs.msg import Bool
from std_srvs.srv import SetBool
from tf2_ros import Buffer, TransformException, TransformListener


class MagnetNode(Node):

    def __init__(self):
        super().__init__('magnet_node')
        self.declare_parameter('mode', 'mock')
        self.declare_parameter('serial_port', '/dev/ttyACM1')
        self.real = self.get_parameter('mode').value == 'real'
        self.detected = False

        if self.real:
            port = self.get_parameter('serial_port').value
            self.board = serial.Serial(port, 9600, timeout=0)
        else:
            self.coins = {'A': (-0.073, 0.08), 'B': (0.035, 0.14),
                          'C': (0.044, 0.08), 'D': (-0.035, 0.12)}
            self.held_coin = None
            self.tf_buffer = Buffer()
            self.tf_listener = TransformListener(self.tf_buffer, self)

        self.detected_pub = self.create_publisher(Bool, 'coin_detected', 10)
        self.create_service(SetBool, 'magnet', self.magnet_callback)
        self.create_timer(0.1, self.publish_detection)

    def magnet_position(self):
        tf = self.tf_buffer.lookup_transform('world', 'end_link', Time())
        return tf.transform.translation.x, tf.transform.translation.y

    def coin_under_magnet(self):
        x, y = self.magnet_position()
        for name, (coin_x, coin_y) in self.coins.items():
            if math.hypot(x - coin_x, y - coin_y) < 0.01:
                return name
        return None

    def magnet_callback(self, request, response):
        if self.real:
            self.board.write(bytes([int(request.data)]))
            response.success = self.detected
            response.message = f'Aimant : {request.data}'
        elif request.data:
            self.held_coin = self.coin_under_magnet()
            response.success = self.held_coin is not None
            if self.held_coin:
                del self.coins[self.held_coin]
                response.message = f'Pièce {self.held_coin} attrapée'
            else:
                response.message = 'Aucune pièce sous l\'aimant'
        elif self.held_coin:
            self.coins[self.held_coin] = self.magnet_position()
            response.success = True
            response.message = f'Pièce {self.held_coin} lâchée'
            self.held_coin = None
        else:
            response.message = 'Aimant coupé'
        return response

    def publish_detection(self):
        if self.real:
            data = self.board.read(64)
            if data:
                self.detected = data[-1] == 1
        else:
            try:
                self.detected = self.coin_under_magnet() is not None
            except TransformException:
                self.detected = False
        self.detected_pub.publish(Bool(data=self.detected))


def main():
    rclpy.init()
    try:
        rclpy.spin(MagnetNode())
    except KeyboardInterrupt:
        pass


if __name__ == '__main__':
    main()
