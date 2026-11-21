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
│   ├── timer_button.ino   # ID1  Khoa – nút nhấn + máy trạng thái (TODO)
│   ├── knob_7seg.ino      # ID4  Khoa – núm xoay -> 7 đoạn (TODO)
│   ├── ldr.ino            # ID5  Khoa – LDR -> MQTT (TODO)
│   ├── oled.ino           # ID2  Phát – lệnh web -> OLED (đã có)
│   └── buzzer.ino         # ID10 Phát – lệnh web -> buzzer (đã có)
├── node-red/flows.json    # Import vào Node-RED (Menu ☰ -> Import)
├── ai-service/            # ID7 Phát – Flask + logistic regression (numpy) + CSP
├── supabase/schema.sql    # ID8 Khoa – tạo bảng trên Supabase
└── docs/lo_trinh_lam_pomodoro.md   # thứ tự làm, hợp đồng MQTT/JSON, lỗi hay gặp
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

## Chạy thử

### 1. Firmware (ESP32)
1. Arduino IDE → Boards Manager → cài **esp32 by Espressif**; chọn board **ESP32 Dev Module**.
2. Library Manager → cài **PubSubClient**, **ArduinoJson** (v7), **Adafruit SSD1306**, **Adafruit GFX Library**.
3. Chép `secrets.example.h` thành `secrets.h`, điền WiFi + EMQX (host, user `esp32`).
4. Mở `firmware/pomodoro/pomodoro.ino` → Upload → Serial Monitor 115200.

### 2. Node-RED
1. `node-red` → mở `http://localhost:1880`.
2. Menu ☰ → Import → chọn `node-red/flows.json`.
3. Mở node cấu hình broker **EMQX Cloud** → sửa Server (port 8883, bật TLS), tab Security nhập user/pass `nodered` → Deploy.
4. Bấm các node inject “Thử lời nhắn”, “Thử Nhắc nhở” để gửi lệnh xuống ESP32.
5. Sửa flow xong: Menu ☰ → Export → All flows → ghi đè `node-red/flows.json` rồi commit.

### 3. AI service (ID7)
```bash
cd ai-service
python -m venv .venv
.venv\Scripts\activate        # Windows
pip install -r requirements.txt
python app.py                 # http://localhost:5000
```

### 4. Supabase (ID8)
SQL Editor → dán `supabase/schema.sql` → Run.

## Quy ước làm việc
- Mỗi người chỉ sửa tab/file của mình; sửa `config.h` hoặc `pomodoro.ino` thì báo trước.
- Không commit `secrets.h`, mật khẩu, API key.
- Pull trước khi làm, commit nhỏ và thường xuyên.
