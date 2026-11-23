# Sơ đồ cắm dây – Hộp Pomodoro

> **File bắt buộc.** Mỗi khi cắm thêm, đổi hoặc gỡ một linh kiện thì cập nhật file này **cùng commit đó**, để rút ra cắm lại vẫn đúng.
> Chân trong file này phải khớp với `firmware/pomodoro/config.h`. Đổi chân thì sửa cả 2 chỗ.

**Cập nhật lần cuối:** 23/11 – Giai đoạn 2 phần Khoa: xong núm xoay + 7 đoạn (ID4), LDR (ID5). Còn buzzer (ID10, Phát).

## Quy tắc an toàn (đọc trước khi cắm)

1. **Rút cáp USB** trước khi cắm hoặc gỡ dây. Kiểm tra lại rồi mới cắm USB.
2. Chân tín hiệu ESP32 chỉ chịu **3,3 V**. Không cắm 5V vào GPIO.
3. Mọi linh kiện dùng **GND chung** với ESP32.
4. Tìm chân theo **chữ in trên board** (`D27`, `27`, `G27` đều là GPIO 27), không đếm vị trí. Board 30 chân và 38 chân xếp chân khác nhau.

## Trạng thái hiện tại

| Linh kiện | ID | Giai đoạn | Đã cắm? |
|---|---|---|---|
| Nút nhấn | ID1 | 1 | ✅ |
| OLED SSD1306 0,96" | ID2 | 1 | ✅ |
| Núm xoay KY-040 | ID4 | 2 | ✅ |
| 7 đoạn TM1637 | ID4 | 2 | ✅ |
| Quang trở LDR (module 4 chân) | ID5 | 2 | ✅ |
| MOSFET D4184 + buzzer | ID10 | 2 | ⬜ |

## Bố trí breadboard

- ESP32 cắm **vắt ngang khe giữa** breadboard, mỗi hàng chân nằm một bên. Board to nên một bên chỉ còn 1 hàng lỗ trống: cắm dây vào hàng lỗ trống đó, cùng cột với chân cần dùng.
- Nếu không còn lỗ trống: dùng **dây đực–cái** cắm thẳng vào chân board.
- Dùng 2 thanh nguồn dọc mép breadboard:
  - Thanh **–** (xanh): nối 1 chân **GND** của ESP32 → làm GND chung.
  - Thanh **+** (đỏ): nối chân **3V3** của ESP32 → nguồn 3,3 V cho OLED, KY-040, TM1637, LDR.
  - **5V** (chân `VIN`/`5V`) chỉ dùng cho buzzer qua MOSFET, đi dây riêng, **không** nối vào thanh + 3,3 V.

## Bảng chân tổng hợp

| Linh kiện | Chân linh kiện | → Chân ESP32 | Ghi chú |
|---|---|---|---|
| **Nút nhấn** | chân A | **GPIO 27** | Không cần điện trở: dùng kéo lên trong chip (`INPUT_PULLUP`). Chưa bấm = HIGH, bấm = LOW |
| | chân B | **GND** | |
| **OLED SSD1306** | VCC | **3V3** | Không cắm 5V |
| | GND | **GND** | |
| | SDA | **GPIO 21** | I2C, địa chỉ 0x3C |
| | SCL | **GPIO 22** | |
| *Giai đoạn 2 – chưa cắm:* | | | |
| KY-040 | CLK | GPIO 32 | |
| | DT | GPIO 33 | |
| | SW | GPIO 25 | |
| | + | 3V3 | |
| | GND | GND | |
| TM1637 | CLK | GPIO 18 | |
| | DIO | GPIO 19 | |
| | VCC | 3V3 | |
| | GND | GND | |
| LDR (module 4 chân) | AO | GPIO 34 | Bắt buộc chân ADC1 (32–39) |
| | VCC | 3V3 | Không cắm 5V (AO sẽ vượt 3,3 V) |
| | GND | GND | |
| | DO | *để trống* | Không dùng |
| D4184 | SIG | GPIO 26 | VIN = 5V, OUT → buzzer, GND chung |

