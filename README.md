# Stellar Clock

![Language](https://img.shields.io/badge/language-C%2B%2B-blue) ![Platform](https://img.shields.io/badge/platform-ESP32--C3-green) ![License](https://img.shields.io/badge/license-MIT-lightgrey)

Автономные настольные часы на ESP32-C3 SuperMini с OLED-дисплеем, синхронизацией времени по NTP и датчиком микроклимата.

---

## О проекте

Stellar Clock — автономные настольные часы, синхронизирующие время по протоколу NTP через WiFi при каждом включении. Вертикальный OLED-дисплей 128×32 работает в портретной ориентации (поворот 90°), что даёт компактный столбец с информацией. Устройство отображает часы, минуты, дату, температуру, влажность и атмосферное давление. Питание осуществляется от Li-Ion аккумулятора с зарядкой по USB-C через модуль TP4056.

---

## Функции

- Синхронизация времени по NTP (WiFi), UTC+3 Москва
- Отображение: HH / MM / DD.MM / T°C / H% / P мм рт.ст.
- Датчик BME280 (температура + влажность + давление) или BMP280 (без влажности) — автоопределение по sensor ID
- Мигающие точки-разделители (обновление 2 раза в секунду)
- Световой маяк на GPIO 0
- Автономная работа от аккумулятора, зарядка USB-C

---

## Компоненты

| Компонент          | Описание                         | Интерфейс     |
|--------------------|----------------------------------|---------------|
| ESP32-C3 SuperMini | Микроконтроллер, WiFi            | —             |
| SSD1306 128×32     | OLED-дисплей                     | I²C (0x3C)    |
| BME280 / BMP280    | Температура, влажность, давление | I²C (0x76/77) |
| SHT31              | Температура + влажность (v1)     | I²C (0x44)    |
| TP4056             | Зарядка Li-Ion, USB-C            | —             |
| Li-Ion аккумулятор | Питание                          | —             |
| LED + резистор     | Световой маяк                    | GPIO 0        |
| Выключатель        | Разрыв + линии питания           | —             |

---

## Схема подключения

![Схема](docs/diagram.jpg)

- I²C шина: SDA → GPIO 8, SCL → GPIO 9
- LED: GPIO 0 → резистор → LED → GND
- Питание: батарея → TP4056 → выключатель → 5V/GND ESP32

---

## Сборка и прошивка

**Требования:**
- VSCode + расширение PlatformIO IDE
- Python 3.x (нужен PlatformIO)

**Шаги:**

1. Клонировать репозиторий:
   ```bash
   git clone https://github.com/Vitalymt/Stellar-Clock-ESP32C3-Supermini.git
   cd Stellar-Clock-ESP32C3-Supermini/stellar-clock
   ```

2. Открыть папку `stellar-clock/` в VSCode.

3. В файле `src/StellarClock_v5.ino` заменить:
   ```cpp
   const char* WIFI_SSID     = "Your_WiFi";      // имя своей WiFi сети
   const char* WIFI_PASSWORD = "Your_Password";  // пароль WiFi
   const long  GMT_OFFSET    = 3L * 3600L;       // UTC-офсет в секундах (UTC+3 = 10800)
   ```

4. Подключить ESP32-C3 по USB, при необходимости указать порт в `platformio.ini`:
   ```ini
   upload_port = /dev/ttyUSB0   ; Linux
   upload_port = COM13          ; Windows
   ```

5. **PlatformIO → Upload** (или `pio run --target upload`).

---

## Версии

| Версия       | Файл            | Датчик | Дисплей | Время |
|--------------|-----------------|--------|---------|-------|
| v5 (текущая) | StellarClock_v5 | BME280 | SSD1306 | NTP   |
| Celsius (v1) | Celsius         | SHT31  | SSD1306 | NTP   |

---

## Лицензия

MIT
