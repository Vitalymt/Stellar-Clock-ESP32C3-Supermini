// ============================================================
//   Stellar Clock v5 — ESP32-C3 SuperMini
//
//   WiFi: configTime + getLocalTime (как в Smart Cyber Clock)
//   Дисплей: SSD1306 128x32, портрет (поворот 1 = 32x128)
//   Датчик: BME280 (t + p + h) или BMP280 (t + p)
//   LED: пин 0, двойная вспышка каждые 4 сек (маяк)
//
//   Layout (32×128):
//   ┌────────┐
//   │ ////   │  ← шапка
//   │────────│
//   │  08    │  ← часы
//   │  ··    │  ← мигающие точки
//   │  24    │  ← минуты
//   │────────│
//   │ 21.03  │  ← дата
//   │────────│
//   │ 22°C   │  ← температура
//   │ 61%    │  ← влажность
//   │ 753m   │  ← давление мм рт.ст.
//   └────────┘
// ============================================================

#include <Arduino.h>
#include <WiFi.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_BME280.h>
#include "time.h"

// ============================================================
// ===== НАСТРОЙКИ ============================================
// ============================================================

const char* WIFI_SSID     = "Your_WiFi";
const char* WIFI_PASSWORD = "Your_Password";

const char* NTP_SERVER    = "pool.ntp.org";
const long  GMT_OFFSET    = 3L * 3600L;   // UTC+3 Москва
const int   DAYLIGHT      = 0;

// ============================================================
// ===== ПИНЫ =================================================
// ============================================================

#define I2C_SDA  8
#define I2C_SCL  9
#define LED_PIN  0

// ============================================================
// ===== ОБЪЕКТЫ ==============================================
// ============================================================

Adafruit_SSD1306 display(128, 32, &Wire, -1);
Adafruit_BME280  bme;

bool sensorOK  = false;
bool hasBME280 = false;

// ============================================================
// ===== СОСТОЯНИЕ ============================================
// ============================================================

bool  timeSynced   = false;
float cachedTempC  = NAN;
float cachedHum    = NAN;
float cachedMmHg   = NAN;

unsigned long lastSensorMs = 0;
unsigned long lastDraw     = 0;

// ============================================================
// ===== WiFi + NTP ===========================================
// ============================================================

