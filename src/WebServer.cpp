#include "WebServer.h"
#include <ArduinoJson.h>
#include <FS.h>  // 添加SPIFFS支持

#define BUILD_DATE_STR __DATE__ " " __TIME__

// 定义静态成员变量

const char WebServer::INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <title>ESP8266 Power Supply</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <script src="https://cdn.jsdelivr.net/npm/chart.js"></script>
    <style>
        :root {
            --primary-color: #2196F3;
            --primary-dark: #1976D2;
            --success-color: #4CAF50;
            --error-color: #f44336;
            --text-color: #333;
            --text-light: #666;
            --background: #f5f5f5;
            --card-background: #ffffff;
            --border-radius: 8px;
            --shadow: 0 2px 10px rgba(0,0,0,0.1);
        }

        * {
            box-sizing: border-box;
            margin: 0;
            padding: 0;
        }

        body {
            font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
            background: var(--background);
            color: var(--text-color);
            line-height: 1.6;
            padding: 20px;
        }

        .container {
            max-width: 800px;
            margin: 0 auto;
            padding: 20px;
        }

        .header {
            text-align: center;
            margin-bottom: 30px;
            animation: slideDown 0.5s ease-out;
        }

        @keyframes slideDown {
            from {
                opacity: 0;
                transform: translateY(-20px);
            }
            to {
                opacity: 1;
                transform: translateY(0);
            }
        }

        h1 {
            color: var(--primary-color);
            font-size: 28px;
            margin-bottom: 10px;
        }

        .status {
            color: var(--text-light);
            font-size: 16px;
        }

        .switch {
            position: relative;
            display: inline-block;
            width: 60px;
            height: 34px;
        }

        .switch input {
            opacity: 0;
            width: 0;
            height: 0;
        }

        .slider {
            position: absolute;
            cursor: pointer;
            top: 0;
            left: 0;
            right: 0;
            bottom: 0;
            background-color: #ccc;
            transition: .4s;
            border-radius: 34px;
        }

        .slider:before {
            position: absolute;
            content: "";
            height: 26px;
            width: 26px;
            left: 4px;
            bottom: 4px;
            background-color: white;
            transition: .4s;
            border-radius: 50%;
        }

        input:checked + .slider {
            background-color: var(--success-color);
        }

        input:checked + .slider:before {
            transform: translateX(26px);
        }

        .power-info {
            background: var(--card-background);
            border-radius: var(--border-radius);
            padding: 20px;
            box-shadow: var(--shadow);
            margin-top: 30px;
            animation: fadeIn 0.5s ease-out;
        }

        @keyframes fadeIn {
            from { opacity: 0; }
            to { opacity: 1; }
        }

        .power-value {
            font-size: 32px;
            font-weight: 600;
            color: var(--primary-color);
            margin: 10px 0;
        }

        .power-label {
            font-size: 14px;
            color: var(--text-light);
            margin: 5px 0;
        }

        .power-metrics {
            display: grid;
            grid-template-columns: repeat(3, 1fr);
            gap: 15px;
            margin-top: 20px;
        }

        .metric-item {
            text-align: center;
            padding: 15px;
            background: #f8f9fa;
            border-radius: var(--border-radius);
        }

        .metric-item .power-value {
            font-size: 28px;
            margin: 5px 0;
        }

        .metric-item .power-label {
            font-size: 14px;
            color: var(--text-light);
        }

        .chart-btn {
            display: inline-flex;
            align-items: center;
            justify-content: center;
            padding: 8px 16px;
            background: var(--primary-color);
            color: white;
            border: none;
            border-radius: var(--border-radius);
            cursor: pointer;
            font-size: 14px;
            margin-top: 15px;
            transition: all 0.3s ease;
        }

        .chart-btn:hover {
            background: var(--primary-dark);
            transform: translateY(-2px);
            box-shadow: 0 4px 8px rgba(0,0,0,0.1);
        }

        .chart-btn i {
            margin-right: 8px;
        }

        .voltage-value {
            color: #dc3545;
        }

        .config-link {
            display: inline-block;
            margin-top: 20px;
            padding: 10px 20px;
            background: var(--primary-color);
            color: white;
            text-decoration: none;
            border-radius: var(--border-radius);
            transition: background 0.3s ease;
        }

        .config-link:hover {
            background: var(--primary-dark);
        }

        .footer {
            margin-top: 30px;
            padding-top: 20px;
            border-top: 1px solid #eee;
            display: flex;
            justify-content: space-between;
            align-items: center;
            color: var(--text-light);
            font-size: 14px;
        }

        .upgrade-link {
            color: var(--primary-color);
            text-decoration: none;
            display: flex;
            align-items: center;
            gap: 5px;
        }

        .upgrade-link:hover {
            color: var(--primary-dark);
            text-decoration: underline;
        }

        @media (max-width: 600px) {
            .container {
                padding: 10px;
            }

            h1 {
                font-size: 24px;
            }

            .power-metrics {
                grid-template-columns: 1fr;
            }
        }

        .modal {
            display: none;
            position: fixed;
            top: 0;
            left: 0;
            width: 100%;
            height: 100%;
            background-color: rgba(0,0,0,0.5);
            z-index: 1000;
        }

        .modal-content {
            position: relative;
            background-color: white;
            margin: 10% auto;
            padding: 20px;
            width: 80%;
            max-width: 800px;
            border-radius: var(--border-radius);
            box-shadow: var(--shadow);
        }

        .close {
            position: absolute;
            right: 20px;
            top: 10px;
            font-size: 28px;
            font-weight: bold;
            cursor: pointer;
        }

        .chart-container {
            margin-top: 20px;
            height: 300px;
        }

        .chart-controls {
            margin: 15px 0;
            text-align: right;
        }

        .chart-controls select {
            padding: 5px 10px;
            border: 1px solid #ddd;
            border-radius: 4px;
            background-color: white;
            font-size: 14px;
        }

        .chart-controls select:focus {
            outline: none;
            border-color: var(--primary-color);
        }
    </style>
    <link href="https://cdnjs.cloudflare.com/ajax/libs/font-awesome/5.15.4/css/all.min.css" rel="stylesheet">
