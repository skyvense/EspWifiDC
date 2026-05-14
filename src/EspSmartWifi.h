#pragma once
#include <Arduino.h>
#include <FS.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WebServer.h>
#include <ArduinoJson.h>

struct Config {
  String SSID = "1";
  String Passwd = "1";
  String Server = "mqtt://username:passwd@mqtt.server";  // MQTT服务器地址，支持认证
  String Topic = "/espPd/power";  // MQTT主题
  bool bConfigValid = false;
};

struct EMPTY_SERIAL
{
  void println(const char *){}
  void println(String){}
  void printf(const char *, ...){}
  void print(const char *){}
  //void print(Printable) {}
  void begin(int){}
  void end(){}
};
//_EMPTY_SERIAL _EMPTY_SERIAL;
//#define Serial_debug  _EMPTY_SERIAL
#define Serial_debug  Serial

class EasyLed;
class EspSmartWifi
{
private:
    EasyLed &led_;
    fs::File root;
    Config _config;
    bool _isAPMode;

    void BaseConfig();

    bool LoadConfig();
    bool SaveConfig();
    

public:
    EspSmartWifi(EasyLed &led):
    led_(led), _isAPMode(false)
    {
    }
    ~EspSmartWifi(){

    }

    void initFS();
    bool WiFiWatchDog();
    void ConnectWifi();
    void DisplayIP();
    
    // HTTP client methods
    String httpGet(const String& path);
    const Config& getConfig() const { return _config; }
    WiFiClient client;


    bool SaveConfig(Config config);
    // 获取AP模式状态
    bool isAPMode() const { return _isAPMode; }

    void StartAPMode();
    void StopAPMode();
    void TryConnectWifi();
  };




