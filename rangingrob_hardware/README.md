# rangingrob_hardware

Plugins ros2_control pour les capteurs Sharp GP2Y0A51SK0F.

| Plugin | Type | Utilisation |
|---|---|---|
| `SharpMultiHardwareInterface` | `SensorInterface` | simplebot reel (TP4) |
| `GazeboSharpSimSystem` | `GazeboSimSystemInterface` | simplebot dans Gazebo (TP3) |
| `SharpHardwareInterface` | `SensorInterface` | Demo a un capteur |
| `GazeboSharpHardwareInterface` | `SensorInterface` | Demo a un capteur, `controller_manager` separe |

## `rangingrob_hardware/SharpMultiHardwareInterface`

Lit N capteurs sur un seul port serie. Protocole : `0.080,0.105,0.062\n` (metres, 50 Hz).
L'ordre des valeurs doit correspondre a l'ordre des `<sensor>` dans l'URDF.

| Parametre URDF | Defaut | Description |
|---|---|---|
| `serial_port` | obligatoire | Port serie de l'ESP32 |
| `baud_rate` | obligatoire | 9600, 57600 ou 115200 |
| `timeout_ms` | 1000 | Delai maximal entre deux trames valides |

Si aucune trame valide n'arrive dans ce delai, `read()` renvoie `ERROR` : ros2_control desactive
le composant et ses controleurs, au lieu de publier une valeur figee. Le delai est de 5 s avant
la premiere trame, car l'ESP32 redemarre a l'ouverture du port.

Prerequis : `sudo usermod -aG dialout $USER` puis reconnexion.

## `rangingrob_hardware/GazeboSharpSimSystem`

Lit un `sensor_msgs/LaserScan` a un rayon publie par `ros_gz_bridge` depuis un `gpu_lidar`.

| Parametre URDF | Description |
|---|---|
| `lidar_topic` | Topic LaserScan source |

`gpu_lidar` renvoie `+inf` aussi bien sans obstacle que sous `range_min`. Le plugin considere
donc qu'un obstacle vu a moins de 5 cm reste "trop pres" tant qu'aucune mesure ne le contredit.

Tous ces plugins exportent la meme interface d'etat `<sensor>/range`, en metres.