</head>
<body>
    <div class="container">
        <div class="header">
            <h1>ESP8266 Power Supply</h1>
            <div class="status">Status: Connected</div>
        </div>

        <div class="power-info power-monitor">
            <h2>Power Monitoring</h2>
            <div class="power-metrics">
                <div class="metric-item">
                    <div class="power-label">Power</div>
                    <div class="power-value" id="power1">0.00 W</div>
                </div>
                <div class="metric-item">
                    <div class="power-label">Current</div>
                    <div class="power-value" id="current1">0.00 mA</div>
                </div>
                <div class="metric-item">
                    <div class="power-label">Voltage</div>
                    <div class="power-value voltage-value" id="voltage1">0.00 V</div>
                </div>
            </div>
            <div style="text-align: center;">
                <button class="chart-btn" onclick="showPowerChart(0)">
                    <i class="fas fa-chart-line"></i>
                    Show Power Chart
                </button>
            </div>
        </div>

        <div class="power-info output-control">
            <h2>Output Control</h2>
            <div style="text-align:center; padding:20px;">
                <label class="switch">
                    <input type="checkbox" id="outputEnable" onchange="setOutputEnable(this.checked)">
                    <span class="slider"></span>
                </label>
                <div class="power-label" style="margin-top:12px; font-size:16px;" id="outputStatus">Output: OFF</div>
            </div>
        </div>

        <a href="/config" class="config-link">
            <i class="fas fa-cog"></i> WiFi Configuration
        </a>

        <div class="footer">
            <div class="build-date">Build: <span id="buildDate">Loading...</span></div>
            <a href="/upgrade" class="upgrade-link">
                <i class="fas fa-upload"></i> Upgrade
            </a>
        </div>
    </div>

    <div id="powerChartModal" class="modal">
        <div class="modal-content">
            <span class="close">&times;</span>
            <h3 id="modalTitle">Power Monitor Chart</h3>
            <div class="chart-controls">
                <select id="dataTypeSelect" onchange="updateChartType()">
                    <option value="power">Power (W)</option>
                    <option value="current">Current (mA)</option>
                    <option value="voltage">Voltage (V)</option>
                </select>
            </div>
            <div class="chart-container">
                <canvas id="powerChart"></canvas>
            </div>
        </div>
    </div>

    <script>
        let ws;
        let powerChart;
        let chartData = {
            labels: [],
            current: [],
            voltage: [],
            power: []
        };
        const maxDataPoints = 60;
        let currentChannelIndex = 0;
        let currentDataType = 'power';

        function updatePowerData() {
            fetch('/power')
                .then(response => response.json())
                .then(data => {
                    const channel = data['channel1'];
                    if (channel) {
                        document.getElementById('power1').textContent = channel.power.toFixed(2) + ' W';
                        document.getElementById('current1').textContent = channel.current.toFixed(2) + ' mA';
                        document.getElementById('voltage1').textContent = channel.voltage.toFixed(2) + ' V';
                        
                        // 如果图表正在显示，更新图表数据
                        if (powerChart && currentChannelIndex === 0) {
                            updateChartData(channel);
                        }
                    }
                })
                .catch(error => console.error('Error updating power data:', error));
        }

        // 显示功率图表
        function showPowerChart(channelIndex) {
            currentChannelIndex = channelIndex;
            const modal = document.getElementById('powerChartModal');
            const modalTitle = document.getElementById('modalTitle');
            modalTitle.textContent = `Power Monitor Chart`;
            
            // 初始化图表
            if (!powerChart) {
                const ctx = document.getElementById('powerChart').getContext('2d');
                powerChart = new Chart(ctx, {
                    type: 'line',
                    data: {
                        labels: [],
                        datasets: [{
                            label: 'Power (W)',
                            data: [],
                            borderColor: 'rgb(54, 162, 235)',
                            tension: 0.1
                        }]
                    },
                    options: {
                        responsive: true,
                        maintainAspectRatio: false,
                        scales: {
                            y: {
                                beginAtZero: true
                            }
                        }
                    }
                });
            }

            // 清空现有数据
            chartData.labels = [];
            chartData.current = [];
            chartData.voltage = [];
            chartData.power = [];
            
            // 显示模态框
            modal.style.display = 'block';
        }

        function updateChartType() {
            const select = document.getElementById('dataTypeSelect');
            currentDataType = select.value;
            
            if (powerChart) {
                const labels = {
                    'power': 'Power (W)',
                    'current': 'Current (mA)',
                    'voltage': 'Voltage (V)'
                };
                
                powerChart.data.datasets[0].label = labels[currentDataType];
                powerChart.data.datasets[0].data = chartData[currentDataType];
                powerChart.update();
            }
        }

        function updateChartData(data) {
            if (!powerChart) return;

            const timestamp = new Date().toLocaleTimeString();
            
            // 添加新数据
            chartData.labels.push(timestamp);
            chartData.current.push(data.current);
            chartData.voltage.push(data.voltage);
            chartData.power.push(data.power);

            // 保持最近60个数据点
            if (chartData.labels.length > maxDataPoints) {
                chartData.labels.shift();
                chartData.current.shift();
                chartData.voltage.shift();
                chartData.power.shift();
            }

            // 更新图表
            powerChart.data.labels = chartData.labels;
            powerChart.data.datasets[0].data = chartData[currentDataType];
            powerChart.update();
        }

        // 关闭模态框
        document.querySelectorAll('.close').forEach(closeBtn => {
            closeBtn.onclick = function() {
                document.getElementById('powerChartModal').style.display = 'none';
            }
        });

        // 点击模态框外部关闭
        window.onclick = function(event) {
            const powerModal = document.getElementById('powerChartModal');
            if (event.target == powerModal) {
                powerModal.style.display = 'none';
            }
        }

        // 为功率监测项添加点击事件
        document.addEventListener('DOMContentLoaded', function() {
            const powerItems = document.querySelectorAll('.power-monitor .power-item');
            powerItems.forEach((item, index) => {
                item.classList.add('monitor');
                item.addEventListener('click', () => showPowerChart(index));
            });

            // 初始更新
            updatePowerData();
            updateBuildDate();
            checkOutput();
        });

        // 定期更新数据
        setInterval(updatePowerData, 1000);   // 每秒更新电源数据

        // 更新构建日期
        function updateBuildDate() {
            fetch('/status')
                .then(response => response.json())
                .then(data => {
                    if (data.build_date) {
                        document.getElementById('buildDate').textContent = data.build_date;
                    }
                })
                .catch(error => console.error('Error updating build date:', error));
        }

        function updateOutputStatus(enabled) {
            document.getElementById('outputEnable').checked = enabled;
            document.getElementById('outputStatus').textContent = enabled ? 'Output: ON' : 'Output: OFF';
            document.getElementById('outputStatus').style.color = enabled ? '#4CAF50' : '#666';
        }

        function setOutputEnable(enabled) {
            fetch('/output?enable=' + (enabled ? '1' : '0'))
                .then(response => response.json())
                .then(data => updateOutputStatus(data.enabled))
                .catch(error => console.error('Error setting output:', error));
        }

        function checkOutput() {
            fetch('/output')
                .then(response => response.json())
                .then(data => updateOutputStatus(data.enabled))
                .catch(error => console.error('Error checking output:', error));
        }

        // 每2秒同步一次输出状态
        setInterval(checkOutput, 2000);
    </script>
