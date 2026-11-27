// =====================================================================
//  ID10 – Dashboard -> Buzzer (yêu cầu #3) – PHÁT
//  Luồng: Dashboard (nút "Nhắc nhở") -> Node-RED -> MQTT (pomodoro/cmd)
//         -> ESP32 -> module relay 5V (proposal: MOSFET D4184, mua nhầm relay) -> buzzer 5V
//  Lệnh: {"type":"beep","times":3}
//  Nối dây relay (xem docs/cam_day.md): VCC -> V5, GND -> GND, IN -> GPIO 26,
//                 COM -> V5, NO -> buzzer (+), buzzer (-) -> GND. KHÔNG dùng NC.
//  Nếu đổi lại D4184: VIN+ -> V5, VIN- -> GND, OUT+/OUT- -> buzzer, TRIG -> GPIO 26 (vẫn HIGH).
// =====================================================================

// Mức điện bật buzzer: HIGH cho D4184 và relay kích mức cao (jumper H).
// Relay kích mức thấp (jumper L, hoặc không có jumper mà bật khi IN nối GND) -> đổi thành LOW.
#define BUZZER_ON_LEVEL HIGH

static const uint16_t BEEP_ON_MS = 150;
static const uint16_t BEEP_OFF_MS = 150;
static uint8_t beepLeft = 0;     // số tiếng còn phải kêu
static bool beepOn = false;
static uint32_t beepT = 0;

// Relay kích mức thấp nuôi 5V: mức HIGH của ESP32 chỉ 3,3 V, không đủ nhả relay.
// Nên khi tắt thì thả nổi chân (open-drain): không có dòng qua relay -> relay nhả chắc chắn.
static void buzzerWrite(bool on) {
  if (BUZZER_ON_LEVEL == LOW) digitalWrite(PIN_BUZZER, on ? LOW : HIGH);  // HIGH = thả nổi
  else digitalWrite(PIN_BUZZER, on ? HIGH : LOW);
}

void buzzerSetup() {
  pinMode(PIN_BUZZER, BUZZER_ON_LEVEL == LOW ? OUTPUT_OPEN_DRAIN : OUTPUT);
  buzzerWrite(false);
}

void buzzerBeep(int times) {
  beepLeft = (uint8_t)constrain(times, 1, 10);
  beepOn = false;
  beepT = millis() - BEEP_OFF_MS;  // kêu ngay tiếng đầu
  Serial.printf("Buzzer: %d tieng\n", beepLeft);
}

// Kêu/tắt theo millis() – không dùng delay() để MQTT và đồng hồ không bị đứng
void buzzerLoop() {
  if (beepLeft == 0 && !beepOn) return;
  uint32_t now = millis();
  if (beepOn) {
    if (now - beepT >= BEEP_ON_MS) {
      buzzerWrite(false);
      beepOn = false;
      beepT = now;
      beepLeft--;
    }
  } else if (beepLeft > 0 && now - beepT >= BEEP_OFF_MS) {
    buzzerWrite(true);
    beepOn = true;
    beepT = now;
  }
}
