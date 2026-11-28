// Sao chép file này thành secrets.h rồi điền thông tin thật.
// secrets.h đã nằm trong .gitignore – KHÔNG đẩy mật khẩu lên GitHub.
#pragma once

// WiFi KHÔNG ghi ở đây nữa: cấu hình bằng điện thoại qua WiFi "Pomodoro-Setup" (ID3 – WiFiManager).

// EMQX Cloud Serverless: Overview -> Address (chỉ tên miền, không có mqtts:// và :8883)
//                        Access Control -> Authentication -> Add user "esp32"
#define MQTT_HOST "xxxxxxxx.ala.asia-southeast1.emqxsl.com"
#define MQTT_PORT 8883
#define MQTT_USER "esp32"
#define MQTT_PASS "MatKhauMQTT"