</body>
</html>
)rawliteral";

// extern 访问 main.cpp 中的输出使能状态
extern bool outputEnabled;
extern void setOutputEnable(bool en);

WebServer::WebServer(EspSmartWifi& wifi, EasyLed& led, Display& display)
    : server(80), wifi(wifi), led(led), display(display) {
    Serial.println("\n=== WebServer Initialization ===");
    
    // 初始化SPIFFS
    Serial.println("Initializing SPIFFS...");
    if (!SPIFFS.begin()) {
        Serial.println("ERROR: SPIFFS initialization failed!");
        return;
    }
    Serial.println("SUCCESS: SPIFFS initialized successfully");
    
    // 检查文件系统状态
    FSInfo fs_info;
    SPIFFS.info(fs_info);
    Serial.print("Total bytes: ");
    Serial.println(fs_info.totalBytes);
    Serial.print("Used bytes: ");
    Serial.println(fs_info.usedBytes);
    

    
    // 初始化按钮引脚
    Serial.println("\nInitializing button pin...");
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    Serial.println("Button pin initialized with internal pull-up");
    
    // 初始化PowerMonitor
    Serial.println("\nInitializing PowerMonitor...");
    if (initPowerMonitor()) {
        Serial.println("SUCCESS: PowerMonitor initialized successfully");
    } else {
        Serial.println("ERROR: PowerMonitor initialization failed!");
    }
    

    
    Serial.println("=== WebServer Initialization Complete ===\n");
}

