# Pomodoro Để Bàn – Kế hoạch thực hiện (plan.md)

> Cách dùng: làm xong bước nào thì đổi `- [ ]` thành `- [x]` rồi commit cùng code của bước đó.
> Nhãn người làm: **[Khoa]** = Nguyễn Hoàng Đăng Khoa · **[Phát]** = Nguyễn Thành Phát · **[Cả 2]** = ngồi làm chung.
> Làm **theo thứ tự giai đoạn**. Trong một giai đoạn, cột Khoa và cột Phát chạy **song song**. Hết giai đoạn có **Mốc ghép**: chưa đạt mốc thì chưa sang giai đoạn sau.

**Mục tiêu:** hoàn thiện 11 chức năng ID1–ID11 trong proposal, chạy được demo trọn vẹn trên 1 hộp + 1 dashboard.

**Kiến trúc:** ESP32 ⇄ MQTT (**EMQX Cloud Serverless**, TLS 8883 – thay HiveMQ Cloud vì HiveMQ đã ngừng cho tạo cluster Serverless free, khai tử 31/12/2026) ⇄ Node-RED (back-end + Dashboard 2) ⇄ Supabase (REST). Node-RED gọi dịch vụ AI Flask (`localhost:5000`), Telegram Bot và Gmail SMTP.

**Công nghệ:** Arduino ESP32 (PubSubClient, ArduinoJson v7, Adafruit SSD1306/GFX, TM1637Display, WiFiManager) · Node-RED + `@flowfuse/node-red-dashboard`, `node-red-node-email`, `node-red-contrib-telegrambot` · Supabase (PostgreSQL) · Python 3 + Flask + numpy.

**Tài liệu gốc:** proposal `6_24127187_24127483_PROPOSAL.pdf` và [docs/lo_trinh_lam_pomodoro.md](docs/lo_trinh_lam_pomodoro.md) (sơ đồ chân, lỗi hay gặp). Nếu plan này khác lộ trình thì **làm theo plan này** (plan đã chốt hợp đồng v2 ở Giai đoạn 0).

---

## Bảng tiến độ tổng

| ID | Chức năng | Người làm | Giai đoạn | Xong |
|---|---|---|---|---|
| — | Giai đoạn 0: tài khoản, hợp đồng v2, tách file | Cả 2 | 0 | [ ] |
| ID1 | Nút nhấn → Dashboard | Khoa | 1 | [ ] |
| ID2 | Dashboard → OLED | Phát | 1 | [ ] |
| ID4 | Núm xoay → 7 đoạn | Khoa | 2 | [ ] |
| ID5 | LDR → Dashboard (+ cảnh báo thiếu sáng) | Khoa | 2, 3 | [ ] |
| ID10 | Dashboard → Buzzer | Phát | 2 | [ ] |
| ID7 | AI xếp lịch (Flask) | Phát | 2, 3, 4 | [ ] |
| ID8 | Lưu Supabase + ack + gửi bù | Khoa | 3 | [ ] |
| ID9 | Biểu đồ thống kê | Phát | 3 | [ ] |
| ID6 | Telegram | Khoa | 4 | [ ] |
| ID3 | WiFiManager | Khoa | 4 | [ ] |
| ID11 | Email báo cáo tuần | Phát | 4 | [ ] |
| — | Hàn mạch, lắp hộp, demo, báo cáo | Cả 2 | 5 | [ ] |

### Hiện trạng repo (lúc lập plan, 21/11/2026)

- Đã có: `pomodoro.ino` (WiFi ghi cứng, NTP, MQTT TLS, chia lệnh `cmd`), `config.h`, `oled.ino` (ID2), `buzzer.ino` (ID10), `ai-service/` (model, scheduler, Flask), `supabase/schema.sql` v1, `node-red/flows.json` (xương sống + inject thử).
- Còn TODO: `timer_button.ino` (ID1), `knob_7seg.ino` (ID4), `ldr.ino` (ID5), xử lý ack (ID8), toàn bộ dashboard, ID3, ID6, ID8 phía Node-RED, ID9, ID11, phần ghép ID7.
- Chưa test trên phần cứng: OLED, buzzer.

---

## Quy tắc chống "đạp chân nhau"

### 1. Mỗi file có đúng 1 chủ

| File | Chủ | Người kia |
|---|---|---|
| `firmware/pomodoro/config.h`, `pomodoro.ino` | **Khoa** (chỉ sửa ở Task 0.3 và 5.2) | Sau Task 0.3 coi như **đóng băng**. Cần sửa thì nhắn trước, 1 người sửa, người kia pull ngay |
| `timer_button.ino`, `knob_7seg.ino`, `ldr.ino`, `session_queue.ino`, `wifi_setup.ino` | Khoa | Chỉ đọc |
| `oled.ino`, `buzzer.ino` | Phát | Chỉ đọc |
| `supabase/schema.sql` (bảng) | Khoa | Chỉ đọc |
| `supabase/views.sql`, `supabase/seed.sql` | Phát | Chỉ đọc |
| `node-red/common.json` | Phát (chỉ sửa ở Task 0.4) | Đóng băng sau Task 0.4 |
| `node-red/khoa.json` | Khoa | Chỉ import để chạy, không sửa |
| `node-red/phat.json` | Phát | Chỉ import để chạy, không sửa |
| `ai-service/*` | Phát | Chỉ đọc |
| `plan.md` | Cả 2 | Mỗi người chỉ tick ô của mình |

### 2. Node-RED tách 3 file, nối nhau bằng node `link`

- `common.json`: broker EMQX, TLS, `mqtt in pomodoro/#` → `switch` theo topic → 3 node **link out** tên `state`, `lux`, `session`; 2 node **link in** tên `to-cmd`, `to-ack` → `mqtt out`. Thêm `ui-base`, `ui-theme`, 4 `ui-page` và **tất cả `ui-group`** (khung trống) của dashboard.
- Mỗi người có **1 tab riêng** (`Khoa – ID1 ID4 ID5 ID6 ID8` / `Phát – ID2 ID7 ID9 ID10 ID11`), chỉ lấy dữ liệu qua `link in` (`state`/`lux`/`session`) và gửi lệnh qua `link out` (`to-cmd`/`to-ack`). **Không kéo dây sang tab người kia.**
- Export: chọn đúng tab của mình → Menu ☰ → Export → *Current flow* → ghi đè `node-red/<tên>.json`. Import: chọn **Replace** khi Node-RED hỏi trùng node (không chọn *Import copy*, vì sẽ đổi id làm đứt link).
- Bí mật không bao giờ nằm trong flow: dùng biến môi trường `SUPABASE_URL`, `SUPABASE_KEY`, `TELEGRAM_TOKEN`, `TELEGRAM_CHAT_ID`, `AI_URL` (đặt 1 lần bằng `setx`, khởi động lại Node-RED). Trong function đọc bằng `env.get("SUPABASE_URL")`. Mật khẩu Gmail nằm trong credentials của node email (không bị export).

