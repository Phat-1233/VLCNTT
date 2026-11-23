// =====================================================================
//  ID1 – Nút nhấn -> Dashboard (chức năng cơ bản 1) – KHOA
//  Luồng: Nút nhấn -> ESP32 -> MQTT (pomodoro/state) -> Node-RED -> Dashboard
//  Bấm nhả (< 2 s): bắt đầu / tạm dừng / tiếp tục (giờ giải lao không tạm dừng được).
//  Giữ >= 2 s: dừng hẳn; riêng lúc giải lao thì bỏ nghỉ, vào học chu kỳ tiếp theo.
//  Thử không cần nút: gõ 's' (bấm) hoặc 'l' (giữ 2 s) trong Serial Monitor.
// =====================================================================

// 1 = chạy nhanh để test (1 phút học, 1 phút nghỉ, 2 chu kỳ). Nộp bài để 0.
// Bật tạm khi biên dịch: arduino-cli ... --build-property "compiler.cpp.extra_flags=-DTEST_FAST=1"
#ifndef TEST_FAST
#define TEST_FAST 0
#endif

static const uint16_t DEBOUNCE_MS  = 50;
static const uint16_t LONG_MS      = 2000;
static const uint32_t HEARTBEAT_MS = 30000;   // Node-RED báo Offline nếu > 70 s không có tin

static uint32_t lastTick = 0, lastBeat = 0;

// Nút: trạng thái đã chống rung
static bool rawLast = HIGH, stable = HIGH, longFired = false;
static uint32_t rawChange = 0, pressStart = 0;

// Số liệu của pha học hiện tại -> 1 bản ghi pomodoro/session
static uint32_t studyStartMs = 0, studyStartTs = 0, pauseStartMs = 0;
static uint16_t studiedSec = 0, pauses = 0, pauseSec = 0;

const char* phaseName(TimerPhase p) {
  switch (p) {
    case PH_STUDY:  return "study";
    case PH_BREAK:  return "break";
    case PH_PAUSED: return "paused";
    case PH_DONE:   return "done";
    default:        return "idle";
  }
}

// Gửi trạng thái theo hợp đồng v2 (plan.md). event: start/pause/resume/stop/phase_end/done/online/subject/tick
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
  lastBeat = millis();
  if (strcmp(event, "tick") != 0)
    Serial.printf("[ID1] %s -> %s %u/%u con %lus\n", event, phaseName(timerState.phase),
                  timerState.cycle, timerState.totalCycles, (unsigned long)timerState.remainSec);
}

static void startStudy() {
  timerState.phase = PH_STUDY;
  timerState.remainSec = timerState.studyMin * 60UL;
  lastTick = millis();
  studyStartMs = millis();
  studyStartTs = timeValid() ? nowTs() : 0;   // 0 = chưa có giờ NTP, hàng đợi tính lại lúc gửi
  studiedSec = pauses = pauseSec = 0;
}

// Ghi 1 chu kỳ học (hết giờ: completed = true, dừng giữa chừng: false)
static void pushSession(bool completed) {
  SessionRec r = {};
  r.startMs = studyStartMs;
  r.startTs = studyStartTs;
  r.endTs = timeValid() ? nowTs() : 0;
  strlcpy(r.subject, timerState.subject, sizeof(r.subject));
  r.cycle = timerState.cycle;
  r.total = timerState.totalCycles;
  r.plannedMin = timerState.studyMin;
  r.focusMin = (studiedSec + 30) / 60;
  r.pauses = pauses;
  r.pauseSec = pauseSec;
  r.completed = completed;
  sessionPush(r);
}

static void onShortPress() {
  switch (timerState.phase) {
    case PH_IDLE:
    case PH_DONE:
      timerState.cycle = 1;
      startStudy();
      publishState("start");
      break;
    case PH_STUDY:
      timerState.phase = PH_PAUSED;
      pauseStartMs = millis();
      pauses++;
      publishState("pause");
      break;
    case PH_BREAK:                  // giờ giải lao không cho tạm dừng
      break;
    case PH_PAUSED:                 // chỉ tạm dừng được lúc học -> luôn quay về học
      pauseSec += (millis() - pauseStartMs) / 1000;
      timerState.phase = PH_STUDY;
      lastTick = millis();
      publishState("resume");
      break;
  }
}

static void onLongPress() {
  if (timerState.phase == PH_IDLE) return;
  if (timerState.phase == PH_BREAK) {   // giữ lúc giải lao: bỏ nghỉ, vào chu kỳ học tiếp theo
    timerState.cycle++;
    startStudy();
    publishState("phase_end");
    return;
  }
  bool inStudy = timerState.phase == PH_STUDY || timerState.phase == PH_PAUSED;
  if (timerState.phase == PH_PAUSED) pauseSec += (millis() - pauseStartMs) / 1000;
  if (inStudy && studiedSec >= 60) pushSession(false);   // học >= 1 phút mới tính là "bỏ dở"
  timerState.phase = PH_IDLE;
  timerState.cycle = 0;
  timerState.remainSec = timerState.studyMin * 60UL;
  publishState("stop");
}

static void onPhaseEnd() {
  if (timerState.phase == PH_STUDY) {
    pushSession(true);
    if (timerState.cycle >= timerState.totalCycles) {   // chu kỳ cuối: xong luôn, không nghỉ
      timerState.phase = PH_DONE;
      timerState.remainSec = 0;
      publishState("done");
    } else {
      timerState.phase = PH_BREAK;
      timerState.remainSec = timerState.breakMin * 60UL;
      lastTick = millis();
      publishState("phase_end");
    }
  } else {                                              // hết nghỉ -> chu kỳ học tiếp theo
    timerState.cycle++;
    startStudy();
    publishState("phase_end");
  }
}

static void readButton() {
  bool raw = digitalRead(PIN_BUTTON);
  uint32_t now = millis();
  if (raw != rawLast) { rawLast = raw; rawChange = now; }
  if (now - rawChange >= DEBOUNCE_MS && raw != stable) {
    stable = raw;
    if (stable == LOW) { pressStart = now; longFired = false; }    // vừa nhấn
    else if (!longFired) onShortPress();                           // vừa nhả, chưa tính là giữ
  }
  if (stable == LOW && !longFired && now - pressStart >= LONG_MS) { // giữ đủ 2 s: dừng ngay
    longFired = true;
    onLongPress();
  }
}

void timerSetup() {
  pinMode(PIN_BUTTON, INPUT_PULLUP);
#if TEST_FAST
  timerState.studyMin = 1;
  timerState.breakMin = 1;
  timerState.totalCycles = 2;
#endif
  timerState.remainSec = timerState.studyMin * 60UL;
}

void timerLoop() {
  readButton();
  if (Serial.available()) {                 // thử bằng Serial: 's' = bấm, 'l' = giữ
    char c = Serial.read();
    if (c == 's') onShortPress();
    else if (c == 'l') onLongPress();
  }

  uint32_t now = millis();
  if ((timerState.phase == PH_STUDY || timerState.phase == PH_BREAK) && now - lastTick >= 1000) {
    lastTick += 1000;
    if (timerState.remainSec > 0) timerState.remainSec--;
    if (timerState.phase == PH_STUDY) studiedSec++;
    if (timerState.remainSec == 0) onPhaseEnd();
  }

  if (now - lastBeat >= HEARTBEAT_MS) publishState("tick");   // nhịp tim cho ô "Kết nối hộp"
}
