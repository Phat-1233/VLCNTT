"""Test ID7 – chạy:  python test_ai.py   (không cần pytest)."""
from app import app
from scheduler import to_min

client = app.test_client()


def post(body):
    r = client.post("/schedule", json=body)
    return r.status_code, r.get_json()


def test_khong_trung_tkb():
    code, d = post({"mode": "timetable",
                    "timetable": [{"day": 0, "start": "07:30", "end": "11:30", "subject": "Toan"}],
                    "subjects": [{"name": "Toan", "sessions": 2}, {"name": "Ly", "sessions": 2}],
                    "session_min": 60})
    assert code == 200 and d["ok"], d
    for p in d["plan"]:
        if p["day"] == 0:  # 07:30–11:30 + 30 phút đi lại -> bận 07:00–12:00
            assert not (to_min(p["start"]) < to_min("12:00") and to_min("07:00") < to_min(p["end"])), p


def test_mot_mon_mot_ngay():
    code, d = post({"mode": "free_days", "free_days": list(range(7)),
                    "subjects": [{"name": "Toan", "sessions": 3}, {"name": "Ly", "sessions": 3}],
                    "session_min": 60})
    assert code == 200 and d["ok"], d
    seen, per_day = set(), {}
    for p in d["plan"]:
        assert (p["subject"], p["day"]) not in seen, p
        seen.add((p["subject"], p["day"]))
        per_day[p["day"]] = per_day.get(p["day"], 0) + 1
    assert all(n <= 3 for n in per_day.values()), per_day


def test_khong_xep_duoc():
    code, d = post({"mode": "free_days", "free_days": [0],
                    "subjects": [{"name": "Toan", "sessions": 5}], "session_min": 60})
    assert code == 200 and d["ok"] is False and d.get("message"), d


def test_cycles():
    code, d = post({"mode": "free_days", "free_days": [1, 3],
                    "subjects": [{"name": "Toan", "sessions": 2}],
                    "session_min": 90, "study_min": 25, "break_min": 5})
    assert code == 200 and d["ok"], d
    assert all(p.get("cycles") == 3 for p in d["plan"]), d["plan"]
    assert all(isinstance(p.get("reason"), str) and p["reason"] for p in d["plan"]), d["plan"]


def test_uu_tien():
    code, d = post({"mode": "free_days", "free_days": list(range(7)),
                    "subjects": [{"name": "Toan", "sessions": 3}, {"name": "Ly", "sessions": 3}],
                    "session_min": 60, "priority": "Ly"})
    assert code == 200 and d["ok"], d
    avg = lambda m: sum(p["score"] for p in d["plan"] if p["subject"] == m) / 3
    assert avg("Ly") >= avg("Toan"), (avg("Ly"), avg("Toan"))
    assert any(p["reason"] == "Mon uu tien" for p in d["plan"] if p["subject"] == "Ly"), d["plan"]


def test_reason_truoc_tiet():
    # Thứ 4 có tiết Hóa -> buổi Hóa xếp vào Thứ 2 hoặc Thứ 3 phải ghi "Truoc tiet Hoa Thu 4"
    code, d = post({"mode": "timetable",
                    "timetable": [{"day": 2, "start": "07:30", "end": "11:30", "subject": "Hoa"}],
                    "subjects": [{"name": "Hoa", "sessions": 3}], "session_min": 60})
    assert code == 200 and d["ok"], d
    for p in d["plan"]:
        if p["day"] in (0, 1):
            assert p["reason"] == "Truoc tiet Hoa Thu 4", p


def test_model_hoc_duoc():
    r = client.post("/train", json={})
    d = r.get_json()
    assert r.status_code == 200 and d["test_accuracy"] >= 0.7, d
    s = client.get("/scores?day=0").get_json()
    assert s.get("trained_at") and s.get("test_accuracy") == d["test_accuracy"], s


if __name__ == "__main__":
    tests = [v for k, v in list(globals().items()) if k.startswith("test_")]
    failed = 0
    for t in tests:
        try:
            t()
            print("OK  ", t.__name__)
        except Exception as e:  # noqa: BLE001
            failed += 1
            print("FAIL", t.__name__, "->", repr(e)[:200])
    print(f"\n{len(tests) - failed}/{len(tests)} test qua")
    raise SystemExit(1 if failed else 0)
