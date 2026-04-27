#pragma once

#include <map>
#include <string>
#include <vector>

#include "gz_ros2_control/gz_system_interface.hpp"
#include "hardware_interface/types/hardware_interface_return_values.hpp"
#include "rclcpp/rclcpp.hpp"
#include "sensor_msgs/msg/laser_scan.hpp"

namespace rangingrob_hardware
{

/// Plugin gz_ros2_control pour un capteur Sharp GP2Y0A51SK0F.
/// Implémente GazeboSimSystemInterface (requis par le chargeur gz_ros2_control).
/// Souscrit a un topic sensor_msgs/LaserScan publie par ros_gz_bridge depuis
/// un gpu_lidar Gazebo et expose l'interface d'etat <sensor_name>/range.
class GazeboSharpSimSystem : public gz_ros2_control::GazeboSimSystemInterface
{
public:
  bool initSim(
    rclcpp::Node::SharedPtr & model_nh,
    std::map<std::string, gz::sim::Entity> & joints,
    const hardware_interface::HardwareInfo & hardware_info,
    gz::sim::EntityComponentManager & ecm,
    unsigned int update_rate) override;

  std::vector<hardware_interface::StateInterface> export_state_interfaces() override;
  std::vector<hardware_interface::CommandInterface> export_command_interfaces() override;

  hardware_interface::return_type read(
    const rclcpp::Time & time, const rclcpp::Duration & period) override;

  hardware_interface::return_type write(
    const rclcpp::Time & time, const rclcpp::Duration & period) override;

private:
  rclcpp::Subscription<sensor_msgs::msg::LaserScan>::SharedPtr scan_sub_;
  double range_value_{0.15};  // max_range au demarrage : pas de detection
};

}  // namespace rangingrob_hardware
