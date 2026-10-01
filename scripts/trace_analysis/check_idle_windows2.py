# -*- coding: utf-8 -*-
"""按切片名（不做 thread 连接，避免 thread 表重复行放大计数）检验生成活动是否真的停止。

若 executeStatusStep 在突发结束后长期为 0，则"最近一次生成步骤距今 >= 1s"这一空闲判据可达。
"""
from perfetto.trace_processor import TraceProcessor, TraceProcessorConfig

TRACE = 'E:/dev/minecraft-reborn-branch-1/server_trace.perfetto-trace'


def rows(tp, sql, cols):
    return [{c: getattr(r, c) for c in cols} for r in tp.query(sql)]


def main():
    tp = TraceProcessor(trace=TRACE, config=TraceProcessorConfig(load_timeout=900))
    b = rows(tp, 'SELECT start_ts, end_ts FROM trace_bounds', ['start_ts', 'end_ts'])[0]
    T0 = b['start_ts']
    dur = int((b['end_ts'] - T0) / 1e9)

    for name in ['ChunkProgressionTask::executeStatusStep',
                 'ChunkLoadLightTask::execute',
                 'RuntimeLightTask::execute']:
        per_sec = {r['sec']: r['n'] for r in rows(tp, f"""
            SELECT CAST((s.ts - {T0})/1000000000 AS INT) AS sec, count(*) AS n
            FROM slice s WHERE s.name = '{name}' GROUP BY 1 ORDER BY 1
        """, ['sec', 'n'])}
        total = sum(per_sec.values())
        last = max(per_sec) if per_sec else None
        line = ''.join('.' if per_sec.get(i, 0) == 0 else ('*' if per_sec[i] >= 10 else str(per_sec[i]))
                       for i in range(0, dur + 1))
        print(f'[1] {name}\n    总={total} 最后活动秒={last}\n    {line}')

    print('\n[2] 生成活动停止后的最长连续零活动窗口（以 executeStatusStep 为准）:')
    per_sec = {r['sec']: r['n'] for r in rows(tp, f"""
        SELECT CAST((s.ts - {T0})/1000000000 AS INT) AS sec, count(*) AS n
        FROM slice s WHERE s.name = 'ChunkProgressionTask::executeStatusStep' GROUP BY 1
    """, ['sec', 'n'])}
    best_len, best_start, cur_start = 0, None, None
    for sec in range(0, dur + 1):
        if per_sec.get(sec, 0) == 0:
            if cur_start is None:
                cur_start = sec
        else:
            if cur_start is not None and sec - cur_start > best_len:
                best_len, best_start = sec - cur_start, cur_start
            cur_start = None
    if cur_start is not None and dur - cur_start + 1 > best_len:
        best_len, best_start = dur - cur_start + 1, cur_start
    print(f'    最长={best_len}s（起点 {best_start}s）→ 1s 空闲判据: '
          f'{"可达" if best_len >= 1 else "不可达"}')

    tp.close()
    print('\nDONE')


if __name__ == '__main__':
    main()
