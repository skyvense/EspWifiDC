# EspWifiDC 硬件重构修改计划书

**版本**：v1.0  
**日期**：2026-05-14  
**状态**：待审阅

---

## 一、背景说明

原项目基于 ESP8266 + INA219，硬件上搭载了 USB PD 控制器，通过三路 GPIO（PD_CFG1/CFG2/CFG3）控制 PD 协议的电压档位（5V/9V/12V/15V/20V）。  

**本次硬件已完全变更**，具体变更如下：

| 变更项 | 旧硬件 | 新硬件 |
|--------|--------|--------|
| PD 电压选择 | PD 控制器，CFG1/CFG2/CFG3 三引脚控制 | **已去除**，不再需要电压档位切换 |
| LED 控制器 | 无 | **新增 CH455G**（I2C 数码管/LED 驱动芯片） |
| 输出使能控制 | 无 | **新增** `PD_CFG1`（GPIO16/D4）控制输出是否有效 |
| 电压/电流检测 | INA219 | INA219 **保持不变** |

> **注意**：`PD_CFG1` 引脚（GPIO16，即 D4）被复用为**输出使能**引脚，功能语义与原 PD 控制完全不同，需要重命名以反映新用途。

---

## 二、需要删除的代码（VoltageCtl 相关）

### 2.1 删除文件

| 文件 | 操作 | 说明 |
|------|------|------|
| `src/VoltageCtl.h` | **删除** | PD 电压控制头文件 |
| `src/VoltageCtl.cpp` | **删除** | PD 电压控制实现 |

### 2.2 `src/main.cpp` 中需要删除/修改的内容

| 行号 | 内容 | 操作 |
|------|------|------|
| 29 | `VoltageCtl voltageCtl;` | 删除全局对象 |
| 30 | `WebServer webServer(wifi, led, display, voltageCtl);` | 修改构造，去除 voltageCtl 参数 |
| 49–50 | `voltageLevels[]` 数组和 `currentVoltageIndex` | 删除 |
| 203–219 | `checkButton()` 中短按切换电压逻辑 | 删除或替换为空操作 |
| 250 | `voltageCtl.begin();` | 删除 |
| 307–320 | `switch(voltageCtl.getCurrentVoltage())` 及 RGB 颜色逻辑对 `setTarget` 的计算 | 删除 `setTarget` 比较逻辑；RGB 颜色指示可改为仅基于实测电压区间（已有逻辑，第 288–298 行保留） |

### 2.3 `src/WebServer.h` 中需要删除/修改的内容

| 行号 | 内容 | 操作 |
|------|------|------|
| 9 | `#include "VoltageCtl.h"` | 删除 |
| 17 | `WebServer(... VoltageCtl &voltagectl)` 构造声明 | 去掉 voltageCtl 参数 |
| 32 | `VoltageCtl &voltageCtl;` 成员变量 | 删除 |
| 41 | `void handleVoltage();` | 删除声明 |

### 2.4 `src/WebServer.cpp` 中需要删除/修改的内容

| 行号 | 内容 | 操作 |
|------|------|------|
| 905–906 | `WebServer::WebServer(... VoltageCtl &voltagectl)` 构造实现 | 去掉 voltageCtl 参数及初始化 |
| 974 | `server.on("/voltage", ...)` 路由注册 | 删除 |
| 1356–1411 | `WebServer::handleVoltage()` 全部实现 | 删除 |
| 641–668 | `INDEX_HTML` 中 "Voltage Control" 区块（`<div class="relay-control">...</div>`） | 删除整个 Voltage Control UI 块 |
| 862–896 | `INDEX_HTML` JS 中 `setVoltage()` 和 `checkVoltage()` 函数 | 删除 |
| 844 | `checkVoltage()` 调用 | 删除 |
| 899 | `setInterval(checkVoltage, 1000)` | 删除 |

### 2.5 SPIFFS 配置文件

