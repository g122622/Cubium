# -*- coding: utf-8 -*-
"""检验"生成空闲消抖"是否可达：统计 ServerCompute 线程逐秒切片数，找出最长连续零活动秒数。

用途：确认方案 A（生成空闲后回收 halo）在真实负载下能真正触发——若突发结束后
不存在 ≥1 秒的完全空闲窗口，UNLOAD_IDLE_DEBOUNCE_TICKS 永远不会达成。
"""
import collections
import os

from perfetto.trace_processor import TraceProcessor, TraceProcessorConfig

TRACE = 'E:/dev/minecraft-reborn-branch-1/server_trace.perfetto-trace'


def rows(tp, sql, cols):
    return [{c: getattr(r, c) for c in cols} for r in tp.query(sql)]


def main():
    tp = TraceProcessor(trace=TRACE, config=TraceProcessorConfig(load_timeout=900))
    b = rows(tp, 'SELECT start_ts, end_ts FROM trace_bounds', ['start_ts', 'end_ts'])[0]
    T0 = b['start_ts']
    dur = int((b['end_ts'] - T0) / 1e9)

    print('[1] ServerCompute 线程逐秒切片数（仅统计有活动秒，15s 之后）:')
    per_sec = {}
    for r in rows(tp, f"""
        SELECT CAST((s.ts - {T0})/1000000000 AS INT) AS sec, count(*) AS n
        FROM slice s JOIN thread_track tt ON s.track_id = tt.id JOIN thread t ON tt.utid = t.utid
        WHERE t.name LIKE 'ServerCompute-%' AND s.ts > {T0} + 15000000000
        GROUP BY 1 ORDER BY 1
    """, ['sec', 'n']):
        per_sec[r['sec']] = r['n']

    active = sorted(per_sec)
    print(f"    15s 后共 {len(active)} 个秒有活动；活动秒: {active[:40]}{' ...' if len(active) > 40 else ''}")

    # 最长连续零活动窗口
    best_len = 0
    best_start = None
    cur_start = None
    for sec in range(16, dur + 1):
        if per_sec.get(sec, 0) == 0:
            if cur_start is None:
                cur_start = sec
        else:
            if cur_start is not None:
                length = sec - cur_start
                if length > best_len:
                    best_len, best_start = length, cur_start
                cur_start = None
    if cur_start is not None:
        length = dur - cur_start + 1
        if length > best_len:
            best_len, best_start = length, cur_start

    print(f'\n[2] 最长连续零活动窗口: {best_len}s（起点 {best_start}s）')
    print(f'    → 需要 {UNLOAD_IDLE_TICKS if False else 20} tick(=1s) 连续空闲: '
          f'{"可达" if best_len >= 1 else "不可达（需放宽判据或改用固定延迟）"}')

    print('\n[3] 15s 后各任务类切片总数（判断零活动是否只是"未埋点"）:')
    for r in rows(tp, f"""
        SELECT s.name AS name, count(*) AS n
        FROM slice s JOIN thread_track tt ON s.track_id = tt.id JOIN thread t ON t.utid = t.utid
        WHERE t.name LIKE 'ServerCompute-%' AND s.ts > {T0} + 15000000000
        GROUP BY s.name ORDER BY n DESC LIMIT 12
    """, ['name', 'n']):
        print(f"    {str(r['name']):56s} n={r['n']}")

    tp.close()
    print('\nDONE')


UNLOAD_IDLE_TICKS = 20

if __name__ == '__main__':
    main()
