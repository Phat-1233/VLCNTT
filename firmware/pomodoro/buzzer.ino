// =====================================================================
//  ID10 – Dashboard -> Buzzer (yêu cầu #3) – PHÁT
//  Luồng: Dashboard (nút "Nhắc nhở") -> Node-RED -> MQTT (pomodoro/cmd)
//         -> ESP32 -> module MOSFET D4184 -> buzzer 5V
//  Lệnh: {"type":"beep","times":3}
//  Nối dây D4184: VIN+ -> 5V (chân VIN), VIN- -> GND, OUT+/OUT- -> buzzer,
//                 TRIG/PWM -> GPIO 26, GND module nối chung GND ESP32.
// =====================================================================

static const uint16_t BEEP_ON_MS = 150;
static const uint16_t BEEP_OFF_MS = 150;
static uint8_t beepLeft = 0;     // số tiếng còn phải kêu
static bool beepOn = false;
static uint32_t beepT = 0;

void buzzerSetup() {
  pinMode(PIN_BUZZER, OUTPUT);
  digitalWrite(PIN_BUZZER, LOW);
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
      digitalWrite(PIN_BUZZER, LOW);
      beepOn = false;
      beepT = now;
      beepLeft--;
    }
  } else if (beepLeft > 0 && now - beepT >= BEEP_OFF_MS) {
    digitalWrite(PIN_BUZZER, HIGH);
    beepOn = true;
    beepT = now;
  }
}