### 3. Một thiết bị, một nơi ghi dữ liệu

- **Board ESP32:** Khoa giữ trong Giai đoạn 1–3 (phần lớn firmware là của Khoa). Phát test OLED/buzzer ở buổi ghép và dùng **giả lập** khi không có board.
- **Giả lập** trong `common.json` gửi `"dev":"sim-01"`. Tab của Khoa **bỏ qua mọi bản tin `dev` ≠ `pomo-01`** khi ghi Supabase, nên Phát bấm thử thoải mái mà không làm bẩn dữ liệu.
- **Chỉ 1 Node-RED chạy tab Khoa tại một thời điểm.** Trước Mốc M3: máy Khoa. Từ Mốc M3 trở đi: **máy chính = laptop Phát** (máy chạy demo), Khoa import `khoa.json` vào đó và **tắt** Node-RED của mình (hoặc Disable tab của mình) khi máy chính đang chạy.

### 4. Git

- Luôn `git pull --rebase` trước khi làm và trước khi push. Commit nhỏ, chỉ add file của mình.
- Commit message dạng `ID1: ...`, `ID9: ...` để dễ dò lại.
- Không commit `secrets.h`, `.env`, key Supabase, token Telegram, App Password.

---

## Hợp đồng v2 (chốt ở Giai đoạn 0, sau đó không tự ý đổi)

Thay đổi so với README cũ: **`pomodoro/session` gửi theo từng chu kỳ học** (1 Pomodoro = 1 dòng), vì wireframe Nhật ký học, tỉ lệ hoàn thành "13/15 phiên" và dữ liệu huấn luyện AI đều tính theo chu kỳ.

| Topic | Chiều | JSON | Khi nào gửi |
|---|---|---|---|
| `pomodoro/state` | ESP32 → NR | `{"dev":"pomo-01","ts":U,"event":"start","state":"study","subject":"Toan","cycle":1,"total":4,"remain":1500}` | Mỗi lần đổi trạng thái. `event` ∈ `online, start, pause, resume, stop, phase_end, done, subject` |
| `pomodoro/lux` | ESP32 → NR | `{"dev":"pomo-01","ts":U,"lux":1830}` | Mỗi 2 giây, `lux` = ADC thô 0–4095 |
| `pomodoro/session` | ESP32 → NR | `{"dev":"pomo-01","start_ts":U,"end_ts":U,"subject":"Toan","cycle":2,"total":3,"planned_min":25,"focus_min":25,"pauses":1,"pause_sec":120,"completed":true}` | Hết 1 pha học (`completed:true`) hoặc bị giữ nút dừng giữa pha học (`completed:false`). Pha nghỉ không gửi |
| `pomodoro/ack` | NR → ESP32 | `{"start_ts":U}` | Sau khi Supabase upsert trả 2xx |
| `pomodoro/cmd` | NR → ESP32 | `{"type":"msg","text":"...","sec":10}` · `{"type":"subject","name":"Toan"}` · `{"type":"beep","times":3}` | Không đổi. Text **đã bỏ dấu**, tối đa 150 ký tự |

`U` = Unix giây (UTC) lấy từ NTP. Lưu UTC, chỉ cộng +7 khi hiển thị.

Các loại `events.type`: các `event` của state ở trên, cộng `low_light` (Khoa), `parent_beep` (Phát), `schedule_reminder` (Phát).

Bảng `settings` (key → value jsonb): `lux_threshold` (số %, mặc định 30), `parent_emails` (chuỗi, cách nhau dấu phẩy), `telegram_enabled` (bool).

---

## Giai đoạn 0 – Chuẩn bị và chốt hợp đồng (Cả 2 · 1 buổi, đầu tuần 7)

> Thứ tự: 0.1 → (0.2 [Khoa] ‖ 0.4 [Phát]) → 0.3 [Khoa] → Mốc M0.

### Task 0.1 – Tài khoản và công cụ [Cả 2]

- [x] **[Khoa]** Tạo deployment **EMQX Cloud Serverless** (Spend Limit = 0); Access Control → Authentication tạo 2 user `esp32`, `nodered`; ghi Address + cổng 8883 vào chỗ riêng (không vào Git). Lưu ý: 30 ngày không có client kết nối thì EMQX tự dừng deployment → vào console bật lại.
- [x] **[Khoa]** Tạo project Supabase; lấy `Project URL` + `service_role key`.
- [x] **[Phát]** Tạo Telegram bot với @BotFather (lấy token), lấy `chat_id` của điện thoại nhận tin → gửi riêng cho Khoa. _(Khoa tạo thay vì tài khoản Telegram mới ở VN bị thu phí SMS; bot `@pomodoros_kp_bot`, `getMe` OK 21/11)_
- [x] **[Phát]** Bật xác minh 2 bước Gmail, tạo **App Password** cho Node-RED. _(21/11, lưu riêng, nhập vào node email ở Task 4.P2)_
- [ ] **[Cả 2]** Cài Node-RED + 3 palette (`@flowfuse/node-red-dashboard`, `node-red-node-email`, `node-red-contrib-telegrambot`); cài Arduino IDE + core esp32 + thư viện: PubSubClient, ArduinoJson v7, Adafruit SSD1306, Adafruit GFX, TM1637Display (avishorp), WiFiManager (tzapu). — **Phát ✔ 21/11** (Partition Scheme: Huge APP, Verify `pomodoro.ino` OK)
- [ ] **[Cả 2]** Mỗi máy đặt biến môi trường: `setx SUPABASE_URL "..."`, `setx SUPABASE_KEY "..."`, `setx AI_URL "http://localhost:5000"`, `setx TELEGRAM_TOKEN "..."`, `setx TELEGRAM_CHAT_ID "..."`. — **Phát ✔ 21/11** (curl Supabase `settings` ra 3 dòng, Telegram `getMe` OK, Node-RED `env.get` OK)

