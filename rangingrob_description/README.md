# rangingrob_description

Description URDF du robot avec capteur Sharp GP2Y0A51SK0F.

## Contenu

```
urdf/
  robot.urdf.xacro              -- robot de base avec le lien capteur
  ros2_control_mockup.urdf.xacro   -- hardware simule (mock_components)
  ros2_control_hardware.urdf.xacro -- hardware reel (ESP32 via serie)
  ros2_control_gazebo.urdf.xacro   -- simulation Gazebo Harmonic
```

## Argument xacro

Le type de hardware est selectionne par l'argument `hardware_type` :

```bash
xacro robot.urdf.xacro hardware_type:=mockup
xacro robot.urdf.xacro hardware_type:=hardware serial_port:=/dev/ttyUSB0
xacro robot.urdf.xacro hardware_type:=gazebo
```

## Principe du bloc ros2_control

Chaque variante declare le meme sensor `sharp_sensor` avec l'interface d'etat `range`,
mais avec un plugin hardware different. Le reste du systeme (controllers, RViz)
est identique dans les trois cas.
