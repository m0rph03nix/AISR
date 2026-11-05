# arduino

Code embarque pour l'ESP32 TTGO T-Display V1.1.

## Sketches disponibles

| Dossier | Capteurs | Protocole | Hardware interface ROS |
|---|---|---|---|
| `sharp_sensor/` | 1 Sharp (GPIO 36) | `"0.080\n"` | `SharpHardwareInterface` |
| `sharp_multi_sensor/` | 3 Sharp (GPIO 36/39/34) | `"0.080,0.105,0.062\n"` | `SharpMultiHardwareInterface` |

## Materiel

- **Microcontroleur** : TTGO T-Display V1.1 (ESP32)
- **Capteur** : Sharp GP2Y0A51SK0F (infrarouge, 2-15 cm)

## Cablage

### 1 capteur (`sharp_sensor`)

```
Sharp GP2Y0A51SK0F      TTGO T-Display V1.1
---------------------   --------------------
Vcc  (pin 1)        --> 5V
GND  (pin 2)        --> GND
Vout (pin 3)        --> GPIO 36 (SVP / ADC1_CH0)
```

### 3 capteurs (`sharp_multi_sensor`)

```
Capteur   Vcc   GND   Vout -> GPIO    ADC
--------  ----  ----  ---------------  ----------
Gauche    5V    GND   GPIO 36 (SVP)    ADC1_CH0
Droit     5V    GND   GPIO 39 (VN)     ADC1_CH3
Centre    5V    GND   GPIO 34          ADC1_CH6
```

Le GP2Y0A51SK0F s'alimente entre 4,5 et 5,5 V : ne pas l'alimenter en 3,3 V.
Placer un condensateur d'au moins 10 uF entre Vcc et GND, au plus pres de chaque capteur.
La sortie typique va d'environ 0,4 V a 15 cm a environ 2 V a 2 cm. Verifier au multimetre
qu'elle reste sous 3,3 V avant de la relier a l'ESP32.

La loi distance/tension des sketchs (`CAL_K`, `CAL_P`) est une valeur de depart :
elle doit etre recalibree avec des mesures reelles en 5 V (voir le TP4).

Les GPIO 36, 39 et 34 sont en entree uniquement (input-only) et appartiennent
tous a ADC1 — pas de conflit avec le Wi-Fi.

## Configuration Arduino IDE

1. Ajouter le support ESP32 dans les URLs de gestionnaire :
   `https://espressif.github.io/arduino-esp32/package_esp32_index.json`
2. Installer la carte **"ESP32 Arduino > TTGO T1"**
3. Installer la bibliotheque **TFT_eSPI** (Bodmer)
4. Dans `Bibliotheques/TFT_eSPI/User_Setup_Select.h`, activer la ligne
   `#include <User_Setups/Setup25_TTGO_T_Display.h>`.
5. Selectionner le port serie et flasher le sketch voulu.

## Protocoles serie

### 1 capteur — `sharp_sensor`

Une valeur par ligne, 50 Hz, 115200 bauds :

```
0.080
0.082
0.079
```

### 3 capteurs — `sharp_multi_sensor`

Trois valeurs separees par virgules (CSV), meme cadence :

```
0.080,0.105,0.062
0.081,0.104,0.063
```

Ordre : gauche, droit, centre. Unite : metres (float ASCII, 3 decimales).