bool WebServer::initPowerMonitor() {
    return powerMonitor.begin();
}

void WebServer::begin() {
    // 先停止服务器
    server.stop();
    
    // // 重新初始化路由
    // if (wifi.isAPMode())
    // {
    //     server.on("/", HTTP_GET, [this]() { HandleConfigRoot(); });
    //     server.on("/config/data", HTTP_POST, [this]() { HandleConfigSave(); });
    //     server.on("/config/data", HTTP_GET, [this]() { handleGetConfigData(); });  // 添加新的配置数据接口
    // }
    // else
    // {
    //     server.on("/", HTTP_GET, [this]() { handleRoot(); });
    //     server.on("/config", HTTP_GET, [this]() { HandleConfigRoot(); });
    //     server.on("/config/data", HTTP_POST, [this]() { HandleConfigSave(); });
    //     server.on("/config/data", HTTP_GET, [this]() { handleGetConfigData(); });  // 添加新的配置数据接口
    // }
    server.on("/", HTTP_GET, [this]() { handleRoot(); });
    server.on("/config", HTTP_GET, [this]() { HandleConfigRoot(); });
    server.on("/config/data", HTTP_POST, [this]() { HandleConfigSave(); });
    server.on("/config/data", HTTP_GET, [this]() { handleGetConfigData(); });  // 添加新的配置数据接口  
    
    server.on("/status", HTTP_GET, [this]() { handleStatus(); });
    server.on("/power",   HTTP_GET,  [this]() { handlePower(); });
    server.on("/output",  HTTP_GET,  [this]() { handleOutput(); });  // 输出使能控制
    server.on("/restart", HTTP_POST, [this]() { handleRestart(); });
    server.on("/upgrade", HTTP_GET, [this]() { handleUpgrade(); });
    server.on("/update", HTTP_POST, [this]() { handleUpdate(); }, [this]() { handleUpdateUpload(); });
    

    
    server.onNotFound([this]() { handleNotFound(); });
    
    // 启动服务器
    server.begin();
    Serial.println("HTTP server started");
    
    // 初始化时间
    initTime();
}

