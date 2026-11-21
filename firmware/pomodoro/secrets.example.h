// Sao chép file này thành secrets.h rồi điền thông tin thật.
// secrets.h đã nằm trong .gitignore – KHÔNG đẩy mật khẩu lên GitHub.
#pragma once

// WiFi tạm thời (giai đoạn 1). Khi làm xong ID3 (WiFiManager) thì bỏ 2 dòng này.
#define WIFI_SSID "TenWiFiNha"
#define WIFI_PASS "MatKhauWiFi"

// HiveMQ Cloud: Clusters -> Overview -> URL ; Access Management -> tạo user "esp32"
#define MQTT_HOST "xxxxxxxxxxxxxxxx.s1.eu.hivemq.cloud"
#define MQTT_PORT 8883
#define MQTT_USER "esp32"
#define MQTT_PASS "MatKhauMQTT"
