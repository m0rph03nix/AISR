cmake_minimum_required(VERSION 3.8)
project(scara_station)

find_package(ament_cmake REQUIRED)

install(DIRECTORY config launch urdf
  DESTINATION share/${PROJECT_NAME}
)

install(PROGRAMS
  scripts/ik_solver.py
  scripts/magnet_node.py
  scripts/scara_arm.py
  scripts/station_node.py
  DESTINATION lib/${PROJECT_NAME}
)

ament_package()
