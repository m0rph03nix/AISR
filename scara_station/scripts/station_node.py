#!/usr/bin/env python3
"""Poste de chargement. Pour le reste du système, il n'existe qu'un service :

/station/load_coin (std_srvs/Trigger) pose la pièce suivante dans le godet du robot.
"""
import rclpy
from ik_solver import inverse_kinematics
from rclpy.executors import SingleThreadedExecutor
from rclpy.node import Node
from scara_arm import ScaraArm
from std_srvs.srv import Trigger

POINTS = {
    'A': (-0.073, 0.08),
    'B': (0.035, 0.14),
    'C': (0.044, 0.08),
    'D': (-0.035, 0.12),
    'godet': (0.0, 0.15),
}


class Station(Node):

    def __init__(self):
        super().__init__('station')
        self.declare_parameter('backend', 'ik')
        self.backend = self.get_parameter('backend').value
        self.coins = ['A', 'B', 'C', 'D']
        self.arm = ScaraArm()
        self.create_service(Trigger, '/station/load_coin', self.load_coin)
        self.get_logger().info(f'Poste prêt, commande du bras : {self.backend}')

    def go_to(self, point):
        x, y = POINTS[point]
        if self.backend == 'moveit':
            self.arm.move_with_moveit(x, y)
        else:
            self.arm.move(*inverse_kinematics(x, y))

    def load_coin(self, request, response):
        if not self.coins:
            response.message = 'Plus de pièce au poste'
            return response

        coin = self.coins.pop(0)
        self.go_to(coin)
        response.success = self.arm.magnet(True)
        self.go_to('godet')
        self.arm.magnet(False)
        response.message = f'Pièce {coin} chargée' if response.success else f'Pièce {coin} ratée'
        return response


def main():
    rclpy.init()
    # Exécuteur à part : le service attend le bras, qui utilise l'exécuteur par défaut
    executor = SingleThreadedExecutor()
    executor.add_node(Station())
    try:
        executor.spin()
    except KeyboardInterrupt:
        pass


if __name__ == '__main__':
    main()
