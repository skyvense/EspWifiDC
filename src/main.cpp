#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <Wire.h>
#include <EasyLed.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <Adafruit_NeoPixel.h>

#include "EspSmartWifi.h"
#include "WebServer.h"
#include "PowerMonitor.h"
#include "LedDisplay.h"

// ---- 引脚定义 ------------------------------------------------------------
#define STATUS_LED      D4      // EasyLed 状态灯（低电平有效）
#define OUTPUT_EN_PIN   16      // GPIO16, HIGH=output enabled, LOW=disabled
#define RGB_LED_PIN     13      // GPIO13，NeoPixel
#define NUM_LEDS        1
#define BRIGHTNESS      50

// ---- 全局对象 ------------------------------------------------------------
Adafruit_NeoPixel pixels(NUM_LEDS, RGB_LED_PIN, NEO_GRB + NEO_KHZ800);

EasyLed        led(STATUS_LED, EasyLed::ActiveLevel::Low, EasyLed::State::Off);
EspSmartWifi   wifi(led);
LedDisplay     ledDisplay;
WebServer      webServer(wifi, led);
PowerMonitor   powerMonitor;
PubSubClient   mqtt(wifi.client);

// ---- 输出使能状态（WebServer 通过 extern 访问） --------------------------
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
    digitalWrite(OUTPUT_EN_PIN, outputEnabled ? HIGH : LOW);
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
    digitalWrite(OUTPUT_EN_PIN, en ? HIGH : LOW);
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

    String clientId = "EspRouterPower" + String(ESP.getChipId(), HEX);
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

    Serial.println("==============================\n");
    char buffer[512];
    serializeJson(doc, buffer);
    const Config& config = wifi.getConfig();
    publishMQTT(config.Topic.c_str(), buffer);
}

// ---- 按钮处理 ------------------------------------------------------------
void checkButton() {
    bool currentState = digitalRead(BUTTON_PIN);
    if (currentState == lastButtonState) { lastButtonState = currentState; return; }
    if (millis() - lastButtonPress <= DEBOUNCE_DELAY) { lastButtonState = currentState; return; }

    lastButtonPress = millis();

    if (currentState == LOW) {  // 按下
        unsigned long pressStart = millis();
        while (digitalRead(BUTTON_PIN) == LOW) {
            delay(50);
            if (millis() - pressStart > 3000) {
                // 长按 3 秒：清除配置重启
                if (SPIFFS.remove("/config.json")) {
                    Serial.println("Config cleared, restarting...");
                    led.flash(5, 100, 100, 0, 0);
                    delay(1000);
                    ESP.restart();
                }
                lastButtonState = currentState;
                return;
            }
        }
        // 短按：切换输出使能
        setOutputEnable(!outputEnabled);
        led.flash(1, 100, 100, 0, 0);
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
    if (a > 10.0f) a = 10.0f;

    float t = a / 10.0f;
    uint8_t brightness = (uint8_t)(30 + t * 225);
    uint8_t r, g;

    if (t < 0.5f) {
        float s = t * 2.0f;
        r = (uint8_t)(s * 255);
        g = 255;
    } else {
        float s = (t - 0.5f) * 2.0f;
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

    // 输出使能引脚（默认禁用）
    pinMode(OUTPUT_EN_PIN, OUTPUT);
    digitalWrite(OUTPUT_EN_PIN, LOW);

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
    if (WiFi.status() == WL_CONNECTED) {
        ledDisplay.setIP(WiFi.localIP());
    }

    // WebServer
    webServer.begin();

    // MQTT
    if (!connectMQTT()) {
        Serial.println("Failed to connect to MQTT server");
    }
    delay(500);

    // 按钮引脚
    pinMode(BUTTON_PIN, INPUT_PULLUP);
}

// ---- Loop ----------------------------------------------------------------
int loop_count = 0;

void loop() {
    checkButton();
    wifi.WiFiWatchDog();
    webServer.handleClient();
    yield();

    if (loop_count % 200 == 0) {
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
            led.flash(1, 25, 25, 0, 0);
        }
    } else {
        if (wifi.isAPMode()) {
            loop_count++;
            if (loop_count % 2000 == 0) {
                led.flash(1, 10, 50, 0, 0);
            }
        } else {
            if (!mqtt.connected()) {
                loop_count++;
                if (loop_count % 5000 == 0) {
                    if (!connectMQTT()) {
                        Serial.println("Failed to reconnect to MQTT server");
                    }
                    led.flash(2, 50, 50, 0, 0);
                }
            }
        }
    }

    delay(1);
}