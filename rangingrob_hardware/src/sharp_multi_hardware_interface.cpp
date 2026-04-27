#include "rangingrob_hardware/sharp_multi_hardware_interface.hpp"

#include <fcntl.h>
#include <termios.h>
#include <unistd.h>

#include <sstream>
#include <stdexcept>

#include "pluginlib/class_list_macros.hpp"
#include "rclcpp/rclcpp.hpp"

namespace rangingrob_hardware
{

static const rclcpp::Logger LOGGER = rclcpp::get_logger("SharpMultiHardwareInterface");

hardware_interface::CallbackReturn SharpMultiHardwareInterface::on_init(
  const hardware_interface::HardwareInfo & info)
{
  if (hardware_interface::SensorInterface::on_init(info) !=
    hardware_interface::CallbackReturn::SUCCESS)
  {
    return hardware_interface::CallbackReturn::ERROR;
  }

  serial_port_ = info_.hardware_parameters.at("serial_port");
  baud_rate_   = std::stoi(info_.hardware_parameters.at("baud_rate"));

  // Allouer un slot par capteur declare dans l'URDF, initialise a max_range (pas de detection)
  range_values_.assign(info_.sensors.size(), 0.15);

  RCLCPP_INFO(LOGGER, "Initialise avec %zu capteur(s) sur %s @ %d bauds",
    info_.sensors.size(), serial_port_.c_str(), baud_rate_);

  return hardware_interface::CallbackReturn::SUCCESS;
}

hardware_interface::CallbackReturn SharpMultiHardwareInterface::on_activate(
  const rclcpp_lifecycle::State &)
{
  if (!open_serial()) {
    RCLCPP_ERROR(LOGGER, "Impossible d'ouvrir le port serie : %s", serial_port_.c_str());
    return hardware_interface::CallbackReturn::ERROR;
  }
  read_buffer_.clear();
  RCLCPP_INFO(LOGGER, "Port serie ouvert : %s @ %d bauds", serial_port_.c_str(), baud_rate_);
  return hardware_interface::CallbackReturn::SUCCESS;
}

hardware_interface::CallbackReturn SharpMultiHardwareInterface::on_deactivate(
  const rclcpp_lifecycle::State &)
{
  close_serial();
  return hardware_interface::CallbackReturn::SUCCESS;
}

std::vector<hardware_interface::StateInterface>
SharpMultiHardwareInterface::export_state_interfaces()
{
  std::vector<hardware_interface::StateInterface> state_interfaces;
  for (size_t i = 0; i < info_.sensors.size(); ++i) {
    state_interfaces.emplace_back(info_.sensors[i].name, "range", &range_values_[i]);
  }
  return state_interfaces;
}

hardware_interface::return_type SharpMultiHardwareInterface::read(
  const rclcpp::Time &, const rclcpp::Duration &)
{
  char buf[128];
  ssize_t n = ::read(serial_fd_, buf, sizeof(buf));
  if (n > 0) {
    read_buffer_.append(buf, static_cast<size_t>(n));
  }

  size_t pos;
  while ((pos = read_buffer_.find('\n')) != std::string::npos) {
    std::string line = read_buffer_.substr(0, pos);
    read_buffer_.erase(0, pos + 1);

    if (!line.empty() && line.back() == '\r') {
      line.pop_back();
    }
    if (line.empty()) {
      continue;
    }

    // Parser la ligne CSV : "0.080,0.105,0.062"
    std::stringstream ss(line);
    std::string token;
    size_t idx = 0;
    bool parse_error = false;

    while (std::getline(ss, token, ',') && idx < range_values_.size()) {
      try {
        double value = std::stod(token);
        if (value >= 0.01 && value <= 0.20) {
          range_values_[idx] = value;
        }
      } catch (const std::exception &) {
        parse_error = true;
      }
      ++idx;
    }

    if (parse_error || idx != range_values_.size()) {
      RCLCPP_WARN_THROTTLE(LOGGER, *rclcpp::Clock::make_shared(), 1000,
        "Trame CSV inattendue (attendu %zu valeurs) : '%s'",
        range_values_.size(), line.c_str());
    }
  }

  if (read_buffer_.size() > 512) {
    read_buffer_.clear();
  }

  return hardware_interface::return_type::OK;
}

bool SharpMultiHardwareInterface::open_serial()
{
  serial_fd_ = ::open(serial_port_.c_str(), O_RDWR | O_NOCTTY | O_NONBLOCK);
  if (serial_fd_ < 0) {
    return false;
  }

  struct termios tty{};
  if (tcgetattr(serial_fd_, &tty) != 0) {
    close_serial();
    return false;
  }

  speed_t speed = B115200;
  if (baud_rate_ == 9600)   { speed = B9600;   }
  else if (baud_rate_ == 57600)  { speed = B57600;  }
  else if (baud_rate_ == 115200) { speed = B115200; }

  cfsetispeed(&tty, speed);
  cfsetospeed(&tty, speed);
  cfmakeraw(&tty);
  tty.c_cc[VMIN]  = 0;
  tty.c_cc[VTIME] = 0;

  if (tcsetattr(serial_fd_, TCSANOW, &tty) != 0) {
    close_serial();
    return false;
  }

  tcflush(serial_fd_, TCIOFLUSH);
  return true;
}

void SharpMultiHardwareInterface::close_serial()
{
  if (serial_fd_ >= 0) {
    ::close(serial_fd_);
    serial_fd_ = -1;
  }
}

}  // namespace rangingrob_hardware

PLUGINLIB_EXPORT_CLASS(
  rangingrob_hardware::SharpMultiHardwareInterface,
  hardware_interface::SensorInterface)
