-- Chạy trong Supabase: SQL Editor -> New query -> dán toàn bộ -> Run
-- Node-RED dùng service_role key (Project Settings -> API) nên không cần cấu hình RLS.

create table events (
  id bigserial primary key,
  dev text, ts timestamptz not null,
  type text,              -- start/pause/stop/low_light...
  detail jsonb
);

create table sessions (
  id bigserial primary key,
  dev text, start_ts timestamptz not null, end_ts timestamptz,
  subject text, planned int, done int, focus_min int, completed boolean,
  unique (dev, start_ts)  -- gửi lại không bị trùng (upsert)
);

create table lux_readings (
  id bigserial primary key,
  dev text, ts timestamptz not null, lux_avg real   -- trung bình mỗi phút
);

create table plans (
  id bigserial primary key,
  week_start date, day date, start_time time, end_time time,
  subject text, cycles int, created_at timestamptz default now()
);

create table email_reports (
  id bigserial primary key,
  sent_at timestamptz default now(), to_email text, status text
);
