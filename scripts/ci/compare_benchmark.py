#!/usr/bin/env python3
"""跨日 benchmark 对比：把本次结果与上一次 nightly 的结果逐用例比对。

背景：nightly CI 跑在 GitHub 托管的 ubuntu-latest 上，**每次都是全新的机器实例**，
性能绝对值本身带有实例间噪声。因此本脚本的定位是「仅记录 + 告警，不阻塞」——
输出 Markdown 报告，把变化幅度超过阈值的用例标出来，由人（或 AI 助手）判断是真回归
还是噪声。判定性能回归前，应结合连续多晚的趋势，而不是单晚的单点差异。

用法：
    python3 scripts/ci/compare_benchmark.py \
        --current  benchmark_results/<ts>/results.json \
        --previous previous/results.json \
        --out      benchmark-compare.md \
        --cpu-threshold 10 --mem-threshold 20

退出码恒为 0（不阻塞 CI）。找不到 previous 时只输出本次结果概览。
"""

from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path

# google/benchmark 的 time_unit 到纳秒的换算系数。
_TIME_UNIT_TO_NS = {
    "ns": 1.0,
    "us": 1_000.0,
    "ms": 1_000_000.0,
    "s": 1_000_000_000.0,
}

# 内存指标（由 benchmark/MemoryProfiler.cpp 注入的 User Counters）。
_MEMORY_COUNTERS = ("num_allocs", "max_bytes_used", "total_allocated_bytes", "net_heap_growth")


def _to_ns(value: float, unit: str) -> float:
    factor = _TIME_UNIT_TO_NS.get(unit)
    if factor is None:
        raise ValueError(f"unknown time_unit: {unit!r}")
    return value * factor


def load_results(path: Path) -> dict:
    with path.open("r", encoding="utf-8") as handle:
        data = json.load(handle)
    if "benchmarks" not in data:
        raise ValueError(f"{path}: 缺少 'benchmarks' 字段，不是 google/benchmark 的 JSON 输出")
    return data


def index_entries(data: dict) -> dict[tuple[str, str], dict]:
    """把 benchmarks 数组索引成 {(用例名, 条目类型): 条目}。

    同一个用例会有多条记录：逐次重复（run_type=iteration/repetition）与聚合行
    （aggregate_name=mean/median/stddev/cv）。这里**优先取聚合行**——它是对多次重复
    的统计，比单次重复更能代表该用例的表现。若只有逐次重复，则对它们取中位数。
    """
    grouped: dict[str, dict[str, list[dict]]] = {}
    for entry in data["benchmarks"]:
        name = entry["name"]
        kind = entry.get("aggregate_name") or entry.get("run_type") or "unknown"
        grouped.setdefault(name, {}).setdefault(kind, []).append(entry)

    indexed: dict[tuple[str, str], dict] = {}
    for name, kinds in grouped.items():
        for kind, entries in kinds.items():
            indexed[(name, kind)] = entries[0] if len(entries) == 1 else _median_entry(entries)
    return indexed


