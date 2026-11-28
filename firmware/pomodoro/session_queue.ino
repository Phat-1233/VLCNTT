// =====================================================================
//  ID8 – Hàng đợi phiên học, gửi bù khi mất mạng – KHOA
//  Luồng: sessionPush() -> pomodoro/session -> Node-RED upsert Supabase
//         -> pomodoro/ack {"start_ts":...} -> sessionAck() xóa bản đã lưu
//  Chưa có ack thì cứ 30 s (và ngay khi MQTT vừa nối lại) gửi lại.
//  Node-RED upsert theo (dev, start_ts) nên gửi lại nhiều lần không tạo dòng trùng.
// =====================================================================

// ponytail: mảng vòng 10 bản trong RAM, mất khi rút điện -> chuyển sang Preferences nếu cần giữ qua mất nguồn
static const uint8_t  QMAX = 10;
static const uint32_t RETRY_MS = 30000;

struct QItem {
  SessionRec r;
  uint32_t endMs;   // millis() lúc kết thúc, để tính end_ts nếu lúc đó chưa có giờ NTP
  uint32_t seq;     // thứ tự vào hàng, đầy thì bỏ bản nhỏ nhất (cũ nhất)
  bool used;
};
static QItem q[QMAX];
static uint32_t nextSeq = 1, lastRetry = 0;
static bool wasOnline = false;

static uint8_t queueCount() {
  uint8_t n = 0;
  for (auto& it : q) n += it.used;
  return n;
}

// Chưa có giờ lúc bắt đầu/kết thúc -> tính lùi từ millis() khi đã có NTP, và GHI LẠI vào bản ghi
// để mọi lần gửi lại dùng cùng start_ts (ack khớp, Supabase không tạo dòng mới).
// (Hàm nhận chỉ số i thay vì QItem&: Arduino tự sinh khai báo hàm lên trước struct -> lỗi biên dịch.)
static bool fillTimes(int i) {
  QItem& it = q[i];
  if (!timeValid()) return false;
  uint32_t now = nowTs(), ms = millis();
  if (!it.r.startTs) it.r.startTs = now - (ms - it.r.startMs) / 1000;
  if (!it.r.endTs) it.r.endTs = now - (ms - it.endMs) / 1000;
  return true;
}

static bool sendItem(int i) {
  if (!fillTimes(i)) return false;    // vẫn chưa có giờ -> chưa gửi
  QItem& it = q[i];
  JsonDocument d;
  d["dev"] = DEVICE_ID;
  d["start_ts"] = it.r.startTs;
  d["end_ts"] = it.r.endTs;
  d["subject"] = it.r.subject;
  d["cycle"] = it.r.cycle;
  d["total"] = it.r.total;
  d["planned_min"] = it.r.plannedMin;
  d["focus_min"] = it.r.focusMin;
  d["pauses"] = it.r.pauses;
  d["pause_sec"] = it.r.pauseSec;
  d["completed"] = it.r.completed;
  return mqttPublish(T_SESSION, d);
}

void sessionPush(const SessionRec& r) {
  int slot = -1;
  for (int i = 0; i < QMAX; i++) if (!q[i].used) { slot = i; break; }
  if (slot < 0) {                                 // đầy: bỏ bản cũ nhất
    slot = 0;
    for (int i = 1; i < QMAX; i++) if (q[i].seq < q[slot].seq) slot = i;
    Serial.printf("[ID8] hang doi day, bo phien start_ts=%lu\n", (unsigned long)q[slot].r.startTs);
  }
  q[slot] = { r, millis(), nextSeq++, true };
  bool ok = sendItem(slot);
  lastRetry = millis();                           // vừa gửi xong, đợi ack đủ 30 s mới gửi lại
  Serial.printf("[ID8] session %u/%u %u phut completed=%d -> %s (hang doi %u)\n",
                r.cycle, r.total, r.focusMin, r.completed, ok ? "da gui, cho ack" : "CHUA GUI, se gui lai",
                queueCount());
}

void sessionQueueLoop() {
  bool online = mqttOnline();
  bool justOnline = online && !wasOnline;
  wasOnline = online;
  if (!online || queueCount() == 0) return;
  if (!justOnline && millis() - lastRetry < RETRY_MS) return;
  lastRetry = millis();
  for (int i = 0; i < QMAX; i++)
    if (q[i].used && sendItem(i))
      Serial.printf("[ID8] gui lai start_ts=%lu\n", (unsigned long)q[i].r.startTs);
}

void sessionAck(uint32_t startTs) {
  for (auto& it : q) {
    if (it.used && it.r.startTs == startTs) {
      it.used = false;
      Serial.printf("[ID8] ack start_ts=%lu -> da luu Supabase (hang doi con %u)\n",
                    (unsigned long)startTs, queueCount());
      return;
    }
  }
  Serial.printf("[ID8] ack start_ts=%lu (khong co trong hang doi)\n", (unsigned long)startTs);
}
