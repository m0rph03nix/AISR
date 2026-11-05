#!/usr/bin/env python3
"""Genere la carte Nav2 et le monde Gazebo du mini-entrepot a partir d'une seule description.

Usage : python3 scripts/gen_warehouse.py   (depuis la racine du paquet)
"""
from pathlib import Path

PITCH = 0.38    # pas de la grille : couloirs de 36 cm entre cloisons de 2 cm
WALL = 0.02     # epaisseur des cloisons
HEIGHT = 0.12   # hauteur des murs
RES = 0.01      # 1 cm par pixel
NX, NY = 4, 3

# Passages ouverts entre cases. Tout le reste est mure.
OPEN = {
    ((0, 0), (1, 0)), ((1, 0), (2, 0)), ((2, 0), (3, 0)), ((3, 0), (3, 1)),
    ((3, 1), (3, 2)), ((2, 2), (3, 2)), ((1, 2), (2, 2)), ((0, 2), (1, 2)),
    ((0, 1), (0, 2)), ((0, 0), (0, 1)),
    ((2, 0), (2, 1)),  # alcove du poste SCARA
}
SOLID_CELLS = [(1, 1)]  # case fermee : marquee occupee sur la carte

X0, Y0 = -PITCH / 2, -PITCH / 2  # le centre de la case (0, 0) est l'origine du repere map
X1, Y1 = X0 + NX * PITCH, Y0 + NY * PITCH


def build_walls():
    walls = [
        (X0 - WALL, Y0 - WALL, X1 + WALL, Y0), (X0 - WALL, Y1, X1 + WALL, Y1 + WALL),
        (X0 - WALL, Y0, X0, Y1), (X1, Y0, X1 + WALL, Y1),
    ]
    for cx in range(NX):
        for cy in range(NY):
            if cx < NX - 1 and ((cx, cy), (cx + 1, cy)) not in OPEN:
                x = X0 + (cx + 1) * PITCH
                walls.append((x - WALL / 2, Y0 + cy * PITCH - WALL / 2,
                              x + WALL / 2, Y0 + (cy + 1) * PITCH + WALL / 2))
            if cy < NY - 1 and ((cx, cy), (cx, cy + 1)) not in OPEN:
                y = Y0 + (cy + 1) * PITCH
                walls.append((X0 + cx * PITCH - WALL / 2, y - WALL / 2,
                              X0 + (cx + 1) * PITCH + WALL / 2, y + WALL / 2))
    return walls


def write_map(walls, folder):
    ox, oy = X0 - WALL - 0.02, Y0 - WALL - 0.02
    w = int(round((X1 + WALL + 0.02 - ox) / RES))
    h = int(round((Y1 + WALL + 0.02 - oy) / RES))
    solid = [(X0 + cx * PITCH, Y0 + cy * PITCH, X0 + (cx + 1) * PITCH, Y0 + (cy + 1) * PITCH)
             for cx, cy in SOLID_CELLS]
    pixels = bytearray()
    for j in range(h):
        y = oy + (h - 1 - j + 0.5) * RES
        for i in range(w):
            x = ox + (i + 0.5) * RES
            occupied = any(a <= x <= c and b <= y <= d for a, b, c, d in walls + solid)
            pixels.append(0 if occupied else 254)
    (folder / 'warehouse.pgm').write_bytes(f'P5\n{w} {h}\n255\n'.encode() + bytes(pixels))
    (folder / 'warehouse.yaml').write_text(
        f'image: warehouse.pgm\nresolution: {RES}\norigin: [{ox:.3f}, {oy:.3f}, 0.0]\n'
        'negate: 0\noccupied_thresh: 0.65\nfree_thresh: 0.25\n')


def write_world(walls, path):
    models = ''
    for k, (a, b, c, d) in enumerate(walls):
        geometry = (f'<geometry><box><size>{c - a:.3f} {d - b:.3f} {HEIGHT}</size></box>'
                    '</geometry>')
        models += (f'    <model name="wall_{k}"><static>true</static>'
                   f'<pose>{(a + c) / 2:.3f} {(b + d) / 2:.3f} {HEIGHT / 2} 0 0 0</pose>'
                   f'<link name="link"><collision name="collision">{geometry}</collision>'
                   f'<visual name="visual">{geometry}</visual></link></model>\n')
    path.write_text(f'''<?xml version="1.0"?>
<sdf version="1.9">
  <world name="warehouse">
    <physics name="1ms" type="ignored">
      <max_step_size>0.001</max_step_size>
      <real_time_factor>1.0</real_time_factor>
    </physics>
    <plugin filename="gz-sim-sensors-system" name="gz::sim::systems::Sensors">
      <render_engine>ogre2</render_engine>
    </plugin>
    <plugin filename="gz-sim-physics-system" name="gz::sim::systems::Physics"/>
    <plugin filename="gz-sim-scene-broadcaster-system" name="gz::sim::systems::SceneBroadcaster"/>
    <plugin filename="gz-sim-user-commands-system" name="gz::sim::systems::UserCommands"/>
    <light type="directional" name="sun">
      <pose>0 0 10 0 0 0</pose>
      <diffuse>0.8 0.8 0.8 1</diffuse>
      <direction>-0.5 0.1 -0.9</direction>
    </light>
    <model name="ground">
      <static>true</static>
      <link name="link">
        <collision name="collision">
          <geometry><plane><normal>0 0 1</normal><size>20 20</size></plane></geometry>
        </collision>
        <visual name="visual">
          <geometry><plane><normal>0 0 1</normal><size>20 20</size></plane></geometry>
        </visual>
      </link>
    </model>
{models}  </world>
</sdf>
''')


def main():
    root = Path(__file__).resolve().parent.parent
    walls = build_walls()
    write_map(walls, root / 'maps')
    write_world(walls, root / 'worlds' / 'warehouse.sdf')
    print(f'{len(walls)} murs, plateau {X1 - X0 + 2 * WALL:.2f} x {Y1 - Y0 + 2 * WALL:.2f} m')


if __name__ == '__main__':
    main()
