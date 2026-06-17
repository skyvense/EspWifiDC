#pragma once
#include <Arduino.h>
#include <Wire.h>
#include <IPAddress.h>

// ---------------------------------------------------------------------------
// CH455G 2-wire 协议说明：
//   Wire.beginTransmission(addr7) 实际发送 addr7<<1，
//   CH455G 期望的命令字节：
//     系统命令 0x48 → addr7 = 0x24
//     数位0    0x68 → addr7 = 0x34
//     数位1    0x6A → addr7 = 0x35
//     数位2    0x6C → addr7 = 0x36
//     数位3    0x6E → addr7 = 0x37
// ---------------------------------------------------------------------------
#define CH455_SYS_ADDR   0x24
#define CH455_DIG0_ADDR  0x34
#define CH455_DIG1_ADDR  0x35
#define CH455_DIG2_ADDR  0x36
#define CH455_DIG3_ADDR  0x37

// CH455G 系统参数字节格式（数据手册）:
//   bit 7:   KOFF  (1=仅显示驱动, 0=显示驱动+键盘扫描)
//   bit 6:4: INTENS(亮度, 000=1/8最暗 ~ 111=8/8最亮)
//   bit 3:   7SEG  (1=7段模式无小数点, 0=8段模式支持小数点)
//   bit 2:   SLEEP (1=低功耗, 0=正常)
//   bit 1:   保留(0)
//   bit 0:   ENA   (1=开启显示)
#define CH455_BIT_ENA     0x01
#define CH455_BIT_SLEEP   0x04
#define CH455_BIT_7SEG    0x08
#define CH455_BIT_KOFF    0x80
#define CH455_DISP_ON     CH455_BIT_ENA
#define CH455_DISP_OFF    0x00

// 7段编码（共阴极，bit7=小数点）
#define SEG_0   0x3F
#define SEG_1   0x06
#define SEG_2   0x5B
#define SEG_3   0x4F
#define SEG_4   0x66
#define SEG_5   0x6D
#define SEG_6   0x7D
#define SEG_7   0x07
#define SEG_8   0x7F
#define SEG_9   0x6F
#define SEG_DASH 0x40
#define SEG_OFF  0x00
#define SEG_DP   0x80   // 小数点标志，与数字段码 OR

// 特殊字母段码
#define SEG_I   0x06    // I
#define SEG_P   0x73    // P
#define SEG_A   0x77    // A
#define SEG_V_  0x3E    // V
#define SEG_n   0x54    // n
#define SEG_m   0x54    // m (same as n on 7-seg)

// 时序常量
#define IP_DISPLAY_MS   10000
#define DATA_CYCLE_MS   2000
#define REINIT_INTERVAL_MS 30000
#define REINIT_BACKOFF_MS  5000   // 出错后两次 _reinit 之间最短间隔

class LedDisplay {
public:
    void begin(uint8_t brightness = 6);
    void setIP(IPAddress ip);

    void update(float voltage_V, float current_mA);

    void clear();

private:
    unsigned long _startMs    = 0;
    unsigned long _switchMs   = 0;
    unsigned long _lastReinit = 0;
    uint8_t       _dataPhase  = 0;
    uint8_t       _brightness = 6;
    uint8_t       _i2cErrCnt  = 0;
    IPAddress     _ip;
    bool          _ipSet      = false;

    void _writeCmd(uint8_t addr7, uint8_t data);
    void _showDigits(uint8_t d0, uint8_t d1, uint8_t d2, uint8_t d3);
    void _reinit();

    void _showIPLabel();
    void _showIPOctet(uint8_t val);
    void _showVoltage(float v);
    void _showCurrent(float mA);

    static uint8_t _seg(uint8_t n);
};
