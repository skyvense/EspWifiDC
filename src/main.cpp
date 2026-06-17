#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <Wire.h>
#include <SPIFFS.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <Adafruit_NeoPixel.h>

#include "PinConfig.h"
#include "EspSmartWifi.h"
#include "AppWebServer.h"
#include "PowerMonitor.h"
#include "LedDisplay.h"
#include "CH224K.h"

#define NUM_LEDS        1
#define BRIGHTNESS      50

// ---- 全局对象 ------------------------------------------------------------
Adafruit_NeoPixel pixels(NUM_LEDS, PIN_RGB_LED, NEO_GRB + NEO_KHZ800);

EspSmartWifi   wifi;
LedDisplay     ledDisplay;
AppWebServer   webServer(wifi);
PowerMonitor   powerMonitor;
PubSubClient   mqtt(wifi.client);
CH224K         ch224k;

// ---- 输出使能状态（AppWebServer 通过 extern 访问） ----------------------
bool outputEnabled = false;

void loadOutputState() {
    File f = SPIFFS.open("/output.json", "r");
    if (f) {
        StaticJsonDocument<64> doc;
        if (!deserializeJson(doc, f)) {
            outputEnabled = doc["enabled"] | false;
        }
        f.close();
    }
    digitalWrite(PIN_OUTPUT_EN, outputEnabled ? HIGH : LOW);
    Serial.printf("Output state loaded: %s\n", outputEnabled ? "ON" : "OFF");
}

void saveOutputState() {
    StaticJsonDocument<64> doc;
    doc["enabled"] = outputEnabled;
    File f = SPIFFS.open("/output.json", "w");
    if (f) {
        serializeJson(doc, f);
        f.close();
    }
}

void setOutputEnable(bool en) {
    outputEnabled = en;
    digitalWrite(PIN_OUTPUT_EN, en ? HIGH : LOW);
    saveOutputState();
    Serial.printf("Output %s\n", en ? "ENABLED" : "DISABLED");
}

// ---- 按钮状态 ------------------------------------------------------------
bool lastButtonState      = HIGH;
unsigned long lastButtonPress = 0;
const unsigned long DEBOUNCE_DELAY = 50;

// ---- MQTT 辅助函数 -------------------------------------------------------
bool parseMQTTServer(const String& serverUrl, String& host, int& port,
                     String& username, String& password) {
    if (!serverUrl.startsWith("mqtt://")) return false;
    String url = serverUrl.substring(7);
    int atPos = url.indexOf('@');
    if (atPos != -1) {
        String auth = url.substring(0, atPos);
        int colonPos = auth.indexOf(':');
        if (colonPos != -1) {
            username = auth.substring(0, colonPos);
            password = auth.substring(colonPos + 1);
        }
        url = url.substring(atPos + 1);
    }
    int colonPos = url.indexOf(':');
    if (colonPos != -1) {
        host = url.substring(0, colonPos);
        port = url.substring(colonPos + 1).toInt();
    } else {
        host = url;
        port = 1883;
    }
    return true;
}

bool connectMQTT() {
    const Config& config = wifi.getConfig();
    String host, username, password;
    int port;
    if (!parseMQTTServer(config.Server, host, port, username, password)) return false;

    mqtt.setServer(host.c_str(), port);
    mqtt.setBufferSize(2048);
    mqtt.setCallback([](char* topic, byte* payload, unsigned int length) {
        const Config& config = wifi.getConfig();
        if (strcmp(topic, config.Topic.c_str()) == 0) {
            String message;
            for (unsigned int i = 0; i < length; i++) message += (char)payload[i];
            Serial.print(message.c_str());
        }
    });

    uint32_t chipId = (uint32_t)(ESP.getEfuseMac() & 0xFFFFFFFF);
    String clientId = "EspRouterPower" + String(chipId, HEX);
    if (username.length() > 0)
        return mqtt.connect(clientId.c_str(), username.c_str(), password.c_str());
    return mqtt.connect(clientId.c_str());
}

void publishMQTT(const char* topic, const char* message) {
    if (WiFi.status() == WL_CONNECTED && mqtt.connected()) {
        mqtt.publish(topic, message);
        mqtt.loop();
    }
}

void publishPowerData() {
    StaticJsonDocument<512> doc;
    Serial.println("\n=== Power Monitor Readings ===");

    float current = powerMonitor.getCurrent_mA();
    float voltage = powerMonitor.getBusVoltage_V();
    float power   = powerMonitor.getPower_mW();

    Serial.printf("  Current: %.3f mA\n", current);
    Serial.printf("  Voltage: %.3f V\n",  voltage);
    Serial.printf("  Power:   %.3f mW\n", power);

    JsonObject channel = doc.createNestedObject("channel1");
    channel["current"] = current;
    channel["voltage"] = voltage;
    channel["power"]   = power;
    channel["output_en"] = outputEnabled;
    channel["pd_voltage"] = (int)ch224k.getVoltage();

    Serial.println("==============================\n");
    char buffer[512];
    serializeJson(doc, buffer);
    const Config& config = wifi.getConfig();
    publishMQTT(config.Topic.c_str(), buffer);
}