def _median_entry(entries: list[dict]) -> dict:
    """对多条同类条目按 real_time 取中位数，返回一个合成的条目。"""
    ordered = sorted(entries, key=lambda item: item.get("real_time", 0.0))
    return ordered[len(ordered) // 2]


def _percent_change(current: float, previous: float) -> float | None:
    if previous == 0:
        return None
    return (current - previous) / previous * 100.0


def _fmt_pct(value: float | None) -> str:
    return "n/a" if value is None else f"{value:+.1f}%"


def build_report(current: dict, previous: dict | None, cpu_threshold: float, mem_threshold: float) -> str:
    lines: list[str] = []
    lines.append("# Benchmark 跨日对比")
    lines.append("")

    # 切换发布配置后重新建立基线，不把优化、profiler 或 sanitizer 差异误报为代码性能变化。
    if previous is not None:
        current_profile = current.get("context", {}).get("ci_build_profile")
        previous_profile = previous.get("context", {}).get("ci_build_profile")
        if current_profile != previous_profile:
            lines.extend(["> 两轮 benchmark 的构建配置不同，本次重新建立基线，不做跨配置性能比较。", ""])
            previous = None

    current_entries = index_entries(current)
    if previous is None:
        lines.append("> 没有可比较的同配置 benchmark 基线，本次仅列出概览，不做对比。")
        lines.append("")
        lines.append("| 用例 | 条目 | 时间 (ns) | 变化 |")
        lines.append("|---|---|---|---|")
        for (name, kind), entry in sorted(current_entries.items()):
            ns = _to_ns(entry.get("real_time", 0.0), entry.get("time_unit", "ns"))
            lines.append(f"| `{name}` | {kind} | {ns:,.0f} | — |")
        lines.append("")
        return "\n".join(lines)

    previous_entries = index_entries(previous)
    rows: list[tuple[str, str, float, float, float | None, str]] = []
    memory_rows: list[tuple[str, str, float, float, float | None, str]] = []
    warnings: list[str] = []

    for key, cur in sorted(current_entries.items()):
        prev = previous_entries.get(key)
        if prev is None:
            continue
        name, kind = key
        cur_ns = _to_ns(cur.get("real_time", 0.0), cur.get("time_unit", "ns"))
        prev_ns = _to_ns(prev.get("real_time", 0.0), prev.get("time_unit", "ns"))
        pct = _percent_change(cur_ns, prev_ns)
        rows.append((name, kind, cur_ns, prev_ns, pct, ""))

        if pct is not None and abs(pct) >= cpu_threshold:
            direction = "劣化" if pct > 0 else "改善"
            warnings.append(f"CPU {direction}: `{name}` ({kind}) {_fmt_pct(pct)}（阈值 {cpu_threshold:g}%）")

        # 内存指标：逐项比对（只在两端都有该计数器时比较）。
        for counter in _MEMORY_COUNTERS:
            if counter not in cur or counter not in prev:
                continue
            cur_mem = float(cur[counter])
            prev_mem = float(prev[counter])
            mem_pct = _percent_change(cur_mem, prev_mem)
            memory_rows.append((f"{name} :: {counter}", kind, cur_mem, prev_mem, mem_pct, ""))
            if mem_pct is not None and abs(mem_pct) >= mem_threshold:
                direction = "劣化" if mem_pct > 0 else "改善"
                warnings.append(
                    f"内存 {direction}: `{name}` 的 `{counter}` ({kind}) {_fmt_pct(mem_pct)}"
                    f"（阈值 {mem_threshold:g}%）"
                )

    if warnings:
        lines.append(f"## ⚠️ 超过阈值的项（{len(warnings)}）")
        lines.append("")
        lines.append("以下项的变化幅度超过阈值。**这不代表一定是性能回归**——托管 runner 每次是"
                     "全新实例，单晚差异可能来自机器噪声。判定回归请结合连续多晚趋势。")
        lines.append("")
        for warning in warnings:
            lines.append(f"- {warning}")
        lines.append("")
    else:
        lines.append("## ✅ 无超过阈值的项")
        lines.append("")
        lines.append(f"所有用例的变化幅度均在 CPU {cpu_threshold:g}% / 内存 {mem_threshold:g}% 以内。")
        lines.append("")

    lines.append("## CPU 时间对比（real_time，已归一化到纳秒）")
    lines.append("")
    lines.append("| 用例 | 条目 | 上次 (ns) | 本次 (ns) | 变化 |")
    lines.append("|---|---|---|---|---|")
    for name, kind, cur_ns, prev_ns, pct, _ in rows:
        lines.append(f"| `{name}` | {kind} | {prev_ns:,.0f} | {cur_ns:,.0f} | {_fmt_pct(pct)} |")
    lines.append("")

    if memory_rows:
        lines.append("## 内存指标对比")
        lines.append("")
        lines.append("| 用例 :: 指标 | 条目 | 上次 | 本次 | 变化 |")
        lines.append("|---|---|---|---|---|")
        for name, kind, cur_mem, prev_mem, pct, _ in memory_rows:
            lines.append(f"| `{name}` | {kind} | {prev_mem:,.0f} | {cur_mem:,.0f} | {_fmt_pct(pct)} |")
        lines.append("")

    only_current = sorted(set(current_entries) - set(previous_entries))
    only_previous = sorted(set(previous_entries) - set(current_entries))
    if only_current or only_previous:
        lines.append("## 用例增减")
        lines.append("")
        for key in only_current:
            lines.append(f"- 新增：`{key[0]}` ({key[1]})")
        for key in only_previous:
            lines.append(f"- 消失：`{key[0]}` ({key[1]})")
        lines.append("")

    return "\n".join(lines)


def main(argv: list[str]) -> int:
    parser = argparse.ArgumentParser(description="跨日 benchmark 对比（仅记录 + 告警，不阻塞）")
    parser.add_argument("--current", required=True, type=Path, help="本次 results.json")
    parser.add_argument("--previous", type=Path, default=None, help="上次 results.json（可缺省）")
    parser.add_argument("--out", type=Path, default=None, help="报告输出路径（缺省写 stdout）")
    parser.add_argument("--cpu-threshold", type=float, default=10.0, help="CPU 告警阈值（百分比）")
    parser.add_argument("--mem-threshold", type=float, default=20.0, help="内存告警阈值（百分比）")
    args = parser.parse_args(argv)

    try:
        current = load_results(args.current)
        previous = load_results(args.previous) if args.previous and args.previous.is_file() else None
        report = build_report(current, previous, args.cpu_threshold, args.mem_threshold)
    except (OSError, ValueError, KeyError) as exc:
        # 对比脚本自身的失败不应阻塞 nightly：输出一行说明并正常退出。
        message = f"# Benchmark 跨日对比\n\n> 对比失败：{exc}\n"
        if args.out:
            args.out.write_text(message, encoding="utf-8")
        else:
            print(message)
        return 0

    if args.out:
        args.out.parent.mkdir(parents=True, exist_ok=True)
        args.out.write_text(report, encoding="utf-8")
    else:
        print(report)
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
