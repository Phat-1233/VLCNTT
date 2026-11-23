// =====================================================================
//  ID5 – Cảm biến LDR -> Dashboard (yêu cầu #2) – KHOA
//  Luồng: LDR -> ESP32 -> MQTT (pomodoro/lux) -> Node-RED -> Dashboard (gauge)
// =====================================================================

void ldrSetup() {
  analogReadResolution(12);  // 0..4095
}

// Mỗi 2 giây: trung bình 8 lần đọc ADC (chân AO của module) -> {"dev","ts","lux":0..4095} lên T_LUX.
// Đổi ra % và so ngưỡng làm ở Node-RED (tab Khoa), firmware chỉ gửi số thô.
void ldrLoop() {
  static uint32_t last = 0;
  if (millis() - last < 2000) return;
  last = millis();
  uint32_t sum = 0;
  for (int i = 0; i < 8; i++) sum += analogRead(PIN_LDR);
  JsonDocument d;
  d["dev"] = DEVICE_ID;
  d["ts"] = nowTs();
  d["lux"] = sum / 8;
  mqttPublish(T_LUX, d);
}
