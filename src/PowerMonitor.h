#pragma once

#include <Wire.h>
#include <Adafruit_INA219.h>

#define SHUNT_OHM 0.01f
#define SHUNT_CALIBRATION_OHM 0.1f
#define SHUNT_FACTOR (SHUNT_CALIBRATION_OHM / SHUNT_OHM)

class PowerMonitor {
public:
    PowerMonitor() : ina219(), _initialized(false) {
        for (int i = 0; i < 10; ++i) {
            currentBuffer[i] = 0;
            powerBuffer[i] = 0;
        }
        bufferIndex = 0;
        bufferCount = 0;
        powerIndex = 0;
        powerCount = 0;
    }
    
    bool begin() {
        // Wire bus is initialized by main setup() with the correct pins
        if (!ina219.begin()) {
            Serial.println("Failed to find INA219 chip");
            _initialized = false;
            return false;
        }

        ina219.setCalibration_32V_2A();

        Serial.printf("INA219 init, shunt=%.3f ohm, factor=%.1fx\n", SHUNT_OHM, SHUNT_FACTOR);

        _initialized = true;
        return true;
    }

    // 在 loop 里周期调用：未初始化时每 5 秒重试一次
    bool tryReinit() {
        if (_initialized) return true;
        unsigned long now = millis();
        if (now - _lastInitAttempt < 5000) return false;
        _lastInitAttempt = now;
        return begin();
    }

    float getBusVoltage_V() {
        if (!_initialized) return 0;
        return ina219.getBusVoltage_V();
    }

    float getCurrent_mA() {
        if (!_initialized) return 0;
        float rawCurrent = ina219.getCurrent_mA() * SHUNT_FACTOR;
        if (rawCurrent < 0) {
            if (bufferCount == 0) return 0;
            float sum = 0;
            for (int i = 0; i < bufferCount; i++) sum += currentBuffer[i];
            return sum / bufferCount;
        }
        currentBuffer[bufferIndex] = rawCurrent;
        bufferIndex = (bufferIndex + 1) % 10;
        if (bufferCount < 10) bufferCount++;
        float sum = 0;
        for (int i = 0; i < bufferCount; i++) sum += currentBuffer[i];
        return sum / bufferCount;
    }
    
    float getPower_mW() {
        if (!_initialized) return 0;
        float rawPower = ina219.getPower_mW() * SHUNT_FACTOR;
        if (rawPower < 0) {
            if (powerCount == 0) return 0;
            float sum = 0;
            for (int i = 0; i < powerCount; i++) sum += powerBuffer[i];
            return sum / powerCount;
        }
        powerBuffer[powerIndex] = rawPower;
        powerIndex = (powerIndex + 1) % 10;
        if (powerCount < 10) powerCount++;
        float sum = 0;
        for (int i = 0; i < powerCount; i++) sum += powerBuffer[i];
        return sum / powerCount;
    }
    
    bool isInitialized() {
        return _initialized;
    }

private:
    Adafruit_INA219 ina219;
    bool _initialized;
    unsigned long _lastInitAttempt = 0;
    float currentBuffer[10];
    int bufferIndex;
    int bufferCount;
    float powerBuffer[10];
    int powerIndex;
    int powerCount;
}; 