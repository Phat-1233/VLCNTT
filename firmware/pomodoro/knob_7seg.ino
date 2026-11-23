// =====================================================================
//  ID4 – Núm xoay -> Màn hình 7 đoạn (yêu cầu #1) – KHOA
//  Luồng: Núm xoay KY-040 -> ESP32 -> TM1637 (chạy hoàn toàn trên hộp, không cần mạng)
//  Lúc chờ (idle/done): vặn = chỉnh, nhấn SW = đổi chế độ "phút" <-> "chu kỳ".
//  Đang chạy: hiện mm:ss còn lại, vặn núm không có tác dụng.
// =====================================================================
#include <TM1637Display.h>

static TM1637Display seg(PIN_TM_CLK, PIN_TM_DIO);
static bool editCycles = false;                 // false = chỉnh phút, true = chỉnh số chu kỳ
static bool clkLast = HIGH, swRaw = HIGH, swStable = HIGH;
static uint32_t swChange = 0;
static int32_t shown = -1;                      // giá trị đang hiện, tránh vẽ lại liên tục

static const uint8_t CHAR_C = SEG_A | SEG_D | SEG_E | SEG_F;   // chữ "C"

static void showNow() {
  bool editing = timerState.phase == PH_IDLE || timerState.phase == PH_DONE;
  int32_t key = editing ? (editCycles ? 100000 + timerState.totalCycles : 200000 + timerState.studyMin)
                        : (int32_t)timerState.remainSec;
  if (key == shown) return;
  shown = key;
  if (!editing) {
    uint32_t r = timerState.remainSec;
    seg.showNumberDecEx((r / 60) * 100 + r % 60, 0b01000000, true);   // mm:ss
  } else if (editCycles) {
    uint8_t d[4] = { CHAR_C, 0, 0, seg.encodeDigit(timerState.totalCycles) };  // "C  3"
    seg.setSegments(d);
  } else {
    seg.showNumberDec(timerState.studyMin, false);                     // "  25"
  }
}

void knobSetup() {
  pinMode(PIN_ENC_CLK, INPUT);          // KY-040 có sẵn điện trở kéo lên 10k trên CLK/DT
  pinMode(PIN_ENC_DT, INPUT);
  pinMode(PIN_ENC_SW, INPUT_PULLUP);    // SW thường không có điện trở -> dùng kéo lên trong chip
  clkLast = digitalRead(PIN_ENC_CLK);
  seg.setBrightness(5);
  showNow();
}

void knobLoop() {
  bool editing = timerState.phase == PH_IDLE || timerState.phase == PH_DONE;

  // Vặn: mỗi nấc CLK đi xuống 1 lần; DT khác CLK = vặn phải (+1)
  // ponytail: đọc bằng vòng lặp, vặn quá nhanh có thể mất nấc -> chuyển sang attachInterrupt nếu cần
  bool clk = digitalRead(PIN_ENC_CLK);
  if (clk != clkLast) {
    clkLast = clk;
    if (clk == LOW && editing) {
      int step = digitalRead(PIN_ENC_DT) != clk ? 1 : -1;
      if (editCycles) timerState.totalCycles = constrain(timerState.totalCycles + step, 1, 8);
      else            timerState.studyMin    = constrain(timerState.studyMin + step, 1, 60);
      timerState.remainSec = timerState.studyMin * 60UL;
      Serial.printf("[ID4] %s = %u\n", editCycles ? "chu ky" : "phut",
                    editCycles ? timerState.totalCycles : timerState.studyMin);
    }
  }

  // Nhấn SW (chống rung 50 ms): đổi chế độ chỉnh
  bool sw = digitalRead(PIN_ENC_SW);
  uint32_t now = millis();
  if (sw != swRaw) { swRaw = sw; swChange = now; }
  if (now - swChange >= 50 && sw != swStable) {
    swStable = sw;
    if (swStable == LOW && editing) {
      editCycles = !editCycles;
      Serial.printf("[ID4] che do chinh: %s\n", editCycles ? "chu ky" : "phut");
    }
  }

  showNow();
}
