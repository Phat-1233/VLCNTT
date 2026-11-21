// =====================================================================
//  config.h – cấu hình chung cho cả nhóm (chân, topic MQTT, trạng thái)
//  Ai sửa file này thì báo người kia, vì mọi tab đều dùng chung.
// =====================================================================
#pragma once
#include <Arduino.h>
#include <ArduinoJson.h>

#define DEVICE_ID "pomo-01"

// ----------------------------- Sơ đồ chân ----------------------------
#define PIN_OLED_SDA 21   // ID2  – OLED SSD1306 (I2C)
#define PIN_OLED_SCL 22
#define PIN_TM_CLK   18   // ID4  – màn hình 7 đoạn TM1637
#define PIN_TM_DIO   19
#define PIN_ENC_CLK  32   // ID4  – núm xoay KY-040
#define PIN_ENC_DT   33
#define PIN_ENC_SW   25   // đọc digital nên dùng chân ADC2 vẫn được
#define PIN_BUTTON   27   // ID1  – nút nhấn (INPUT_PULLUP, nút nối GND)
#define PIN_LDR      34   // ID5  – LDR. BẮT BUỘC chân ADC1 (32–39) vì ADC2 không đọc được khi bật WiFi
#define PIN_BUZZER   26   // ID10 – chân tín hiệu module MOSFET D4184

// ----------------------------- Topic MQTT ----------------------------
#define T_STATE   "pomodoro/state"    // ESP32 -> Node-RED  (ID1, ID6, ID8)
#define T_LUX     "pomodoro/lux"      // ESP32 -> Node-RED  (ID5, ID8)
#define T_SESSION "pomodoro/session"  // ESP32 -> Node-RED  (ID8)
#define T_ACK     "pomodoro/ack"      // Node-RED -> ESP32  (ID8)
#define T_CMD     "pomodoro/cmd"      // Node-RED -> ESP32  (ID2 msg/subject, ID10 beep)

// --------------------- Trạng thái đồng hồ (dùng chung) ---------------
// Khoa (ID1, ID4) GHI vào đây; Phát (OLED – ID2) chỉ ĐỌC để vẽ.
enum TimerPhase { PH_IDLE, PH_STUDY, PH_BREAK, PH_PAUSED, PH_DONE };

struct TimerState {
  TimerPhase phase = PH_IDLE;
  uint16_t studyMin = 25;        // chỉnh bằng núm xoay (ID4)
  uint16_t breakMin = 5;
  uint8_t  cycle = 0;            // chu kỳ hiện tại (1..totalCycles)
  uint8_t  totalCycles = 4;      // chỉnh bằng núm xoay (ID4)
  uint32_t remainSec = 25 * 60;  // giây còn lại của pha hiện tại
  char     subject[24] = "Chua chon mon";  // đổi từ web (ID2, type "subject")
};
extern TimerState timerState;

// ------------------ Bản ghi 1 chu kỳ học (ID1 -> ID8) -----------------
// Timer (ID1) tạo khi hết/dừng 1 pha học, hàng đợi (ID8) gửi lên pomodoro/session
// và giữ lại tới khi nhận pomodoro/ack. Xem hợp đồng v2 trong plan.md.
struct SessionRec {
  uint32_t startMs;      // millis() lúc bắt đầu (dùng khi chưa có giờ NTP)
  uint32_t startTs;      // Unix giây (UTC); 0 = chưa biết, tính lại lúc gửi
  uint32_t endTs;
  char     subject[24];
  uint8_t  cycle, total;
  uint16_t plannedMin, focusMin, pauses, pauseSec;
  bool     completed;
};

// ------------------------- Khai báo hàm các tab ------------------------
// pomodoro.ino (chung)
uint32_t nowTs();
bool timeValid();      // true khi đã có giờ NTP
bool mqttPublish(const char* topic, JsonDocument& doc);
bool mqttOnline();
// wifi_setup.ino (Khoa – ID3)
void connectWiFi();
// session_queue.ino (Khoa – ID8)
void sessionPush(const SessionRec& r);
void sessionQueueLoop();
void sessionAck(uint32_t startTs);
// timer_button.ino (Khoa – ID1)
void timerSetup();
void timerLoop();
void publishState(const char* event);
const char* phaseName(TimerPhase p);
// knob_7seg.ino (Khoa – ID4)
void knobSetup();
void knobLoop();
// ldr.ino (Khoa – ID5)
void ldrSetup();
void ldrLoop();
// oled.ino (Phát – ID2)
void oledSetup();
void oledLoop();
void oledShowMessage(const char* text, int seconds);
// buzzer.ino (Phát – ID10)
void buzzerSetup();
void buzzerLoop();
void buzzerBeep(int times);
