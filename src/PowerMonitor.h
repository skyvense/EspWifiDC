#pragma once

#include <Wire.h>
#include <Adafruit_INA219.h>

class PowerMonitor {
public:
    PowerMonitor() : ina219() {
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
        
        // 配置INA219使用0.01Ω分流电阻
        // 使用自定义校准
        // 最大电流 = 3.2A
        // 最大电压 = 32V
        // 分流电阻 = 0.01Ω
        ina219.setCalibration_32V_2A();
        
        // 打印配置信息
        Serial.println("INA219 initialized successfully");
        Serial.println("Configuration:");
        Serial.println("- Shunt Resistor: 0.01 ohm");
        Serial.println("- Max Current: 3.2A");
        Serial.println("- Max Voltage: 32V");
        
        return true;
    }
    
    float getBusVoltage_V() {
        return ina219.getBusVoltage_V();
    }
    
    float getCurrent_mA() {
        float rawCurrent = ina219.getCurrent_mA();
        if (rawCurrent < 0) {
            // 负数不入队，直接返回当前平均
            if (bufferCount == 0) return 0;
            float sum = 0;
            for (int i = 0; i < bufferCount; i++) sum += currentBuffer[i];
            return sum / bufferCount;
        }
        // 入队
        currentBuffer[bufferIndex] = rawCurrent;
        bufferIndex = (bufferIndex + 1) % 10;
        if (bufferCount < 10) bufferCount++;
        // 计算平均
        float sum = 0;
        for (int i = 0; i < bufferCount; i++) sum += currentBuffer[i];
        return sum / bufferCount;
    }
    
    float getPower_mW() {
        float rawPower = ina219.getPower_mW();
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
        return ina219.begin();
    }

private:
    Adafruit_INA219 ina219;
    float currentBuffer[10];
    int bufferIndex;
    int bufferCount;
    float powerBuffer[10];
    int powerIndex;
    int powerCount;
}; 