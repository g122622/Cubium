# -*- coding: utf-8 -*-
"""脚本3：关键切片逐秒直方图 + 滑落窗/回升窗分别画像。"""
import csv
import os

from perfetto.trace_processor import TraceProcessor, TraceProcessorConfig

TRACE = 'E:/dev/minecraft-reborn-branch-1/server_trace.perfetto-trace'
OUT_DIR = 'E:/dev/minecraft-reborn-branch-1/trace_analysis'
os.makedirs(OUT_DIR, exist_ok=True)

KEY_SLICES = [
    'SectionCodec::serializeFromChunkSection',
    'SectionCodec::compress',
    'RocksDBDatabase::writeBatch',
    'ChunkSendManager::onChunkPreUnload',
    'ChunkSendManager::unloadChunkFromPlayers',
    'ChunkTaskScheduler::cancelGeneration',
    'ChunkDistanceGraph::onLevelChanged',
    'ServerChunkManager::executeStepTask',
    'BroadcastLightUpdate',
    'markLightChanged',
]


def rows(tp, sql, cols):
    return [{c: getattr(r, c) for c in cols} for r in tp.query(sql)]


def sec(ns):
    return ns / 1e9


def fmt(v, w, prec=1):
    if v is None:
        return ' ' * (w - 3) + 'nan'
    return f"{v:>{w}.{prec}f}"