void connectAndSync() {
  Serial.print("[WiFi] Connecting");

  display.clearDisplay();
  display.setCursor(2, 45);
  display.print("wifi..");
  display.display();

  WiFi.disconnect(true, true); // true, true = сброс сохранённых настроек
  delay(1000);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  uint8_t retry = 0;
  while (WiFi.status() != WL_CONNECTED && retry < 20) {
    delay(300);
    Serial.print(".");
    retry++;
  }

  if (WiFi.status() != WL_CONNECTED) {
    Serial.printf(" FAIL status=%d\n", WiFi.status());
    return;
  }

  Serial.printf(" OK IP=%s\n", WiFi.localIP().toString().c_str());

  configTime(GMT_OFFSET, DAYLIGHT, NTP_SERVER);

  display.clearDisplay();
  display.setCursor(2, 45);
  display.print("ntp...");
  display.display();

  struct tm timeinfo;
  for (int i = 0; i < 20; i++) {
    if (getLocalTime(&timeinfo)) {
      timeSynced = true;
      Serial.printf("[NTP] OK %02d:%02d:%02d\n",
        timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
      break;
    }
    delay(500);
  }

  if (!timeSynced) Serial.println("[NTP] FAIL");
}

// ============================================================
// ===== Датчик ===============================================
// ============================================================

void readSensor() {
  if (!sensorOK) return;
  if (millis() - lastSensorMs < 2000) return;
  lastSensorMs = millis();

  float t = bme.readTemperature();
  float p = bme.readPressure() / 100.0F;

  if (!isnan(t) && t > -40.0F && t < 85.0F)
    cachedTempC = t;

  if (!isnan(p) && p > 800.0F && p < 1200.0F)
    cachedMmHg = p * 0.75006F;

  if (hasBME280) {
    float h = bme.readHumidity();
    if (!isnan(h) && h >= 0.0F && h <= 100.0F)
      cachedHum = h;
  }
}

// ============================================================
// ===== LED маяк — ОТКЛЮЧЁН (пин 0 занят) ===================
// ============================================================

void updateBeacon() {
  // пин 0 отключён — светодиод не используется
}

// ============================================================
// ===== Дисплей ==============================================
// ============================================================

void drawDisplay() {
  struct tm ti;
  bool hasTime = timeSynced && getLocalTime(&ti);

  bool showColon = (millis() / 500) % 2 == 0;

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  // ── Шапка ──
  display.setTextSize(1);
  display.setCursor(1, 1);
  display.print("////");

  // ── Линия ── y=12
  display.drawLine(0, 12, 32, 12, SSD1306_WHITE);

  // ── ЧАСЫ ──
  display.setTextSize(2);
  display.setCursor(3, 16);
  if (hasTime) {
    char buf[3];
    sprintf(buf, "%02d", ti.tm_hour);
    display.print(buf);
  } else {
    display.print("--");
  }

  // ── Мигающие точки ──
  if (showColon) {
    display.fillRect(3,  36, 3, 3, SSD1306_WHITE);
    display.fillRect(11, 36, 3, 3, SSD1306_WHITE);
  }

  // ── МИНУТЫ ──
  display.setCursor(3, 44);
  if (hasTime) {
    char buf[3];
    sprintf(buf, "%02d", ti.tm_min);
    display.print(buf);
  } else {
    display.print("--");
  }

  // ── Линия ── y=64
  display.drawLine(0, 64, 32, 64, SSD1306_WHITE);

  // ── ДАТА ──
  display.setTextSize(1);
  display.setCursor(1, 68);
  if (hasTime) {
    char buf[6];
    sprintf(buf, "%02d.%02d", ti.tm_mday, ti.tm_mon + 1);
    display.print(buf);
  } else {
    display.print("--.-");
  }

  // ── Линия ── y=79
  display.drawLine(0, 79, 32, 79, SSD1306_WHITE);

  // ── Температура ──
  display.setCursor(3, 96);
  if (!isnan(cachedTempC)) {
    display.print((int)round(cachedTempC));
    display.print((char)247);
    display.print("C");
  } else {
    display.print("--C");
  }

  // ── Влажность ──
  if (hasBME280) {
    display.setCursor(3, 108);
    if (!isnan(cachedHum)) {
      display.print((int)round(cachedHum));
      display.print("%");
    } else {
      display.print("--%");
    }
  }

  // ── Давление ──
  display.setCursor(3, 120);
  if (!isnan(cachedMmHg)) {
    display.print((int)round(cachedMmHg));
    display.print("m");
  } else {
    display.print("---");
  }

  display.display();
}

// ============================================================
// ===== SETUP ================================================
// ============================================================

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("\n=== Stellar Clock v5 ===");

  // ── LED отключён (пин 0) ──

  Wire.begin(I2C_SDA, I2C_SCL);

  // ── OLED ──
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("OLED не найден!");
    while (true) delay(1000);
  }
  display.setRotation(1);
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(2, 55);
  display.print("boot..");
  display.display();

  // ── BME280 ──
  if      (bme.begin(0x76, &Wire)) { sensorOK = true; Serial.println("[BME] 0x76"); }
  else if (bme.begin(0x77, &Wire)) { sensorOK = true; Serial.println("[BME] 0x77"); }
  else                              { Serial.println("[BME] не найден"); }

  if (sensorOK) {
    uint8_t id = bme.sensorID();
    hasBME280  = (id == 0x60);
    Serial.printf("[BME] ID=0x%02X → %s\n", id,
      hasBME280 ? "BME280 (t+p+h)" : "BMP280 (t+p)");
    readSensor();
  }

  // ── WiFi + NTP ──
  display.clearDisplay();
  display.setCursor(2, 55);
  display.print("wifi..");
  display.display();

  connectAndSync();

  // ── Тест LED ──
  for (int i = 0; i < 3; i++) {
    digitalWrite(LED_PIN, HIGH); delay(200);
    digitalWrite(LED_PIN, LOW);  delay(200);
  }

  Serial.println("[BOOT] OK");
}

// ============================================================
// ===== LOOP =================================================
// ============================================================

void loop() {
  unsigned long now = millis();

  readSensor();
  updateBeacon();

  if (now - lastDraw >= 500) {
    lastDraw = now;
    drawDisplay();
  }
}
