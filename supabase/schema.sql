-- Schema v2 – chủ file: Khoa (ID8). Chạy trong Supabase: SQL Editor -> New query -> dán toàn bộ -> Run
-- Node-RED dùng service_role key (Project Settings -> API) nên không cần cấu hình RLS.
-- CẢNH BÁO: các lệnh drop xóa sạch dữ liệu. Chỉ chạy lại khi chưa có dữ liệu thật.
-- View (views.sql) và dữ liệu mẫu (seed.sql) là của Phát, chạy SAU file này.

drop table if exists events, sessions, lux_readings, plans, email_reports, settings cascade;

-- Mỗi thao tác trên hộp + mỗi lần cảnh báo (ID1, ID5, ID7, ID10)
create table events (
  id bigserial primary key,
  dev text not null,
  ts timestamptz not null,
  type text not null,     -- online/start/pause/resume/stop/phase_end/done/subject/low_light/parent_beep/schedule_reminder
  detail jsonb
);
create index events_ts_idx on events (ts);

-- Mỗi chu kỳ học (1 Pomodoro = 1 dòng) – hợp đồng v2
create table sessions (
  id bigserial primary key,
  dev text not null,
  start_ts timestamptz not null,
  end_ts timestamptz,
  subject text,
  cycle int,              -- chu kỳ thứ mấy
  total int,              -- tổng số chu kỳ đã đặt
  planned_min int,        -- số phút đặt cho pha học
  focus_min int,          -- số phút học thật
  pauses int default 0,
  pause_sec int default 0,
  completed boolean,
  unique (dev, start_ts)  -- gửi lại không bị trùng (upsert on_conflict=dev,start_ts)
);
create index sessions_start_idx on sessions (start_ts);

-- Độ sáng trung bình mỗi phút (ID5, ID9)
create table lux_readings (
  id bigserial primary key,
  dev text not null,
  ts timestamptz not null,
  lux_avg real            -- % độ sáng 0..100
);
create index lux_ts_idx on lux_readings (ts);

-- Lịch tuần do AI xếp (ID7)
create table plans (
  id bigserial primary key,
  week_start date, day date, start_time time, end_time time,
  subject text, cycles int, score real, reason text,
  created_at timestamptz default now()
);

-- Lịch sử gửi email (ID11)
create table email_reports (
  id bigserial primary key,
  sent_at timestamptz default now(),
  week text, to_email text, status text, attempt int default 1
);

-- Cài đặt dùng chung giữa các máy Node-RED
create table settings (
  key text primary key,
  value jsonb,
  updated_at timestamptz default now()
);
insert into settings (key, value) values
  ('lux_threshold', '30'),
  ('parent_emails', '""'),
  ('telegram_enabled', 'true');

-- Bật RLS, không tạo policy: key public (anon) bị chặn hết,
-- Node-RED dùng service_role/secret key nên vẫn đọc ghi bình thường.
alter table events        enable row level security;
alter table sessions      enable row level security;
alter table lux_readings  enable row level security;
alter table plans         enable row level security;
alter table email_reports enable row level security;
alter table settings      enable row level security;
