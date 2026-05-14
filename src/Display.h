#pragma once

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C

class Display {
private:
    Adafruit_SSD1306 display;
    unsigned long lastUpdate = 0;
    int currentPage = 0;
    const int totalPages = 2; // WiFi状态、电源信息

    void displayWiFiStatus();
    void displayRelayStatus();
    void displayPowerInfo();

public:
    Display() : display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET) {}

    bool begin() {
        if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
            Serial.println(F("SSD1306 allocation failed"));
            return false;
        }
        
        display.clearDisplay();
        display.setTextSize(1);
        display.setTextColor(SSD1306_WHITE);
        display.setCursor(0,0);
        display.println(F("Initializing..."));
        display.display();
        return true;
    }

    void update() {
        if (millis() - lastUpdate >= 3000) { // 每3秒切换一次显示页面
            lastUpdate = millis();
            currentPage = (currentPage + 1) % totalPages;
            
            display.clearDisplay();
            display.setCursor(0,0);
            
            switch(currentPage) {
                case 0:
                    displayWiFiStatus();
                    break;
                case 1:
                    displayPowerInfo();
                    break;
            }
            
            display.display();
        }
    }

}; 