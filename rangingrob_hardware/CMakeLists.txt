cmake_minimum_required(VERSION 3.8)
project(rangingrob_hardware)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(ament_cmake REQUIRED)
find_package(hardware_interface REQUIRED)

# gz-sim8 est vendorise dans ros-jazzy-gz-sim-vendor, pas dans le prefix standard.
# Il faut l'ajouter au CMAKE_PREFIX_PATH avant de chercher gz_ros2_control
# qui en depend transitivement.
list(APPEND CMAKE_PREFIX_PATH "/opt/ros/jazzy/opt/gz_sim_vendor")
find_package(gz-sim8 REQUIRED)
find_package(gz_ros2_control REQUIRED)
find_package(pluginlib REQUIRED)
find_package(rclcpp REQUIRED)
find_package(rclcpp_lifecycle REQUIRED)
find_package(sensor_msgs REQUIRED)

# Plugin 1 : interface hardware serie pour l'ESP32 reel
add_library(rangingrob_hardware_interface SHARED
  src/rangingrob_hardware_interface.cpp
)
target_include_directories(rangingrob_hardware_interface PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)
ament_target_dependencies(rangingrob_hardware_interface
  hardware_interface
  pluginlib
  rclcpp
  rclcpp_lifecycle
)

# Plugin 2 : interface hardware pour Gazebo via hardware_interface::SensorInterface
# (utilise par rangingrob_bringup avec CM externe, NON compatible gz_ros2_control)
add_library(gazebo_rangingrob_hardware_interface SHARED
  src/gazebo_rangingrob_hardware_interface.cpp
)
target_include_directories(gazebo_rangingrob_hardware_interface PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)
ament_target_dependencies(gazebo_rangingrob_hardware_interface
  hardware_interface
  pluginlib
  rclcpp
  rclcpp_lifecycle
  sensor_msgs
)

# Plugin 3 : interface hardware serie pour N capteurs Sharp sur un seul port (protocole CSV)
add_library(sharp_multi_hardware_interface SHARED
  src/sharp_multi_hardware_interface.cpp
)
target_include_directories(sharp_multi_hardware_interface PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)
ament_target_dependencies(sharp_multi_hardware_interface
  hardware_interface
  pluginlib
  rclcpp
  rclcpp_lifecycle
)

# Plugin 4 : interface gz_ros2_control pour capteur Sharp
# (compatible gz_ros2_control, utilise par simplebot en mode Gazebo)
add_library(gz_sharp_sim_system SHARED
  src/gz_sharp_sim_system.cpp
)
target_include_directories(gz_sharp_sim_system PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)
ament_target_dependencies(gz_sharp_sim_system
  hardware_interface
  gz_ros2_control
  pluginlib
  rclcpp
  sensor_msgs
)

# plugins.xml pour hardware_interface (plugins 1, 2, 3)
pluginlib_export_plugin_description_file(hardware_interface plugins.xml)
# gz_plugins.xml pour gz_ros2_control (plugin 4)
pluginlib_export_plugin_description_file(gz_ros2_control gz_plugins.xml)

install(TARGETS
  rangingrob_hardware_interface
  gazebo_rangingrob_hardware_interface
  sharp_multi_hardware_interface
  gz_sharp_sim_system
  EXPORT export_${PROJECT_NAME}
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION bin
)
install(DIRECTORY include/
  DESTINATION include
)
install(FILES plugins.xml gz_plugins.xml
  DESTINATION share/${PROJECT_NAME}
)

ament_export_targets(export_${PROJECT_NAME} HAS_LIBRARY_TARGET)
ament_export_dependencies(
  hardware_interface gz_ros2_control pluginlib rclcpp rclcpp_lifecycle sensor_msgs
)

ament_package()