**Xong khi:** cả 2 máy mở được `http://localhost:1880` và Arduino IDE biên dịch được `pomodoro.ino` (với `secrets.h` đã điền).

### Task 0.2 – Schema v2 trên Supabase [Khoa]

**Files:** Modify `supabase/schema.sql`.

- [x] (Đã viết sẵn trong `schema.sql` – Khoa chỉ cần chạy và kiểm tra.) Sửa bảng `sessions` thành: `dev, start_ts timestamptz not null, end_ts timestamptz, subject text, cycle int, total int, planned_min int, focus_min int, pauses int default 0, pause_sec int default 0, completed boolean, unique (dev, start_ts)`.
- [x] Thêm vào `plans`: `score real, reason text`.
- [x] Thêm vào `email_reports`: `week text, attempt int default 1`.
- [x] Thêm bảng `settings (key text primary key, value jsonb, updated_at timestamptz default now())` và 3 dòng mặc định: `lux_threshold=30`, `parent_emails=""`, `telegram_enabled=true`.
- [x] Thêm index: `events (ts)`, `lux_readings (ts)`, `sessions (start_ts)`.
- [x] Cuối file bật RLS cho 6 bảng (không policy → chỉ service_role đọc/ghi được).
- [x] Đầu file thêm `drop table if exists ... cascade;` cho cả 6 bảng (chưa có dữ liệu thật nên chạy lại an toàn) → chạy trong SQL Editor.
- [x] Kiểm tra: `curl "$SUPABASE_URL/rest/v1/settings" -H "apikey: $SUPABASE_KEY" -H "Authorization: Bearer $SUPABASE_KEY"` trả về 3 dòng.
- [ ] Commit `ID8: schema v2`.

### Task 0.3 – Chốt file chung của firmware [Khoa]

Mục tiêu: sửa `config.h` và `pomodoro.ino` **một lần duy nhất** cho tất cả chức năng về sau, rồi đóng băng.

**Files:** Modify `config.h`, `pomodoro.ino`; Create `session_queue.ino`, `wifi_setup.ino` (chỉ có hàm rỗng / code cũ chuyển sang).

**Interfaces (Produces):**
- `struct SessionRec { uint32_t startMs; uint32_t startTs; uint32_t endTs; char subject[24]; uint8_t cycle, total; uint16_t plannedMin, focusMin, pauses, pauseSec; bool completed; };` trong `config.h`
- `void sessionPush(const SessionRec& r);` – timer gọi khi hết/dừng 1 pha học (ID1 → ID8)
- `void sessionQueueLoop();` – gửi lại các phiên chưa có ack (ID8)
- `void sessionAck(uint32_t startTs);` – xóa phiên đã được xác nhận (ID8)
- `bool timeValid();` → `time(nullptr) > 1700000000`
- `void connectWiFi();` chuyển sang `wifi_setup.ino` (ID3 sẽ thay ruột hàm này)

- [x] Thêm các khai báo trên vào `config.h`.
- [x] `pomodoro.ino`: nhánh `T_ACK` gọi `sessionAck(doc["start_ts"] | 0)`; `loop()` gọi thêm `sessionQueueLoop()`; thêm `timeValid()`; chuyển nguyên hàm `connectWiFi()` (bản ghi cứng hiện tại) sang `wifi_setup.ino`.
- [x] Tạo `session_queue.ino` với 3 hàm rỗng.
- [ ] Biên dịch không lỗi; nạp board, Serial in "Ket noi MQTT (EMQX)... OK".
- [ ] Commit `chung: chot config.h + pomodoro.ino cho ID3/ID8` → **nhắn Phát pull**.

### Task 0.4 – Tách Node-RED + khung dashboard [Phát]

**Files:** Create `node-red/common.json`, `node-red/khoa.json` (tab trống), `node-red/phat.json`; Delete `node-red/flows.json`; Modify `README.md` (mục Node-RED).

- [x] **[Khoa]** Export dashboard wireframe đang chạy trên máy Khoa (4 tab `UI – ...`, 4 page, 20 group, inject `[MẪU]`) → `node-red/ui_wireframe.json`. **Không dựng lại UI từ đầu**: import file này, giữ nguyên `ui-base/theme/page/group` (đưa vào `common.json`), chuyển widget của group Khoa (Trạng thái hộp, Độ sáng phòng, Nhật ký học hôm nay, Thông báo nhanh Telegram) sang tab Khoa, còn lại sang tab Phát. Inject `[MẪU]` giữ lại tới khi có dữ liệu thật rồi xóa.
- [x] Từ `flows.json` hiện có, dựng tab `Chung` theo mục "Quy tắc 2": switch → link out `state`/`lux`/`session`; link in `to-cmd`/`to-ack` → mqtt out. Đổi giả lập sang `"dev":"sim-01"`.
- [x] Chuyển 3 node inject thử + 2 function ID2/ID10 sang tab `Phát`, nối ra link out `to-cmd`.
- [x] (Đã có sẵn trong `ui_wireframe.json` – chỉ cần đổi tên group thêm "(Khoa)"/"(Phát)".) Tạo `ui-base`, 4 `ui-page`: **Tổng quan**, **Lịch học AI**, **Thống kê**, **Phụ huynh và cài đặt**; tạo sẵn `ui-group` trống đúng như wireframe, ghi chủ trong tên group:
  - Tổng quan: `Trạng thái hộp (Khoa)`, `Độ sáng phòng (Khoa)`, `Gửi xuống hộp (Phát)`, `Hôm nay (Phát)`, `Nhật ký học hôm nay (Khoa)`
  - Lịch học AI: `Chọn cách nhập (Phát)`, `Nhập lịch (Phát)`, `Lịch tuần do AI xếp (Phát)`, `Khả năng hoàn thành theo giờ (Phát)`
  - Thống kê: `Chọn khoảng (Phát)`, `Tổng kết (Phát)`, 4 group biểu đồ `(Phát)`
  - Phụ huynh: `Email báo cáo (Phát)`, `Telegram (Khoa)`, `Trạng thái hệ thống (Phát)`, `Lịch sử gửi email (Phát)`
- [x] Export 3 file; xóa `flows.json`; sửa README: "Import theo thứ tự common → khoa → phat, chọn Replace khi trùng". _(Xong 22/11. Ghi chú: Khoa sửa trong tab Khoa trên máy mình rồi export *Current flow* → `khoa.json`; KHÔNG export *All flows*. `ui_wireframe.json` đã gộp vào 3 file nên xóa luôn.)_
- [ ] Kiểm tra: import 3 file vào Node-RED trống → Deploy không lỗi; bấm "Giả lập state" thấy bản tin đi qua link `state` (debug); dashboard có 4 trang.
- [ ] Commit `chung: tach Node-RED thanh common/khoa/phat` → **nhắn Khoa pull**.

