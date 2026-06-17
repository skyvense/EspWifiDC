#pragma once

// ---- ESP32-C3 pin assignments ------------------------------------------
//  Per board schematic (CH224K + INA219 + CH455G + WS2812 RGB)

#define PIN_I2C_SDA      4    // INA219 + CH455G
#define PIN_I2C_SCL      5

#define PIN_RGB_LED      1    // WS2812 single pixel
#define PIN_BUTTON       9    // BOOT key
#define PIN_OUTPUT_EN    6    // PMOS gate (HIGH = output enabled)

// CH224K USB-PD trigger
#define PIN_CH224_CFG1   7
#define PIN_CH224_CFG2   10
#define PIN_CH224_CFG3   3