void WebServer::handleClient() {
    server.handleClient();

}

void WebServer::stop() {
    server.stop();
    Serial.println("HTTP server stopped");
}


void WebServer::handlePower() {
    StaticJsonDocument<512> doc;
    
    if (powerMonitor.isInitialized()) {
        for (int i = 0; i < 3; i++) {
            JsonObject channel = doc.createNestedObject("channel" + String(i + 1));
            channel["current"] = powerMonitor.getCurrent_mA();
            channel["voltage"] = powerMonitor.getBusVoltage_V();
            channel["power"] = powerMonitor.getPower_mW() / 1000.0;  // 转换为瓦特
        }
    }
    
    String response;
    serializeJson(doc, response);
    server.send(200, "application/json", response);
}

void WebServer::handleRoot() {
    Serial.println("Handling root request");
    
    // 检查 INDEX_HTML 是否有效
    if (strlen(INDEX_HTML) == 0) {
        Serial.println("ERROR: INDEX_HTML is empty!");
        server.send(500, "text/plain", "Internal Server Error");
        return;
    }
    
    // 发送 HTML 内容
    server.send(200, "text/html", INDEX_HTML);  // 直接使用 INDEX_HTML，不使用 FPSTR
    Serial.println("HTML content sent successfully");
}

void WebServer::handleStatus() {
    StaticJsonDocument<512> doc;
    
    // WiFi status
    doc["wifi"]["connected"] = WiFi.status() == WL_CONNECTED;
    doc["wifi"]["ssid"] = WiFi.SSID();
    doc["wifi"]["rssi"] = WiFi.RSSI();
    doc["wifi"]["ip"] = WiFi.localIP().toString();
    

    // Build date
    doc["build_date"] = __DATE__ " " __TIME__;
    
    String response;
    serializeJson(doc, response);
    server.send(200, "application/json", response);
}


void WebServer::handleRestart() {
    server.send(200, "text/plain", "Restarting...");
    delay(1000);
    ESP.restart();
}


