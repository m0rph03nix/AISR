#include "rangingrob_hardware/gz_sharp_sim_system.hpp"

#include <algorithm>
#include <cmath>
#include <pluginlib/class_list_macros.hpp>

namespace rangingrob_hardware
{

static constexpr float NEAR_MARGIN_M = 0.03f;

bool GazeboSharpSimSystem::initSim(
  rclcpp::Node::SharedPtr & model_nh,
  std::map<std::string, gz::sim::Entity> &,
  const hardware_interface::HardwareInfo & hardware_info,
  gz::sim::EntityComponentManager &,
  unsigned int)
{
  // Recupere le topic depuis les parametres URDF
  auto it = hardware_info.hardware_parameters.find("lidar_topic");
  if (it == hardware_info.hardware_parameters.end()) {
    RCLCPP_ERROR(model_nh->get_logger(),
      "[GazeboSharpSimSystem] parametre 'lidar_topic' manquant dans le URDF (%s)",
      hardware_info.name.c_str());
    return false;
  }
  const std::string topic = it->second;

  // model_nh est gere par l'executeur de gz_ros2_control : pas besoin de spin_some.
  nh_ = model_nh;
  scan_sub_ = nh_->create_subscription<sensor_msgs::msg::LaserScan>(
    topic,
    rclcpp::SensorDataQoS(),
    [this](const sensor_msgs::msg::LaserScan::SharedPtr msg) {
      if (msg->ranges.empty()) {return;}
      const float r = msg->ranges[0];
      // REP 117 : -inf = trop pres, +inf = rien detecte, NaN = mesure invalide.
      if (std::isnan(r)) {
        return;
      }
      // gpu_lidar renvoie aussi +inf sous range_min. Un obstacle a moins de 5 cm ne pouvant pas
      // disparaitre en un cycle, +inf signifie alors "trop pres" et non "rien detecte".
      if (std::isinf(r) && r > 0.0f && range_value_ < msg->range_min + NEAR_MARGIN_M) {
        range_value_ = static_cast<double>(msg->range_min);
        return;
      }
      range_value_ = static_cast<double>(std::clamp(r, msg->range_min, msg->range_max));
    });

  RCLCPP_INFO(model_nh->get_logger(),
    "[GazeboSharpSimSystem] %s souscrit a %s",
    hardware_info.name.c_str(), topic.c_str());
  return true;
}

std::vector<hardware_interface::StateInterface>
GazeboSharpSimSystem::export_state_interfaces()
{
  std::vector<hardware_interface::StateInterface> interfaces;
  for (const auto & sensor : info_.sensors) {
    interfaces.emplace_back(sensor.name, "range", &range_value_);
  }
  return interfaces;
}

std::vector<hardware_interface::CommandInterface>
GazeboSharpSimSystem::export_command_interfaces()
{
  return {};  // capteur : pas d'interface de commande
}

hardware_interface::return_type GazeboSharpSimSystem::read(
  const rclcpp::Time &, const rclcpp::Duration &)
{
  // Les callbacks sont traites par l'executeur de gz_ros2_control.
  return hardware_interface::return_type::OK;
}

hardware_interface::return_type GazeboSharpSimSystem::write(
  const rclcpp::Time &, const rclcpp::Duration &)
{
  return hardware_interface::return_type::OK;
}

}  // namespace rangingrob_hardware

PLUGINLIB_EXPORT_CLASS(
  rangingrob_hardware::GazeboSharpSimSystem,
  gz_ros2_control::GazeboSimSystemInterface)
