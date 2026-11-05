// Cible : TTGO T-Display V1.1 (ESP32)
// Capteurs : 3x Sharp GP2Y0A51SK0F
//   GPIO 36 (SVP / ADC1_CH0) -> capteur gauche
//   GPIO 39 (VN  / ADC1_CH3) -> capteur droit
//   GPIO 34       (ADC1_CH6) -> capteur centre
//
// Alimentation : 5V de la TTGO -> Vcc de chaque capteur (plage Sharp : 4.5 a 5.5 V),
// avec un condensateur d'au moins 10 uF entre Vcc et GND pres de chaque capteur.
// Sortie Sharp -> GPIO directement : verifier au multimetre qu'elle reste sous 3.3 V.
//
// Protocole serie : "0.080,0.105,0.062\n"  (CSV, metres, 3 decimales, 50 Hz, 115200 bauds)
// Format attendu par SharpMultiHardwareInterface : N valeurs separees par virgules.
//
// Bibliotheque requise : TFT_eSPI (Bodmer)

#include <TFT_eSPI.h>
#include <SPI.h>

static const int   N_SENSORS     = 3;
static const int   SENSOR_PINS[N_SENSORS]  = {36, 39, 34};
static const char* SENSOR_LABELS[N_SENSORS] = {"G", "D", "C"};  // gauche, droit, centre

// Valeurs de depart a remplacer par votre calibration (TP4)
static const float CAL_K = 4.8f;
static const float CAL_P = 1.22f;

static const int          SEND_RATE_HZ = 50;
static const unsigned long PERIOD_MS   = 1000UL / SEND_RATE_HZ;

// Hauteur d'une ligne sur l'ecran (rotation paysage : 240x135 px)
static const int ROW_H      = 38;
static const int ROW_Y0     = 18;   // y du debut de la premiere ligne capteur
static const int MARGIN     = 8;
static const int BAR_HEIGHT = 8;
static const int LABEL_W    = 16;   // largeur reservee au label (1 char taille 2)

TFT_eSPI tft = TFT_eSPI();

unsigned long last_send_ms        = 0;
float distances_m[N_SENSORS]      = {0.15f, 0.15f, 0.15f};
float last_distances_m[N_SENSORS] = {-1.0f, -1.0f, -1.0f};  // force premier affichage

// ---------------------------------------------------------------------------

void setup()
{
  Serial.begin(115200);

  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, HIGH);

  tft.init();
  tft.setRotation(1);  // paysage sur TTGO T-Display V1.1 (240x135)
  tft.fillScreen(TFT_BLACK);

  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(1);
  tft.setCursor(MARGIN, 4);
  tft.print("Sharp GP2Y0A51SK0F x3  ROS2");

  analogReadResolution(12);  // ADC 12 bits (0-4095)
}

void loop()
{
  unsigned long now = millis();
  if (now - last_send_ms < PERIOD_MS) {
    return;
  }
  last_send_ms = now;

  for (int i = 0; i < N_SENSORS; i++) {
    distances_m[i] = mv_to_meters(analogReadMilliVolts(SENSOR_PINS[i]));
  }

  // Trame CSV : "0.080,0.105,0.062\n"
  for (int i = 0; i < N_SENSORS; i++) {
    if (i > 0) Serial.print(',');
    Serial.print(distances_m[i], 3);
  }
  Serial.print('\n');

  // Rafraichir uniquement les lignes qui changent (evite le scintillement)
  for (int i = 0; i < N_SENSORS; i++) {
    if (fabsf(distances_m[i] - last_distances_m[i]) > 0.004f) {
      draw_sensor_row(i, distances_m[i]);
      last_distances_m[i] = distances_m[i];
    }
  }
}

// ---------------------------------------------------------------------------

// Conversion tension -> metres pour Sharp GP2Y0A51SK0F alimente en 5 V.
// Modele : d(cm) = CAL_K / Vout^CAL_P. CAL_K et CAL_P sont a recalibrer sur vos capteurs (TP4).
float mv_to_meters(int millivolts)
{
  if (millivolts <= 10) {
    return 0.15f;  // pas de signal exploitable : hors plage
  }

  float d_cm = CAL_K / powf(millivolts / 1000.0f, CAL_P);

  if (d_cm < 2.0f)  d_cm = 2.0f;
  if (d_cm > 15.0f) d_cm = 15.0f;

  return d_cm / 100.0f;
}

// Affiche une ligne capteur : label, valeur cm, barre de progression.
// Les lignes sont disposees verticalement : row 0 = y=18, row 1 = y=56, row 2 = y=94.
void draw_sensor_row(int idx, float dist_m)
{
  int y      = ROW_Y0 + idx * ROW_H;
  int bar_w  = tft.width() - 2 * MARGIN - LABEL_W - 70;  // 70 px pour le texte valeur
  float d_cm = dist_m * 100.0f;

  // Effacement de la zone
  tft.fillRect(0, y, tft.width(), ROW_H - 2, TFT_BLACK);

  // Label (G / D / C)
  tft.setTextSize(2);
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.setCursor(MARGIN, y + 2);
  tft.print(SENSOR_LABELS[idx]);

  // Valeur numerique en cm
  uint16_t col = (d_cm < 5.0f) ? TFT_RED :
                 (d_cm < 10.0f) ? TFT_YELLOW : TFT_GREEN;
  tft.setTextColor(col, TFT_BLACK);
  tft.setCursor(MARGIN + LABEL_W + 4, y + 2);
  tft.printf("%5.1f cm", d_cm);

  // Barre proportionnelle a la plage 2-15 cm
  int x_bar  = MARGIN + LABEL_W + 4 + 70;
  int filled = (int)((d_cm - 2.0f) / 13.0f * bar_w);
  if (filled < 0)      filled = 0;
  if (filled > bar_w)  filled = bar_w;

  tft.fillRect(x_bar, y + 18, filled, BAR_HEIGHT, col);
  tft.drawRect(x_bar, y + 18, bar_w, BAR_HEIGHT, TFT_WHITE);
}
