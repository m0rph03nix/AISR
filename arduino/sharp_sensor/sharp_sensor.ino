// Cible : TTGO T-Display V1.1 (ESP32)
// Capteur : Sharp GP2Y0A51SK0F sur GPIO 36 (SVP / ADC1_CH0)
// Protocole serie : "0.080\n" (float ASCII en metres, 50 Hz, 115200 bauds)
//
// Alimentation du capteur : 3V3 (pin 3.3V de la TTGO) -> Sharp Vcc
// Sortie Sharp -> GPIO 36 directement (3.3V natif, aucun diviseur requis)
//
// Bibliotheque requise : TFT_eSPI (Bodmer)
//   Configuration dans User_Setup.h : selectionner "TTGO T-Display" ou
//   utiliser le fichier de configuration fourni dans le repertoire de la bibliotheque.

#include <TFT_eSPI.h>
#include <SPI.h>

static const int    SENSOR_PIN    = 36;  // SVP, ADC1_CH0, input-only
static const int    SEND_RATE_HZ  = 50;
static const unsigned long PERIOD_MS = 1000UL / SEND_RATE_HZ;

TFT_eSPI tft = TFT_eSPI();

unsigned long last_send_ms = 0;
float last_distance_m = 0.0f;

void setup()
{
  Serial.begin(115200);

  // Le retroeclairage (GPIO 4) doit etre active explicitement
  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, HIGH);

  tft.init();
  tft.setRotation(0);  // paysage sur TTGO T-Display V1.1 (essayer 2 si image inversee)
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(2);
  tft.setCursor(15, 5);
  tft.println("Sharp GP2Y0A51SK0F");
  tft.setCursor(15, 22);
  tft.println("ROS2 ros2_control");

  analogReadResolution(12);  // ADC 12 bits (0-4095)
}

void loop()
{
  unsigned long now = millis();
  if (now - last_send_ms < PERIOD_MS) {
    return;
  }
  last_send_ms = now;

  int raw = analogRead(SENSOR_PIN);
  float distance_m = raw_to_meters(raw);

  // Envoi serie pour ros2_control (protocole ASCII)
  Serial.println(distance_m, 3);

  // Mise a jour de l'affichage uniquement si la valeur change significativement
  if (fabsf(distance_m - last_distance_m) > 0.005f) {
    update_display(distance_m);
    last_distance_m = distance_m;
  }
}

// Conversion ADC -> metres pour Sharp GP2Y0A51SK0F alimente en 3.3V.
// Le capteur est alimente directement par la broche 3V3 de la TTGO.
// La tension de sortie est lue directement sur GPIO 36 (ADC pleine echelle = 3.3V).
//
// Formule ajustee depuis la courbe datasheet (Vcc = 3.3V, ADC 12 bits) :
//   d(cm) = 4.8 / Vout^1.22   (approximation puissance, valable 2-15 cm)
//   Vout = raw * 3.3 / 4095
float raw_to_meters(int raw)
{
  if (raw <= 10) {
    return 0.15f;  // ADC sature bas = obstacle trop loin ou hors plage
  }

  float vout = raw * 3.3f / 4095.0f;
  float distance_cm = 4.8f / powf(vout, 1.22f);

  if (distance_cm < 2.0f)  distance_cm = 2.0f;
  if (distance_cm > 15.0f) distance_cm = 15.0f;

  return distance_cm / 100.0f;
}

void update_display(float distance_m)
{
  const int margin  = 15;
  const int bar_max = tft.width() - 2 * margin;

  tft.fillRect(0, 50, tft.width(), 85, TFT_BLACK);

  float distance_cm = distance_m * 100.0f;

  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.setTextSize(4);
  tft.setCursor(margin, 60);
  tft.printf("%.1f cm", distance_cm);

  // Barre de progression proportionnelle a la plage 2-15 cm
  int bar_fill = (int)((distance_cm - 2.0f) / 13.0f * bar_max);
  if (bar_fill < 0)        bar_fill = 0;
  if (bar_fill > bar_max)  bar_fill = bar_max;

  tft.fillRect(margin, 110, bar_fill, 15, TFT_GREEN);
  tft.drawRect(margin, 110, bar_max, 15, TFT_WHITE);
}
