# ID7 – Dịch vụ AI xếp lịch học (Phát)

Tự xây dựng bằng Python + numpy, không dùng thư viện hoặc dịch vụ AI có sẵn.

| File | Nội dung |
|---|---|
| `model.py` | Logistic regression tự viết: sigmoid, log-loss, gradient descent, lưu/đọc trọng số |
| `synth_data.py` | Sinh dữ liệu giả lập để huấn luyện lúc chưa có dữ liệu thật (cold start) |
| `scheduler.py` | Tìm khung giờ trống + CSP quay lui với heuristic MRV |
| `app.py` | Flask: `/schedule`, `/train`, `/scores`, `/health` |

## Chạy
```bash
python -m venv .venv
.venv\Scripts\activate
pip install -r requirements.txt
python app.py
```
Lần đầu chạy sẽ tự huấn luyện bằng dữ liệu giả và lưu `model_weights.json` + `model_meta.json` (không commit).

## Test
```bash
python test_ai.py      # in "OK" từng test, cuối cùng "7/7 test qua"
```

## Thử bằng curl / Postman
```bash
# Cách 1: ngày rảnh + môn
curl -X POST localhost:5000/schedule -H "Content-Type: application/json" -d "{\"mode\":\"free_days\",\"free_days\":[0,2,4,6],\"subjects\":[{\"name\":\"Toan\",\"sessions\":2},{\"name\":\"Ly\",\"sessions\":1}],\"session_min\":90,\"study_min\":25,\"break_min\":5,\"priority\":\"Ly\"}"

# Cách 2: thời khóa biểu (day: 0 = Thứ 2 ... 6 = Chủ nhật)
curl -X POST localhost:5000/schedule -H "Content-Type: application/json" -d "{\"mode\":\"timetable\",\"timetable\":[{\"day\":2,\"start\":\"07:30\",\"end\":\"11:30\",\"subject\":\"Hoa\"}],\"subjects\":[{\"name\":\"Hoa\",\"sessions\":2}],\"session_min\":60}"

# Huấn luyện lại (body: các dòng bảng sessions lấy từ Supabase)
curl -X POST localhost:5000/train -H "Content-Type: application/json" -d "{\"sessions\":[]}"

# Xác suất hoàn thành theo giờ (vẽ biểu đồ) + trained_at, test_accuracy
curl "localhost:5000/scores?day=2"
```

Mỗi buổi trong `plan` có: `day`, `day_name`, `start`, `end`, `subject`, `score`,
`cycles` = `max(1, session_min // (study_min + break_min))`, và `reason`:
1. TKB có tiết cùng môn 1–2 ngày sau → `"Truoc tiet {mon} {thu}"`
2. môn ưu tiên → `"Mon uu tien"`
3. còn lại → `"Kha nang hoan thanh cao ({p}%)"`

## Ràng buộc khi xếp lịch
- Không trùng thời khóa biểu (cộng 30 phút đi lại trước và sau giờ học).
- Các buổi không chồng nhau; tối đa 3 buổi mỗi ngày (`max_per_day`).
- Một môn tối đa 1 buổi mỗi ngày.
- MRV: xếp trước buổi còn ít khung hợp lệ nhất (hòa thì môn ưu tiên trước); trong mỗi buổi, thử khung có xác suất hoàn thành cao nhất trước.
- Môn ưu tiên (`priority`) được cộng 0.1 vào điểm khung khi giải CSP. Tên môn so sánh không phân biệt hoa/thường, có dấu/không dấu.
