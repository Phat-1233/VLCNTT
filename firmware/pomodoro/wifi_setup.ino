// =====================================================================
//  ID3 – Cấu hình WiFi (yêu cầu #12) – KHOA
//  Hiện tại: WiFi ghi cứng trong secrets.h để dễ debug.
//  Task 4.K2: thay bằng WiFiManager: wm.autoConnect("Pomodoro-Setup")
// =====================================================================

void connectWiFi() {
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
