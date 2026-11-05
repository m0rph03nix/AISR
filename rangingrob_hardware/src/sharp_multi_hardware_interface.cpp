#include "rangingrob_hardware/sharp_multi_hardware_interface.hpp"

#include <fcntl.h>
#include <termios.h>
#include <unistd.h>

#include <cstdlib>
#include <string>

#include "pluginlib/class_list_macros.hpp"
#include "rclcpp/rclcpp.hpp"

namespace rangingrob_hardware
{

static const rclcpp::Logger LOGGER = rclcpp::get_logger("SharpMultiHardwareInterface");
static constexpr double STARTUP_TIMEOUT_S = 5.0;  // l'ESP32 redemarre a l'ouverture du port

hardware_interface::CallbackReturn SharpMultiHardwareInterface::on_init(
  const hardware_interface::HardwareComponentInterfaceParams & params)
{
  if (hardware_interface::SensorInterface::on_init(params) !=
    hardware_interface::CallbackReturn::SUCCESS)
  {
    return hardware_interface::CallbackReturn::ERROR;
  }

  serial_port_ = info_.hardware_parameters.at("serial_port");
  baud_rate_   = std::stoi(info_.hardware_parameters.at("baud_rate"));

  // Allouer un slot par capteur declare dans l'URDF, initialise a max_range (pas de detection)
  range_values_.assign(info_.sensors.size(), 0.15);
  frame_values_.assign(info_.sensors.size(), 0.0);

  const auto timeout = info_.hardware_parameters.find("timeout_ms");
  if (timeout != info_.hardware_parameters.end()) {
    timeout_s_ = std::stod(timeout->second) / 1000.0;
  }
  read_buffer_.reserve(1024);

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
  frame_received_ = false;
  watchdog_armed_ = false;
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
  const rclcpp::Time & time, const rclcpp::Duration &)
{
  if (!watchdog_armed_) {
    last_frame_time_ = time;
    watchdog_armed_ = true;
  }

  char buf[128];
  const ssize_t n = ::read(serial_fd_, buf, sizeof(buf));
  if (n > 0) {
    read_buffer_.append(buf, static_cast<size_t>(n));
  }

  // Traite les lignes completes sur place, sans allocation dans la boucle temps reel.
  size_t start = 0;
  size_t pos;
  while ((pos = read_buffer_.find('\n', start)) != std::string::npos) {
    read_buffer_[pos] = '\0';
    char * line = read_buffer_.data() + start;
    start = pos + 1;
    if (*line == '\0' || *line == '\r') {
      continue;
    }
    if (parse_frame(line)) {
      frame_received_ = true;
      last_frame_time_ = time;
    } else {
      RCLCPP_WARN_THROTTLE(get_logger(), *get_clock(), 1000,
        "Trame CSV inattendue (attendu %zu valeurs) : '%s'", range_values_.size(), line);
    }
  }
  read_buffer_.erase(0, start);

  // Evite l'accumulation si le microcontroleur n'envoie pas de terminateur
  if (read_buffer_.size() > 512) {
    read_buffer_.clear();
  }

  const double limit = frame_received_ ? timeout_s_ : STARTUP_TIMEOUT_S;
  if ((time - last_frame_time_).seconds() > limit) {
    RCLCPP_ERROR(get_logger(), "Aucune trame valide depuis %.1f s sur %s", limit,
      serial_port_.c_str());
    return hardware_interface::return_type::ERROR;
  }

  return hardware_interface::return_type::OK;
}

bool SharpMultiHardwareInterface::parse_frame(char * line)
{
  char * cursor = line;
  for (size_t i = 0; i < frame_values_.size(); ++i) {
    char * end = nullptr;
    frame_values_[i] = std::strtod(cursor, &end);
    if (end == cursor) {
      return false;
    }
    const bool last = (i + 1 == frame_values_.size());
    if (last ? (*end != '\0' && *end != '\r') : (*end != ',')) {
      return false;
    }
    cursor = end + 1;
  }

  for (size_t i = 0; i < frame_values_.size(); ++i) {
    // Filtre de plausibilite : le GP2Y0A51SK0F couvre 0.02 a 0.15 m
    if (frame_values_[i] >= 0.01 && frame_values_[i] <= 0.20) {
      range_values_[i] = frame_values_[i];
    }
  }
  return true;
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