- `/vol-config.json`（由 `VoltageCtl::saveConfig()` 创建）不再需要，可在首次烧录新固件时删除，或在 `setup()` 中调用 `SPIFFS.remove("/vol-config.json")` 清理。

---

## 三、新增功能（输出使能控制）

### 3.1 输出使能引脚

原 `VoltageCtl.h` 中 `PD_CFG1 16  // D4` 被复用为输出使能脚。需要：

1. 将该引脚定义**移出** `VoltageCtl.h`，在合适位置重新定义，建议放入 `src/OutputCtl.h` 或直接在 `main.cpp` 顶部定义：

```cpp
// 输出使能引脚（高电平有效 / 低电平有效，根据实际硬件确认）
#define OUTPUT_EN_PIN  16  // GPIO16 / D4
```

2. 在 `setup()` 中初始化：

```cpp
pinMode(OUTPUT_EN_PIN, OUTPUT);
digitalWrite(OUTPUT_EN_PIN, LOW);  // 默认关闭输出，根据实际逻辑确认电平
```

3. 提供 Web 界面或 MQTT 接口控制使能状态（见下方 Web 界面改造）。

> ⚠️ **需要确认**：`PD_CFG1`（GPIO16）控制输出使能时，**高电平有效还是低电平有效**？GPIO16 在 ESP8266 上有特殊性（无内部上拉，WAKE 功能），需在硬件设计中确认。

---

## 四、新增功能（CH455G LED 控制器）

### 4.1 CH455G 简介

- **接口**：I2C（与 INA219 共用同一 I2C 总线 SDA/SCL）
- **功能**：驱动 7 段数码管（最多 4 位）或 32 路 LED，支持 8 级亮度调节
- **I2C 地址**：固定地址，需查阅数据手册确认（通常为 `0x40` 区段）

### 4.2 新增文件

#### [NEW] `src/LedDisplay.h`

```cpp
#pragma once
#include <Arduino.h>
#include <Wire.h>

// CH455G I2C 地址（根据数据手册确认）
#define CH455_I2C_ADDR   0x40  // 待确认

// CH455G 系统命令
#define CH455_SYS_ON     0x01  // 开启显示
#define CH455_SYS_OFF    0x00  // 关闭显示

class LedDisplay {
public:
    LedDisplay();
    bool begin();
    void setBrightness(uint8_t level);  // 0-7
    void showNumber(float voltage, float current); // 显示电压/电流
    void clear();
    void setOutputStatus(bool enabled); // 显示输出使能状态
private:
    void writeCmd(uint8_t cmd);
    void writeDigit(uint8_t digit, uint8_t data);
};
```

#### [NEW] `src/LedDisplay.cpp`

实现上述接口，通过 `Wire.beginTransmission` / `Wire.write` / `Wire.endTransmission` 与 CH455G 通信。

### 4.3 `platformio.ini` 依赖

若使用社区 CH455G 库，在 `lib_deps` 中添加（或自行实现无需外部库）：

```ini
# 可选，如有合适的 CH455G Arduino 库
# lib_deps 中暂无标准库，建议自行实现
```

### 4.4 `src/main.cpp` 集成

```cpp
#include "LedDisplay.h"
LedDisplay ledDisplay;

// setup() 中：
if (!ledDisplay.begin()) {
    Serial.println("Failed to initialize CH455G!");
}

// loop() 中（每 100 次循环更新一次）：
ledDisplay.showNumber(voltage, current);
ledDisplay.setOutputStatus(digitalRead(OUTPUT_EN_PIN));
```

---

## 五、Web 界面改造

### 5.1 移除电压选择 UI

- 删除 `INDEX_HTML` 中整个 `<div class="relay-control">` Voltage Control 区块。
- 删除前端 JS 函数 `setVoltage()`、`checkVoltage()` 及相关 `setInterval`。

### 5.2 新增输出使能控制 UI

