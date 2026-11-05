#include "rangingrob_hardware/gazebo_rangingrob_hardware_interface.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>

#include "pluginlib/class_list_macros.hpp"
#include "rclcpp/rclcpp.hpp"

namespace rangingrob_hardware
{

static const rclcpp::Logger LOGGER = rclcpp::get_logger("GazeboSharpHardwareInterface");

hardware_interface::CallbackReturn GazeboSharpHardwareInterface::on_init(
  const hardware_interface::HardwareComponentInterfaceParams & params)
{
  if (hardware_interface::SensorInterface::on_init(params) !=
    hardware_interface::CallbackReturn::SUCCESS)
  {
    return hardware_interface::CallbackReturn::ERROR;
  }

  lidar_topic_ = info_.hardware_parameters.at("lidar_topic");

  // Le node interne permet de souscrire au topic ROS sans dependre
  // du mecanisme de lifecycle du controller_manager.
  node_ = rclcpp::Node::make_shared("gazebo_rangingrob_hardware_interface");
  executor_.add_node(node_);

  return hardware_interface::CallbackReturn::SUCCESS;
}

hardware_interface::CallbackReturn GazeboSharpHardwareInterface::on_activate(
  const rclcpp_lifecycle::State &)
{
  // SensorDataQoS : BEST_EFFORT, historique de 1, adapte aux capteurs
  scan_sub_ = node_->create_subscription<sensor_msgs::msg::LaserScan>(
    lidar_topic_,
    rclcpp::SensorDataQoS(),
    [this](const sensor_msgs::msg::LaserScan::SharedPtr msg) {
      if (msg->ranges.empty()) {
        return;
      }
      const float r = msg->ranges[0];
      // REP 117 : -inf = trop pres, +inf = rien detecte, NaN = mesure invalide.
      if (std::isnan(r)) {
        return;
      }
      range_value_ = static_cast<double>(std::clamp(r, msg->range_min, msg->range_max));
    }
  );

  RCLCPP_INFO(LOGGER, "Souscription au topic lidar : %s", lidar_topic_.c_str());
  return hardware_interface::CallbackReturn::SUCCESS;
}

hardware_interface::CallbackReturn GazeboSharpHardwareInterface::on_deactivate(
  const rclcpp_lifecycle::State &)
{
  scan_sub_.reset();
  return hardware_interface::CallbackReturn::SUCCESS;
}

std::vector<hardware_interface::StateInterface>
GazeboSharpHardwareInterface::export_state_interfaces()
{
  std::vector<hardware_interface::StateInterface> state_interfaces;
  state_interfaces.emplace_back(info_.sensors[0].name, "range", &range_value_);
  return state_interfaces;
}

hardware_interface::return_type GazeboSharpHardwareInterface::read(
  const rclcpp::Time &, const rclcpp::Duration &)
{
  // Traite les callbacks disponibles sans bloquer la boucle de controle
  executor_.spin_some(std::chrono::milliseconds(0));
  return hardware_interface::return_type::OK;
}

}  // namespace rangingrob_hardware

PLUGINLIB_EXPORT_CLASS(
  rangingrob_hardware::GazeboSharpHardwareInterface,
  hardware_interface::SensorInterface)