// ---- 按钮处理 ------------------------------------------------------------
void checkButton() {
    bool currentState = digitalRead(PIN_BUTTON);
    if (currentState == lastButtonState) { lastButtonState = currentState; return; }
    if (millis() - lastButtonPress <= DEBOUNCE_DELAY) { lastButtonState = currentState; return; }

    lastButtonPress = millis();

    if (currentState == LOW) {  // 按下
        unsigned long pressStart = millis();
        while (digitalRead(PIN_BUTTON) == LOW) {
            delay(50);
            if (millis() - pressStart > 3000) {
                // 长按 3 秒：清除配置重启
                if (SPIFFS.remove("/config.json")) {
                    Serial.println("Config cleared, restarting...");
                    delay(1000);
                    ESP.restart();
                }
                lastButtonState = currentState;
                return;
            }
        }
        // 短按：切换输出使能
        setOutputEnable(!outputEnabled);
    }
    lastButtonState = currentState;
}

void updateRGBLed(float current_mA) {
    if (!outputEnabled) {
        pixels.setPixelColor(0, 0);
        pixels.show();
        return;
    }

    const Config& config = wifi.getConfig();
    if (!config.bConfigValid) {
        pixels.setBrightness(80);
        pixels.setPixelColor(0, pixels.Color(0, 0, 255));
        pixels.show();
        return;
    }

    float a = current_mA / 1000.0f;
    if (a < 0) a = 0;
    if (a > 3.0f) a = 3.0f;

    float t = a / 3.0f;
    uint8_t brightness = (uint8_t)(5 + pow(t, 0.4f) * 250);
    uint8_t r, g;

    if (t < 0.33f) {
        float s = t / 0.33f;
        r = (uint8_t)(s * 255);
        g = 255;
    } else {
        float s = (t - 0.33f) / 0.67f;
        r = 255;
        g = (uint8_t)((1.0f - s) * 255);
    }

    pixels.setBrightness(brightness);
    pixels.setPixelColor(0, pixels.Color(r, g, 0));
    pixels.show();
}

// ---- Setup ---------------------------------------------------------------
void setup() {
    Serial.begin(115200);
    Serial.setRxBufferSize(2048);

    // I2C 总线（INA219 + CH455G 共用）
    Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);

    // 输出使能引脚（默认禁用）
    pinMode(PIN_OUTPUT_EN, OUTPUT);
    digitalWrite(PIN_OUTPUT_EN, LOW);

    // CH224K USB-PD 协商，默认 9V
    ch224k.begin(PIN_CH224_CFG1, PIN_CH224_CFG2, PIN_CH224_CFG3, CH224K::V9);

    // NeoPixel 初始化
    pixels.begin();
    pixels.setBrightness(BRIGHTNESS);
    pixels.show();

    // 电源监控
    if (!powerMonitor.begin()) {
        Serial.println("Failed to initialize power monitor!");
    }

    // WiFi
    wifi.initFS();
    wifi.ConnectWifi();
    wifi.DisplayIP();

    loadOutputState();

    // CH455G 数码管
    ledDisplay.begin(6);
    ledDisplay.setIP(wifi.isAPMode() ? WiFi.softAPIP() : WiFi.localIP());

    // WebServer
    webServer.begin();

    // MQTT
    if (!connectMQTT()) {
        Serial.println("Failed to connect to MQTT server");
    }
    delay(500);

    // 按钮引脚
    pinMode(PIN_BUTTON, INPUT_PULLUP);
}

// ---- Loop ----------------------------------------------------------------
int loop_count = 0;

void loop() {
    checkButton();
    wifi.WiFiWatchDog();
    webServer.handleClient();
    yield();

    // 跟踪 WiFi 模式/IP 变化，及时刷新数码管
    IPAddress nowIP = wifi.isAPMode() ? WiFi.softAPIP() : WiFi.localIP();
    ledDisplay.setIP(nowIP);  // setIP 自带"变化才重置"逻辑

    if (loop_count % 200 == 0) {
        powerMonitor.tryReinit();

        float current = powerMonitor.getCurrent_mA();
        float voltage = powerMonitor.getBusVoltage_V();

        ledDisplay.update(voltage, current);

        updateRGBLed(current);
    }

    if (WiFi.status() == WL_CONNECTED && mqtt.connected()) {
        mqtt.loop();
        loop_count++;
        if (loop_count % 10000 == 0) {
            publishPowerData();
        }
    } else {
        if (wifi.isAPMode()) {
            loop_count++;
        } else {
            if (!mqtt.connected()) {
                loop_count++;
                if (loop_count % 5000 == 0) {
                    if (!connectMQTT()) {
                        Serial.println("Failed to reconnect to MQTT server");
                    }
                }
            }
        }
    }

    delay(1);
}
