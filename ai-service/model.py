"""ID7 – Logistic regression tự viết bằng numpy (không dùng scikit-learn).

Dự đoán xác suất người học HOÀN THÀNH một phiên học nếu bắt đầu ở khung giờ đó.
    p = sigmoid(w·x + b)
    loss = log-loss (+ L2)
    cập nhật: w <- w - lr * dL/dw   (gradient descent theo batch)
"""
import json
from pathlib import Path

import numpy as np

# Khung giờ dùng làm đặc trưng one-hot (giờ bắt đầu phiên học)
HOUR_BINS = [(5, 9), (9, 12), (12, 14), (14, 17), (17, 19), (19, 22), (22, 24)]
N_FEATURES = 7 + len(HOUR_BINS) + 3  # thứ (7) + khung giờ (7) + độ dài, số phiên trước đó, độ sáng


def make_features(weekday, hour, duration_min, sessions_before, lux_norm=0.5):
    """weekday: 0=Thứ 2 ... 6=Chủ nhật; hour: 0..23 (có thể lẻ, vd 19.5);
    duration_min: độ dài phiên; sessions_before: số phiên đã học trong ngày;
    lux_norm: độ sáng trung bình 0..1 (chưa có thì để 0.5)."""
    x = np.zeros(N_FEATURES)
    x[int(weekday) % 7] = 1.0
    for i, (lo, hi) in enumerate(HOUR_BINS):
        if lo <= hour < hi:
            x[7 + i] = 1.0
    x[14] = duration_min / 120.0
    x[15] = min(sessions_before, 5) / 5.0
    x[16] = lux_norm
    return x


def sigmoid(z):
    return 1.0 / (1.0 + np.exp(-np.clip(z, -30, 30)))


class LogisticRegression:
    def __init__(self, n_features=N_FEATURES):
        self.w = np.zeros(n_features)
        self.b = 0.0

    def predict_proba(self, X):
        X = np.atleast_2d(X)
        return sigmoid(X @ self.w + self.b)

    def fit(self, X, y, lr=0.3, epochs=800, l2=1e-3):
        X, y = np.asarray(X, float), np.asarray(y, float)
        n = len(y)
        for _ in range(epochs):
            p = sigmoid(X @ self.w + self.b)
            err = p - y                          # dL/dz của log-loss
            self.w -= lr * (X.T @ err / n + l2 * self.w)
            self.b -= lr * err.mean()
        return self

    def log_loss(self, X, y):
        p = np.clip(self.predict_proba(X), 1e-9, 1 - 1e-9)
        return float(-np.mean(y * np.log(p) + (1 - y) * np.log(1 - p)))

    def accuracy(self, X, y):
        return float(np.mean((self.predict_proba(X) >= 0.5) == np.asarray(y)))

    def save(self, path):
        Path(path).write_text(json.dumps({"w": self.w.tolist(), "b": self.b}))

    @classmethod
    def load(cls, path):
        data = json.loads(Path(path).read_text())
        m = cls(len(data["w"]))
        m.w, m.b = np.array(data["w"]), float(data["b"])
        return m


def train_test_split(X, y, test_ratio=0.2, seed=0):
    rng = np.random.default_rng(seed)
    idx = rng.permutation(len(y))
    cut = int(len(y) * (1 - test_ratio))
    return X[idx[:cut]], X[idx[cut:]], y[idx[:cut]], y[idx[cut:]]
