#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""perfetto_query.py — Cubium Perfetto trace 只读查询工具（性能异常排查用）。

必须用装了 perfetto 包的解释器运行（Windows 上是 Python310，见 SKILL.md 前置条件）：

    C:\\Users\\Administrator\\AppData\\Local\\Programs\\Python\\Python310\\python.exe \\
        perfetto_query.py <trace> <子命令> [参数...]

子命令：
    info                        trace 概览：时长、进程/线程、切片总量、最长的少数切片
    longest [--name N] [--min-ms M] [--limit K]
                                最长的切片（可按名字过滤），打印 id/相对起点/时长/线程/深度
    stats NAME [NAME...]        指定切片名的次数、总时长、平均/最大时长
    tree ID [--depth D]         某个切片的完整子切片树（按总时长排序），用于归因
    ancestors ID                某个切片的祖先链（从根到父）
    context ID [--top K]        该切片时间窗内所有线程在做什么（判断锁竞争/CPU 密集）
    busy [--t0 MS] [--t1 MS]    各线程 depth=0 忙碌时长（默认全 trace）
    args NAME                   某个切片名的 debug 参数取值分布
    buckets NAME                某个切片名的时长分布直方图

设计约束（踩过的坑，勿改）：
- 时间一律相对 trace_bounds.start_ts 以毫秒打印，避免把纳秒增量时钟当秒看。
- 统计过滤 dur >= 0：trace 结束时未闭合的切片 dur 为 -1。
- 只用 stdlib + perfetto；不写临时文件、不改 trace。
"""

import sys

if hasattr(sys.stdout, "reconfigure"):
    sys.stdout.reconfigure(encoding="utf-8", errors="replace")

try:
    from perfetto.trace_processor import TraceProcessor, TraceProcessorConfig
except ImportError:  # pragma: no cover - 环境问题，直接给出可执行的修复提示
    sys.stderr.write(
        "perfetto 包不可用。本项目只有 Python310 装了它：\n"
        "  C:\\Users\\Administrator\\AppData\\Local\\Programs\\Python\\Python310\\python.exe\n"
        "若换了机器：pip install perfetto（首次运行会下载 trace_processor_shell）。\n"
    )
    raise SystemExit(2)


def _q(value):
    """SQL 字符串字面量转义（单引号加倍）。"""
    return "'" + str(value).replace("'", "''") + "'"


class Trace:
    def __init__(self, path):
        self.tp = TraceProcessor(trace=path, config=TraceProcessorConfig(load_timeout=900))
        row = next(iter(self.tp.query("select start_ts, end_ts from trace_bounds")))
        self.t0 = row.start_ts
        self.t1 = row.end_ts

    def sql(self, statement):
        cur = self.tp.query(statement)
        cols = None
        rows = []
        for r in cur:
            d = {k: getattr(r, k) for k in r.__dict__.keys()}
            if cols is None:
                cols = list(d.keys())
            rows.append(d)
        return cols or [], rows

    def print(self, title, statement):
        print(f"\n=== {title} ===")
        cols, rows = self.sql(statement)
        if not cols:
            print("(无结果)")
            return []
        print(" | ".join(cols))
        for d in rows:
            print(" | ".join("" if d.get(c) is None else str(d.get(c)) for c in cols))
        return rows

    def close(self):
        self.tp.close()


def _slice_ts(tr, slice_id):
    _, rows = tr.sql(f"select ts, dur, name from slice where id = {int(slice_id)}")
    if not rows:
        raise SystemExit(f"slice id {slice_id} 不存在")
    return rows[0]["ts"], rows[0]["dur"], rows[0]["name"]


def main(argv):
    if len(argv) < 3:
        print(__doc__)
        return 1
    trace_path, cmd = argv[1], argv[2]
    rest = argv[3:]
    tr = Trace(trace_path)
    start_ms = f"({tr.t0})"
    try:
        if cmd == "info":
            tr.print(
                "trace 概览",
                f"select round((end_ts-{start_ms})/1e9,2) dur_s, {start_ms} as t0_ns from trace_bounds",
            )
            tr.print(
                "进程",
                "select p.upid, p.pid, p.name, count(distinct t.utid) threads "
                "from process p left join thread t on t.upid=p.upid group by p.upid",
            )
            tr.print(
                "切片总量",
                "select count(*) slices, round(sum(case when dur>0 then dur else 0 end)/1e6,1) total_ms "
                "from slice",
            )
            tr.print(
                "最长的 15 个切片",
                f"""select id, round((ts-{start_ms})/1e6,1) start_ms, round(dur/1e6,2) dur_ms, depth, name
                    from slice where dur >= 0 order by dur desc limit 15""",
            )

        elif cmd == "longest":
            name = _opt(rest, "--name")
            min_ms = float(_opt(rest, "--min-ms", 0) or 0)
            limit = int(_opt(rest, "--limit", 30) or 30)
            where = ["s.dur >= 0", f"s.dur >= {min_ms} * 1e6"]
            if name:
                where.append(f"s.name = {_q(name)}")
            tr.print(
                "最长切片",
                f"""select s.id, round((s.ts-{start_ms})/1e6,1) start_ms, round(s.dur/1e6,2) dur_ms,
                           s.depth, t.name as thread, s.name
                    from slice s
                    left join thread_track tt on s.track_id = tt.id
                    left join thread t on tt.utid = t.utid
                    where {' and '.join(where)}
                    order by s.dur desc limit {limit}""",
            )

        elif cmd == "stats":
            for name in rest:
                tr.print(
                    f"切片统计 {name}",
                    f"""select count(*) n, round(sum(dur)/1e6,1) total_ms, round(avg(dur)/1e3,2) avg_us,
                               round(max(dur)/1e6,2) max_ms
                        from slice where name = {_q(name)} and dur >= 0""",
                )

        elif cmd == "tree":
            sid = int(rest[0])
            max_depth = int(_opt(rest, "--depth", 6) or 6)
            ts, dur, name = _slice_ts(tr, sid)
            print(f"\n=== 切片树 #{sid} {name} ({dur/1e6:.2f}ms) ===")
            rows = []
            _walk(tr, sid, 0, max_depth, rows)
            total_self = dur
            for depth, cid, cname, cdur in rows:
                total_self -= max(cdur, 0)
                print(f"{'  ' * depth}[#{cid}] {cdur/1e3:9.2f}us  {cname}")
            print(f"{'  ' * 0}其余（自身耗时，未被子切片覆盖）: {max(total_self, 0)/1e3:.2f}us")

        elif cmd == "ancestors":
            sid = int(rest[0])
            ts, dur, name = _slice_ts(tr, sid)
            print(f"\n=== #{sid} {name} ({dur/1e6:.3f}ms) 的祖先 ===")
            rows = tr.sql(
                f"""select a.id, a.depth, round(a.dur/1e6,2) dur_ms, a.name
                    from ancestor_slice({sid}) a order by a.depth asc"""
            )[1]
            for d in rows:
                print(f"{'  ' * d['depth']}[#{d['id']}] {d['dur_ms']}ms  {d['name']}")

        elif cmd == "context":
            sid = int(rest[0])
            top = int(_opt(rest, "--top", 25) or 25)
            ts, dur, name = _slice_ts(tr, sid)
            end = ts + max(dur, 0)
            tr.print(
                f"#{sid} 时间窗内（{(ts - tr.t0)/1e6:.1f}~{(end - tr.t0)/1e6:.1f}ms）各线程活动",
                f"""select t.name as thread, s.name, count(*) n, round(sum(s.dur)/1e6,1) ms
                    from slice s
                    left join thread_track tt on s.track_id = tt.id
                    left join thread t on tt.utid = t.utid
                    where s.ts < {end} and s.ts + s.dur > {ts} and s.dur >= 0
                    group by s.name, t.utid order by ms desc limit {top}""",
            )

        elif cmd == "busy":
            t0 = tr.t0 + int(float(_opt(rest, "--t0", 0) or 0) * 1e6)
            t1 = tr.t0 + int(float(_opt(rest, "--t1", 0) or 0) * 1e6) if _opt(rest, "--t1") else tr.t1
            tr.print(
                "各线程 depth=0 忙碌时长",
                f"""select t.name as thread, count(*) n, round(sum(s.dur)/1e6,1) busy_ms
                    from slice s
                    left join thread_track tt on s.track_id = tt.id
                    left join thread t on tt.utid = t.utid
                    where s.depth = 0 and s.dur >= 0 and s.ts < {t1} and s.ts + s.dur > {t0}
                    group by t.utid order by busy_ms desc""",
            )

        elif cmd == "args":
            name = rest[0]
            # 数值参数与字符串参数分两张表：字符串参数（如给切片打的 debug.set 结构集名）
            # 用 min/max 看不出来，必须按取值分组才看得出"谁贵"。
            tr.print(
                f"{name} 的 debug 参数分布（数值）",
                f"""select a.key, count(*) n, min(a.int_value) min_i, max(a.int_value) max_i,
                           count(distinct a.int_value) distinct_i
                    from slice s join args a on a.arg_set_id = s.arg_set_id
                    where s.name = {_q(name)} and a.int_value is not null
                    group by a.key order by n desc""",
            )
            tr.print(
                f"{name} 的 debug 参数分布（字符串取值 top10）",
                f"""select a.key, a.string_value, count(*) n
                    from slice s join args a on a.arg_set_id = s.arg_set_id
                    where s.name = {_q(name)} and a.string_value is not null
                    group by a.key, a.string_value order by n desc limit 10""",
            )

        elif cmd == "buckets":
            name = rest[0]
            tr.print(
                f"{name} 时长分布",
                f"""select case
                             when dur < 100e3 then '<100us'
                             when dur < 1e6   then '100us-1ms'
                             when dur < 1e7   then '1-10ms'
                             when dur < 1e8   then '10-100ms'
                             when dur < 1e9   then '100ms-1s'
                             else '>1s' end as bucket,
                           count(*) n, round(sum(dur)/1e6,1) total_ms
                    from slice where name = {_q(name)} and dur >= 0
                    group by 1 order by min(dur)""",
            )

        else:
            print(f"未知子命令: {cmd}")
            print(__doc__)
            return 1
    finally:
        tr.close()
    return 0


def _walk(tr, sid, depth, max_depth, out):
    if depth >= max_depth:
        return
    _, rows = tr.sql(
        f"""select id, name, dur from slice
            where parent_id = {sid} and dur >= 0 order by dur desc"""
    )
    for r in rows:
        out.append((depth + 1, r["id"], r["name"], r["dur"]))
        _walk(tr, r["id"], depth + 1, max_depth, out)


def _opt(args, key, default=None):
    if key in args:
        i = args.index(key)
        if i + 1 < len(args):
            return args[i + 1]
    return default


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
