// =====================================================================
//  ID3 – Cấu hình WiFi bằng điện thoại (yêu cầu #12) – KHOA
//  WiFiManager: có WiFi đã lưu -> tự vào. Chưa có / vào không được -> hộp phát WiFi
//  "Pomodoro-Setup", điện thoại kết nối rồi mở 192.168.4.1 để chọn WiFi + nhập mật khẩu.
//  Giữ nút SW của núm xoay lúc cắm điện = xóa WiFi đã lưu, mở lại trang cấu hình.
//  Hết 180 s không ai cấu hình -> chạy offline (đồng hồ, núm, 7 đoạn vẫn dùng được).
// =====================================================================
#include <WiFiManager.h>

static const char* AP_NAME = "Pomodoro-Setup";

void connectWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);     // mất WiFi giữa chừng thì tự vào lại WiFi đã lưu
  WiFiManager wm;

  if (digitalRead(PIN_ENC_SW) == LOW) {   // knobSetup() đã bật INPUT_PULLUP cho SW
    Serial.println("[ID3] Giu nut num xoay luc cam dien -> xoa WiFi da luu");
    wm.resetSettings();
  }

  wm.setConnectTimeout(20);        // thử WiFi đã lưu tối đa 20 s
  wm.setConfigPortalTimeout(180);  // trang cấu hình mở tối đa 180 s
  wm.setAPCallback([](WiFiManager*) {
    Serial.printf("[ID3] Mo trang cau hinh: ket noi WiFi \"%s\" roi mo 192.168.4.1 (180 s)\n", AP_NAME);
    oledShowMessage("Ket noi WiFi Pomodoro-Setup, mo 192.168.4.1", 60);
    oledLoop();                    // vẽ 1 lần vì autoConnect() chặn loop() trong lúc chờ
  });

  // ponytail: autoConnect() chặn tối đa ~200 s lúc khởi động (đồng hồ chưa chạy trong lúc đó).
  // Muốn đồng hồ chạy song song thì đổi sang setConfigPortalBlocking(false) + wm.process() trong loop().
  Serial.println("Dang ket noi WiFi...");
  if (wm.autoConnect(AP_NAME)) {
    Serial.println("WiFi OK: " + WiFi.SSID() + ", IP: " + WiFi.localIP().toString());
  } else {
    Serial.println("WiFi THAT BAI -> chay offline (giu nut num xoay luc cam dien de cau hinh lai)");
  }
}
