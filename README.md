# AISR : architecture et integration de systemes robotiques

Paquets fournis pour les travaux pratiques simplebot.
Les sujets sont sur le [wiki](https://github.com/m0rph03nix/AISR/wiki).

| Dossier | Role | Utilise par simplebot |
|---|---|---|
| `rangingrob_description` | Mesh du capteur Sharp | Oui (mesh) |
| `rangingrob_hardware` | Plugins ros2_control des capteurs Sharp | Oui (`SharpMultiHardwareInterface`, `GazeboSharpSimSystem`) |
| `simplebot_warehouse` | Plateau du mini-entrepot, carte, monde et parametres Nav2 | Oui (TP5, TP6) |
| `scara_station` | Poste de chargement, vu comme un service | Oui (TP6) |
| `arduino` | Firmware ESP32 | Oui (`sharp_multi_sensor`) |
| `demo_1_capteur` | Exemple minimal a un seul capteur | Non |

## Docker

Toute la chaine tourne dans l'image
[cpe_tps_robotique.docker](https://gitlab.com/cpelyon/rob_ressources/cpe_tps_robotique.docker).

## Demo a un capteur

`demo_1_capteur/rangingrob_bringup` est un exemple independant des TP. Son mode Gazebo utilise
un `controller_manager` separe de Gazebo, alors que le TP3 utilise `gz_ros2_control`.

```bash
ros2 launch rangingrob_bringup mockup.launch.py
ros2 topic pub --once /forward_command_controller/commands \
  std_msgs/msg/Float64MultiArray "data: [0.08]"

ros2 launch rangingrob_bringup gazebo.launch.py
ros2 launch rangingrob_bringup hardware.launch.py serial_port:=/dev/ttyUSB0
```
