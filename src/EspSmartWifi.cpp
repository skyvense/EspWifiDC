#include <Arduino.h>
#include <ArduinoJson.h>
#include <FS.h>
#include <SPIFFS.h>
#include "EspSmartWifi.h"

bool EspSmartWifi::LoadConfig()
{
    Serial.println("\n=== Loading WiFi Configuration ===");
    if (!SPIFFS.exists("/config.json")) {
        Serial.println("Config file not found, using defaults");
        return false;
    }
    Serial.println("Found config.json");

    File configFile = SPIFFS.open("/config.json", "r");
    if (!configFile) {
        Serial.println("Failed to open config file");
        return false;
    }
    Serial.println("Config file opened successfully");

    StaticJsonDocument<512> doc;
    DeserializationError error = deserializeJson(doc, configFile);
    configFile.close();

    if (error) {
        Serial.print("Failed to parse config file: ");
        Serial.println(error.c_str());
        return false;
    }
    Serial.println("JSON parsed successfully");

    _config.SSID = doc["ssid"] | "1";
    _config.Passwd = doc["passwd"] | "1";
    _config.Server = doc["server"] | "192.1.8.3";
    _config.Topic = doc["topic"] | "/espRouterPower/power";
    _config.bConfigValid = true;

    Serial.println("WiFi configuration loaded successfully");
    Serial.println("=== WiFi Configuration Load Complete ===\n");

    return true;
}

bool EspSmartWifi::SaveConfig(Config config)
{
    _config = config;
    return SaveConfig();
}

bool EspSmartWifi::SaveConfig()
{
    Serial.println("\n=== Saving WiFi Configuration ===");
    StaticJsonDocument<512> doc;
    doc["ssid"] = _config.SSID;
    doc["passwd"] = _config.Passwd;
    doc["server"] = _config.Server;
    doc["topic"] = _config.Topic;

    File configFile = SPIFFS.open("/config.json", "w");
    if (!configFile) {
        Serial.println("Failed to open config file for writing");
        return false;
    }

    if (serializeJson(doc, configFile) == 0) {
        Serial.println("Failed to write to config file");
        configFile.close();
        return false;
    }

    configFile.close();
    Serial.println("WiFi configuration saved successfully");
    Serial.println("=== WiFi Configuration Save Complete ===\n");
    return true;
}


void EspSmartWifi::BaseConfig()
{
    WiFi.mode(WIFI_STA);
    WiFi.setSleep(false);                     // 关 modem-sleep，灵敏度/延迟更稳
    WiFi.setTxPower(WIFI_POWER_19_5dBm);      // 显式拉到最大发射功率
    WiFi.begin(_config.SSID.c_str(), _config.Passwd.c_str());

    int waitCount = 0;
    while (WiFi.status() != WL_CONNECTED && waitCount < 20) {
        delay(500);
        Serial.print(".");
        waitCount++;
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\nWiFi connected");
        Serial.print("IP address: ");
        Serial.println(WiFi.localIP());
    } else {
        Serial.println("\nWiFi connection failed, will retry...");
    }
}

void EspSmartWifi::StartAPMode()
{
    if (_isAPMode) {
        Serial_debug.println("Already in AP mode, skipping initialization");
        return;
    }

    Serial_debug.println("Starting AP mode...");

    uint32_t chipId = (uint32_t)(ESP.getEfuseMac() & 0xFFFFFFFF);
    String apName = "ESP_Config_" + String(chipId, HEX);

    WiFi.mode(WIFI_AP);
    WiFi.setSleep(false);
    WiFi.setTxPower(WIFI_POWER_19_5dBm);
    WiFi.softAP(apName.c_str(), "12345678", 6);

    Serial_debug.print("AP started with SSID: ");
    Serial_debug.println(apName);
    Serial_debug.print("AP IP address: ");
    Serial_debug.println(WiFi.softAPIP());

    _isAPMode = true;
}

void EspSmartWifi::StopAPMode() {
    if (!_isAPMode) return;
    Serial_debug.println("Stopping AP mode, switching to STA mode...");
    WiFi.softAPdisconnect(true);
    WiFi.mode(WIFI_STA);
    _isAPMode = false;
}

