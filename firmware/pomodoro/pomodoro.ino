// =====================================================================
//  Pomodoro Để Bàn – Nhóm 6 (Khoa 24127187, Phát 24127483)
//  File chính: WiFi + giờ NTP + MQTT (HiveMQ Cloud, TLS) + chia lệnh.
//
//  Mỗi chức năng nằm ở một tab riêng để 2 người sửa không đụng nhau:
//    timer_button.ino  – ID1  (Khoa)  nút nhấn + máy trạng thái đồng hồ
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

// ------------------------------- WiFi --------------------------------
void connectWiFi() {
  // TODO(Khoa – ID3): thay đoạn này bằng WiFiManager: wm.autoConnect("Pomodoro-Setup")
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.print("Dang ket noi WiFi");
  uint32_t t0 = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - t0 < 20000) {
    delay(300);
    Serial.print(".");
  }
  Serial.println(WiFi.status() == WL_CONNECTED ? " OK, IP: " + WiFi.localIP().toString()
                                               : " THAT BAI (se thu lai sau)");
}

// --------------------------- Giờ thực (NTP) --------------------------
void syncTime() {
  configTime(0, 0, "pool.ntp.org", "time.google.com");  // lưu UTC, hiển thị mới cộng +7
  uint32_t t0 = millis();
  while (time(nullptr) < 1700000000 && millis() - t0 < 10000) delay(200);
  Serial.printf("Gio NTP (Unix): %lu\n", (unsigned long)time(nullptr));
}

uint32_t nowTs() { return (uint32_t)time(nullptr); }

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
    // TODO(Khoa – ID8): xóa bản ghi phiên đã lưu tạm có start_ts == doc["start_ts"]
  }
}

void mqttMaintain() {
  static uint32_t lastTry = 0;
  if (mqtt.connected() || WiFi.status() != WL_CONNECTED) return;
  if (millis() - lastTry < 5000) return;  // thử lại mỗi 5 giây, không chặn loop
  lastTry = millis();

  String clientId = String(DEVICE_ID) + "-" + String((uint32_t)ESP.getEfuseMac(), HEX);
  Serial.print("Ket noi HiveMQ... ");
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

  net.setInsecure();  // giai đoạn dev: bỏ kiểm tra chứng chỉ. Bản nộp: net.setCACert(rootCA)
  mqtt.setServer(MQTT_HOST, MQTT_PORT);
  mqtt.setBufferSize(1024);  // mặc định 256 byte -> JSON dài bị cắt mà không báo lỗi
  mqtt.setKeepAlive(30);
  mqtt.setCallback(onMqttMessage);
}

void loop() {
  mqttMaintain();
  mqtt.loop();      // phải chạy liên tục -> KHÔNG dùng delay() dài ở bất kỳ tab nào

  timerLoop();      // ID1
  knobLoop();       // ID4
  ldrLoop();        // ID5
  oledLoop();       // ID2
  buzzerLoop();     // ID10
}