在主页面 Power Monitoring 卡片下方，新增一个简洁的开关控件：

```html
<div class="power-info">
    <h2>输出控制</h2>
    <div style="text-align:center; padding:20px;">
        <label class="switch">
            <input type="checkbox" id="outputEnable" onchange="setOutputEnable(this.checked)">
            <span class="slider"></span>
        </label>
        <div class="power-label" style="margin-top:10px;">输出使能</div>
    </div>
</div>
```

对应的后端接口：

- `GET /output` → 返回 `{"enabled": true/false}`
- `POST /output?enable=1` 或 `POST /output?enable=0` → 控制 `OUTPUT_EN_PIN`

### 5.3 新增 WebServer 接口

在 `WebServer.h` 声明：

```cpp
void handleOutput();
```

在 `WebServer.cpp` 实现：

```cpp
void WebServer::handleOutput() {
    if (server.hasArg("enable")) {
        bool enable = server.arg("enable").toInt() != 0;
        digitalWrite(OUTPUT_EN_PIN, enable ? HIGH : LOW);
        server.send(200, "application/json", enable ? "{\"enabled\":true}" : "{\"enabled\":false}");
    } else {
        bool current = digitalRead(OUTPUT_EN_PIN);
        server.send(200, "application/json", current ? "{\"enabled\":true}" : "{\"enabled\":false}");
    }
}
```

并在 `begin()` 中注册路由：

```cpp
server.on("/output", HTTP_GET, [this]() { handleOutput(); });
```

---

## 六、README 更新

更新 `README.md` 以反映新硬件：

| 章节 | 变更 |
|------|------|
| Features → USB PD Support | **删除**，改为"输出使能控制" |
| Hardware Requirements | 去掉"USB PD controller"，增加"CH455G LED驱动芯片" |
| Software Requirements | 增加 CH455G 相关说明（如有库则列出） |
| Usage | 去掉"Configure PD profiles"，改为"控制输出使能" |
| Troubleshooting → PD Issues | **删除**，改为 CH455G 显示相关排障 |

---

## 七、修改文件汇总

```
src/
├── VoltageCtl.h      ← 【删除】
├── VoltageCtl.cpp    ← 【删除】
├── LedDisplay.h      ← 【新增】CH455G LED控制器
├── LedDisplay.cpp    ← 【新增】CH455G LED控制器实现
├── main.cpp          ← 【修改】去除VoltageCtl，新增OutputCtl和LedDisplay集成
├── WebServer.h       ← 【修改】去除VoltageCtl依赖，新增handleOutput
├── WebServer.cpp     ← 【修改】删除voltage路由和UI，新增output路由和UI
└── Display.h/.cpp    ← 【可选修改】若OLED显示内容需同步更新
README.md             ← 【修改】更新硬件描述
```

---

## 八、待确认事项

> 以下问题在执行代码修改前需要明确：

1. **`OUTPUT_EN_PIN`（GPIO16）使能电平**：高电平有效还是低电平有效？
2. **CH455G I2C 地址**：请提供或确认地址（数据手册上 CH455G 固定地址为 `0x40`，但需确认实际连接的引脚配置）。
3. **CH455G 显示内容**：数码管打算显示什么？（建议显示当前电压/电流或输出状态）
4. **CH455G 位数**：连接几位数码管？（1~4位）
5. **短按按钮新功能**：原短按功能为切换电压，新硬件下按钮短按是否改为切换输出使能？

---

## 九、验证计划

1. 编译通过（`pio run`），无编译错误
2. 烧录后串口日志正常，不出现 `VoltageCtl` 相关输出
3. Web 界面访问 `/`，电压选择 UI 已消失，输出使能开关正常显示
4. 拨动输出使能开关，`OUTPUT_EN_PIN`（GPIO16）电平变化正确（用万用表确认）
5. CH455G 数码管显示正常（亮度、数字内容正确）
6. INA219 电压/电流读取功能正常（不受影响）
