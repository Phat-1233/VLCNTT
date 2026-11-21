// =====================================================================
//  ID5 – Cảm biến LDR -> Dashboard (yêu cầu #2) – KHOA
//  Luồng: LDR -> ESP32 -> MQTT (pomodoro/lux) -> Node-RED -> Dashboard (gauge)
// =====================================================================

void ldrSetup() {
  analogReadResolution(12);  // 0..4095
}

void ldrLoop() {
  // TODO(Khoa – ID5): mỗi 2 giây đọc analogRead(PIN_LDR) rồi gửi:
  //   {"dev":"pomo-01","ts":<nowTs()>,"lux":<giá trị 0..4095>}  lên T_LUX
  // Gợi ý:
  //   static uint32_t last = 0;
  //   if (millis() - last < 2000) return;
  //   last = millis();
  //   JsonDocument d; d["dev"] = DEVICE_ID; d["ts"] = nowTs(); d["lux"] = analogRead(PIN_LDR);
  //   mqttPublish(T_LUX, d);
}
