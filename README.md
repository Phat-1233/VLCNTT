# Pomodoro Để Bàn – Nhóm 6 (Vật lý cho CNTT)

Đồng hồ Pomodoro để bàn chạy ESP32, kết nối MQTT (EMQX Cloud Serverless – thay HiveMQ Cloud vì HiveMQ đã ngừng gói free), xử lý bằng Node-RED, lưu dữ liệu trên Supabase. Có thêm dịch vụ AI tự xây để xếp lịch học.

| Thành viên | MSSV | Phụ trách |
|---|---|---|
| Nguyễn Hoàng Đăng Khoa | 24127187 | ID1, ID3, ID4, ID5, ID6, ID8 |
| Nguyễn Thành Phát | 24127483 | ID2, ID7, ID9, ID10, ID11 |

**Kế hoạch + tick tiến độ:** [plan.md](plan.md) (thứ tự làm, ai làm gì, file nào của ai).

## Cấu trúc

```
VLCNTT/
├── firmware/pomodoro/     # Arduino sketch cho ESP32 (mở pomodoro.ino bằng Arduino IDE)
│   ├── pomodoro.ino       # chung: WiFi, NTP, MQTT, chia lệnh pomodoro/cmd
│   ├── config.h           # chung: sơ đồ chân, topic, struct trạng thái đồng hồ
│   ├── secrets.example.h  # mẫu -> chép thành secrets.h (đã gitignore)
│   ├── wifi_setup.ino     # ID3  Khoa – kết nối WiFi (sau này WiFiManager)
│   ├── timer_button.ino   # ID1  Khoa – nút nhấn + máy trạng thái
│   ├── session_queue.ino  # ID8  Khoa – lưu tạm phiên học, gửi bù khi có ack
│   ├── knob_7seg.ino      # ID4  Khoa – núm xoay -> 7 đoạn
│   ├── ldr.ino            # ID5  Khoa – LDR -> MQTT
│   ├── oled.ino           # ID2  Phát – lệnh web -> OLED
│   └── buzzer.ino         # ID10 Phát – lệnh web -> buzzer
├── node-red/
│   ├── common.json        # tab Chung + broker + khung dashboard (đóng băng)
│   ├── khoa.json          # tab Khoa: ID1 ID3 ID4 ID5 ID6 ID8
│   └── phat.json          # tab Phát: ID2 ID7 ID9 ID10 ID11
├── ai-service/            # ID7 Phát – Flask + logistic regression (numpy) + CSP
├── supabase/schema.sql    # ID8 Khoa – schema v2, bật RLS
├── docs/lo_trinh_lam_pomodoro.md   # sơ đồ chân, lỗi hay gặp
└── plan.md                # KẾ HOẠCH CHÍNH: task từng giai đoạn, hợp đồng v2, ai làm gì
```

## Topic MQTT (hợp đồng chung)

| Topic | Chiều | Ví dụ |
|---|---|---|
| `pomodoro/state` | ESP32 → Node-RED | `{"dev":"pomo-01","ts":1732170000,"event":"start","state":"study","subject":"Toan","cycle":1,"total":4,"remain":1500}` |
| `pomodoro/lux` | ESP32 → Node-RED | `{"dev":"pomo-01","ts":1732170002,"lux":1830}` |
| `pomodoro/session` | ESP32 → Node-RED | Mỗi **chu kỳ học** 1 bản tin: `{"dev":"pomo-01","start_ts":...,"end_ts":...,"subject":"Toan","cycle":2,"total":3,"planned_min":25,"focus_min":25,"pauses":1,"pause_sec":120,"completed":true}` |
| `pomodoro/ack` | Node-RED → ESP32 | `{"start_ts":...}` |
| `pomodoro/cmd` | Node-RED → ESP32 | `{"type":"msg","text":"...","sec":10}` · `{"type":"subject","name":"Toan"}` · `{"type":"beep","times":3}` |

Đổi JSON thì phải sửa cả 2 phía (ESP32 và Node-RED) và báo người kia. Hợp đồng đầy đủ (v2): xem mục "Hợp đồng v2" trong [plan.md](plan.md). Bản tin giả lập dùng `"dev":"sim-01"` và không được ghi lên Supabase.

## Cài đặt (mỗi máy làm 1 lần)

### 1. Arduino IDE – firmware ESP32
1. **File → Preferences** → ô *Additional boards manager URLs* dán:
   `https://espressif.github.io/arduino-esp32/package_esp32_index.json`
2. **Boards Manager** → cài **esp32 by Espressif Systems**. Không cài nhầm gói *Arduino ESP32 Boards* (by Arduino): gói đó dành cho Arduino Nano ESP32.
3. **Library Manager** → cài đúng tác giả:

   | Thư viện | Tác giả | Dùng cho |
   |---|---|---|
   | PubSubClient | Nick O'Leary | MQTT |
   | ArduinoJson (v7) | Benoit Blanchon | JSON |
   | Adafruit SSD1306 (chọn *Install all*) | Adafruit | ID2 OLED |
   | Adafruit GFX Library | Adafruit | ID2 OLED |
   | TM1637 | Avishay Orpaz | ID4 7 đoạn |
   | WiFiManager | tzapu | ID3 |

