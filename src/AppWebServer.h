#pragma once

#include <WebServer.h>
#include <WiFi.h>
#include <Update.h>
#include "EspSmartWifi.h"
#include "PowerMonitor.h"

class AppWebServer {
public:
    AppWebServer(EspSmartWifi& wifi);
    void begin();
    void handleClient();
    void stop();
    void handleButtonPress();

private:
    static const char INDEX_HTML[] PROGMEM;

    WebServer        server;
    EspSmartWifi&    wifi;
    PowerMonitor     powerMonitor;

    // 基本处理函数
    void handleRoot();
    void handleStatus();
    void handlePower();
    void handleOutput();     // 输出使能控制
    void handleVoltage();    // CH224K PD 电压选择
    void handleRestart();
    void handleUpgrade();
    void handleUpdate();
    void handleUpdateUpload();
    void handleNotFound();
    bool initPowerMonitor();

    // 配置页面
    void HandleConfigRoot();
    void HandleConfigSave();
    void handleGetConfigData();

    void initTime();
    void syncTime();
};
