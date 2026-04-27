# rangingrob_hardware

Plugins `hardware_interface::SensorInterface` pour ros2_control.

## Plugins fournis

### `rangingrob_hardware/SharpHardwareInterface`

Lit la distance depuis un ESP32 TTGO T-Display via port serie.

**Protocole serie :** `0.350\n` (float ASCII en metres, 50 Hz, 115200 bauds)

**Parametres URDF :**
| Parametre    | Valeur par defaut | Description              |
|--------------|-------------------|--------------------------|
| serial_port  | /dev/ttyUSB0      | Port serie de l'ESP32    |
| baud_rate    | 115200            | Vitesse de communication |

**Prerequis :** `sudo usermod -aG dialout $USER` puis reconnexion.

### `rangingrob_hardware/GazeboSharpHardwareInterface`

Lit la distance depuis un topic `sensor_msgs/msg/LaserScan` (1 rayon)
publie par `ros_gz_bridge` depuis un capteur `gpu_lidar` Gazebo Harmonic.

**Parametre URDF :**
| Parametre   | Valeur par defaut | Description              |
|-------------|-------------------|--------------------------|
| lidar_topic | /sharp_lidar      | Topic LaserScan source   |

## Architecture

Les deux plugins exportent la meme interface d'etat `sharp_sensor/range`
(float en metres). Le `RangeSensorBroadcaster` et les autres consumers
ne voient pas la difference entre les deux sources.
