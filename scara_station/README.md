# scara_station

Poste de chargement du mini-entrepôt. Le sujet est dans le TP6 du wiki.

```bash
ros2 launch scara_station scara.launch.py                       # bras simulé
ros2 launch scara_station scara.launch.py backend:=moveit       # bras piloté par MoveIt
ros2 launch scara_station scara.launch.py hardware_type:=real   # bras réel
ros2 launch scara_station moveit_rviz.launch.py                 # RViz, avec backend:=moveit
```

| Service | Type | Effet |
|---|---|---|
| `/station/load_coin` | `std_srvs/Trigger` | Pose la pièce suivante dans le godet |

Tout le reste, y compris MoveIt, est dans le namespace `/scara`.
