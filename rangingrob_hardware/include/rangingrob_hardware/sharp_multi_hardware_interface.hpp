#pragma once

#include <string>
#include <vector>

#include "hardware_interface/sensor_interface.hpp"
#include "hardware_interface/handle.hpp"
#include "hardware_interface/hardware_info.hpp"
#include "hardware_interface/types/hardware_interface_return_values.hpp"
#include "rclcpp/macros.hpp"
#include "rclcpp_lifecycle/state.hpp"

namespace rangingrob_hardware
{

// Interface hardware serie pour N capteurs Sharp sur un seul port serie.
//
// Protocole ESP32 attendu : N valeurs en metres separees par des virgules, une ligne par cycle.
//   Ex. (3 capteurs) : "0.080,0.105,0.062\n"
//
// L'ordre des valeurs dans la ligne CSV doit correspondre a l'ordre des <sensor> dans l'URDF.
// Tout nombre de capteurs est supporte ; la taille du vecteur est determinee a on_init()
// d'apres info_.sensors.size().

class SharpMultiHardwareInterface : public hardware_interface::SensorInterface
{
public:
  RCLCPP_SHARED_PTR_DEFINITIONS(SharpMultiHardwareInterface)

  hardware_interface::CallbackReturn on_init(
    const hardware_interface::HardwareInfo & info) override;

  hardware_interface::CallbackReturn on_activate(
    const rclcpp_lifecycle::State & previous_state) override;

  hardware_interface::CallbackReturn on_deactivate(
    const rclcpp_lifecycle::State & previous_state) override;

  std::vector<hardware_interface::StateInterface> export_state_interfaces() override;

  hardware_interface::return_type read(
    const rclcpp::Time & time, const rclcpp::Duration & period) override;

private:
  bool open_serial();
  void close_serial();

  std::string serial_port_;
  int baud_rate_{115200};
  int serial_fd_{-1};
  std::string read_buffer_;

  // Un element par capteur (meme ordre que info_.sensors / CSV)
  std::vector<double> range_values_;
};

}  // namespace rangingrob_hardware
