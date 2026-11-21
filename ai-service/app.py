"""ID7 – Dịch vụ AI xếp lịch học (Flask) – PHÁT
Chạy:  python app.py   ->  http://localhost:5000
Node-RED gọi bằng node "http request".

POST /schedule  – xếp lịch tuần
  Cách 1 (ngày rảnh):
    {"mode":"free_days","free_days":[0,2,4,6],
     "subjects":[{"name":"Toan","sessions":2},{"name":"Ly","sessions":1}],
     "session_min":60}
  Cách 2 (thời khóa biểu):
    {"mode":"timetable",
     "timetable":[{"day":0,"start":"07:30","end":"11:30"},{"day":2,"start":"13:00","end":"17:00"}],
     "subjects":[...], "session_min":60}
POST /train     – huấn luyện lại; body {"sessions":[...]} lấy từ bảng sessions (Supabase).
                  Không gửi gì thì huấn luyện bằng dữ liệu giả lập.
GET  /scores?day=0&session_min=60  – xác suất hoàn thành theo giờ (vẽ biểu đồ)
GET  /health
"""
from datetime import datetime, timezone, timedelta
from pathlib import Path

import numpy as np
from flask import Flask, jsonify, request

import synth_data
from model import LogisticRegression, make_features, train_test_split
from scheduler import DAY_NAMES, build_slots, solve, to_hhmm

WEIGHTS = Path(__file__).with_name("model_weights.json")
VN_TZ = timezone(timedelta(hours=7))
app = Flask(__name__)


def train_model(real_sessions=None):
    """Huấn luyện từ dữ liệu giả lập + dữ liệu thật (nếu có)."""
    Xs, ys = synth_data.generate()
    n_real = 0
    if real_sessions:
        Xr, yr = sessions_to_xy(real_sessions)
        n_real = len(yr)
        if n_real:
            # Dữ liệu thật quan trọng hơn: lặp lại để có trọng số lớn hơn dữ liệu giả
            rep = max(1, len(ys) // max(n_real, 1))
            Xs = np.vstack([Xs] + [Xr] * rep)
            ys = np.concatenate([ys] + [yr] * rep)
    Xtr, Xte, ytr, yte = train_test_split(Xs, ys)
    m = LogisticRegression().fit(Xtr, ytr)
    m.save(WEIGHTS)
    return m, {"real_sessions": n_real, "train_size": int(len(ytr)),
               "test_accuracy": round(m.accuracy(Xte, yte), 3),
               "test_log_loss": round(m.log_loss(Xte, yte), 3)}


def sessions_to_xy(rows):
    """rows: bản ghi bảng sessions {start_ts (ISO/Unix), focus_min, completed, ...}."""
    X, y, per_day = [], [], {}
    for r in sorted(rows, key=lambda r: str(r.get("start_ts"))):
        ts = r.get("start_ts")
        try:
            dt = (datetime.fromtimestamp(int(ts), VN_TZ) if str(ts).isdigit()
                  else datetime.fromisoformat(str(ts).replace("Z", "+00:00")).astimezone(VN_TZ))
        except (TypeError, ValueError):
            continue
        key = dt.date()
        before = per_day.get(key, 0)
        per_day[key] = before + 1
        X.append(make_features(dt.weekday(), dt.hour + dt.minute / 60,
                               float(r.get("focus_min") or 50), before))
        y.append(1.0 if r.get("completed") else 0.0)
    return np.array(X).reshape(-1, make_features(0, 0, 0, 0).size), np.array(y)


model = LogisticRegression.load(WEIGHTS) if WEIGHTS.exists() else train_model()[0]


def slot_score(slot, session_min):
    return float(model.predict_proba(make_features(slot.day, slot.start / 60, session_min, 0))[0])


@app.post("/schedule")
def schedule():
    body = request.get_json(force=True, silent=True) or {}
    subjects = [s for s in body.get("subjects", []) if s.get("name") and int(s.get("sessions", 0)) > 0]
    session_min = int(body.get("session_min", 60))
    if not subjects:
        return jsonify(ok=False, message="Chua nhap mon can hoc"), 400

    if body.get("mode") == "timetable":
        days, busy = list(range(7)), body.get("timetable", [])
    else:
        days, busy = body.get("free_days", []), []
    if not days:
        return jsonify(ok=False, message="Chua chon ngay ranh"), 400

    slots = build_slots(days, session_min, busy,
                        day_start=body.get("day_start", "07:00"), day_end=body.get("day_end", "22:30"))
    result, scores = solve(subjects, slots, lambda s: slot_score(s, session_min),
                           max_per_day=int(body.get("max_per_day", 3)))
    if result is None:
        return jsonify(ok=False, message="Khong xep duoc: giam so buoi hoac them ngay ranh"), 200

    plan = sorted(({"day": sl.day, "day_name": DAY_NAMES[sl.day], "start": to_hhmm(sl.start),
                    "end": to_hhmm(sl.end), "subject": var[0], "score": round(scores[sl], 2)}
                   for var, sl in result.items()), key=lambda p: (p["day"], p["start"]))
    return jsonify(ok=True, plan=plan, slots_considered=len(slots))


@app.post("/train")
def train():
    global model
    body = request.get_json(force=True, silent=True) or {}
    model, info = train_model(body.get("sessions"))
    return jsonify(ok=True, **info)


@app.get("/scores")
def scores():
    day = int(request.args.get("day", datetime.now(VN_TZ).weekday()))
    session_min = int(request.args.get("session_min", 60))
    data = [{"hour": h, "p": round(float(model.predict_proba(make_features(day, h, session_min, 0))[0]), 3)}
            for h in range(6, 24)]
    return jsonify(day=day, scores=data)


@app.get("/health")
def health():
    return jsonify(ok=True)


if __name__ == "__main__":
    app.run(host="0.0.0.0", port=5000, debug=True)
