# rangingrob_bringup

Launch files, configuration controllers et RViz pour l'exemple ros2_control Sharp.

## Launch files

| Fichier             | Hardware     | Prerequis                              |
|---------------------|--------------|----------------------------------------|
| mockup.launch.py    | Virtuel      | Aucun                                  |
| hardware.launch.py  | ESP32 reel   | Arduino programme, port serie          |
| gazebo.launch.py    | Gazebo Harmonic | `gz sim` installe, ros_gz_bridge    |

## Demarrage rapide

```bash
# Mockup (test sans materiel)
ros2 launch rangingrob_bringup mockup.launch.py

# Hardware reel
ros2 launch rangingrob_bringup hardware.launch.py serial_port:=/dev/ttyUSB0

# Simulation Gazebo
ros2 launch rangingrob_bringup gazebo.launch.py
```

## Visualisation de la courbe

```bash
# rqt_plot (inclus dans ROS2)
# Note : le topic en argument ligne de commande est buggue sous Jazzy.
# Lancer sans argument et saisir le topic dans le champ de l'interface.
ros2 run rqt_plot rqt_plot
# Topic : /range_sensor_broadcaster/range/range

# PlotJuggler (a installer separement)
ros2 run plotjuggler plotjuggler
```

## Topic publie

`/range_sensor_broadcaster/range` -- `sensor_msgs/msg/Range`
