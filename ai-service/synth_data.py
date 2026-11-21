"""Sinh dữ liệu phiên học GIẢ LẬP để huấn luyện ban đầu (cold start).

Khi chưa có dữ liệu thật trong bảng sessions, mô hình học từ các thói quen phổ biến:
  - buổi tối 19h–22h và sáng 5h–9h dễ hoàn thành
  - sau 22h, giờ trưa 12h–14h khó hoàn thành
  - phiên càng dài / đã học càng nhiều phiên trong ngày thì càng dễ bỏ dở
  - phòng tối thì dễ bỏ dở
"""
import numpy as np

from model import make_features

HOUR_BONUS = {5: 0.6, 6: 0.6, 7: 0.5, 8: 0.4, 9: 0.2, 10: 0.2, 11: 0.0, 12: -0.6, 13: -0.6,
              14: 0.0, 15: 0.1, 16: 0.1, 17: -0.2, 18: -0.1, 19: 0.8, 20: 0.9, 21: 0.6,
              22: -0.5, 23: -1.0}


def generate(n=300, seed=42):
    rng = np.random.default_rng(seed)
    X, y = [], []
    for _ in range(n):
        weekday = int(rng.integers(0, 7))
        hour = int(rng.integers(5, 24))
        duration = int(rng.choice([30, 60, 90, 120]))
        before = int(rng.integers(0, 5))
        lux = float(rng.uniform(0.1, 1.0))
        z = (0.4 + HOUR_BONUS.get(hour, 0) - 0.6 * (duration / 120) - 0.3 * before
             + 0.8 * (lux - 0.5) - (0.3 if weekday == 6 else 0))
        p = 1 / (1 + np.exp(-2 * z))
        X.append(make_features(weekday, hour, duration, before, lux))
        y.append(1.0 if rng.random() < p else 0.0)
    return np.array(X), np.array(y)
