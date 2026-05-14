#include "LedDisplay.h"

// ---- 底层 ---------------------------------------------------------------

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
    Wire.endTransmission();
}

void LedDisplay::_showDigits(uint8_t d0, uint8_t d1, uint8_t d2, uint8_t d3) {
    _writeCmd(CH455_DIG0_ADDR, d0);
    _writeCmd(CH455_DIG1_ADDR, d1);
    _writeCmd(CH455_DIG2_ADDR, d2);
    _writeCmd(CH455_DIG3_ADDR, d3);
}

// ---- 公共接口 ------------------------------------------------------------

void LedDisplay::begin(uint8_t brightness) {
    // 系统参数: bit[6:4]=BRT, bit3=RON=1, bit[2:0]=0(DP全灭)
    uint8_t sysParam = CH455_DISP_ON | ((brightness & 0x07) << 4);
    _writeCmd(CH455_SYS_ADDR, sysParam);
    _startMs  = millis();
    _switchMs = millis();
    _dataPhase = 0;
    clear();
    Serial.println("CH455G LedDisplay initialized");
}

void LedDisplay::setIP(IPAddress ip) {
    _ip    = ip;
    _ipSet = true;
}

void LedDisplay::clear() {
    _showDigits(SEG_OFF, SEG_OFF, SEG_OFF, SEG_OFF);
}

// ---- 显示子函数 ----------------------------------------------------------

void LedDisplay::_showIPLabel() {
    // 显示 "IP  "
    _showDigits(SEG_I, SEG_P, SEG_OFF, SEG_OFF);
}

void LedDisplay::_showIPOctet(uint8_t val) {
    // 最多3位整数，左对齐空格填充 → " XXX"
    uint8_t d0 = SEG_OFF;
    uint8_t d1 = SEG_OFF;
    uint8_t d2 = SEG_OFF;
    uint8_t d3 = _seg(val % 10);

    if (val >= 10)  d2 = _seg((val / 10) % 10);
    if (val >= 100) d1 = _seg((val / 100) % 10);

    _showDigits(d0, d1, d2, d3);
}

// 电压：格式 "XX.XX"（最高2位整数 + 小数点 + 2位小数）
// 例：5.12V → " 5.12", 12.5V → "12.50"
void LedDisplay::_showVoltage(float v) {
    if (v < 0) v = 0;
    if (v > 99.99f) v = 99.99f;

    uint16_t val = (uint16_t)(v * 100 + 0.5f);  // 单位：0.01V
    uint8_t tens  = val / 1000;
    uint8_t ones  = (val / 100) % 10;
    uint8_t dec1  = (val / 10)  % 10;
    uint8_t dec2  = val % 10;

    uint8_t d0 = (tens > 0) ? _seg(tens) : SEG_OFF;
    uint8_t d1 = _seg(ones) | SEG_DP;   // 小数点在个位后
    uint8_t d2 = _seg(dec1);
    uint8_t d3 = _seg(dec2);

    _showDigits(d0, d1, d2, d3);
}

// 电流：显示 mA 整数，0~9999
// 例：1234mA → "1234", 500mA → " 500"
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

// ---- 状态机 update ------------------------------------------------------

void LedDisplay::update(float voltage_V, float current_mA) {
    unsigned long now     = millis();
    unsigned long elapsed = now - _startMs;

    if (elapsed < IP_DISPLAY_MS) {
        // ---- IP 显示阶段（10秒） ----
        // 分5个子阶段，每2秒一个
        uint8_t sub = (uint8_t)(elapsed / 2000);
        switch (sub) {
            case 0:
                _showIPLabel();
                break;
            case 1:
                _showIPOctet(_ipSet ? _ip[0] : 0);
                break;
            case 2:
                _showIPOctet(_ipSet ? _ip[1] : 0);
                break;
            case 3:
                _showIPOctet(_ipSet ? _ip[2] : 0);
                break;
            default:
                _showIPOctet(_ipSet ? _ip[3] : 0);
                break;
        }
    } else {
        // ---- 数据显示阶段（电压/电流轮换） ----
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
