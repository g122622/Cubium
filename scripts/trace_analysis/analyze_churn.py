# -*- coding: utf-8 -*-
"""定位票级振荡热点：确认「生成 halo 被当作卸载候选」导致 churn 的假设。

onTicketLevelChanged / onLevelChanged 切片带 x/z/oldLevel/newLevel 参数，
可据此统计升到加载阈值以上（>34）的区块坐标分布与重复次数。
"""
import collections
import os

from perfetto.trace_processor import TraceProcessor, TraceProcessorConfig

TRACE = 'E:/dev/minecraft-reborn-branch-1/server_trace.perfetto-trace'
OUT_DIR = 'E:/dev/minecraft-reborn-branch-1/scripts/trace_analysis'
MAX_LOADED_LEVEL = 34


def rows(tp, sql, cols):
    return [{c: getattr(r, c) for c in cols} for r in tp.query(sql)]


def main():
    tp = TraceProcessor(trace=TRACE, config=TraceProcessorConfig(load_timeout=900))

    print('[1] onTicketLevelChanged 的 newLevel 分布:')
    for r in rows(tp, """
        SELECT a.int_value AS lvl, count(*) AS n
        FROM slice s JOIN args a ON s.arg_set_id = a.arg_set_id
        WHERE s.name = 'ServerChunkManager::onTicketLevelChanged' AND a.key = 'newLevel'
        GROUP BY 1 ORDER BY 1
    """, ['lvl', 'n']):
        print(f"    newLevel={r['lvl']:>3d}  n={r['n']}")

    print('\n[2] 升到阈值以上（newLevel > 34）的坐标热点 TOP20（x, z, 次数, 距原点 Chebyshev 距离）:')
    hot = rows(tp, f"""
        SELECT ax.int_value AS x, az.int_value AS z, count(*) AS n
        FROM slice s
        JOIN args ax ON s.arg_set_id = ax.arg_set_id AND ax.key = 'x'
        JOIN args az ON s.arg_set_id = az.arg_set_id AND az.key = 'z'
        JOIN args an ON s.arg_set_id = an.arg_set_id AND an.key = 'newLevel'
        WHERE s.name = 'ServerChunkManager::onTicketLevelChanged' AND an.int_value > {MAX_LOADED_LEVEL}
        GROUP BY 1, 2 ORDER BY n DESC LIMIT 20
    """, ['x', 'z', 'n'])
    for r in hot:
        dist = max(abs(r['x']), abs(r['z']))
        print(f"    ({r['x']:>5d},{r['z']:>5d})  n={r['n']:>5d}  距原点={dist}")

    print('\n[3] 升阈值事件按距离分桶（距原点 Chebyshev 区块数）:')
    buckets = collections.Counter()
    allrise = rows(tp, f"""
        SELECT ax.int_value AS x, az.int_value AS z
        FROM slice s
        JOIN args ax ON s.arg_set_id = ax.arg_set_id AND ax.key = 'x'
        JOIN args az ON s.arg_set_id = az.arg_set_id AND az.key = 'z'
        JOIN args an ON s.arg_set_id = an.arg_set_id AND an.key = 'newLevel'
        WHERE s.name = 'ServerChunkManager::onTicketLevelChanged' AND an.int_value > {MAX_LOADED_LEVEL}
    """, ['x', 'z'])
    for r in allrise:
        d = max(abs(r['x']), abs(r['z']))
        buckets[(d // 4) * 4] += 1
    for k in sorted(buckets):
        print(f"    距离 {k:>2d}-{k + 3:<3d} 区块: {buckets[k]:>6d} 次")

    print('\n[4] 同一坐标被反复「升阈值」的比例:')
    cnt = collections.Counter()
    for r in allrise:
        cnt[(r['x'], r['z'])] += 1
    if cnt:
        rep = collections.Counter(cnt.values())
        for times in sorted(rep)[:12]:
            print(f"    被升阈值 {times:>3d} 次的坐标数: {rep[times]}")
        print(f"    去重坐标数={len(cnt)}  升阈值事件总数={len(allrise)}  "
              f"平均每坐标={len(allrise) / len(cnt):.1f} 次")

    print('\n[5] 生成 halo（level 35..45）持有者的目标状态分布取样:')
    for r in rows(tp, """
        SELECT a.string_value AS status, count(*) AS n
        FROM slice s JOIN args a ON s.arg_set_id = a.arg_set_id
        WHERE s.name = 'ServerChunkManager::executeStepTask' AND a.key = 'status'
        GROUP BY 1 ORDER BY n DESC LIMIT 15
    """, ['status', 'n']):
        print(f"    {str(r['status']):20s} n={r['n']}")

    tp.close()
    print('\nDONE')


if __name__ == '__main__':
    main()
