// =====================================================================
//  ID8 – Hàng đợi phiên học, gửi bù khi mất mạng – KHOA
//  Luồng: sessionPush() -> pomodoro/session -> Node-RED upsert Supabase
//         -> pomodoro/ack {"start_ts":...} -> sessionAck() xóa bản đã lưu
//  Task 3.K2 sẽ viết ruột 3 hàm dưới đây.
// =====================================================================

void sessionPush(const SessionRec& r) {
  // ponytail: gửi 1 lần, mất mạng là mất. Task 3.K2 thêm mảng vòng 10 phần tử + gửi lại tới khi có ack.
  uint32_t startTs = r.startTs ? r.startTs : (timeValid() ? nowTs() - (millis() - r.startMs) / 1000 : 0);
  JsonDocument d;
  d["dev"] = DEVICE_ID;
  d["start_ts"] = startTs;
  d["end_ts"] = r.endTs ? r.endTs : (timeValid() ? nowTs() : 0);
  d["subject"] = r.subject;
  d["cycle"] = r.cycle;
  d["total"] = r.total;
  d["planned_min"] = r.plannedMin;
  d["focus_min"] = r.focusMin;
  d["pauses"] = r.pauses;
  d["pause_sec"] = r.pauseSec;
  d["completed"] = r.completed;
  bool ok = startTs && mqttPublish(T_SESSION, d);
  Serial.printf("[ID8] session %u/%u %u phut completed=%d -> %s\n",
                r.cycle, r.total, r.focusMin, r.completed, ok ? "da gui" : "CHUA GUI");
}

void sessionQueueLoop() {
  // TODO(Khoa – Task 3.K2): mỗi 30 s gửi lại các bản chưa có ack
}

void sessionAck(uint32_t startTs) {
  // TODO(Khoa – Task 3.K2): xóa bản có startTs == startTs
  Serial.printf("ack start_ts=%lu\n", (unsigned long)startTs);
}