### ✅ Mốc M0 [Cả 2]
- [ ] Cả 2 pull về, import 3 file Node-RED, nạp firmware: ESP32 gửi `online` → thấy ở debug Node-RED; bấm inject "Thử lời nhắn" → OLED hiện chữ.
- [ ] **[Khoa]** Cập nhật bảng topic trong `README.md` theo hợp đồng v2.

---

## Giai đoạn 1 – Hai luồng cơ bản (tuần 7)

| Khoa | Phát |
|---|---|
| Task 1.K – ID1 | Task 1.P – ID2 |

### Task 1.K – ID1 Nút nhấn → Dashboard [Khoa]

**Files:** Modify `firmware/pomodoro/timer_button.ino`, `node-red/khoa.json`.

**Interfaces:** Consumes `timerState`, `publishState()`, `sessionPush()` (Task 0.3). Produces: `timerState` luôn đúng để OLED (Phát) và 7 đoạn đọc.

- [ ] Nút: chống rung 50 ms bằng `millis()`; **bấm nhả** (< 2 s): `idle/done → study` (cycle = 1, `start`), `study/break → paused` (`pause`, nhớ pha trước), `paused → pha trước` (`resume`). **Giữ ≥ 2 s**: `→ idle` (`stop`), báo ngay lúc đủ 2 s, lúc nhả **không** tính thêm 1 lần bấm.
- [ ] Đếm ngược mỗi 1 s theo `millis()` khi `study`/`break`; đếm `pauses`, `pause_sec` của pha học hiện tại.
- [ ] Hết giờ: `study → break` (`phase_end`, gọi `sessionPush(completed=true)`); `break → study`, `cycle++`; vượt `totalCycles` → `done` (`done`).
- [ ] Dừng giữa pha học đã học ≥ 1 phút → `sessionPush(completed=false)`.
- [ ] Node-RED tab Khoa: link in `state` → group **Trạng thái hộp**: Kết nối hộp (Offline nếu > 70 s không có bản tin), Trạng thái đồng hồ, Môn, Chu kỳ `2 / 4`, Thời gian còn lại `mm:ss`.
- [ ] Group **Nhật ký học hôm nay** (`ui-table`): tạm thời đổ từ link in `state` (giờ, sự kiện, môn, chu kỳ); Task 4.K3 sẽ đổi sang đọc `sessions`.
- [ ] Kiểm tra với `studyMin = 1`, `breakMin = 1`, 2 chu kỳ: chạy hết → dashboard đi `study → break → study → break → done`; bấm khi đang học → `paused`, bấm lại → tiếp tục đúng số giây; giữ 2 s → `idle` và chỉ có 1 bản tin `stop`.
- [ ] Export `khoa.json`, commit `ID1: may trang thai + dashboard trang thai`.

### Task 1.P – ID2 Dashboard → OLED [Phát]

**Files:** Modify `node-red/phat.json`, (nếu cần) `firmware/pomodoro/oled.ino`.

- [ ] Group **Gửi xuống hộp**: `ui-dropdown` môn (Toán, Lý, Hóa, Anh văn, Văn, Sinh, Khác), `ui-text-input` lời nhắn, `ui-button` "Gửi lời nhắn" → function bỏ dấu (dùng lại function có sẵn), cắt còn 150 ký tự → `{"type":"msg","text":...,"sec":10}` → link out `to-cmd`. Chọn môn → `{"type":"subject","name":...}`.
- [ ] Bỏ qua lời nhắn rỗng (function trả `null`).
- [ ] Buổi ghép có board: OLED hiện lời nhắn đúng 10 s rồi quay về đồng hồ; "Con học bài nhé" hiện "Con hoc bai nhe"; chuỗi 150 ký tự không tràn khỏi màn (nếu tràn thì giảm còn 120 ở function).
- [ ] Đổi môn trên web → dòng trên OLED đổi tên môn và dashboard Khoa nhận event `subject`.
- [ ] Export `phat.json`, commit `ID2: dashboard gui loi nhan + doi mon`.

### ✅ Mốc M1 – ghép lần 1 [Cả 2, có board]
- [ ] Bấm nút trên hộp → dashboard đổi trạng thái trong < 2 s.
- [ ] Gõ lời nhắn trên web → OLED hiện, 10 s sau trở lại đồng hồ, đồng hồ **không bị đứng** trong lúc hiện lời nhắn.
- [ ] Tắt WiFi router 30 s rồi bật lại → hộp vẫn đếm, tự kết nối lại MQTT.

---

## Giai đoạn 2 – Phần cứng trên hộp + AI độc lập (tuần 8)

| Khoa | Phát |
|---|---|
| Task 2.K1 – ID4 | Task 2.P1 – ID10 |
| Task 2.K2 – ID5 (đo + gauge) | Task 2.P2 – hoàn thiện AI service |

### Task 2.K1 – ID4 Núm xoay → 7 đoạn [Khoa]

**Files:** Modify `firmware/pomodoro/knob_7seg.ino`.

- [ ] Khởi tạo `TM1637Display(PIN_TM_CLK, PIN_TM_DIO)`, độ sáng 5.
- [ ] Đọc encoder bằng so cạnh CLK/DT trong `knobLoop()` (chưa dùng ngắt; nếu vặn nhanh bị mất bước mới chuyển sang ngắt).
- [ ] Chỉ chỉnh khi `phase == PH_IDLE` hoặc `PH_DONE`. Chế độ "phút": `studyMin` 1..60 (cho phép 1 để demo). Nhấn SW (chống rung) → chế độ "chu kỳ": `totalCycles` 1..8. Chỉnh xong cập nhật `remainSec = studyMin*60`.
- [ ] Hiển thị: chế độ phút `"  25"`, chế độ chu kỳ `"C  3"`; đang chạy hiện `mm:ss` còn lại có dấu hai chấm.
- [ ] Kiểm tra: vặn phải/trái 10 nấc → số tăng/giảm đúng 10; đang học vặn núm → không đổi.
- [ ] Commit `ID4: num xoay + 7 doan`.

### Task 2.K2 – ID5 LDR → Dashboard [Khoa]

