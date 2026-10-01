# -*- coding: utf-8 -*-
"""分析 server_trace.perfetto-trace：定位内存滑落窗口并关联当时的执行切片。"""
import csv
import os
import sys

from perfetto.trace_processor import TraceProcessor, TraceProcessorConfig

TRACE = 'E:/dev/minecraft-reborn-branch-1/server_trace.perfetto-trace'
OUT_DIR = 'E:/dev/minecraft-reborn-branch-1/trace_analysis'
os.makedirs(OUT_DIR, exist_ok=True)


def rows(tp, sql, cols):
    return [{c: getattr(r, c) for c in cols} for r in tp.query(sql)]


def sec(ns):
    return ns / 1e9


def main():
    tp = TraceProcessor(trace=TRACE, config=TraceProcessorConfig(load_timeout=600))

    # ---------- A. 总览 ----------
    b = rows(tp, 'SELECT start_ts, end_ts FROM trace_bounds', ['start_ts', 'end_ts'])[0]
    dur_s = sec(b['end_ts'] - b['start_ts'])
    print(f"[A] trace 时长: {dur_s:.1f}s  起点 ts={b['start_ts']}")

    print('\n[A] 线程切片数 TOP30（线程名 / 进程名 / 切片数）:')
    for r in rows(tp, """
        SELECT t.name AS tname, p.name AS pname, count(s.id) AS n
        FROM slice s
        JOIN thread_track tt ON s.track_id = tt.id
        JOIN thread t ON tt.utid = t.utid
        LEFT JOIN process p ON t.upid = p.upid
        GROUP BY t.utid ORDER BY n DESC LIMIT 30
    """, ['tname', 'pname', 'n']):
        print(f"    {str(r['tname']):40s} {str(r['pname']):20s} {r['n']:>9d}")

    # ---------- B. 计数器轨道 ----------
    print('\n[B] 计数器轨道（名称 / 点数 / 最小 / 最大 / 覆盖时长s）:')
    for r in rows(tp, """
        SELECT t.id, t.name, count(*) AS n, min(c.value) AS mn, max(c.value) AS mx,
               min(c.ts) AS t0, max(c.ts) AS t1
        FROM counter c JOIN counter_track t ON c.track_id = t.id
        GROUP BY t.id ORDER BY n DESC LIMIT 40
    """, ['id', 'name', 'n', 'mn', 'mx', 't0', 't1']):
        print(f"    {str(r['name']):40s} n={r['n']:>7d} min={r['mn']:>12.1f} max={r['mx']:>12.1f} "
              f"span={sec(r['t1'] - r['t0']):.1f}s")

    # ---------- C. 内存序列（按秒聚合） ----------
    mem_sql = """
        SELECT CAST((c.ts - {t0})/1000000000 AS INT) AS s,
               avg(c.value) AS avgmb, min(c.value) AS minmb, max(c.value) AS maxmb, count(*) AS n
        FROM counter c JOIN counter_track t ON c.track_id = t.id
        WHERE t.name = '{name}'
        GROUP BY 1 ORDER BY 1
    """
    series = {}
    for name in ('ProcessMemory', 'ProcessCommit'):
        series[name] = rows(tp, mem_sql.format(t0=b['start_ts'], name=name),
                            ['s', 'avgmb', 'minmb', 'maxmb', 'n'])
        path = os.path.join(OUT_DIR, name.lower() + '_1s.csv')
        with open(path, 'w', newline='') as f:
            w = csv.writer(f)
            w.writerow(['sec', 'avg_mb', 'min_mb', 'max_mb', 'samples'])
            for r in series[name]:
                w.writerow([r['s'], round(r['avgmb'], 1), round(r['minmb'], 1),
                            round(r['maxmb'], 1), r['n']])
        print(f"\n[C] {name}: {len(series[name])}s 有效数据, CSV -> {path}")

    def find_drops(av, drop_mb=30.0):
        """扫描每秒均值，返回 [(峰值秒, 峰值MB, 谷值秒, 谷值MB), ...]"""
        if not av:
            return []
        peak_i, peak_v = 0, av[0][1]
        valley_v = av[0][1]
        in_drop = False
        drops = []
        for i, (s, v) in enumerate(av[1:], 1):
            if not in_drop:
                if v > peak_v:
                    peak_i, peak_v = i, v
                elif peak_v - v >= drop_mb:
                    in_drop = True
                    valley_v = v
            else:
                if v < valley_v:
                    valley_v = v
                if v >= valley_v + drop_mb * 0.5 or i == len(av) - 1:
                    drops.append((av[peak_i][0], peak_v, s, valley_v))
                    in_drop = False
                    peak_i, peak_v = i, v
        drops.sort(key=lambda d: d[1] - d[3], reverse=True)
        return drops

    for name in ('ProcessMemory', 'ProcessCommit'):
        av = [(r['s'], r['avgmb']) for r in series[name]]
        if not av:
            continue
        drops = find_drops(av)
        print(f"\n[C] {name} 滑落窗口 TOP8（回落>=30MB, 按幅度排序）:")
        print('    峰值时刻s  峰值MB    谷值时刻s  谷值MB    跌幅MB')
        for p_s, p_v, v_s, v_v in drops[:8]:
            print(f"    {p_s:>8d}  {p_v:>8.1f}  {v_s:>9d}  {v_v:>8.1f}  {p_v - v_v:>8.1f}")
        first, last = av[0], av[-1]
        mx = max(av, key=lambda x: x[1])
        print(f"    全程: 起点 {first[1]:.1f}MB@{first[0]}s  峰值 {mx[1]:.1f}MB@{mx[0]}s  终点 {last[1]:.1f}MB@{last[0]}s")

    # ---------- D. 全 trace 切片画像 ----------
    print('\n[D] 全 trace 切片画像 TOP50（按总时长，name / 次数 / 总ms / 均us / 最大ms / 线程）:')
    for r in rows(tp, """
        SELECT s.name AS name, count(*) AS n, sum(s.dur)/1e6 AS total_ms,
               avg(s.dur)/1e3 AS avg_us, max(s.dur)/1e6 AS max_ms,
               t.name AS tname
        FROM slice s
        JOIN thread_track tt ON s.track_id = tt.id
        JOIN thread t ON tt.utid = t.utid
        WHERE s.dur >= 0
        GROUP BY s.name, t.utid ORDER BY total_ms DESC LIMIT 50
    """, ['name', 'n', 'total_ms', 'avg_us', 'max_ms', 'tname']):
        print(f"    {str(r['name']):55s} n={r['n']:>8d} tot={r['total_ms']:>10.1f}ms "
              f"avg={r['avg_us']:>9.1f}us max={r['max_ms']:>9.1f}ms [{r['tname']}]")

    print('\n[D] 全 trace 切片次数 TOP30（name / 次数 / 线程）:')
    for r in rows(tp, """
        SELECT s.name AS name, count(*) AS n, t.name AS tname
        FROM slice s
        JOIN thread_track tt ON s.track_id = tt.id
        JOIN thread t ON tt.utid = t.utid
        GROUP BY s.name, t.utid ORDER BY n DESC LIMIT 30
    """, ['name', 'n', 'tname']):
        print(f"    {str(r['name']):55s} n={r['n']:>9d} [{r['tname']}]")

    # ---------- E. 最大滑落窗口内的切片 ----------
    av = [(r['s'], r['avgmb']) for r in series['ProcessMemory']]
    drops = find_drops(av)
    if drops:
        drop, ps, pv, vs, vv = drops[0][1] - drops[0][3], drops[0][0], drops[0][1], drops[0][2], drops[0][3]
        t_start = b['start_ts'] + ps * 1_000_000_000
        t_end = b['start_ts'] + (vs + 1) * 1_000_000_000
        print(f"\n[E] 最大滑落窗口: {ps}s({pv:.1f}MB) -> {vs}s({vv:.1f}MB), 跌幅 {drop:.1f}MB")
        print('    窗口内切片 TOP40（name / 次数 / 总ms / 线程）:')
        sql = f"""
            SELECT s.name AS name, count(*) AS n, sum(s.dur)/1e6 AS total_ms, t.name AS tname
            FROM slice s
            JOIN thread_track tt ON s.track_id = tt.id
            JOIN thread t ON tt.utid = t.utid
            WHERE s.ts < {t_end} AND s.ts + s.dur > {t_start} AND s.dur >= 0
            GROUP BY s.name, t.utid ORDER BY total_ms DESC LIMIT 40
        """
        for r in rows(tp, sql, ['name', 'n', 'total_ms', 'tname']):
            print(f"    {str(r['name']):55s} n={r['n']:>7d} tot={r['total_ms']:>10.1f}ms [{r['tname']}]")

    tp.close()
    print('\nDONE')


if __name__ == '__main__':
    sys.exit(main())
