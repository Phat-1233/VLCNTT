// =====================================================================
//  Pomodoro Để Bàn – Nhóm 6 (Khoa 24127187, Phát 24127483)
//  File chính: WiFi + giờ NTP + MQTT (EMQX Cloud Serverless, TLS 8883) + chia lệnh.
//
//  Mỗi chức năng nằm ở một tab riêng để 2 người sửa không đụng nhau:
//    timer_button.ino  – ID1  (Khoa)  nút nhấn + máy trạng thái đồng hồ
//    session_queue.ino – ID8  (Khoa)  hàng đợi phiên học, gửi bù khi mất mạng
//    wifi_setup.ino    – ID3  (Khoa)  kết nối WiFi (sau này: WiFiManager)
//    knob_7seg.ino     – ID4  (Khoa)  núm xoay -> màn hình 7 đoạn
//    ldr.ino           – ID5  (Khoa)  cảm biến ánh sáng -> MQTT
//    oled.ino          – ID2  (Phát)  lệnh từ web -> OLED
//    buzzer.ino        – ID10 (Phát)  lệnh từ web -> buzzer
//
//  Thư viện cần cài (Arduino IDE -> Library Manager):
//    PubSubClient (Nick O'Leary), ArduinoJson (Benoit Blanchon, v7),
//    Adafruit SSD1306, Adafruit GFX Library
//    (sau này: WiFiManager (tzapu) cho ID3, TM1637 (Avishay Orpaz) cho ID4)
//  Board: "ESP32 Dev Module"
// =====================================================================
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <time.h>
#include "config.h"
#include "secrets.h"   // tạo từ secrets.example.h

TimerState timerState;
WiFiClientSecure net;
PubSubClient mqtt(net);

// CA gốc của EMQX Cloud Serverless (DigiCert Global Root G2, hết hạn 15/01/2038).
// Nguồn: https://assets.emqx.com/data/emqxsl-ca.crt – đã đối chiếu thumbprint
// DF3C24F9BFD666761B268073FE06D1CC8D4F82A4 với chuỗi chứng chỉ thật của broker (28/11/2026).
// Board chỉ kết nối khi chứng chỉ broker được CA này ký -> chặn giả mạo broker (MITM).
static const char rootCA[] PROGMEM = R"EOF(
-----BEGIN CERTIFICATE-----
MIIDjjCCAnagAwIBAgIQAzrx5qcRqaC7KGSxHQn65TANBgkqhkiG9w0BAQsFADBh
MQswCQYDVQQGEwJVUzEVMBMGA1UEChMMRGlnaUNlcnQgSW5jMRkwFwYDVQQLExB3
d3cuZGlnaWNlcnQuY29tMSAwHgYDVQQDExdEaWdpQ2VydCBHbG9iYWwgUm9vdCBH
MjAeFw0xMzA4MDExMjAwMDBaFw0zODAxMTUxMjAwMDBaMGExCzAJBgNVBAYTAlVT
MRUwEwYDVQQKEwxEaWdpQ2VydCBJbmMxGTAXBgNVBAsTEHd3dy5kaWdpY2VydC5j
b20xIDAeBgNVBAMTF0RpZ2lDZXJ0IEdsb2JhbCBSb290IEcyMIIBIjANBgkqhkiG
9w0BAQEFAAOCAQ8AMIIBCgKCAQEAuzfNNNx7a8myaJCtSnX/RrohCgiN9RlUyfuI
2/Ou8jqJkTx65qsGGmvPrC3oXgkkRLpimn7Wo6h+4FR1IAWsULecYxpsMNzaHxmx
1x7e/dfgy5SDN67sH0NO3Xss0r0upS/kqbitOtSZpLYl6ZtrAGCSYP9PIUkY92eQ
q2EGnI/yuum06ZIya7XzV+hdG82MHauVBJVJ8zUtluNJbd134/tJS7SsVQepj5Wz
tCO7TG1F8PapspUwtP1MVYwnSlcUfIKdzXOS0xZKBgyMUNGPHgm+F6HmIcr9g+UQ
vIOlCsRnKPZzFBQ9RnbDhxSJITRNrw9FDKZJobq7nMWxM4MphQIDAQABo0IwQDAP
BgNVHRMBAf8EBTADAQH/MA4GA1UdDwEB/wQEAwIBhjAdBgNVHQ4EFgQUTiJUIBiV
5uNu5g/6+rkS7QYXjzkwDQYJKoZIhvcNAQELBQADggEBAGBnKJRvDkhj6zHd6mcY
1Yl9PMWLSn/pvtsrF9+wX3N3KjITOYFnQoQj8kVnNeyIv/iPsGEMNKSuIEyExtv4
NeF22d+mQrvHRAiGfzZ0JFrabA0UWTW98kndth/Jsw1HKj2ZL7tcu7XUIOGZX1NG
Fdtom/DzMNU+MeKNhJ7jitralj41E6Vf8PlwUHBHQRFXGU7Aj64GxJUTFy8bJZ91
8rGOmaFvE7FBcf6IKshPECBV1/MUReXgRPTqh5Uykw7+U0b6LJ3/iyK5S9kJRaTe
pLiaWN0bfVKfjllDiIGknibVb63dDcY3fe0Dkhvld1927jyNxF1WW6LZZm6zNTfl
MrY=
-----END CERTIFICATE-----
)EOF";