**Files:** Modify `firmware/pomodoro/ldr.ino`, `node-red/khoa.json`.

- [ ] Firmware: mỗi 2 s đọc trung bình 8 mẫu `analogRead(PIN_LDR)` → publish `pomodoro/lux` theo hợp đồng.
- [ ] Node-RED: function đổi ADC → %: `pct = LUX_INVERT ? 100 - raw/40.95 : raw/40.95` (đặt `LUX_INVERT` ở đầu function; che tay mà % tăng thì đảo lại). Ghi chú `// ponytail: ánh xạ tuyến tính, cần hiệu chuẩn với đèn bàn thật`.
- [ ] Group **Độ sáng phòng**: `ui-gauge` 0–100 (đỏ < ngưỡng, vàng, xanh), `ui-slider` "Ngưỡng cảnh báo thiếu sáng (%)", text "Tình trạng: Đủ sáng/Thiếu sáng". Tạm lưu ngưỡng ở `flow` context (Task 3.K2 chuyển sang bảng `settings`).
- [ ] Kiểm tra: che LDR → gauge tụt dưới ngưỡng trong ≤ 4 s và hiện "Thiếu sáng"; bỏ tay → "Đủ sáng".
- [ ] Commit `ID5: LDR -> gauge`.

### Task 2.P1 – ID10 Nút "Nhắc nhở" → Buzzer [Phát]

**Files:** Modify `node-red/phat.json`.

- [ ] Group **Gửi xuống hộp**: `ui-button` "Nhắc nhở (buzzer kêu 3 tiếng)" → `{"type":"beep","times":3}` → link out `to-cmd`; bỏ qua nếu bấm lại trong 5 s.
- [ ] Buổi có board: nối D4184 theo chú thích đầu `buzzer.ino` (GND chung!); bấm nút → kêu đúng 3 tiếng, đồng hồ OLED vẫn chạy.
- [ ] Commit `ID10: nut nhac nho`.

### Task 2.P2 – Hoàn thiện AI service theo wireframe [Phát]

**Files:** Modify `ai-service/app.py`, `ai-service/scheduler.py`; Create `ai-service/test_ai.py`.

**Interfaces (Produces – dùng ở Task 3.P2):**
- `POST /schedule` body thêm: `study_min` (mặc định 25), `break_min` (5), `priority` (tên môn, có thể rỗng); mỗi dòng `timetable` có thể có `subject`.
- Mỗi phần tử `plan` trả về thêm: `"cycles": int`, `"reason": str`.
- `GET /scores` thêm `trained_at` (ISO) và `test_accuracy` (lần huấn luyện gần nhất).

- [ ] Viết test trước trong `test_ai.py` (chạy bằng `python test_ai.py`, chỉ dùng `assert`, gọi qua `app.test_client()`):
  - `test_khong_trung_tkb`: timetable Thứ 2 07:30–11:30 → không buổi nào của Thứ 2 giao với 07:00–12:00 (đã cộng 30 phút đi lại).
  - `test_mot_mon_mot_ngay`: 2 môn × 3 buổi, 7 ngày rảnh → không ngày nào một môn xuất hiện 2 lần, mỗi ngày ≤ 3 buổi.
  - `test_khong_xep_duoc`: 1 ngày rảnh, 1 môn × 5 buổi → HTTP 200, `ok == False`, có `message`.
  - `test_cycles`: `session_min=90, study_min=25, break_min=5` → mọi `cycles == 3`.
  - `test_uu_tien`: 2 môn cùng số buổi, `priority="Ly"` → điểm trung bình các khung của Lý ≥ môn kia.
  - `test_model_hoc_duoc`: huấn luyện bằng dữ liệu giả → `test_accuracy ≥ 0.7`.
- [ ] Chạy `python test_ai.py` → thấy FAIL ở các test mới.
- [ ] Cài: `cycles = max(1, session_min // (study_min + break_min))`; môn ưu tiên được cộng 0.1 vào điểm khung khi giải CSP; `reason` theo thứ tự: TKB có tiết cùng môn 1–2 ngày sau → `"Truoc tiet {mon} {thu}"`; môn ưu tiên → `"Mon uu tien"`; còn lại → `"Kha nang hoan thanh cao ({p}%)"`.
- [ ] `sessions_to_xy` đọc `planned_min` (hợp đồng v2) làm thời lượng, thiếu thì dùng `focus_min`.
- [ ] `python test_ai.py` → in "OK" hết.
- [ ] Commit `ID7: cycles, reason, uu tien + test`.

### ✅ Mốc M2 – ghép lần 2 [Cả 2, có board]
- [ ] Vặn núm chỉnh 1 phút / 2 chu kỳ → bấm nút → OLED và 7 đoạn cùng đếm, khớp nhau từng giây.
- [ ] Che LDR → gauge tụt; bấm "Nhắc nhở" → buzzer kêu 3 tiếng.
- [ ] `curl` `/schedule` cả 2 cách nhập trả lịch có `cycles`, `reason`.

---

## Giai đoạn 3 – Dữ liệu cloud (tuần 8–9) ← nút thắt, ưu tiên cao nhất

| Khoa | Phát |
|---|---|
| Task 3.K1 – ID8 Node-RED ghi Supabase | Task 3.P1 – views + dữ liệu mẫu + ID9 |
| Task 3.K2 – ID8 firmware gửi bù + ID5 cảnh báo | Task 3.P2 – ID7 trang Lịch học AI |

> Phát **không chờ** Khoa: ID9 vẽ trên `seed.sql`, ID7 chạy với dữ liệu giả của AI.

### Task 3.K1 – ID8 Node-RED → Supabase + ack [Khoa]

**Files:** Modify `node-red/khoa.json`.

- [ ] Một function dùng chung trong tab Khoa dựng request: `msg.url = env.get("SUPABASE_URL") + "/rest/v1/<bảng>"`, header `apikey`, `Authorization: Bearer`, `Content-Type: application/json` → `http request` (POST).
- [ ] Mọi luồng ghi bỏ qua bản tin `dev != "pomo-01"`.
- [ ] `state` → bảng `events` (`ts` = `new Date(U*1000).toISOString()`, `type` = `event`, `detail` = cả bản tin).
- [ ] `lux` → gom 60 s (trung bình) → bảng `lux_readings`.
- [ ] `session` → upsert `sessions` với URL `?on_conflict=dev,start_ts` và header `Prefer: resolution=merge-duplicates` → nếu `statusCode` 2xx → `{"start_ts":U}` qua link out `to-ack`. Lỗi → `debug`, **không** gửi ack.
- [ ] Kiểm tra: gửi cùng 1 bản tin `session` 3 lần (inject) → bảng `sessions` chỉ có 1 dòng, có 3 ack. Bản tin `sim-01` → không có dòng nào.
- [ ] Commit `ID8: ghi events/lux/sessions + ack`.

