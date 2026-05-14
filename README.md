# ESP8266 PD WiFi Power Supply

A smart power supply system based on ESP8266 and INA219, featuring USB PD (Power Delivery) support, real-time power monitoring, and WiFi connectivity.

## Features

- **USB PD Support**
  - USB Power Delivery protocol support
  - Multiple voltage/current profiles
  - Dynamic power negotiation

- **High-Precision Power Monitoring**
  - Current measurement range: 0-2A with 0.1mA resolution
  - Voltage measurement range: 0-32V
  - Power measurement with 2mW resolution
  - Calibrated for 0.1Ω shunt resistor

- **Real-time Web Interface**
  - Live data visualization using Chart.js
  - Responsive design for both desktop and mobile
  - Auto-refreshing data every 2 seconds
  - Historical data display with configurable time range

- **Network Features**
  - WiFi connectivity with automatic reconnection
  - Access Point mode for direct connection
  - Configurable WiFi credentials
  - Network status monitoring

## Hardware Requirements

- ESP8266 NodeMCU development board
- INA219 current sensor
- 0.1Ω shunt resistor
- USB PD controller
- Power supply (3.3V-5V)
- USB cable for programming

## Software Requirements

- PlatformIO IDE
- ESP8266 Arduino Core
- Required Libraries:
  - Adafruit INA219
  - ESPAsyncWebServer
  - AsyncTCP
  - ArduinoJson
  - SPIFFS

## Installation

1. Clone this repository
2. Open the project in PlatformIO
3. Install required libraries
4. Configure WiFi credentials in `config.h`
5. Upload the code to ESP8266
6. Upload the web interface files to SPIFFS

## Configuration

### WiFi Settings
Edit `config.h` to set your WiFi credentials:
```cpp
#define WIFI_SSID "your_ssid"
#define WIFI_PASSWORD "your_password"
```

### Measurement Settings
- Current measurement is calibrated for 0.1Ω shunt resistor
- Default sampling interval: 2 seconds
- Default data retention: 24 hours

## Usage

1. Power on the ESP8266
2. Connect to the ESP8266's WiFi network
3. Open a web browser and navigate to `http://192.168.4.1`
4. View real-time measurements and historical data
5. Use the interface to:
   - Monitor current, voltage, and power
   - Configure PD profiles
   - View network status

## Data Format

The system uses two data formats:

### Real-time Data (JSON)
```json
{
    "channel1": {
        "current": 1.599999905,  // Current in mA
        "voltage": 11.56799984,  // Voltage in V
        "power": 18.5087986      // Power in mW
    }
}
```

### Units
- Current: milliamperes (mA)
- Voltage: volts (V)
- Power: milliwatts (mW)

### Data Storage
- Data is stored in JSON format
- Timestamps are in Unix epoch format
- Values are stored with 6 decimal places precision

## Calibration

The system is pre-calibrated for a 0.1Ω shunt resistor. If using a different shunt resistor, adjust the calibration values in the INA219 library:

```cpp
ina219_calValue = 4096;  // Calibration value for 0.1Ω shunt
ina219_currentDivider_mA = 10;  // 1 bit = 0.1mA
ina219_powerMultiplier_mW = 2;  // 1 bit = 2mW
```

## Troubleshooting

1. **No WiFi Connection**
   - Check WiFi credentials
   - Ensure ESP8266 is in range
   - Verify power supply

2. **Inaccurate Measurements**
   - Verify shunt resistor value
   - Check calibration values
   - Ensure proper connections

3. **Web Interface Not Loading**
   - Clear browser cache
   - Check SPIFFS upload
   - Verify network connection

4. **PD Issues**
   - Check USB cable quality
   - Verify PD controller connections
   - Ensure proper power supply

## Contributing

Feel free to submit issues and enhancement requests!

## License

This project is licensed under the MIT License - see the LICENSE file for details.
