# -*- coding: utf-8 -*-
"""校验修复后的 trace：化石放置路径是否触发、是否有异常终止迹象、内存斜率归因。

- FossilFeature 切片：确认 getZeroPositionWithTransform 修复被真实执行（若本次种子无化石，则未覆盖）。
- 末尾是否为干净关闭（stopCore）而非断言中止。
- 内存缓慢爬升斜率：与 trace 缓冲增长对齐（游戏侧常驻应基本持平）。
"""
from perfetto.trace_processor import TraceProcessor, TraceProcessorConfig

TRACE = 'E:/dev/minecraft-reborn-branch-1/server_trace.perfetto-trace'


def rows(tp, sql, cols):
    return [{c: getattr(r, c) for c in cols} for r in tp.query(sql)]


def main():
    tp = TraceProcessor(trace=TRACE, config=TraceProcessorConfig(load_timeout=900))
    b = rows(tp, 'SELECT start_ts, end_ts FROM trace_bounds', ['start_ts', 'end_ts'])[0]
    T0 = b['start_ts']
    dur = (b['end_ts'] - T0) / 1e9

    print('[1] 化石/模板特征切片（确认世界生成崩溃修复是否被覆盖）:')
    for r in rows(tp, """
        SELECT s.name AS name, count(*) AS n, min((s.ts - %d)/1000000000.0) AS first_s,
               max((s.ts - %d)/1000000000.0) AS last_s
        FROM slice s
        WHERE s.name LIKE '%%Fossil%%' OR s.name LIKE '%%ConfiguredFossil%%'
        GROUP BY s.name ORDER BY n DESC LIMIT 10
    """ % (T0, T0), ['name', 'n', 'first_s', 'last_s']):
        print(f"    {str(r['name']):48s} n={r['n']:>5d} 首={r['first_s']:.1f}s 末={r['last_s']:.1f}s")

    print('\n[2] 世界生成/模板相关切片 TOP10（若化石未触发，用模板放置类切片佐证生成路径健康）:')
    for r in rows(tp, """
        SELECT s.name AS name, count(*) AS n
        FROM slice s
        WHERE s.name LIKE '%%Template::%%' OR s.name LIKE '%%Feature::%%' OR s.name LIKE '%%placeFeatures%%'
        GROUP BY s.name ORDER BY n DESC LIMIT 10
    """, ['name', 'n']):
        print(f"    {str(r['name']):56s} n={r['n']}")

    print('\n[3] 末尾 3 秒关键生命周期切片（判断是干净关闭还是崩溃中止）:')
    t_lo = b['end_ts'] - 3_000_000_000
    for r in rows(tp, f"""
        SELECT s.name AS name, count(*) AS n, (min(s.ts) - {T0})/1000000000.0 AS first_s
        FROM slice s
        WHERE s.ts > {t_lo} AND (s.name LIKE '%%stopCore%%' OR s.name LIKE '%%shutdown%%'
              OR s.name LIKE '%%saveAllWorldData%%' OR s.name LIKE '%%ASSERT%%')
        GROUP BY s.name ORDER BY first_s LIMIT 20
    """, ['name', 'n', 'first_s']):
        print(f"    {r['first_s']:>6.2f}s  {str(r['name']):56s} n={r['n']}")

    print('\n[4] 内存斜率归因（每 5s 的 mem 均值与增量）:')
    mem = {r['s']: r['v'] for r in rows(tp, f"""
        SELECT CAST((c.ts - {T0})/1000000000 AS INT) AS s, avg(c.value) AS v
        FROM counter c JOIN counter_track t ON c.track_id = t.id
        WHERE t.name='ProcessMemory' GROUP BY 1
    """, ['s', 'v'])}
    prev = None
    for s in range(0, int(dur) + 1, 5):
        v = mem.get(s)
        if v is None:
            continue
        delta = '' if prev is None else f'Δ={v - prev:+.1f}MB'
        print(f"    {s:>3d}s mem={v:>6.1f}MB {delta}")
        prev = v

    tp.close()
    print('\nDONE')


if __name__ == '__main__':
    main()
