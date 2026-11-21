// =====================================================================
//  ID2 – Dashboard -> Màn hình OLED (chức năng cơ bản 2) – PHÁT
//  Luồng: Dashboard -> Node-RED -> MQTT (pomodoro/cmd) -> ESP32 -> OLED
//    {"type":"msg","text":"Con hoc bai nhe","sec":10}  -> hiện lời nhắn 10 giây
//    {"type":"subject","name":"Toan"}                  -> đổi môn (xử lý ở pomodoro.ino)
//  Font mặc định KHÔNG có dấu tiếng Việt -> Node-RED bỏ dấu trước khi gửi.
// =====================================================================
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

Adafruit_SSD1306 oled(128, 64, &Wire, -1);
static bool oledOk = false;
static char msgText[160];
static uint32_t msgUntil = 0;   // millis() lúc hết hiện lời nhắn (0 = không có)
static uint32_t lastDraw = 0;

void oledSetup() {
  Wire.begin(PIN_OLED_SDA, PIN_OLED_SCL);
  oledOk = oled.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  if (!oledOk) {
    Serial.println("Khong tim thay OLED (kiem tra day SDA/SCL, dia chi 0x3C)");
    return;
  }
  oled.setTextColor(SSD1306_WHITE);
  oled.setTextWrap(true);
  oled.clearDisplay();
  oled.setTextSize(2);
  oled.setCursor(4, 20);
  oled.print("POMODORO");
  oled.display();
}

void oledShowMessage(const char* text, int seconds) {
  strlcpy(msgText, text, sizeof(msgText));
  if (seconds < 1 || seconds > 60) seconds = 10;
  msgUntil = millis() + seconds * 1000UL;
  if (msgUntil == 0) msgUntil = 1;
  lastDraw = 0;  // vẽ lại ngay
  Serial.printf("OLED loi nhan: %s (%ds)\n", msgText, seconds);
}

static void drawMessage() {
  oled.setTextSize(1);
  oled.setCursor(0, 0);
  oled.print("LOI NHAN");
  oled.drawFastHLine(0, 10, 128, SSD1306_WHITE);
  oled.setCursor(0, 14);
  oled.print(msgText);
}

static void drawClock() {
  // Dòng trên: môn đang học + chấm báo kết nối MQTT
  oled.setTextSize(1);
  oled.setCursor(0, 0);
  oled.print(timerState.subject);
  if (mqttOnline()) oled.fillCircle(124, 3, 3, SSD1306_WHITE);
  else oled.drawCircle(124, 3, 3, SSD1306_WHITE);

  // Giữa: đồng hồ mm:ss
  char buf[8];
  snprintf(buf, sizeof(buf), "%02lu:%02lu",
           (unsigned long)(timerState.remainSec / 60), (unsigned long)(timerState.remainSec % 60));
  oled.setTextSize(3);
  oled.setCursor(20, 20);
  oled.print(buf);

  // Dòng dưới: pha + chu kỳ
  oled.setTextSize(1);
  oled.setCursor(0, 56);
  switch (timerState.phase) {
    case PH_STUDY:  oled.print("Dang hoc"); break;
    case PH_BREAK:  oled.print("Nghi giai lao"); break;
    case PH_PAUSED: oled.print("Tam dung"); break;
    case PH_DONE:   oled.print("Hoan thanh!"); break;
    default:        oled.print("Bam nut de bat dau"); break;
  }
  if (timerState.phase != PH_IDLE) {
    oled.setCursor(92, 56);
    oled.printf("%u/%u", timerState.cycle, timerState.totalCycles);
  }
}

void oledLoop() {
  if (!oledOk) return;
  if (millis() - lastDraw < 200) return;  // vẽ lại 5 lần/giây là đủ
  lastDraw = millis();

  oled.clearDisplay();
  if (msgUntil != 0 && (int32_t)(msgUntil - millis()) > 0) {
    drawMessage();
  } else {
    msgUntil = 0;
    drawClock();
  }
  oled.display();
}
