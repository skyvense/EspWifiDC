#include "Display.h"
#include "EspSmartWifi.h"
#include "PowerMonitor.h"

extern EspSmartWifi wifi;
extern PowerMonitor powerMonitor;

// 缓存继电器状态
static bool cachedRelayStates[4] = {false, false, false, false};

void Display::displayWiFiStatus() {
    display.setTextSize(1);
    
    // 显示当前时间
    time_t now = time(nullptr);
    struct tm* timeinfo = localtime(&now);
    if (timeinfo) {
        char timeStr[9];
        strftime(timeStr, sizeof(timeStr), "%H:%M:%S", timeinfo);
        display.print(timeStr);
    }
    
    display.println(F(" WiFi:"));
    
    if (WiFi.status() == WL_CONNECTED) {
        display.println(F("Connected"));
        display.print(F("IP: "));
        display.println(WiFi.localIP().toString());
        display.print(F("RSSI: "));
        display.print(WiFi.RSSI());
        display.println(F(" dBm"));
    } else {
        display.println(F("Disconnected"));
        if (wifi.isAPMode()) {
            display.println(F("AP Mode Active"));
            display.print(F("SSID: ESP32_AP"));
        }
    }
}


void Display::displayPowerInfo() {
    display.setTextSize(1);
    display.println(F("Power Monitor:"));
    
    for (int ch = 0; ch < 1; ch++) {
        float voltage = powerMonitor.getBusVoltage_V();
        float current = powerMonitor.getCurrent_mA();
        
        display.print(F("CH"));
        display.print(ch + 1);
        display.print(F(": "));
        display.print(voltage, 1);
        display.print(F("V "));
        display.print(current, 0);
        display.println(F("mA"));
    }
}

