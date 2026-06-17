#pragma once
#include <Arduino.h>

// CH224K USB-PD sink controller.
// CFG pins driven directly by MCU GPIO levels (1 = HIGH, 0 = LOW):
//   VBUS | CFG1 | CFG2 | CFG3
//   5V   |  1   |  -   |  -
//   9V   |  0   |  0   |  0
//   12V  |  0   |  0   |  1
//   15V  |  0   |  1   |  1
//   20V  |  0   |  1   |  0
class CH224K {
public:
    enum Voltage { V5 = 5, V9 = 9, V12 = 12, V15 = 15, V20 = 20 };

    void begin(uint8_t cfg1Pin, uint8_t cfg2Pin, uint8_t cfg3Pin, Voltage initial = V5) {
        _cfg1 = cfg1Pin;
        _cfg2 = cfg2Pin;
        _cfg3 = cfg3Pin;
        setVoltage(initial);
    }

    void setVoltage(Voltage v) {
        bool c1Hi, c2Hi, c3Hi;
        switch (v) {
            case V5:  c1Hi = true;  c2Hi = true;  c3Hi = true;  break;
            case V9:  c1Hi = false; c2Hi = false; c3Hi = false; break;
            case V12: c1Hi = false; c2Hi = false; c3Hi = true;  break;
            case V15: c1Hi = false; c2Hi = true;  c3Hi = true;  break;
            case V20: c1Hi = false; c2Hi = true;  c3Hi = false; break;
            default:  return;
        }
        _drive(c1Hi, c2Hi, c3Hi);
        _v = v;
        Serial.printf("CH224K: %dV  CFG1=%s CFG2=%s CFG3=%s\n",
                      (int)v,
                      c1Hi ? "HIGH" : "LOW",
                      c2Hi ? "HIGH" : "LOW",
                      c3Hi ? "HIGH" : "LOW");
    }

    Voltage getVoltage() const { return _v; }

private:
    uint8_t _cfg1 = 0, _cfg2 = 0, _cfg3 = 0;
    Voltage _v = V5;

    static void _pin(uint8_t pin, bool highLevel) {
        pinMode(pin, OUTPUT);
        digitalWrite(pin, highLevel ? HIGH : LOW);
    }

    void _drive(bool c1Hi, bool c2Hi, bool c3Hi) {
        _pin(_cfg1, c1Hi);
        _pin(_cfg2, c2Hi);
        _pin(_cfg3, c3Hi);
    }
};
