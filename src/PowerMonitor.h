#pragma once

#include <Wire.h>
#include <Adafruit_INA219.h>

#define SHUNT_OHM 0.005f
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
        Wire.begin();
        if (!ina219.begin()) {
            Serial.println("Failed to find INA219 chip");
            return false;
        }
        
        ina219.setCalibration_32V_2A();
        
        Serial.printf("INA219 init, shunt=%.3f ohm, factor=%.1fx\n", SHUNT_OHM, SHUNT_FACTOR);
        
        _initialized = true;
        return true;
    }
    
    float getBusVoltage_V() {
        return ina219.getBusVoltage_V();
    }
    
    float getCurrent_mA() {
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
    float currentBuffer[10];
    int bufferIndex;
    int bufferCount;
    float powerBuffer[10];
    int powerIndex;
    int powerCount;
}; 
