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
Lần đầu chạy sẽ tự huấn luyện bằng dữ liệu giả và lưu `model_weights.json`.

## Thử bằng curl / Postman
```bash
# Cách 1: ngày rảnh + môn
curl -X POST localhost:5000/schedule -H "Content-Type: application/json" -d "{\"mode\":\"free_days\",\"free_days\":[0,2,4,6],\"subjects\":[{\"name\":\"Toan\",\"sessions\":2},{\"name\":\"Ly\",\"sessions\":1}],\"session_min\":60}"

# Cách 2: thời khóa biểu (day: 0 = Thứ 2 ... 6 = Chủ nhật)
curl -X POST localhost:5000/schedule -H "Content-Type: application/json" -d "{\"mode\":\"timetable\",\"timetable\":[{\"day\":0,\"start\":\"07:30\",\"end\":\"11:30\"}],\"subjects\":[{\"name\":\"Hoa\",\"sessions\":2}]}"

# Huấn luyện lại (body: các dòng bảng sessions lấy từ Supabase)
curl -X POST localhost:5000/train -H "Content-Type: application/json" -d "{\"sessions\":[]}"

# Xác suất hoàn thành theo giờ (vẽ biểu đồ)
curl "localhost:5000/scores?day=2"
```

## Ràng buộc khi xếp lịch
- Không trùng thời khóa biểu (cộng 30 phút đi lại trước và sau giờ học).
- Các buổi không chồng nhau; tối đa 3 buổi mỗi ngày (`max_per_day`).
- Một môn tối đa 1 buổi mỗi ngày.
- MRV: xếp trước buổi còn ít khung hợp lệ nhất; trong mỗi buổi, thử khung có xác suất hoàn thành cao nhất trước.
