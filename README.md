# Travaux pratiques 

[wiki](https://github.com/m0rph03nix/AISR/wiki)


# ------------ ranginrob -------------------

# Mock
ros2 launch rangingrob_bringup mockup.launch.py

ros2 topic pub --once /forward_command_controller/commands \
  std_msgs/msg/Float64MultiArray "data: [0.08]"

# Gazebo
ros2 launch rangingrob_bringup gazebo.launch.py

# Réel
ros2 launch rangingrob_bringup hardware.launch.py serial_port:=/dev/ttyUSB0