/*
  Celsius — Stellar Clock prototype (v1)
  =======================================
  ESP32-C3 SuperMini  |  SSD1306 128x32 OLED (I2C)  |  SHT31
  NTP time sync  |  UTC+3 Moscow

  Wiring:
    SDA  → GPIO 8
    SCL  → GPIO 9
*/

#include <WiFi.h>
#include <time.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_SHT31.h>

// ── WiFi ─────────────────────────────────────────────────────────────────────
const char* WIFI_SSID     = "Your_WiFi";
const char* WIFI_PASSWORD = "Your_Password";

// ── NTP ──────────────────────────────────────────────────────────────────────
const char*  NTP_SERVER  = "pool.ntp.org";
const long   GMT_OFFSET  = 10800;   // UTC+3 (Moscow)
const int    DST_OFFSET  = 0;

// ── Hardware pins ────────────────────────────────────────────────────────────
#define SDA_PIN  8
#define SCL_PIN  9

// ── OLED ─────────────────────────────────────────────────────────────────────
#define SCREEN_WIDTH  128
#define SCREEN_HEIGHT  32
#define OLED_ADDR     0x3C
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// ── SHT31 ────────────────────────────────────────────────────────────────────
Adafruit_SHT31 sht31;
bool sensorOk = false;

// ── State ────────────────────────────────────────────────────────────────────
bool     colonVisible  = true;
uint32_t lastColonFlip = 0;

// ─────────────────────────────────────────────────────────────────────────────

void connectWiFi() {
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  uint8_t attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    attempts++;
  }
}

void syncNTP() {
  if (WiFi.status() != WL_CONNECTED) return;
  configTime(GMT_OFFSET, DST_OFFSET, NTP_SERVER);
  struct tm t;
  getLocalTime(&t, 5000);
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
}

void drawFrame() {
  struct tm t;
  if (!getLocalTime(&t)) return;

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  // HH:MM — large
  display.setTextSize(2);
  char timeBuf[6];
  if (colonVisible) {
    snprintf(timeBuf, sizeof(timeBuf), "%02d:%02d", t.tm_hour, t.tm_min);
  } else {
    snprintf(timeBuf, sizeof(timeBuf), "%02d %02d", t.tm_hour, t.tm_min);
  }
  display.setCursor(0, 0);
  display.print(timeBuf);

  // Date + sensor
  display.setTextSize(1);
  char dateBuf[9];
  snprintf(dateBuf, sizeof(dateBuf), "%02d.%02d", t.tm_mday, t.tm_mon + 1);
  display.setCursor(0, 17);
  display.print(dateBuf);

  if (sensorOk) {
    float temp = sht31.readTemperature();
    float hum  = sht31.readHumidity();
    if (!isnan(temp) && !isnan(hum)) {
      char buf[14];
      snprintf(buf, sizeof(buf), " %+.1fC %2.0f%%", temp, hum);
      display.setCursor(36, 17);
      display.print(buf);
    }
  }

  display.display();
}

// ─────────────────────────────────────────────────────────────────────────────

void setup() {
  Wire.begin(SDA_PIN, SCL_PIN);

  display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR);
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print("Celsius Clock");
  display.setCursor(0, 12);
  display.print("Connecting WiFi...");
  display.display();

  sensorOk = sht31.begin(0x44, &Wire);

  connectWiFi();
  syncNTP();

  display.clearDisplay();
  display.display();
}

void loop() {
  uint32_t now = millis();

  if (now - lastColonFlip >= 500) {
    colonVisible  = !colonVisible;
    lastColonFlip = now;
    drawFrame();
  }
}
