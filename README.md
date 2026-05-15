# EspWifiDC

WiFi-controlled DC power supply with real-time monitoring, based on ESP8266.

## Features

- **Output Enable Control** — Remote on/off switch via web UI, with state persistence across reboots
- **Power Monitoring** — INA219 measures voltage, current, and power in real time (0.01Ω shunt, 10x calibrated)
- **7-Segment Display** — CH455G drives a 4-digit display showing voltage (XX.XV) and current (X.XXA / XX.XA), alternating every 2 seconds
- **OLED Display** — SSD1306 shows WiFi status and power info in rotation
- **Web Interface** — Responsive dashboard with live power data, output toggle, and power chart
- **WiFi Management** — Auto-connect with AP fallback; configure credentials via captive portal
- **MQTT** — Publish power data to an MQTT broker
- **OTA Upgrade** — Firmware update via web interface
- **RGB LED** — Green indicator when output voltage is present
- **Button Control** — Short press toggles output, long press (3s) clears config and reboots

## Hardware

| Component | Description |
|-----------|-------------|
| ESP8266 NodeMCU | Main controller |
| INA219 | Voltage/current sensor (0.01Ω shunt) |
| CH455G | I2C 7-segment LED driver (4 digits) |
| SSD1306 | 128×32 I2C OLED |
| NeoPixel | WS2812B RGB LED (×1) |

### Pin Mapping

| Pin | GPIO | Function |
|-----|------|----------|
| D4 | 16 | Output enable (HIGH=on, LOW=off) |
| D4 | 2 | Status LED (active low, EasyLed) |
| D7 | 13 | NeoPixel data |
| D3 | 0 | Button (internal pull-up) |
| D1 | 5 | I2C SCL |
| D2 | 4 | I2C SDA |

## Software

### Dependencies

- PlatformIO (espressif8266 platform)
- ArduinoJson
- Adafruit INA219
- Adafruit SSD1306 + GFX
- Adafruit NeoPixel
- EasyLed
- PubSubClient

### Build & Flash

```bash
pio run -t upload
pio run -t uploadfs   # SPIFFS (config storage)
```

### First Boot

1. If no WiFi config is found, the device starts in AP mode (SSID: `ESP_Config_XXXXXXXX`)
2. Connect to the AP and navigate to `http://192.168.4.1/config`
3. Enter your WiFi credentials and MQTT server, then save
4. The device reboots and connects to your WiFi
5. The 7-segment display shows the IP address for 10 seconds, then alternates voltage/current

## Web Interface

- `/` — Main dashboard: power monitoring, output toggle, power chart
- `/config` — WiFi & MQTT configuration
- `/upgrade` — OTA firmware upload
- `/power` — JSON API: `{"channel1":{"current":...,"voltage":...,"power":...}}`
- `/output` — JSON API: `GET` reads state, `GET ?enable=1|0` sets state
- `/status` — JSON API: build date, WiFi info

## 7-Segment Display Format

| Phase | Display | Example |
|-------|---------|---------|
| Boot (10s) | IP address | `IP  ` → `192 ` → `168 ` → ... |
| Voltage | XX.XV | `12.5V` |
| Current < 10A | X.XXA | `1.50A` |
| Current ≥ 10A | XX.XA | `12.5A` |

Voltage and current alternate every 2 seconds.

## Project Structure

```
src/
├── main.cpp           # Main loop, setup, button, MQTT
├── PowerMonitor.h     # INA219 wrapper with 10x shunt correction
├── LedDisplay.h/cpp   # CH455G 7-segment driver
├── Display.h/cpp      # SSD1306 OLED display
├── WebServer.h/cpp    # HTTP server + embedded HTML
└── EspSmartWifi.h/cpp # WiFi config, AP/STA management
```