def main():
    tp = TraceProcessor(trace=TRACE, config=TraceProcessorConfig(load_timeout=600))
    b = rows(tp, 'SELECT start_ts, end_ts FROM trace_bounds', ['start_ts', 'end_ts'])[0]
    T0 = b['start_ts']
    dur = int(sec(b['end_ts'] - T0))

    stop = rows(tp, "SELECT ts, dur FROM slice WHERE name='MinecraftServer::stopCore' ORDER BY ts",
                ['ts', 'dur'])
    stop_s = sec(stop[0]['ts'] - T0) if stop else None

    # ---------- 1. 关键切片逐秒直方图 ----------
    print('[1] 关键切片逐秒计数（线程分组，主线程/其他）:')
    names_csv = ','.join(f"'{n}'" for n in KEY_SLICES)
    hist = rows(tp, f"""
        SELECT s.name AS name, t.name AS tname,
               CAST((s.ts - {T0})/1000000000 AS INT) AS s, count(*) AS n
        FROM slice s
        JOIN thread_track tt ON s.track_id=tt.id JOIN thread t ON tt.utid=t.utid
        WHERE s.name IN ({names_csv})
        GROUP BY s.name, t.utid, 3 ORDER BY s.name, t.utid, 3
    """, ['name', 'tname', 's', 'n'])
    tables = {}
    for r in hist:
        key = (r['name'], str(r['tname']))
        tables.setdefault(key, {})[r['s']] = r['n']

    for (name, tname), tab in sorted(tables.items()):
        nonzero = [s for s, n in tab.items() if n > 0]
        if not nonzero:
            continue
        lo, hi = min(nonzero), max(nonzero)
        total = sum(tab.values())
        print(f"\n  {name} [{tname}] total={total} 活跃区间={lo}-{hi}s")
        # 每秒打印成 60 列紧凑图：>9 才显示数字
        line = '     '
        for s in range(0, dur + 1):
            n = tab.get(s, 0)
            if n == 0:
                line += '.'
            elif n < 10:
                line += str(n)
            elif n < 36:
                line += chr(ord('a') + n - 10)
            else:
                line += '*'
        print(line)

    # ---------- 2. 内存 + 区块数逐秒（紧凑） ----------
    def ctr_series(name):
        return {r['s']: r['v'] for r in rows(tp, f"""
            SELECT CAST((c.ts - {T0})/1000000000 AS INT) AS s, avg(c.value) AS v
            FROM counter c JOIN counter_track t ON c.track_id=t.id
            WHERE t.name='{name}' GROUP BY 1
        """, ['s', 'v'])}

    mem = ctr_series('ProcessMemory')
    com = ctr_series('ProcessCommit')
    lc = ctr_series('TotalLoadedChunkCount')
    ch = ctr_series('TotalChunkHolderCount')

    print('\n[2] 逐秒内存/区块（0-62s，5s 一档均值）:')
    for s in range(0, dur, 5):
        vals = [mem.get(i) for i in range(s, min(s + 5, dur + 1)) if mem.get(i) is not None]
        vals_c = [com.get(i) for i in range(s, min(s + 5, dur + 1)) if com.get(i) is not None]
        vals_l = [lc.get(i) for i in range(s, min(s + 5, dur + 1)) if lc.get(i) is not None]
        vals_h = [ch.get(i) for i in range(s, min(s + 5, dur + 1)) if ch.get(i) is not None]
        if vals:
            m = sum(vals) / len(vals)
            c = sum(vals_c) / len(vals_c) if vals_c else float('nan')
            l = sum(vals_l) / len(vals_l) if vals_l else float('nan')
            h = sum(vals_h) / len(vals_h) if vals_h else float('nan')
            print(f"    {s:>2d}-{min(s+4, dur):>2d}s: mem={m:>6.1f}MB commit={c:>6.1f}MB "
                  f"chunks={l:>6.0f} holders={h:>6.0f}")

    # ---------- 3. 滑落窗（45-52s）与回升窗（52-61s）切片画像 ----------
    for label, lo_s, hi_s in (('滑落窗', 45, 52), ('回升窗', 52, 61), ('爬升卸载窗', 35, 45)):
        t_lo = T0 + lo_s * 1_000_000_000
        t_hi = T0 + hi_s * 1_000_000_000
        print(f"\n[3] {label} {lo_s}-{hi_s}s 切片 TOP25（按总时长）:")
        for r in rows(tp, f"""
            SELECT s.name AS name, count(*) AS n, sum(s.dur)/1e6 AS total_ms,
                   avg(s.dur)/1e3 AS avg_us, t.name AS tname
            FROM slice s
            JOIN thread_track tt ON s.track_id=tt.id JOIN thread t ON tt.utid=t.utid
            WHERE s.ts >= {t_lo} AND s.ts < {t_hi} AND s.dur >= 0
            GROUP BY s.name, t.utid ORDER BY total_ms DESC LIMIT 25
        """, ['name', 'n', 'total_ms', 'avg_us', 'tname']):
            print(f"    {str(r['name']):52s} n={r['n']:>7d} tot={r['total_ms']:>9.1f}ms "
                  f"avg={r['avg_us']:>8.1f}us [{r['tname']}]")

    # ---------- 4. 回升窗内高频小切片（次数 TOP） ----------
    t_lo = T0 + 52 * 1_000_000_000
    t_hi = T0 + 61 * 1_000_000_000
    print(f"\n[4] 回升窗 52-61s 切片次数 TOP20:")
    for r in rows(tp, f"""
        SELECT s.name AS name, count(*) AS n, t.name AS tname
        FROM slice s
        JOIN thread_track tt ON s.track_id=tt.id JOIN thread t ON tt.utid=t.utid
        WHERE s.ts >= {t_lo} AND s.ts < {t_hi}
        GROUP BY s.name, t.utid ORDER BY n DESC LIMIT 20
    """, ['name', 'n', 'tname']):
        print(f"    {str(r['name']):52s} n={r['n']:>8d} [{r['tname']}]")

    # ---------- 5. 内存计数器逐秒明细 CSV（含 holder/chunks）----------
    path = os.path.join(OUT_DIR, 'timeline_1s.csv')
    with open(path, 'w', newline='') as f:
        w = csv.writer(f)
        w.writerow(['sec', 'mem_mb', 'commit_mb', 'loaded_chunks', 'chunk_holders',
                    'stopcore_started'])
        for s in range(0, dur + 1):
            w.writerow([s,
                        round(mem.get(s), 1) if mem.get(s) is not None else '',
                        round(com.get(s), 1) if com.get(s) is not None else '',
                        round(lc.get(s), 0) if lc.get(s) is not None else '',
                        round(ch.get(s), 0) if ch.get(s) is not None else '',
                        1 if (stop_s is not None and s >= int(stop_s)) else 0])
    print(f"\n[5] 时间线 CSV -> {path}")

    tp.close()
    print('\nDONE')


if __name__ == '__main__':
    main()