### Task 3.K2 – ID8 firmware gửi bù + ID5 cảnh báo thiếu sáng [Khoa]

**Files:** Modify `firmware/pomodoro/session_queue.ino`, `node-red/khoa.json`.

- [ ] `session_queue.ino`: mảng vòng 10 `SessionRec` trong RAM (`// ponytail: mất khi rút điện, chuyển sang Preferences nếu cần giữ qua mất nguồn`); đầy thì bỏ bản cũ nhất.
- [ ] `sessionPush()` thêm vào hàng đợi rồi gửi ngay; `sessionQueueLoop()` mỗi 30 s (và ngay khi MQTT vừa nối lại) gửi lại các bản chưa ack; `sessionAck(ts)` xóa đúng bản có `startTs == ts`.
- [ ] Lúc bắt đầu mà chưa có giờ NTP (`!timeValid()`): lưu `startMs`; đến lúc gửi mới tính `start_ts = nowTs() - (millis()-startMs)/1000`; vẫn chưa có giờ thì chưa gửi.
- [ ] Node-RED: luật cảnh báo – `%` < ngưỡng **liên tục 30 s** và trạng thái gần nhất là `study` → gửi `{"type":"msg","text":"Phong thieu sang, hay bat den!","sec":10}` và `{"type":"beep","times":2}` qua `to-cmd`, ghi `events` type `low_light`; tối đa 1 lần / 5 phút.
- [ ] Ngưỡng đọc/ghi bảng `settings` (`lux_threshold`) thay cho flow context: đọc khi Node-RED khởi động, kéo slider thì PATCH.
- [ ] Kiểm tra gửi bù: chỉnh 1 phút, bắt đầu học, **tắt WiFi router** đến hết pha, bật lại → trong ≤ 30 s bảng `sessions` có dòng, Serial in đã nhận ack, không có dòng trùng.
- [ ] Kiểm tra cảnh báo: che LDR khi đang học → sau ~30 s OLED hiện lời nhắc + kêu 2 tiếng; che tiếp 2 phút → không kêu lại.
- [ ] Commit `ID8: gui bu phien` và `ID5: canh bao thieu sang`.

### Task 3.P1 – Views, dữ liệu mẫu và ID9 biểu đồ [Phát]

**Files:** Create `supabase/views.sql`, `supabase/seed.sql`; Modify `node-red/phat.json`.

- [ ] `views.sql` (giờ VN = `at time zone 'Asia/Ho_Chi_Minh'`):
  - `v_daily_minutes(day date, minutes int)` – tổng `focus_min` theo ngày
  - `v_subject_minutes(day date, subject text, minutes int)`
  - `v_lux_hourly(day date, hour int, lux_avg real)`
  - `v_plan_vs_actual(day date, planned int, actual int)` – `planned` = số dòng `plans`, `actual` = số cặp (ngày, môn) có ≥ 1 chu kỳ `completed`
- [ ] `seed.sql`: 4 tuần gần nhất, `dev='pomo-01'`, 3 môn, ~60 chu kỳ (≈ 85% completed, dồn vào 19h–22h), `lux_readings` mỗi giờ, vài `low_light`, `plans` khớp một phần. Dòng đầu ghi cách xóa: `delete from sessions where start_ts < '<ngày bắt đầu dùng thật>';` (tương tự các bảng khác).
- [ ] Chạy `views.sql` rồi `seed.sql` trên Supabase (báo Khoa trước khi chạy seed).
- [ ] Trang **Thống kê**: `ui-button-group` Tuần này / Tháng này → tính khoảng ngày → GET các view với `?day=gte.X&day=lte.Y` → 4 `ui-chart` (phút/ngày; phút theo môn; kế hoạch vs thực tế cột đôi; độ sáng TB theo giờ dạng đường) + group **Tổng kết** (tổng thời gian, so với tuần trước, số phiên/tỉ lệ hoàn thành, số lần cảnh báo thiếu sáng).
- [ ] Tự tải lại khi mở trang và khi có link in `session` mới.
- [ ] Kiểm tra: so cột "phút/ngày" với `select sum(focus_min) ...` chạy tay trong SQL Editor → khớp; khoảng không có dữ liệu → biểu đồ trống, không báo lỗi.
- [ ] Commit `ID9: views + seed + trang Thong ke`.

### Task 3.P2 – ID7 trang Lịch học AI [Phát]

**Files:** Modify `node-red/phat.json`.

**Interfaces:** Consumes API của Task 2.P2. Produces dòng bảng `plans` (`week_start, day, start_time, end_time, subject, cycles, score, reason`) – Task 4.P1 và 4.P2 dùng.

- [ ] Group **Chọn cách nhập**: `ui-button-group` "Chọn ngày rảnh và môn cần học" / "Gửi thời khóa biểu trên lớp" → ẩn/hiện form tương ứng.
- [ ] Cách 1: 7 checkbox Thứ 2…CN, `ui-textarea` môn (mỗi dòng `Ten mon, so buoi`), ô "Môn ưu tiên", nút "Sinh lịch tuần bằng AI", "Xóa".
- [ ] Cách 2: `ui-file-input` CSV (`day,start,end,subject`) hoặc textarea dán cùng định dạng; dòng sai định dạng → báo số dòng, không gửi.
- [ ] Gọi `POST ${AI_URL}/schedule` → `ui-table` (Ngày, Giờ, Môn, Số chu kỳ "3 × 25 phút", Khả năng hoàn thành %, Lý do). `ok:false` hoặc AI không chạy → hiện thông báo, không treo.
- [ ] "Lưu lịch": xóa `plans` của tuần đó (`DELETE ?week_start=eq.X`) rồi POST các dòng mới → "Đã lưu N buổi". "Sinh lại lịch" gọi lại `/schedule`.
- [ ] Group **Khả năng hoàn thành theo giờ**: `GET /scores` → `ui-chart` cột 6h–23h, hiện "Huấn luyện lần cuối", "Độ chính xác (tập kiểm tra 20%)".
- [ ] Kiểm tra: tắt Flask rồi bấm sinh lịch → dashboard báo "Dịch vụ AI chưa chạy"; bấm Lưu 2 lần → `plans` không bị nhân đôi.
- [ ] Commit `ID7: trang Lich hoc AI + luu plans`.