void WebServer::HandleConfigRoot()
{
    String html = R"(
<!DOCTYPE html>
<html>
<head>
    <title>ESP8266 WiFi Configuration</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
        body { font-family: Arial, sans-serif; margin: 0; padding: 20px; background: #f0f0f0; }
        .container { max-width: 400px; margin: 0 auto; background: white; padding: 20px; border-radius: 5px; box-shadow: 0 2px 5px rgba(0,0,0,0.1); }
        h1 { text-align: center; color: #333; }
        .form-group { margin-bottom: 15px; }
        label { display: block; margin-bottom: 5px; color: #666; }
        input[type="text"], input[type="password"] { width: 100%; padding: 8px; border: 1px solid #ddd; border-radius: 4px; box-sizing: border-box; }
        button { width: 100%; padding: 10px; background: #4CAF50; color: white; border: none; border-radius: 4px; cursor: pointer; }
        button:hover { background: #45a049; }
        .status { margin-top: 20px; padding: 10px; border-radius: 4px; }
        .success { background: #dff0d8; color: #3c763d; }
        .error { background: #f2dede; color: #a94442; }
        .help-text { font-size: 12px; color: #666; margin-top: 5px; }
    </style>
</head>
<body>
    <div class="container">
        <h1>WiFi Configuration</h1>
        <form action="/config/data" method="post">
            <div class="form-group">
                <label for="ssid">WiFi SSID:</label>
                <input type="text" id="ssid" name="ssid" required>
            </div>
            <div class="form-group">
                <label for="passwd">WiFi Password:</label>
                <input type="password" id="passwd" name="passwd" required>
            </div>
            <div class="form-group">
                <label for="server">MQTT Server:</label>
                <input type="text" id="server" name="server" placeholder="mqtt://username:passwd@mqtt.server" required>
                <div class="help-text">Format: mqtt://username:passwd@mqtt.server</div>
            </div>
            <div class="form-group">
                <label for="topic">MQTT Topic:</label>
                <input type="text" id="topic" name="topic" placeholder="/espRouterPower/power" required>
                <div class="help-text">MQTT topic for power data publishing</div>
            </div>
            <button type="submit">Save Configuration</button>
        </form>
    </div>
    <script>
        // 页面加载完成后获取配置
        window.onload = function() {
            fetch('/config/data')
                .then(response => response.json())
                .then(config => {
                    document.getElementById('ssid').value = config.ssid || '';
                    document.getElementById('passwd').value = config.passwd || '';
                    document.getElementById('server').value = config.server || '';
                    document.getElementById('topic').value = config.topic || '';
                })
                .catch(error => console.error('Error loading config:', error));
        };
    </script>
</body>
</html>
)";
    server.send(200, "text/html", html);
}

void WebServer::HandleConfigSave()
{
    if (!server.hasArg("ssid") || !server.hasArg("passwd") || 
        !server.hasArg("server") || !server.hasArg("topic")) {
        server.send(400, "text/plain", "Missing parameters");
        return;
    }
    Config config;
    config.SSID = server.arg("ssid");
    config.Passwd = server.arg("passwd");
    config.Server = server.arg("server");
    config.Topic = server.arg("topic");
    
    if (wifi.SaveConfig(config)) {
        String html = R"(
<!DOCTYPE html>
<html>
<head>
    <title>Configuration Saved</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
        body { font-family: Arial, sans-serif; margin: 0; padding: 20px; background: #f0f0f0; }
        .container { max-width: 400px; margin: 0 auto; background: white; padding: 20px; border-radius: 5px; box-shadow: 0 2px 5px rgba(0,0,0,0.1); }
        h1 { text-align: center; color: #333; }
        .message { text-align: center; margin: 20px 0; }
        .restart { text-align: center; }
        .restart a { display: inline-block; padding: 10px 20px; background: #4CAF50; color: white; text-decoration: none; border-radius: 4px; }
        .restart a:hover { background: #45a049; }
    </style>
    <script>
        setTimeout(function() {
            window.location.href = '/';
        }, 5000);
    </script>
</head>
<body>
    <div class="container">
        <h1>Configuration Saved</h1>
        <div class="message">
            <p>WiFi configuration has been saved successfully.</p>
            <p>The device will now attempt to connect to the configured WiFi network.</p>
            <p>This page will refresh in 5 seconds...</p>
        </div>
    </div>
</body>
</html>
)";
        server.send(200, "text/html", html);
        
        // 延迟重启，让页面有时间显示
        delay(1000);
        ESP.restart();
    } else {
        server.send(500, "text/plain", "Failed to save configuration");
    }
}


void WebServer::handleUpgrade() {
    String html = R"(
        <!DOCTYPE html>
        <html>
        <head>
            <title>Firmware Upgrade</title>
            <meta name="viewport" content="width=device-width, initial-scale=1">
            <style>
                body {
                    font-family: Arial, sans-serif;
                    max-width: 800px;
                    margin: 0 auto;
                    padding: 20px;
                }
                .upload-form {
                    background: #f8f9fa;
                    padding: 20px;
                    border-radius: 8px;
                    margin-top: 20px;
                }
                .btn {
                    background: #2196F3;
                    color: white;
                    padding: 10px 20px;
                    border: none;
                    border-radius: 4px;
                    cursor: pointer;
                }
                .btn:hover {
                    background: #1976D2;
                }
                .progress {
                    margin-top: 20px;
                    display: none;
                }
                .progress-bar {
                    width: 100%;
                    height: 20px;
                    background: #eee;
                    border-radius: 10px;
                    overflow: hidden;
                }
                .progress-bar-fill {
                    height: 100%;
                    background: #4CAF50;
                    width: 0%;
                    transition: width 0.3s ease;
                }
            </style>
        </head>
        <body>
            <h1>Firmware Upgrade</h1>
            <div class="upload-form">
                <form method="POST" action="/update" enctype="multipart/form-data">
                    <input type="file" name="firmware" accept=".bin">
                    <button type="submit" class="btn">Upload Firmware</button>
                </form>
            </div>
            <div class="progress">
                <div class="progress-bar">
                    <div class="progress-bar-fill"></div>
                </div>
                <div id="status"></div>
            </div>
            <script>
                document.querySelector('form').onsubmit = function() {
                    document.querySelector('.progress').style.display = 'block';
                    var xhr = new XMLHttpRequest();
                    var formData = new FormData(this);
                    
                    xhr.upload.addEventListener('progress', function(e) {
                        if (e.lengthComputable) {
                            var percent = (e.loaded / e.total) * 100;
                            document.querySelector('.progress-bar-fill').style.width = percent + '%';
                        }
                    });
                    
                    xhr.onreadystatechange = function() {
                        if (xhr.readyState === 4) {
                            if (xhr.status === 200) {
                                document.getElementById('status').textContent = 'Upload complete! Rebooting...';
                                setTimeout(function() {
                                    window.location.href = '/';
                                }, 5000);
                            } else {
                                document.getElementById('status').textContent = 'Upload failed: ' + xhr.statusText;
                            }
                        }
                    };
                    
                    xhr.open('POST', '/update', true);
                    xhr.send(formData);
                    return false;
                };
            </script>
        </body>
        </html>
    )";
    server.send(200, "text/html", html);
}

void WebServer::handleUpdate() {
    server.send(200, "text/plain", "Update complete. Rebooting...");
    delay(1000);
    ESP.restart();
}

void WebServer::handleUpdateUpload() {
    HTTPUpload& upload = server.upload();
    
    if (upload.status == UPLOAD_FILE_START) {
        Serial.println("Update: " + upload.filename);
        uint32_t maxSketchSpace = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
        if (!Update.begin(maxSketchSpace)) {
            Serial.println("Update begin failed");
            server.send(500, "text/plain", "Update begin failed");
            return;
        }
    } else if (upload.status == UPLOAD_FILE_WRITE) {
        if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
            Serial.println("Update write failed");
            server.send(500, "text/plain", "Update write failed");
            return;
        }
    } else if (upload.status == UPLOAD_FILE_END) {
        if (Update.end(true)) {
            Serial.println("Update complete");
        } else {
            Serial.println("Update failed");
            server.send(500, "text/plain", "Update failed");
        }
    }
}

void WebServer::handleNotFound() {
    Serial.print("404 Not Found: ");
    Serial.println(server.uri());
    server.send(404, "text/plain", "Not found");
}

void WebServer::handleButtonPress() {
  

}

void WebServer::initTime() {
    configTime(8 * 3600, 0, "pool.ntp.org", "time.nist.gov");
    Serial.println("Waiting for NTP time sync...");
    delay(1000);
    while (!time(nullptr)) {
        Serial.print(".");
        delay(100);
    }
    Serial.println("\nTime synchronized");
}

void WebServer::handleGetConfigData() {
    StaticJsonDocument<512> doc;
    const Config& config = wifi.getConfig();
    
    doc["ssid"] = config.SSID;
    doc["passwd"] = config.Passwd;
    doc["server"] = config.Server;
    doc["topic"] = config.Topic;
    
    String response;
    serializeJson(doc, response);
    server.send(200, "application/json", response);
}

void WebServer::handleOutput() {
    StaticJsonDocument<64> doc;

    if (server.hasArg("enable")) {
        bool en = server.arg("enable").toInt() != 0;
        setOutputEnable(en);
        Serial.printf("WebServer: output %s\n", en ? "enabled" : "disabled");
    }

    doc["enabled"] = outputEnabled;
    String response;
    serializeJson(doc, response);
    server.send(200, "application/json", response);
}