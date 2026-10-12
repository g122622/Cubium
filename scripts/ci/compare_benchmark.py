#!/usr/bin/env python3
"""归一化跨日 benchmark 数据，生成 Matplotlib 图表和机器可读告警。"""

from __future__ import annotations

import argparse
import json
import math
import statistics
import sys
from pathlib import Path

_TIME_UNIT_TO_NS = {"ns": 1.0, "us": 1_000.0, "ms": 1_000_000.0, "s": 1_000_000_000.0}
_MEMORY_COUNTERS = ("num_allocs", "max_bytes_used", "total_allocated_bytes", "net_heap_growth")


def _to_ns(value: float, unit: str) -> float:
    """在排序和比较之前统一时间单位，拒绝非法测量。"""
    if unit not in _TIME_UNIT_TO_NS:
        raise ValueError(f"Unknown time unit: {unit!r}")
    if isinstance(value, bool):
        raise ValueError("Benchmark time cannot be a boolean")
    value = float(value)
    if not math.isfinite(value) or value < 0:
        raise ValueError("Benchmark time must be finite and nonnegative")
    normalized = value * _TIME_UNIT_TO_NS[unit]
    if not math.isfinite(normalized):
        raise ValueError("Normalized benchmark time must be finite")
    return normalized


def load_results(path: Path) -> dict:
    """读取 google/benchmark 的原始结果。"""
    data = json.loads(path.read_text(encoding="utf-8"))
    if not isinstance(data.get("benchmarks"), list):
        raise ValueError("Benchmark JSON must contain a benchmarks array")
    return data


def index_entries(data: dict) -> dict[str, dict]:
    """每个用例优先取聚合中位数；不把标准差和变异系数当成耗时。"""
    grouped = {}
    for entry in data["benchmarks"]:
        kind = entry.get("aggregate_name") or "iteration"
        if kind in ("stddev", "cv") or entry.get("aggregate_unit") == "percentage":
            continue
        if entry.get("error_occurred"):
            continue
        name = entry.get("run_name") or entry["name"]
        if kind in ("mean", "median") and not entry.get("run_name"):
            name = name.removesuffix("_" + kind)
        grouped.setdefault(name, {}).setdefault(kind, []).append(entry)
    result = {}
    for name, kinds in grouped.items():
        kind = "median" if "median" in kinds else "iteration" if "iteration" in kinds else "mean"
        entries = kinds[kind]
        times = [_to_ns(entry["real_time"], entry.get("time_unit", "ns")) for entry in entries]
        memory = {}
        for counter in _MEMORY_COUNTERS:
            values = [float(value) for entry in entries if isinstance(value := entry.get(counter), (int, float))
                      and not isinstance(value, bool) and math.isfinite(value)]
            if values:
                memory[counter] = statistics.median(values)
        result[name] = {"time_ns": statistics.median(times), "memory": memory,
                        "statistic": "mean" if kind == "mean" else "median"}
    return result


def _percent_change(current: float, previous: float) -> float | None:
    """零基线不生成无穷大或虚假的百分比。"""
    return None if previous == 0 else (current - previous) / abs(previous) * 100.0


def build_comparison(current: dict, previous: dict | None, time_threshold: float, memory_threshold: float) -> dict:
    """只比较同构建配置、同统计类型的代表值。"""
    reason = "matched" if previous is not None else "missing"
    if previous is not None and current.get("context", {}).get("ci_build_profile") != previous.get("context", {}).get("ci_build_profile"):
        previous = None
        reason = "profile_mismatch"
    current_entries = index_entries(current)
    previous_entries = index_entries(previous) if previous is not None else {}
    rows, warnings = [], []
    for name in sorted(set(current_entries) | set(previous_entries)):
        cur, prev = current_entries.get(name), previous_entries.get(name)
        compatible = cur is not None and prev is not None and cur["statistic"] == prev["statistic"]
        current_ns = cur["time_ns"] if cur else None
        previous_ns = prev["time_ns"] if prev else None
        pct = _percent_change(current_ns, previous_ns) if compatible else None
        row = {"name": name, "current_ns": current_ns, "previous_ns": previous_ns,
               "comparable": compatible,
               "current_statistic": cur["statistic"] if cur else None,
               "previous_statistic": prev["statistic"] if prev else None,
               "change_pct": pct, "memory": []}
        if pct is not None and abs(pct) >= time_threshold:
            warnings.append({"kind": "real_time", "name": name, "change_pct": pct})
        for counter in _MEMORY_COUNTERS:
            if not compatible or counter not in cur["memory"] or counter not in prev["memory"]:
                continue
            change = _percent_change(cur["memory"][counter], prev["memory"][counter])
            row["memory"].append({"counter": counter, "current": cur["memory"][counter],
                                  "previous": prev["memory"][counter], "change_pct": change})
            if change is not None and abs(change) >= memory_threshold:
                warnings.append({"kind": counter, "name": name, "change_pct": change})
        rows.append(row)
    errors = [f"{entry.get('name', 'Unknown')}: {entry.get('error_message', 'Benchmark failed')}"
              for entry in current["benchmarks"] if entry.get("error_occurred")]
    errors = list(dict.fromkeys(errors))
    if not current_entries:
        errors.append("No valid benchmark measurements")
    return {"schema_version": 1, "status": "error" if errors else "ok", "errors": errors, "baseline_reason": reason,
            "current_context": current.get("context", {}), "previous_context": previous.get("context", {}) if previous else None,
            "time_threshold_pct": time_threshold, "memory_threshold_pct": memory_threshold,
            "rows": rows, "warnings": warnings}


def main(argv: list[str]) -> int:
    parser = argparse.ArgumentParser(description="生成跨日 benchmark 图表（仅记录和告警）")
    parser.add_argument("--current", required=True, type=Path)
    parser.add_argument("--previous", type=Path)
    parser.add_argument("--out", required=True, type=Path)
    parser.add_argument("--time-threshold", type=float, default=10.0)
    parser.add_argument("--mem-threshold", type=float, default=20.0)
    args = parser.parse_args(argv)
    if __package__:
        from .report_plots import plot_benchmark, image_links
    else:
        from report_plots import plot_benchmark, image_links
    if any(not math.isfinite(value) or value < 0 for value in (args.time_threshold, args.mem_threshold)):
        parser.error("Thresholds must be finite and nonnegative")
    try:
        current = load_results(args.current)
        previous = load_results(args.previous) if args.previous and args.previous.is_file() else None
        data = build_comparison(current, previous, args.time_threshold, args.mem_threshold)
    except (OSError, ValueError, KeyError, TypeError, AttributeError) as exc:
        data = build_comparison({"benchmarks": []}, None, args.time_threshold, args.mem_threshold)
        data.update(status="error", errors=[str(exc)])
        print(f"Benchmark comparison failed: {exc}", file=sys.stderr)
    data["images"] = plot_benchmark(data, args.out.parent)
    args.out.write_text("\n\n".join(image_links(data["images"], "")) + "\n", encoding="utf-8")
    args.out.with_suffix(".json").write_text(json.dumps(data, ensure_ascii=False, indent=2, allow_nan=False) + "\n", encoding="utf-8")
    if data["warnings"]:
        print(f"::warning::{len(data['warnings'])} benchmark changes exceed the configured thresholds")
    return 1 if data["status"] == "error" else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