### ✅ Mốc M3 – chuyển sang dữ liệu thật [Cả 2]
- [ ] Từ đây **máy chính = laptop Phát**: import `common` → `khoa` → `phat`; Khoa tắt Node-RED của mình.
- [ ] Học thử 1 chu kỳ 1 phút → `sessions` có dòng mới → trang Thống kê tự cập nhật.
- [ ] Xếp lịch → Lưu → thấy trong bảng `plans`.

---

## Giai đoạn 4 – Thông báo, cấu hình, hoàn thiện giao diện (tuần 9–10)

| Khoa | Phát |
|---|---|
| Task 4.K1 – ID6 Telegram | Task 4.P1 – ID7 nhắc giờ học + huấn luyện lại + "Hôm nay" |
| Task 4.K2 – ID3 WiFiManager | Task 4.P2 – ID11 Email tuần |
| Task 4.K3 – Nhật ký học từ Supabase | Task 4.P3 – Trạng thái hệ thống |

### Task 4.K1 – ID6 Telegram [Khoa]

**Files:** Modify `node-red/khoa.json`.

- [ ] link in `state` → lọc `event == "done"` và `dev == "pomo-01"` → nếu `settings.telegram_enabled` → `telegram sender` tới `TELEGRAM_CHAT_ID`: "Đã học xong {total} chu kỳ môn {subject} lúc {HH:mm}".
- [ ] Group **Telegram** (trang Phụ huynh): `ui-switch` "Báo về điện thoại khi hoàn thành" (ghi `settings`), text "Trạng thái bot" (Đã kết nối / Chưa kết nối theo status của node).
- [ ] Kiểm tra: chạy hết 1 phiên 1 phút × 1 chu kỳ → điện thoại nhận đúng 1 tin; tắt switch → không nhận.
- [ ] Commit `ID6: Telegram khi hoan thanh`.

### Task 4.K2 – ID3 WiFiManager [Khoa]

**Files:** Modify `firmware/pomodoro/wifi_setup.ino` (chỉ file này), `secrets.example.h`.

- [ ] `connectWiFi()`: nếu **giữ nút SW của núm xoay lúc cắm điện** → `wm.resetSettings()`; `wm.setConfigPortalTimeout(180)`; `wm.autoConnect("Pomodoro-Setup")`. Hết giờ mà chưa cấu hình → chạy offline (đồng hồ, núm, 7 đoạn vẫn dùng được), MQTT thử lại như cũ.
- [ ] Khi mở portal: `oledShowMessage("Ket noi WiFi Pomodoro-Setup, mo 192.168.4.1", 60)` rồi gọi `oledLoop()` 1 lần để vẽ.
- [ ] Bỏ `WIFI_SSID/WIFI_PASS` khỏi `secrets.example.h` (giữ phần MQTT).
- [ ] Kiểm tra: giữ SW lúc cắm điện → điện thoại thấy WiFi "Pomodoro-Setup" → 192.168.4.1 → chọn WiFi → hộp khởi động lại và online; rút điện cắm lại → tự vào WiFi cũ, không mở portal.
- [ ] Commit `ID3: WiFiManager`.

### Task 4.K3 – Nhật ký học hôm nay từ Supabase [Khoa]

**Files:** Modify `node-red/khoa.json`.

- [ ] Khi mở trang Tổng quan và mỗi khi có `session` mới: GET `sessions` hôm nay (giờ VN) + `events?type=eq.low_light` hôm nay → bảng: Giờ, Môn, Chu kỳ `1/3`, Thời lượng `25/25 phút`, Tạm dừng `1 lần (2 phút)`/`Không`, Kết quả `Hoàn thành`/`Bỏ dở`/`Cảnh báo thiếu sáng`, sắp theo giờ.
- [ ] Kiểm tra: khớp từng dòng với bảng `sessions` trên Supabase.
- [ ] Commit `ID1/ID8: nhat ky hoc tu Supabase`.

### Task 4.P1 – ID7 nhắc giờ học, huấn luyện lại, thẻ "Hôm nay" [Phát]

**Files:** Modify `node-red/phat.json`.

- [ ] Nhắc giờ học: `inject` mỗi 60 s → GET `plans?day=eq.<hôm nay>&start_time=eq.<HH:mm:00>` → mỗi dòng gửi qua `to-cmd`: `subject` (tên môn), `msg` "Den gio hoc: {mon} - {cycles} chu ky" (10 s), `beep` 3; ghi `events` type `schedule_reminder` (`dev = "pomo-01"`). Nhớ id đã nhắc để không nhắc 2 lần.
- [ ] Nút "Huấn luyện lại": GET `sessions?select=start_ts,planned_min,focus_min,completed` → `POST ${AI_URL}/train` → cập nhật "Huấn luyện lần cuối" + "Độ chính xác".
- [ ] Group **Hôm nay**: Kế hoạch (tổng `cycles` trong `plans` hôm nay), Đã hoàn thành `3 chu kỳ (60%)`, Tổng thời gian học, Buổi tiếp theo `Thứ 4 19:30 – Vật lý 3×25 phút`, Cảnh báo thiếu sáng hôm nay `1 lần (lúc 20:10)`.
- [ ] Nút "Nhắc nhở" (Task 2.P1) ghi thêm `events` type `parent_beep`.
- [ ] Kiểm tra: tạo 1 dòng `plans` cho 2 phút tới → đúng giờ hộp kêu + OLED hiện môn; bấm Huấn luyện lại → `real_sessions` > 0.
- [ ] Commit `ID7: nhac gio hoc + huan luyen lai + the Hom nay`.

### Task 4.P2 – ID11 Email báo cáo tuần [Phát]

**Files:** Modify `node-red/phat.json`.

