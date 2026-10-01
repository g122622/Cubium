# -*- coding: utf-8 -*-
"""脚本2：相位划分 + 内存/区块计数对齐 + 静置期切片画像。"""
import csv
import os

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
    b = rows(tp, 'SELECT start_ts, end_ts FROM trace_bounds', ['start_ts', 'end_ts'])[0]
    T0 = b['start_ts']

    # ---------- 1. 关键时刻 ----------
    # PlayerCount 首次=1
    pc = rows(tp, f"""
        SELECT min(ts) AS ts FROM counter c JOIN counter_track t ON c.track_id=t.id
        WHERE t.name='PlayerCount' AND c.value >= 1
    """, ['ts'])[0]['ts']
    # stopCore 起止
    stop = rows(tp, """
        SELECT s.ts, s.dur FROM slice s WHERE s.name='MinecraftServer::stopCore' ORDER BY s.ts
    """, ['ts', 'dur'])
    stop_ts = stop[0]['ts'] if stop else None
    # 最后一次 tick 切片
    last_tick = rows(tp, """
        SELECT max(s.ts+s.dur) AS ts FROM slice s WHERE s.name='MinecraftServerTick'
    """, ['ts'])[0]['ts']

    print(f"[1] PlayerCount首次=1: {sec(pc - T0):.2f}s")
    print(f"    stopCore 开始: {sec(stop_ts - T0):.2f}s (dur={sec(stop[0]['dur']):.2f}s)" if stop_ts else "    无 stopCore")
    print(f"    最后 MinecraftServerTick 结束: {sec(last_tick - T0):.2f}s")
    print(f"    trace 总长: {sec(b['end_ts'] - T0):.2f}s")

    # 相位边界：0=启动, 1=进服+区块突发, 2=静置, 3=关服
    # 进服突发结束时刻：以最后一次 SectionCodec::serializeFromChunkSection（ServerMainThread 发包序列化）为准
    last_send = rows(tp, """
        SELECT max(s.ts+s.dur) AS ts FROM slice s
        JOIN thread_track tt ON s.track_id=tt.id JOIN thread t ON tt.utid=t.utid
        WHERE s.name='SectionCodec::serializeFromChunkSection' AND t.name='ServerMainThread'
    """, ['ts'])[0]['ts']
    print(f"    主线程最后一次区块序列化发包: {sec(last_send - T0):.2f}s")

    # ---------- 2. 每秒对齐表 ----------
    series = {}
    for name in ('ProcessMemory', 'ProcessCommit', 'TotalLoadedChunkCount',
                 'TotalChunkHolderCount', 'TotalEntityCount', 'PlayerCount', 'TPS'):
        series[name] = {r['s']: r['v'] for r in rows(tp, f"""
            SELECT CAST((c.ts - {T0})/1000000000 AS INT) AS s,
                   avg(c.value) AS v
            FROM counter c JOIN counter_track t ON c.track_id=t.id
            WHERE t.name='{name}' GROUP BY 1
        """, ['s', 'v'])}

    path = os.path.join(OUT_DIR, 'timeline_1s.csv')
    with open(path, 'w', newline='') as f:
        w = csv.writer(f)
        w.writerow(['sec', 'phase', 'mem_mb', 'commit_mb', 'loaded_chunks',
                    'chunk_holders', 'entities', 'players', 'tps'])
        for s in range(0, int(sec(b['end_ts'] - T0)) + 1):
            phase = (0 if s * 1_000_000_000 + T0 < pc else
                     1 if s * 1_000_000_000 + T0 < last_send else
                     2 if stop_ts is None or s * 1_000_000_000 + T0 < stop_ts else 3)
            w.writerow([s, phase,
                        round(series['ProcessMemory'].get(s, float('nan')), 1),
                        round(series['ProcessCommit'].get(s, float('nan')), 1),
                        round(series['TotalLoadedChunkCount'].get(s, float('nan')), 0),
                        round(series['TotalChunkHolderCount'].get(s, float('nan')), 0),
                        round(series['TotalEntityCount'].get(s, float('nan')), 0),
                        round(series['PlayerCount'].get(s, float('nan')), 0),
                        round(series['TPS'].get(s, float('nan')), 1)])
    print(f"\n[2] 对齐时间线 -> {path}")

    # 打印关键段：35s 之后每秒一行
    print('\n[2] 时间线（35s起，每秒：相位/内存/提交/已加载区块/holder/实体/玩家）:')
    print('    sec ph  memMB commitMB chunks holders ent plc tps')
    for s in range(35, int(sec(b['end_ts'] - T0)) + 1):
        phase = (0 if s * 1_000_000_000 + T0 < pc else
                 1 if s * 1_000_000_000 + T0 < last_send else
                 2 if stop_ts is None or s * 1_000_000_000 + T0 < stop_ts else 3)
        m = series['ProcessMemory'].get(s)
        c = series['ProcessCommit'].get(s)
        lc = series['TotalLoadedChunkCount'].get(s)
        ch = series['TotalChunkHolderCount'].get(s)
        en = series['TotalEntityCount'].get(s)
        pl = series['PlayerCount'].get(s)
        tps = series['TPS'].get(s)
        print(f"    {s:>3d} {phase}  {m:>6.1f} {c:>7.1f} {lc:>6.0f} {ch:>7.0f} {en:>5.0f} {pl:>3.0f} {tps:>4.0f}")

    # ---------- 3. 静置期切片画像 ----------
    # 静置期 = last_send 之后 ~ stopCore 之前
    idle_start = last_send
    idle_end = stop_ts if stop_ts else b['end_ts']
    print(f"\n[3] 静置期: {sec(idle_start - T0):.2f}s -> {sec(idle_end - T0):.2f}s "
          f"(共 {sec(idle_end - idle_start):.2f}s)")
    print('    静置期切片 TOP45（name / 次数 / 总ms / 线程）:')
    for r in rows(tp, f"""
        SELECT s.name AS name, count(*) AS n, sum(s.dur)/1e6 AS total_ms, t.name AS tname
        FROM slice s
        JOIN thread_track tt ON s.track_id=tt.id JOIN thread t ON tt.utid=t.utid
        WHERE s.ts >= {idle_start} AND s.ts < {idle_end} AND s.dur >= 0
        GROUP BY s.name, t.utid ORDER BY total_ms DESC LIMIT 45
    """, ['name', 'n', 'total_ms', 'tname']):
        print(f"    {str(r['name']):55s} n={r['n']:>7d} tot={r['total_ms']:>10.1f}ms [{r['tname']}]")

    # 静置期按秒的内存下降斜率
    mems = [(s, series['ProcessMemory'].get(s)) for s in range(0, 63) if series['ProcessMemory'].get(s) is not None]
    print('\n[3] 内存每 5s 采样（起点值 -> 终点值）:')
    for s in range(0, 60, 5):
        v0 = series['ProcessMemory'].get(s)
        v1 = series['ProcessMemory'].get(min(s + 4, 62))
        if v0 is not None and v1 is not None:
            print(f"    {s:>2d}-{min(s+4,62):>2d}s: {v0:>6.1f} -> {v1:>6.1f}  ({v1-v0:>+7.1f}MB)")

    tp.close()
    print('\nDONE')


if __name__ == '__main__':
    main()