4. **Tools → Board → ESP32 Dev Module**.
5. **Tools → Partition Scheme → Huge APP (3MB No OTA/1MB SPIFFS)**. Cả nhóm phải chọn giống nhau: phân vùng mặc định chỉ 1,3 MB, code đã chiếm 85%; với Huge APP còn khoảng 35%.
6. Chép `firmware/pomodoro/secrets.example.h` thành `secrets.h`, điền WiFi + EMQX (Address, user `esp32`). `secrets.h` đã gitignore.
7. Mở `firmware/pomodoro/pomodoro.ino` → **✓ Verify** → **Upload** → Serial Monitor 115200, thấy `Ket noi MQTT (EMQX)... OK` là được.
   - Không thấy cổng COM: cài driver **CH340** hoặc **CP210x** (tùy chip USB trên board).

### 2. Node-RED
1. Cần Node.js ≥ 18 (`node -v`). Chưa có thì cài bản LTS ở nodejs.org.
2. Cài và chạy:
   ```
   npm install -g --unsafe-perm node-red
   node-red
   ```
   Mở `http://localhost:1880`.
3. Menu ☰ → **Manage palette** → **Install** 3 gói:
   `@flowfuse/node-red-dashboard`, `node-red-node-email`, `node-red-contrib-telegrambot`.
4. Import flow: Menu ☰ → **Import**.
   - Import **đúng thứ tự** `node-red/common.json` → `khoa.json` → `phat.json`; khi Node-RED hỏi trùng node thì chọn **Replace** (không chọn *Import copy*, sẽ đổi id làm đứt link).
   - `common.json` = tab **Chung** (MQTT vào/ra, link `state`/`lux`/`session`/`to-cmd`/`to-ack`, giả lập `sim-01`) + broker EMQX + toàn bộ khung dashboard (ui-base, theme, 4 page, 20 group). **Đóng băng**: cần sửa thì nhắn nhau trước.
   - `khoa.json` / `phat.json` = tab riêng của từng người, chỉ nhận/gửi dữ liệu qua các node link.
5. Mở node cấu hình broker **EMQX Cloud** → *Server* = Address của EMQX, port 8883, bật TLS; tab *Security* nhập user/pass `nodered` → **Deploy**.
6. Sửa flow xong: chọn tab của mình → Menu ☰ → **Export** → *Current flow* → ghi đè đúng file của mình rồi commit. Không export *All flows*.
7. Dashboard: `http://localhost:1880/dashboard`.

> EMQX Serverless tự dừng deployment nếu 30 ngày không có client kết nối → vào console EMQX bật lại.

### 3. Biến môi trường (bí mật – không ghi vào Git, không ghi vào flow)
Mở **CMD**, chạy (thay giá trị thật):
```
setx SUPABASE_URL "https://xxxxxxxx.supabase.co"
setx SUPABASE_KEY "eyJ..."
setx AI_URL "http://localhost:5000"
setx TELEGRAM_TOKEN "123456789:AA..."
setx TELEGRAM_CHAT_ID "123456789"
```
Sau đó **đóng hết CMD, mở CMD mới** (`echo %SUPABASE_URL%` để kiểm tra) và **khởi động lại Node-RED**. Trong node function đọc bằng `env.get("SUPABASE_URL")`.

| Biến | Lấy ở đâu | Ai giữ |
|---|---|---|
| `SUPABASE_URL`, `SUPABASE_KEY` | Supabase → Project Settings → API (*Project URL*, **service_role / secret key**). Các bảng đã bật RLS nên key *anon/public* sẽ bị chặn | Khoa |
| `TELEGRAM_TOKEN` | @BotFather khi tạo bot | Khoa |
| `TELEGRAM_CHAT_ID` | Nhắn bot 1 tin rồi mở `https://api.telegram.org/bot<TOKEN>/getUpdates`, lấy `chat.id` | Khoa |
| `AI_URL` | Cố định `http://localhost:5000` | — |
| App Password Gmail | myaccount.google.com/apppasswords (cần bật xác minh 2 bước). Nhập vào node email, **không** đặt biến môi trường | Phát |

> Tài khoản Telegram **mới** đăng ký ở Việt Nam hiện bị thu phí SMS (mua Premium 1 tuần). Dùng tài khoản đã có sẵn để tạo bot.

### 4. AI service (ID7)
```
cd ai-service
python -m venv .venv
.venv\Scripts\activate
pip install -r requirements.txt
python app.py
```
Chạy tại `http://localhost:5000`. Thử nhanh: `http://localhost:5000/health`. Chi tiết API: `ai-service/README.md`.

### 5. Supabase (ID8)
Supabase → **SQL Editor** → **New query** → dán `supabase/schema.sql` → **Run** (file tự xóa và tạo lại 6 bảng, bật RLS).

## Quy ước làm việc
- Mỗi người chỉ sửa tab/file của mình; sửa `config.h` hoặc `pomodoro.ino` thì báo trước.
- Không commit `secrets.h`, mật khẩu, API key.
- Pull trước khi làm, commit nhỏ và thường xuyên.
