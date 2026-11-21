// =====================================================================
//  ID8 – Hàng đợi phiên học, gửi bù khi mất mạng – KHOA
//  Luồng: sessionPush() -> pomodoro/session -> Node-RED upsert Supabase
//         -> pomodoro/ack {"start_ts":...} -> sessionAck() xóa bản đã lưu
//  Task 3.K2 sẽ viết ruột 3 hàm dưới đây.
// =====================================================================

void sessionPush(const SessionRec& r) {
  // TODO(Khoa – Task 3.K2): thêm r vào mảng vòng 10 phần tử rồi gửi ngay
  Serial.printf("sessionPush: chu ky %u/%u, %u phut, completed=%d\n",
                r.cycle, r.total, r.focusMin, r.completed);
}

void sessionQueueLoop() {
  // TODO(Khoa – Task 3.K2): mỗi 30 s gửi lại các bản chưa có ack
}

void sessionAck(uint32_t startTs) {
  // TODO(Khoa – Task 3.K2): xóa bản có startTs == startTs
  Serial.printf("ack start_ts=%lu\n", (unsigned long)startTs);
}
