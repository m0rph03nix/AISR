# simplebot_warehouse

Plateau du mini-entrepot utilise par les TP5 et TP6 : couloirs de 36 cm, murs de 12 cm minimum,
emprise de 1,56 x 1,18 m.

| Fichier | Role |
|---|---|
| `scripts/gen_warehouse.py` | Source unique du plateau : genere la carte et le monde |
| `maps/warehouse.yaml`, `maps/warehouse.pgm` | Carte Nav2, 1 cm par pixel |
| `worlds/warehouse.sdf` | Monde Gazebo |
| `config/nav2_params.yaml` | Parametres Nav2 de reference |
| `scripts/range_to_laserscan.py` | Convertit les `Range` des Sharp en `LaserScan` |
| `simplebot_warehouse/robot.py` | Fonctions de mission : `go_to`, `dock`, `leave_dock`, `load_coin` |

## Stations (repere `map`)

| Station | x (m) | y (m) |
|---|---|---|
| Base | 0.00 | 0.00 |
| Poste SCARA (alcove) | 0.76 | 0.38 |
| Zone de depot 1 | 1.14 | 0.76 |
| Zone de depot 2 | 0.38 | 0.76 |

Pour modifier le plateau, changez `gen_warehouse.py` puis relancez-le : la carte et le monde
restent ainsi coherents.