void EspSmartWifi::TryConnectWifi() {
    Serial_debug.println("Trying to connect to WiFi...");
    Serial_debug.print("SSID: ");
    Serial_debug.println(_config.SSID);
    Serial_debug.print("Password: ");
    Serial_debug.println(_config.Passwd);

    WiFi.mode(WIFI_STA);
    WiFi.setSleep(false);
    WiFi.setTxPower(WIFI_POWER_19_5dBm);
    WiFi.begin(_config.SSID.c_str(), _config.Passwd.c_str());
}

void EspSmartWifi::ConnectWifi()
{
    if (!LoadConfig()) {
        Serial.println("No WiFi configuration found, entering AP mode...");
        StartAPMode();
        return;
    }
    BaseConfig();
}

bool EspSmartWifi::WiFiWatchDog()
{
    static unsigned long lastCheck = 0;
    static unsigned long lastModeSwitchMillis = 0;
    static bool apModeActive = false;
    unsigned long now = millis();

    if (now - lastCheck < 5000) {
        return WiFi.status() == WL_CONNECTED;
    }
    lastCheck = now;

    if (WiFi.status() != WL_CONNECTED) {
        if (!_isAPMode && _config.bConfigValid)
        {
            Serial.println("WiFi disconnected, attempting to reconnect...");
            WiFi.begin(_config.SSID.c_str(), _config.Passwd.c_str());
        }

        if (!_isAPMode && now - lastModeSwitchMillis > 60000) {
            StartAPMode();
            apModeActive = true;
            lastModeSwitchMillis = now;
        } else if (_isAPMode && apModeActive && now - lastModeSwitchMillis > 180000) {
            StopAPMode();
            TryConnectWifi();
            apModeActive = false;
            lastModeSwitchMillis = now;
        }
    } else {
        if (_isAPMode) {
            StopAPMode();
        }
        apModeActive = false;
        lastModeSwitchMillis = now;
    }

    return WiFi.status() == WL_CONNECTED;
}

void EspSmartWifi::initFS()
{
    Serial_debug.println("\n=== Initializing SPIFFS ===");

    if (!SPIFFS.begin(true)) {
        Serial_debug.println("Failed to mount SPIFFS");
        return;
    }

    Serial_debug.print("Total bytes: ");
    Serial_debug.println(SPIFFS.totalBytes());
    Serial_debug.print("Used bytes: ");
    Serial_debug.println(SPIFFS.usedBytes());

    Serial_debug.println("\nFiles in SPIFFS:");
    File root = SPIFFS.open("/");
    bool hasFiles = false;
    if (root && root.isDirectory()) {
        File f = root.openNextFile();
        while (f) {
            hasFiles = true;
            Serial_debug.print("  ");
            Serial_debug.print(f.name());
            Serial_debug.print("  ");
            Serial_debug.print(f.size());
            Serial_debug.println(" bytes");
            f = root.openNextFile();
        }
    }
    if (!hasFiles) {
        Serial_debug.println("  No files found");
    }

    Serial_debug.println("\nSPIFFS mounted successfully");
    Serial_debug.println("=== SPIFFS Initialization Complete ===\n");
}

void EspSmartWifi::DisplayIP()
{

}

String EspSmartWifi::httpGet(const String& path) {
    if (WiFi.status() != WL_CONNECTED) {
        Serial_debug.println("WiFi not connected");
        return "";
    }

    String url = _config.Server + path + "?icon=https://support.arduino.cc/hc/article_attachments/12416033021852.png";
    Serial_debug.print("HTTP GET: ");
    Serial_debug.println(url);

    HTTPClient http;
    http.begin(client, url);
    int httpCode = http.GET();

    String payload = "";
    if (httpCode > 0) {
        if (httpCode == HTTP_CODE_OK) {
            payload = http.getString();
            Serial_debug.println("HTTP Response: " + payload);
        } else {
            Serial_debug.printf("HTTP GET failed, error: %s\n", http.errorToString(httpCode).c_str());
        }
    } else {
        Serial_debug.printf("HTTP GET failed, error: %s\n", http.errorToString(httpCode).c_str());
    }

    http.end();
    return payload;
}
