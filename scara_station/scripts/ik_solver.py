#!/usr/bin/env python3
"""Cinématique inverse du bras SCARA."""
import math

SHOULDER_Y = 0.048  # position de l'épaule
L1 = 0.08           # longueur du bras
L2 = 0.047          # longueur de l'avant-bras


def inverse_kinematics(x, y):
    """Renvoie les angles (q1, q2) qui placent l'aimant en (x, y)."""
    dy = y - SHOULDER_Y
    cos_q2 = (x * x + dy * dy - L1 * L1 - L2 * L2) / (2 * L1 * L2)
    if abs(cos_q2) > 1:
        raise ValueError(f'Point hors de portée : ({x}, {y})')

    # Deux solutions possibles, coude à gauche ou à droite : on garde celle qui respecte les butées
    for q2 in (math.acos(cos_q2), -math.acos(cos_q2)):
        q1 = math.atan2(-x, dy) + math.atan2(L2 * math.sin(q2), L1 + L2 * math.cos(q2))
        if abs(q1) <= math.pi / 2 and abs(q2) <= 2.8:
            return q1, q2

    raise ValueError(f'Point hors des butées : ({x}, {y})')
