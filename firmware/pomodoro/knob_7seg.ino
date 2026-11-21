// =====================================================================
//  ID4 – Núm xoay -> Màn hình 7 đoạn (yêu cầu #1) – KHOA
//  Luồng: Núm xoay KY-040 -> ESP32 -> TM1637 (chạy hoàn toàn trên hộp)
//  Cần cài thư viện: TM1637 (Avishay Orpaz) -> #include <TM1637Display.h>
// =====================================================================

void knobSetup() {
  pinMode(PIN_ENC_CLK, INPUT);
  pinMode(PIN_ENC_DT, INPUT);
  pinMode(PIN_ENC_SW, INPUT_PULLUP);
  // TODO(Khoa): khởi tạo TM1637Display display(PIN_TM_CLK, PIN_TM_DIO); display.setBrightness(5);
}

void knobLoop() {
  // TODO(Khoa – ID4):
  //  - Chỉ cho chỉnh khi timerState.phase == PH_IDLE
  //  - Vặn: đổi timerState.studyMin (5..60) hoặc totalCycles (1..8)
  //  - Nhấn SW: chuyển giữa chế độ chỉnh "phút" và "chu kỳ"
  //  - Hiện lên 7 đoạn: "25" hoặc "C 3"; khi đang học thì hiện mm:ss còn lại
}