- [ ] Group **Email báo cáo hằng tuần**: ô email phụ huynh (nhiều địa chỉ, dấu phẩy; kiểm tra định dạng từng địa chỉ) → "Lưu email" ghi `settings.parent_emails`; text "Tự động gửi: 20:00 tối Chủ nhật"; nút "Gửi thử báo cáo tuần này"; "Lần gửi gần nhất".
- [ ] `inject` cron 20:00 Chủ nhật (giờ máy = giờ VN) → lấy dữ liệu tuần từ các view + `events` + `plans` tuần tới → function dựng HTML (tổng thời gian, tỉ lệ hoàn thành, kế hoạch vs thực tế, theo môn, số lần thiếu sáng, lịch tuần tới) → node `e-mail` (smtp.gmail.com, 465, TLS, App Password).
- [ ] Lỗi gửi: `catch` → `delay` 30 phút → gửi lại, tối đa 3 lần; mỗi lần ghi `email_reports` (`week`, `to_email`, `status`, `attempt`). Chưa có email phụ huynh → không gửi, ghi status "Chua co email".
- [ ] Group **Lịch sử gửi email**: `ui-table` từ `email_reports` (Tuần, Người nhận, Thời điểm gửi, Trạng thái).
- [ ] Kiểm tra: "Gửi thử" → hộp thư nhận email, số liệu khớp trang Thống kê; dùng App Password sai → `email_reports` có dòng lỗi và tự thử lại (khi test đổi delay thành 1 phút, xong trả lại 30).
- [ ] Commit `ID11: email bao cao tuan`.

### Task 4.P3 – Trạng thái hệ thống [Phát]

**Files:** Modify `node-red/phat.json`.

- [ ] Group **Trạng thái hệ thống**: MQTT – EMQX Cloud (node `status` theo dõi `mqtt in` của tab Chung), Supabase (GET `settings?limit=1` mỗi 60 s), Dịch vụ AI (`GET /health` mỗi 30 s), Gmail ("Đã cấu hình" nếu đã lưu email + lần gửi gần nhất thành công).
- [ ] Kiểm tra: tắt Flask → trong ≤ 30 s hiện "Không chạy"; bật lại → "Đang chạy".
- [ ] Commit `dashboard: trang thai he thong`.

### ✅ Mốc M4 – đủ 11 chức năng [Cả 2]
- [ ] Chạy trọn kịch bản demo (Task 5.3) trên breadboard; tick từng ID ở bảng tiến độ tổng.

---

## Giai đoạn 5 – Lắp hộp, bảo mật, demo, báo cáo (tuần 10)

### Task 5.1 – Phần cứng [Cả 2]
- [ ] Test lại toàn bộ trên breadboard → hàn lên phíp đồng (ESP32 cắm trên header cái để còn rút ra nạp lại).
- [ ] Lắp vào hộp in 3D theo Hình 3–5; kiểm tra lỗ LDR không bị che, buzzer sát lỗ thoát âm, cáp USB vừa khe.
- [ ] **[Khoa]** Hiệu chuẩn lại `LUX_INVERT` và ngưỡng mặc định khi LDR đã nằm trong hộp (ánh sáng qua lỗ nhỏ tối hơn lúc trên breadboard).

### Task 5.2 – Bảo mật bản nộp [Khoa]
- [ ] `net.setCACert(rootCA)` thay `setInsecure()` (CA của EMQX Serverless: tải `https://assets.emqx.com/data/emqxsl-ca.crt`, dán vào chuỗi `rootCA`) trong `pomodoro.ino` — báo Phát pull.
- [ ] Rà Git: `git log -p | grep -iE "service_role|apikey|token|password"` không ra bí mật thật.

### Task 5.3 – Kịch bản demo [Cả 2]
- [ ] 1. Giữ SW lúc cắm điện → cấu hình WiFi bằng điện thoại (ID3) **[Khoa]**
- [ ] 2. Vặn núm 1 phút / 2 chu kỳ (ID4) **[Khoa]**
- [ ] 3. Bấm nút bắt đầu → dashboard đổi trạng thái (ID1) **[Khoa]**
- [ ] 4. Gõ lời nhắn → OLED (ID2); bấm "Nhắc nhở" → buzzer (ID10) **[Phát]**
- [ ] 5. Che LDR → gauge tụt, cảnh báo + kêu (ID5) **[Khoa]**
- [ ] 6. Hết phiên → Telegram (ID6), Supabase có dòng (ID8) **[Khoa]**, biểu đồ cập nhật (ID9) **[Phát]**
- [ ] 7. Lịch học AI → xếp lịch → Lưu (ID7) **[Phát]**
- [ ] 8. "Gửi thử" email (ID11) **[Phát]**
- [ ] Chạy trọn kịch bản 2 lần liên tiếp không lỗi.

### Task 5.4 – Báo cáo và proposal [Cả 2]
- [ ] **[Phát]** Đổi mọi chữ "HiveMQ Cloud" → "EMQX Cloud" trong proposal/báo cáo (bảng chức năng, Hình 9 "Trạng thái hệ thống", Hình 10 cột "MQTT Broker"), thêm 1 câu lý do: HiveMQ ngừng gói Serverless free.
- [ ] **[Phát]** Sửa proposal theo [docs/lo_trinh_lam_pomodoro.md §9](docs/lo_trinh_lam_pomodoro.md): bỏ câu thừa ở ID2 và ID10, **sửa bảng điểm mục 4** (nâng cao tối đa 4,0, tổng tối đa 10,0).
- [ ] **[Khoa]** Chụp ảnh/quay video phần cứng + Serial cho ID1, ID3, ID4, ID5, ID6, ID8.
- [ ] **[Phát]** Chụp 4 trang dashboard thật (đối chiếu wireframe Hình 6–9) cho ID2, ID7, ID9, ID10, ID11.
- [ ] **[Cả 2]** Mỗi người tự giải thích được luồng dữ liệu của **mọi** ID (vấn đáp), không chỉ phần mình.
- [ ] **[Cả 2]** Export lần cuối 3 file Node-RED từ máy chính, commit, tag `v1.0`.

---

## Rủi ro cần để ý khi review

1. **Mất mạng giữa phiên học** → đồng hồ vẫn chạy, phiên được gửi bù, Supabase không trùng dòng (kiểm ở Task 3.K2).
2. **NTP chưa đồng bộ lúc bật máy** → không ghi phiên với năm 1970 (kiểm ở Task 3.K2, nhánh `!timeValid()`).
3. **Nút dội / giữ lâu** → giữ 2 s chỉ sinh đúng 1 `stop`, nhả ra không thành `pause` (kiểm ở Task 1.K).
4. **Lời nhắn có dấu hoặc quá dài** → OLED không ra ký tự rác, không vượt bộ đệm MQTT 1024 byte (kiểm ở Task 1.P).
5. **AI không xếp được hoặc không chạy** → dashboard báo lỗi rõ, không treo, `plans` không bị nhân đôi (kiểm ở Task 2.P2, 3.P2).