// WiFi: connectWiFi() nằm ở tab wifi_setup.ino (Khoa – ID3)

// --------------------------- Giờ thực (NTP) --------------------------
void syncTime() {
  configTime(0, 0, "pool.ntp.org", "time.google.com");  // lưu UTC, hiển thị mới cộng +7
  uint32_t t0 = millis();
  while (time(nullptr) < 1700000000 && millis() - t0 < 10000) delay(200);
  Serial.printf("Gio NTP (Unix): %lu\n", (unsigned long)time(nullptr));
}

uint32_t nowTs() { return (uint32_t)time(nullptr); }
bool timeValid() { return time(nullptr) > 1700000000; }

// ------------------------------- MQTT --------------------------------
bool mqttOnline() { return mqtt.connected(); }

bool mqttPublish(const char* topic, JsonDocument& doc) {
  if (!mqtt.connected()) return false;
  char buf[768];
  size_t n = serializeJson(doc, buf, sizeof(buf));
  return mqtt.publish(topic, (const uint8_t*)buf, n, false);
}

// Lệnh từ web xuống hộp (topic pomodoro/cmd) – đọc trường "type"
void handleCommand(JsonDocument& doc) {
  const char* type = doc["type"] | "";
  if (strcmp(type, "msg") == 0) {                 // ID2: lời nhắn lên OLED
    oledShowMessage(doc["text"] | "", doc["sec"] | 10);
  } else if (strcmp(type, "subject") == 0) {      // ID2: đổi môn đang học
    strlcpy(timerState.subject, doc["name"] | "", sizeof(timerState.subject));
    publishState("subject");
  } else if (strcmp(type, "beep") == 0) {         // ID10: kêu còi
    buzzerBeep(doc["times"] | 3);
  } else {
    Serial.printf("Lenh khong ro type: %s\n", type);
  }
}

void onMqttMessage(char* topic, byte* payload, unsigned int len) {
  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, payload, len);
  if (err) {
    Serial.printf("JSON loi (%s) o topic %s\n", err.c_str(), topic);
    return;
  }
  Serial.printf("<- %s\n", topic);
  if (strcmp(topic, T_CMD) == 0) {
    handleCommand(doc);
  } else if (strcmp(topic, T_ACK) == 0) {
    sessionAck(doc["start_ts"] | 0);              // ID8: xóa phiên đã lưu tạm
  }
}

void mqttMaintain() {
  static uint32_t lastTry = 0;
  if (mqtt.connected() || WiFi.status() != WL_CONNECTED) return;
  if (millis() - lastTry < 5000) return;  // thử lại mỗi 5 giây, không chặn loop
  lastTry = millis();

  String clientId = String(DEVICE_ID) + "-" + String((uint32_t)ESP.getEfuseMac(), HEX);
  Serial.print("Ket noi MQTT (EMQX)... ");
  if (mqtt.connect(clientId.c_str(), MQTT_USER, MQTT_PASS)) {
    Serial.println("OK");
    mqtt.subscribe(T_CMD, 1);
    mqtt.subscribe(T_ACK, 1);
    publishState("online");
  } else {
    Serial.printf("loi rc=%d\n", mqtt.state());  // -2: sai host/mạng, 4/5: sai user/pass
  }
}

// ------------------------------- setup/loop --------------------------
void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println("\n=== Pomodoro De Ban ===");

  buzzerSetup();
  oledSetup();
  timerSetup();
  knobSetup();
  ldrSetup();

  connectWiFi();
  syncTime();

  net.setCACert(rootCA);  // TLS có kiểm tra chứng chỉ broker (Task 5.2, thay setInsecure())
  mqtt.setServer(MQTT_HOST, MQTT_PORT);
  mqtt.setBufferSize(1024);  // mặc định 256 byte -> JSON dài bị cắt mà không báo lỗi
  mqtt.setKeepAlive(30);
  mqtt.setCallback(onMqttMessage);
}

void loop() {
  mqttMaintain();
  mqtt.loop();      // phải chạy liên tục -> KHÔNG dùng delay() dài ở bất kỳ tab nào

  timerLoop();      // ID1
  sessionQueueLoop(); // ID8
  knobLoop();       // ID4
  ldrLoop();        // ID5
  oledLoop();       // ID2
  buzzerLoop();     // ID10
}
