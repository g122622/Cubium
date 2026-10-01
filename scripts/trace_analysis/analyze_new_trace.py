# -*- coding: utf-8 -*-
"""新 trace 分析：验证「事件驱动卸载」重构后的实际行为。

场景：进入服务器后原地不动，视距未改（16）。本 trace 因断言失败在退出时写出，
需先确认时长与崩溃时刻，再核对卸载时序、内存与区块计数。
"""
import csv
import os

from perfetto.trace_processor import TraceProcessor, TraceProcessorConfig

TRACE = 'E:/dev/minecraft-reborn-branch-1/server_trace.perfetto-trace'
OUT_DIR = 'E:/dev/minecraft-reborn-branch-1/scripts/trace_analysis'


def rows(tp, sql, cols):
    return [{c: getattr(r, c) for c in cols} for r in tp.query(sql)]


def fmt(v, width, prec=1):
    if v is None:
        return 'nan'.rjust(width)
    return f'{v:>{width}.{prec}f}'


def main():
    tp = TraceProcessor(trace=TRACE, config=TraceProcessorConfig(load_timeout=900))

    b = rows(tp, 'SELECT start_ts, end_ts FROM trace_bounds', ['start_ts', 'end_ts'])[0]
    T0 = b['start_ts']
    dur = (b['end_ts'] - T0) / 1e9
    print(f'[1] trace 时长 {dur:.1f}s（终点 ts={b["end_ts"]}）')

    print('\n[2] 计数器轨道:')
    for r in rows(tp, """
        SELECT t.name AS name, count(*) AS n, min(c.value) AS mn, max(c.value) AS mx
        FROM counter c JOIN counter_track t ON c.track_id = t.id
        GROUP BY t.id ORDER BY n DESC LIMIT 20
    """, ['name', 'n', 'mn', 'mx']):
        print(f"    {str(r['name']):34s} n={r['n']:>7d} min={r['mn']:>10.1f} max={r['mx']:>10.1f}")

    names = ['ProcessMemory', 'ProcessCommit', 'TotalLoadedChunkCount', 'TotalChunkHolderCount',
             'TotalEntityCount', 'PlayerCount', 'TPS']
    series = {}
    for nm in names:
        series[nm] = {r['s']: r['v'] for r in rows(tp, f"""
            SELECT CAST((c.ts - {T0})/1000000000 AS INT) AS s, avg(c.value) AS v
            FROM counter c JOIN counter_track t ON c.track_id = t.id
            WHERE t.name = '{nm}' GROUP BY 1
        """, ['s', 'v'])}

    path = os.path.join(OUT_DIR, 'new_timeline_1s.csv')
    with open(path, 'w', newline='') as f:
        w = csv.writer(f)
        w.writerow(['sec', 'mem_mb', 'commit_mb', 'loaded', 'holders', 'entities', 'players', 'tps'])
        for s in range(0, int(dur) + 1):
            g = lambda nm: series[nm].get(s)  # noqa: E731
            w.writerow([s] + [g(nm) for nm in names])
    print(f'\n[3] 逐秒时间线 -> {path}')
    print('    sec  memMB commitMB loaded holders  ent plc tps')
    for s in range(0, int(dur) + 1):
        g = lambda nm: series[nm].get(s)  # noqa: E731
        print(f"    {s:>3d} {fmt(g('ProcessMemory'), 6)} {fmt(g('ProcessCommit'), 8)} "
              f"{fmt(g('TotalLoadedChunkCount'), 6, 0)} {fmt(g('TotalChunkHolderCount'), 7, 0)} "
              f"{fmt(g('TotalEntityCount'), 4, 0)} {fmt(g('PlayerCount'), 3, 0)} {fmt(g('TPS'), 4, 0)}")

    print('\n[4] 卸载/保存/票据路径切片逐秒计数（. = 0，数字 = 次数，* = ≥10）:')
    for nm in ['ChunkSendManager::onChunkPreUnload', 'ChunkSendManager::unloadChunkFromPlayers',
               'SectionCodec::serializeFromChunkSection', 'RocksDBDatabase::writeBatch',
               'ChunkTaskScheduler::cancelGeneration', 'ChunkDistanceGraph::onLevelChanged',
               'ChunkLoadTicketManager::processUpdates', 'ServerChunkManager::onTicketLevelChanged']:
        hist = rows(tp, f"""
            SELECT CAST((s.ts - {T0})/1000000000 AS INT) AS sec, count(*) AS n
            FROM slice s WHERE s.name = '{nm}' GROUP BY 1 ORDER BY 1
        """, ['sec', 'n'])
        if not hist:
            print(f'    {nm}: (无切片)')
            continue
        vals = {r['sec']: r['n'] for r in hist}
        line = ''.join('.' if vals.get(i, 0) == 0 else (str(vals[i]) if vals[i] < 10 else '*')
                       for i in range(0, int(dur) + 1))
        print(f"    {nm} total={sum(vals.values())}\n        {line}")

    print('\n[5] trace 末尾 400ms 内切片 TOP20:')
    t_lo = b['end_ts'] - 400_000_000
    for r in rows(tp, f"""
        SELECT s.name AS name, count(*) AS n, sum(s.dur)/1e6 AS ms, t.name AS tname
        FROM slice s JOIN thread_track tt ON s.track_id = tt.id JOIN thread t ON tt.utid = t.utid
        WHERE s.ts < {b['end_ts']} AND s.ts + s.dur > {t_lo} AND s.dur >= 0
        GROUP BY s.name, t.utid ORDER BY ms DESC LIMIT 20
    """, ['name', 'n', 'ms', 'tname']):
        print(f"    {str(r['name']):56s} n={r['n']:>5d} {r['ms']:>9.1f}ms [{r['tname']}]")

    print('\n[6] 生成阶段切片（按状态）全程统计:')
    for r in rows(tp, """
        SELECT s.name AS name, count(*) AS n, sum(s.dur)/1e6 AS ms
        FROM slice s WHERE s.name IN ('ServerChunkManager::executeStepTask',
            'ChunkProgressionTask::executeStatusStep') GROUP BY s.name ORDER BY ms DESC
    """, ['name', 'n', 'ms']):
        print(f"    {str(r['name']):56s} n={r['n']:>6d} {r['ms']:>10.1f}ms")

    tp.close()
    print('\nDONE')


if __name__ == '__main__':
    main()
