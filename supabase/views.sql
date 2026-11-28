-- Views cho trang Thống kê (ID9) – chủ file: Phát. Chạy SAU schema.sql (SQL Editor -> dán -> Run).
-- Chạy lại nhiều lần được (create or replace), không mất dữ liệu.
-- Ngày/giờ quy về giờ Việt Nam. security_invoker = true: view dùng quyền của người gọi,
-- nên RLS của bảng gốc vẫn chặn key public (anon); Node-RED dùng service key vẫn đọc được.

-- Tổng phút học thật theo ngày
create or replace view v_daily_minutes with (security_invoker = true) as
select (start_ts at time zone 'Asia/Ho_Chi_Minh')::date as day,
       coalesce(sum(focus_min), 0)::int                 as minutes
from sessions
group by 1;

-- Phút học theo môn theo ngày
create or replace view v_subject_minutes with (security_invoker = true) as
select (start_ts at time zone 'Asia/Ho_Chi_Minh')::date as day,
       coalesce(nullif(subject, ''), 'Khác')            as subject,
       coalesce(sum(focus_min), 0)::int                 as minutes
from sessions
group by 1, 2;

-- Độ sáng trung bình theo giờ trong ngày (%)
create or replace view v_lux_hourly with (security_invoker = true) as
select (ts at time zone 'Asia/Ho_Chi_Minh')::date                 as day,
       extract(hour from ts at time zone 'Asia/Ho_Chi_Minh')::int as hour,
       avg(lux_avg)::real                                         as lux_avg
from lux_readings
group by 1, 2;

-- Kế hoạch so với thực tế theo ngày:
--   planned = số buổi trong bảng plans
--   actual  = số cặp (ngày, môn) có ít nhất 1 chu kỳ completed
create or replace view v_plan_vs_actual with (security_invoker = true) as
with p as (
  select day, count(*)::int as planned from plans group by day
), a as (
  select day, count(*)::int as actual from (
    select distinct (start_ts at time zone 'Asia/Ho_Chi_Minh')::date as day, subject
    from sessions where completed
  ) x group by day
)
select coalesce(p.day, a.day)  as day,
       coalesce(p.planned, 0)  as planned,
       coalesce(a.actual, 0)   as actual
from p full outer join a on p.day = a.day;
