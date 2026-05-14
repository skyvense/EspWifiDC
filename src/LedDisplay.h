#pragma once
#include <Arduino.h>
#include <Wire.h>
#include <ESP8266WiFi.h>

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

// 系统参数字节格式（CH455G）:
//   bit[3:1] = BRT 亮度 0~7
//   bit0     = RON 显示开关（1=开）
#define CH455_DISP_ON    0x01   // RON=1（显示开）
#define CH455_DISP_OFF   0x00

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
#define SEG_V_  0x3E    // V（同U形状）
#define SEG_n   0x54    // n

// 时序常量
#define IP_DISPLAY_MS   10000   // 启动后显示IP的总时长 (ms)
#define DATA_CYCLE_MS   2000    // 电压/电流轮换间隔 (ms)

class LedDisplay {
public:
    void begin(uint8_t brightness = 6);
    void setIP(IPAddress ip);

    // 在 loop() 中每次调用，驱动内部状态机刷新数码管
    void update(float voltage_V, float current_mA);

    void clear();

private:
    unsigned long _startMs  = 0;
    unsigned long _switchMs = 0;
    uint8_t       _dataPhase = 0;   // 0=显示电压, 1=显示电流
    IPAddress     _ip;
    bool          _ipSet    = false;

    // 底层写命令
    void _writeCmd(uint8_t addr7, uint8_t data);

    // 向4位数码管写入段码（dig0=最高位/左）
    void _showDigits(uint8_t d0, uint8_t d1, uint8_t d2, uint8_t d3);

    // 显示相关
    void _showIPLabel();
    void _showIPOctet(uint8_t val);
    void _showVoltage(float v);
    void _showCurrent(float mA);

    // 将数字0~9转为段码，超出范围返回 SEG_DASH
    static uint8_t _seg(uint8_t n);
};
