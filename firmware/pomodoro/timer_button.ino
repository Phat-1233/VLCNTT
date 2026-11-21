// =====================================================================
//  ID1 – Nút nhấn -> Dashboard (chức năng cơ bản 1) – KHOA
//  Luồng: Nút nhấn -> ESP32 -> MQTT (pomodoro/state) -> Node-RED -> Dashboard
// =====================================================================

const char* phaseName(TimerPhase p) {
  switch (p) {
    case PH_STUDY:  return "study";
    case PH_BREAK:  return "break";
    case PH_PAUSED: return "paused";
    case PH_DONE:   return "done";
    default:        return "idle";
  }
}

// Gửi trạng thái theo "hợp đồng" JSON (docs/lo_trinh_lam_pomodoro.md, mục 2)
// event: start / pause / resume / stop / phase_end / done / online / subject
void publishState(const char* event) {
  JsonDocument d;
  d["dev"] = DEVICE_ID;
  d["ts"] = nowTs();
  d["event"] = event;
  d["state"] = phaseName(timerState.phase);
  d["subject"] = timerState.subject;
  d["cycle"] = timerState.cycle;
  d["total"] = timerState.totalCycles;
  d["remain"] = timerState.remainSec;
  mqttPublish(T_STATE, d);
}

void timerSetup() {
  pinMode(PIN_BUTTON, INPUT_PULLUP);
  timerState.remainSec = timerState.studyMin * 60UL;
}

void timerLoop() {
  // TODO(Khoa – ID1):
  //  1. Đọc nút có chống rung (~50 ms):
  //       - bấm nhả        : idle -> study (cycle = 1, publishState("start"))
  //                          study/break -> paused (publishState("pause"))
  //                          paused -> pha trước đó (publishState("resume"))
  //       - giữ >= 2 giây  : -> idle (publishState("stop"))
  //  2. Mỗi 1 giây (dùng millis(), KHÔNG dùng delay()) giảm timerState.remainSec
  //     khi đang study/break.
  //  3. remainSec == 0:
  //       - study -> break (publishState("phase_end"))
  //       - break -> study, cycle++ ; nếu cycle > totalCycles -> PH_DONE (publishState("done"))
  //  4. (ID8) Khi kết thúc phiên: gửi pomodoro/session và lưu tạm tới khi nhận ack.
}