## Chi tiết từng linh kiện

### Nút nhấn (ID1) ✅

```
ESP32 GPIO27 ────┐
               [NÚT]
ESP32 GND ───────┘
```

- **Nút 4 chân:** đặt vắt ngang khe giữa breadboard, lấy **2 chân chéo nhau** (trên-trái và dưới-phải). 2 chân cùng một cạnh có thể luôn thông nhau → nút như bị bấm mãi.
- **Nút 2 chân:** cắm chiều nào cũng được.
- Kiểm tra: mở Serial Monitor 115200, bấm nút → thấy `[ID1] start -> study ...`.

### OLED SSD1306 (ID2) ✅

```
OLED VCC ── 3V3
OLED GND ── GND
OLED SDA ── GPIO21
OLED SCL ── GPIO22
```

- Thứ tự chân khác nhau tùy module (`GND VCC SCL SDA` hoặc `VCC GND SCL SDA`). **Cắm theo chữ in trên module.** Cắm ngược VCC/GND là hỏng OLED.
- Kiểm tra: Serial **không** có dòng `Khong tim thay OLED`. Nếu có → SDA/SCL bị đảo hoặc lỏng.

### Núm xoay KY-040 (ID4) ✅

```
KY-040 CLK ── GPIO32
KY-040 DT  ── GPIO33
KY-040 SW  ── GPIO25
KY-040 +   ── 3V3   (KHÔNG cắm 5V: module kéo CLK/DT lên đúng điện áp chân +)
KY-040 GND ── GND
```

- Không cần điện trở ngoài: module có sẵn điện trở 10k cho CLK/DT; SW dùng kéo lên trong chip.
- Kiểm tra: lúc đồng hồ đang chờ, vặn 1 nấc → Serial in `[ID4] phut = 26`; nhấn núm → `[ID4] che do chinh: chu ky`.
- Vặn phải mà số **giảm** → đảo 2 dây CLK và DT.

### Màn hình 7 đoạn TM1637 (ID4) ✅

```
TM1637 CLK ── GPIO18
TM1637 DIO ── GPIO19
TM1637 VCC ── 3V3
TM1637 GND ── GND
```

- Kiểm tra: cắm điện → màn hình hiện `  25` (số phút). Nhấn núm → `C  4`. Đang học → `mm:ss` đếm ngược.
- Màn hình tối/không hiện gì → kiểm tra CLK/DIO có bị đảo không.

### Cảm biến ánh sáng LDR – module 4 chân (ID5) ✅

```
LDR AO  ── GPIO34   (tín hiệu analog)
LDR VCC ── 3V3      (KHÔNG cắm 5V)
LDR GND ── GND
LDR DO  ── để trống (ngõ ra số so ngưỡng bằng biến trở xanh, không dùng)
```

- Phải là chân **AO**, không phải DO. DO chỉ ra 0/1.
- **Bắt buộc GPIO 34** (hoặc 35/36/39): chân ADC2 không đọc được khi bật WiFi.
- Biến trở xanh trên module chỉ chỉnh DO, vặn nó **không** ảnh hưởng AO.
- Kiểm tra: dashboard trang Tổng quan → gauge Độ sáng phòng; che tay lên cảm biến → % tụt.

## Lỗi hay gặp

| Triệu chứng | Nguyên nhân |
|---|---|
| Nút tự "bấm" liên tục | Lấy 2 chân cùng cạnh của nút 4 chân → dùng 2 chân chéo |
| `Khong tim thay OLED` | SDA/SCL đảo, lỏng dây, hoặc chưa cấp 3V3 |
| Vặn núm phải mà số giảm | Đảo 2 dây CLK và DT của KY-040 |
| LDR luôn ra 0 hoặc 4095 | Cắm nhầm DO thay vì AO, hoặc dùng chân ADC2 |
| Nạp code báo `Connecting...` mãi | Giữ nút **BOOT** trên board lúc hiện `Connecting...` |
