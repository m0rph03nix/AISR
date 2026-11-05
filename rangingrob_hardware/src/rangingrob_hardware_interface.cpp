#include "rangingrob_hardware/rangingrob_hardware_interface.hpp"

#include <fcntl.h>
#include <termios.h>
#include <unistd.h>

#include <stdexcept>

#include "pluginlib/class_list_macros.hpp"
#include "rclcpp/rclcpp.hpp"

namespace rangingrob_hardware
{

static const rclcpp::Logger LOGGER = rclcpp::get_logger("SharpHardwareInterface");

hardware_interface::CallbackReturn SharpHardwareInterface::on_init(
  const hardware_interface::HardwareComponentInterfaceParams & params)
{
  if (hardware_interface::SensorInterface::on_init(params) !=
    hardware_interface::CallbackReturn::SUCCESS)
  {
    return hardware_interface::CallbackReturn::ERROR;
  }

  serial_port_ = info_.hardware_parameters.at("serial_port");
  baud_rate_ = std::stoi(info_.hardware_parameters.at("baud_rate"));

  return hardware_interface::CallbackReturn::SUCCESS;
}

hardware_interface::CallbackReturn SharpHardwareInterface::on_activate(
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

hardware_interface::CallbackReturn SharpHardwareInterface::on_deactivate(
  const rclcpp_lifecycle::State &)
{
  close_serial();
  return hardware_interface::CallbackReturn::SUCCESS;
}

std::vector<hardware_interface::StateInterface> SharpHardwareInterface::export_state_interfaces()
{
  std::vector<hardware_interface::StateInterface> state_interfaces;
  // info_.sensors[0].name == "sharp_sensor" (depuis l'URDF)
  // "range" correspond a <state_interface name="range"/> et au RangeSensorBroadcaster
  state_interfaces.emplace_back(info_.sensors[0].name, "range", &range_value_);
  return state_interfaces;
}

hardware_interface::return_type SharpHardwareInterface::read(
  const rclcpp::Time &, const rclcpp::Duration &)
{
  char buf[64];
  ssize_t n = ::read(serial_fd_, buf, sizeof(buf));
  if (n > 0) {
    read_buffer_.append(buf, static_cast<size_t>(n));
  }

  // Traiter toutes les lignes completes disponibles dans le buffer
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

    try {
      double value = std::stod(line);
      // Filtre de plausibilite : Sharp GP2Y0A51SK0F couvre 0.02 a 0.15 m
      if (value >= 0.01 && value <= 0.20) {
        range_value_ = value;
      }
    } catch (const std::exception &) {
      RCLCPP_WARN_THROTTLE(get_logger(), *get_clock(), 1000,
        "Trame serie illisible : '%s'", line.c_str());
    }
  }

  // Eviter l'accumulation si le microcontroleur n'envoie pas de terminateur
  if (read_buffer_.size() > 256) {
    read_buffer_.clear();
  }

  return hardware_interface::return_type::OK;
}

bool SharpHardwareInterface::open_serial()
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

  speed_t speed;
  if (baud_rate_ == 9600) {
    speed = B9600;
  } else if (baud_rate_ == 57600) {
    speed = B57600;
  } else if (baud_rate_ == 115200) {
    speed = B115200;
  } else {
    RCLCPP_ERROR(LOGGER, "baud_rate non supporte : %d", baud_rate_);
    close_serial();
    return false;
  }

  cfsetispeed(&tty, speed);
  cfsetospeed(&tty, speed);

  // Mode raw : pas d'echo, pas de traitement de signal, pas de conversion de caracteres
  cfmakeraw(&tty);

  // Lecture non bloquante : retourne immediatement meme si 0 octet est disponible
  tty.c_cc[VMIN] = 0;
  tty.c_cc[VTIME] = 0;

  if (tcsetattr(serial_fd_, TCSANOW, &tty) != 0) {
    close_serial();
    return false;
  }

  tcflush(serial_fd_, TCIOFLUSH);
  return true;
}

void SharpHardwareInterface::close_serial()
{
  if (serial_fd_ >= 0) {
    ::close(serial_fd_);
    serial_fd_ = -1;
  }
}

}  // namespace rangingrob_hardware

PLUGINLIB_EXPORT_CLASS(
  rangingrob_hardware::SharpHardwareInterface,
  hardware_interface::SensorInterface)
