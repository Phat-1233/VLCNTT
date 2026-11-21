"""ID7 – Xếp lịch học bằng CSP: quay lui (backtracking) + heuristic MRV.

Biến      : từng buổi học cần xếp, vd ("Toan", 1), ("Toan", 2), ("Ly", 1)...
Miền      : các khung giờ trống (ngày, giờ bắt đầu, giờ kết thúc)
Ràng buộc : - không trùng thời khóa biểu (+ thời gian đi lại)
            - các buổi không chồng lên nhau
            - tối đa MAX_PER_DAY buổi mỗi ngày
            - một môn tối đa 1 buổi mỗi ngày
Thứ tự    : MRV chọn buổi còn ÍT khung hợp lệ nhất; khung có xác suất hoàn thành
            CAO NHẤT (từ logistic regression) được thử trước.
"""
from dataclasses import dataclass

DAY_NAMES = ["Thu 2", "Thu 3", "Thu 4", "Thu 5", "Thu 6", "Thu 7", "Chu nhat"]


def to_min(hhmm):
    h, m = hhmm.split(":")
    return int(h) * 60 + int(m)


def to_hhmm(minutes):
    return f"{minutes // 60:02d}:{minutes % 60:02d}"


@dataclass(frozen=True)
class Slot:
    day: int
    start: int   # phút tính từ 0:00
    end: int


def overlap(a, b):
    return a.day == b.day and a.start < b.end and b.start < a.end


def build_slots(days, session_min, busy=(), day_start="07:00", day_end="22:30",
                step=30, travel_min=30):
    """Tạo các khung trống trong những ngày được phép, né khoảng bận (+ đi lại)."""
    busy_iv = [Slot(b["day"], to_min(b["start"]) - travel_min, to_min(b["end"]) + travel_min)
               for b in busy]
    slots = []
    for d in days:
        t = to_min(day_start)
        while t + session_min <= to_min(day_end):
            s = Slot(d, t, t + session_min)
            if not any(overlap(s, b) for b in busy_iv):
                slots.append(s)
            t += step
    return slots


def solve(subjects, slots, score_fn, max_per_day=3, node_limit=200000):
    """subjects: [{"name": "Toan", "sessions": 2}, ...]
    score_fn(slot) -> xác suất hoàn thành 0..1.
    Trả về (dict biến -> Slot) hoặc None nếu không xếp được."""
    variables = [(s["name"], k) for s in subjects for k in range(int(s["sessions"]))]
    scores = {sl: score_fn(sl) for sl in slots}
    assign = {}
    nodes = 0

    def consistent(var, slot):
        same_day = [sl for sl in assign.values() if sl.day == slot.day]
        if len(same_day) >= max_per_day:
            return False
        if any(overlap(slot, sl) for sl in assign.values()):
            return False
        if any(v[0] == var[0] and sl.day == slot.day for v, sl in assign.items()):
            return False
        return True

    def legal_values(var):
        return [sl for sl in slots if consistent(var, sl)]

    def backtrack():
        nonlocal nodes
        nodes += 1
        if nodes > node_limit:
            return False
        unassigned = [v for v in variables if v not in assign]
        if not unassigned:
            return True
        # MRV: chọn biến còn ít giá trị hợp lệ nhất
        var = min(unassigned, key=lambda v: len(legal_values(v)))
        # Thử khung có xác suất hoàn thành cao nhất trước
        for slot in sorted(legal_values(var), key=lambda sl: -scores[sl]):
            assign[var] = slot
            if backtrack():
                return True
            del assign[var]
        return False

    return (dict(assign) if backtrack() else None), scores
