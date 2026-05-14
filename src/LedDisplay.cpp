#include "LedDisplay.h"

uint8_t LedDisplay::_seg(uint8_t n) {
    const uint8_t tbl[] = {
        SEG_0, SEG_1, SEG_2, SEG_3, SEG_4,
        SEG_5, SEG_6, SEG_7, SEG_8, SEG_9
    };
    return (n <= 9) ? tbl[n] : SEG_DASH;
}

void LedDisplay::_writeCmd(uint8_t addr7, uint8_t data) {
    Wire.beginTransmission(addr7);
    Wire.write(data);
    uint8_t err = Wire.endTransmission();
    if (err != 0) {
        _i2cErrCnt++;
    } else {
        _i2cErrCnt = 0;
    }
}

void LedDisplay::_showDigits(uint8_t d0, uint8_t d1, uint8_t d2, uint8_t d3) {
    _writeCmd(CH455_DIG0_ADDR, d0);
    _writeCmd(CH455_DIG1_ADDR, d1);
    _writeCmd(CH455_DIG2_ADDR, d2);
    _writeCmd(CH455_DIG3_ADDR, d3);
}

void LedDisplay::_reinit() {
    Wire.begin();
    uint8_t sysParam = CH455_BIT_KOFF
                     | ((_brightness & 0x07) << 4)
                     | CH455_BIT_ENA;
    Wire.beginTransmission(CH455_SYS_ADDR);
    Wire.write(sysParam);
    Wire.endTransmission();
    _i2cErrCnt = 0;
    Serial.println("CH455G re-initialized after I2C error");
}

void LedDisplay::begin(uint8_t brightness) {
    _brightness = brightness;
    uint8_t sysParam = CH455_BIT_KOFF
                     | ((brightness & 0x07) << 4)
                     | CH455_BIT_ENA;
    _writeCmd(CH455_SYS_ADDR, sysParam);
    _startMs  = millis();
    _switchMs = millis();
    _dataPhase = 0;
    clear();
    Serial.printf("CH455G init, sysParam=0x%02X\n", sysParam);
}

void LedDisplay::setIP(IPAddress ip) {
    _ip    = ip;
    _ipSet = true;
}

void LedDisplay::clear() {
    _showDigits(SEG_OFF, SEG_OFF, SEG_OFF, SEG_OFF);
}

void LedDisplay::_showIPLabel() {
    _showDigits(SEG_I, SEG_P, SEG_OFF, SEG_OFF);
}

void LedDisplay::_showIPOctet(uint8_t val) {
    uint8_t d0 = SEG_OFF;
    uint8_t d1 = SEG_OFF;
    uint8_t d2 = SEG_OFF;
    uint8_t d3 = _seg(val % 10);

    if (val >= 10)  d2 = _seg((val / 10) % 10);
    if (val >= 100) d1 = _seg((val / 100) % 10);

    _showDigits(d0, d1, d2, d3);
}

void LedDisplay::_showVoltage(float v) {
    if (v < 0) v = 0;
    if (v > 99.9f) v = 99.9f;

    uint16_t val = (uint16_t)(v * 10 + 0.5f);
    uint8_t tens = val / 100;
    uint8_t ones = (val / 10) % 10;
    uint8_t dec  = val % 10;

    uint8_t d0 = (tens > 0) ? _seg(tens) : SEG_OFF;
    uint8_t d1 = _seg(ones) | SEG_DP;
    uint8_t d2 = _seg(dec);
    uint8_t d3 = SEG_OFF;

    _showDigits(d0, d1, d2, d3);
}

void LedDisplay::_showCurrent(float mA) {
    if (mA < 0) mA = 0;
    uint16_t val = (uint16_t)(mA + 0.5f);
    if (val > 9999) val = 9999;

    uint8_t thousands = val / 1000;
    uint8_t hundreds  = (val / 100) % 10;
    uint8_t tens      = (val / 10)  % 10;
    uint8_t ones      = val % 10;

    uint8_t d0 = (thousands > 0) ? _seg(thousands) : SEG_OFF;
    uint8_t d1 = (val >= 100)   ? _seg(hundreds)  : SEG_OFF;
    uint8_t d2 = (val >= 10)    ? _seg(tens)       : SEG_OFF;
    uint8_t d3 = _seg(ones);

    _showDigits(d0, d1, d2, d3);
}

void LedDisplay::update(float voltage_V, float current_mA) {
    if (_i2cErrCnt >= 3) {
        _reinit();
    }

    unsigned long now     = millis();
    unsigned long elapsed = now - _startMs;

    if (elapsed < IP_DISPLAY_MS) {
        uint8_t sub = (uint8_t)(elapsed / 2000);
        switch (sub) {
            case 0: _showIPLabel(); break;
            case 1: _showIPOctet(_ipSet ? _ip[0] : 0); break;
            case 2: _showIPOctet(_ipSet ? _ip[1] : 0); break;
            case 3: _showIPOctet(_ipSet ? _ip[2] : 0); break;
            default: _showIPOctet(_ipSet ? _ip[3] : 0); break;
        }
    } else {
        if (now - _switchMs >= DATA_CYCLE_MS) {
            _switchMs  = now;
            _dataPhase = (_dataPhase + 1) % 2;
        }
        if (_dataPhase == 0) {
            _showVoltage(voltage_V);
        } else {
            _showCurrent(current_mA);
        }
    }
}
