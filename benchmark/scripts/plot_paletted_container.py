#!/usr/bin/env python3
"""PalettedContainer 随机读写基准的折线图与调参摘要。

输入：mc_benchmark 的结果 JSON（默认取 benchmark_results/ 下最新一份）。
输出：PNG 折线图（横轴 = 元素种类数量，纵轴 = 随机读写性能，读/写各一条线），
      并往 stdout 打印每个档位的明细表与阈值对比摘要。

用法：
    python benchmark/scripts/plot_paletted_container.py \
        --results benchmark_results/<时间戳>/results.json \
        --output docs/paletted_container_random_access.png
"""

from __future__ import annotations

import argparse
import json
import math
import sys
from pathlib import Path

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
from matplotlib import font_manager
from matplotlib.ticker import FuncFormatter, LogLocator

REPO_ROOT = Path(__file__).resolve().parents[2]
DEFAULT_OUTPUT = REPO_ROOT / "docs" / "paletted_container_random_access.png"

# 容器只有一种工作模式（调色板 + 位压缩 + 反向哈希表）：k=1 是均匀态（bits=0、无 storage），
# k>=2 时位宽 = max(1, ceil(log2(k)))。下面是用于标注的位宽档位边界。
MIN_BITS = 1
BIT_WIDTH_REGIONS = [
    (1, 1, "uniform"),
    (2, 2, "1 bit"),
    (3, 4, "2 bits"),
    (5, 8, "3 bits"),
    (9, 16, "4 bits"),
    (17, 4096, "5..12 bits"),
]

READ_PREFIX = "PalettedContainerRandomRead"
WRITE_PREFIX = "PalettedContainerRandomWrite"


def pick_cjk_font() -> str | None:
    """挑选一个可用的中文字体，避免图上出现方块字。"""
    for name in ("Microsoft YaHei", "SimHei", "Noto Sans CJK SC", "Source Han Sans SC", "PingFang SC"):
        try:
            font_manager.findfont(font_manager.FontProperties(family=name), fallback_to_default=False)
            return name
        except Exception:
            continue
    return None


def find_latest_results() -> Path:
    candidates = sorted((REPO_ROOT / "benchmark_results").glob("*/results.json"), key=lambda p: p.stat().st_mtime)
    if not candidates:
        sys.exit("找不到任何 benchmark_results/*/results.json；请先运行 mc_benchmark")
    return candidates[-1]


def load_series(results_path: Path) -> dict:
    with results_path.open(encoding="utf-8") as handle:
        payload = json.load(handle)
    context = payload.get("context", {})

    # 同一档位取 _median（repetitions>1 时由 benchmark 聚合输出），否则取单次结果。
    series: dict[str, dict[int, dict]] = {"read": {}, "write": {}}
    for entry in payload["benchmarks"]:
        name = entry["name"]
        is_aggregate = name.endswith("_median") or name.endswith("_mean") or name.endswith("_stddev")
        if name.endswith("_stddev") or name.endswith("_cv"):
            continue
        if name.endswith("_mean"):
            continue  # 优先用中位数
        if name.startswith(READ_PREFIX):
            kind = "read"
            rate_key = "reads_per_second"
        elif name.startswith(WRITE_PREFIX):
            kind = "write"
            rate_key = "writes_per_second"
        else:
            continue
        if rate_key not in entry:
            continue

        kinds = int(entry["distinct_values"])
        rate = float(entry[rate_key])
        record = {
            "kinds": kinds,
            "bits": int(entry["bits_per_entry"]),
            "palette": int(entry["palette_size"]),
            "memory_bytes": float(entry["memory_bytes"]),
            "mops": rate / 1e6,
            "ns_per_op": 1e9 / rate,
            "aggregate": is_aggregate,
        }
        previous = series[kind].get(kinds)
        # 聚合行优先于单次行
        if previous is None or (record["aggregate"] and not previous["aggregate"]):
            series[kind][kinds] = record
    return {"series": series, "context": context, "path": results_path}


def state_of(bits: int) -> str:
    """位宽 → 状态标签（bits=0 即均匀态：无 storage）。"""
    return "uniform" if bits == 0 else f"{bits} bits"


def print_tables(data: dict) -> None:
    read, write = data["series"]["read"], data["series"]["write"]
    kinds_all = sorted(set(read) | set(write))
    print(f"{'kinds':>6} {'bits':>5} {'state':>9} {'palette':>8} {'mem(B)':>8} "
          f"{'read Mops/s':>12} {'write Mops/s':>13} {'read ns':>8} {'write ns':>9}")
    for kinds in kinds_all:
        row = read.get(kinds) or write.get(kinds)
        r, w = read.get(kinds), write.get(kinds)
        print(f"{kinds:>6} {row['bits']:>5} {state_of(row['bits']):>9} {row['palette']:>8} "
              f"{row['memory_bytes']:>8.0f} "
              f"{(r['mops'] if r else float('nan')):>12.1f} {(w['mops'] if w else float('nan')):>13.1f} "
              f"{(r['ns_per_op'] if r else float('nan')):>8.2f} {(w['ns_per_op'] if w else float('nan')):>9.2f}")

    # 均匀态 → 1 位 的拐点（唯一值从 1 增到 2）
    print("\n均匀态(k=1, 无 storage) vs 1 位(k=2):")
    for kinds in (1, 2):
        r, w = read.get(kinds), write.get(kinds)
        if r and w:
            print(f"  k={kinds:>4}: read {r['mops']:8.1f} Mops/s  write {w['mops']:8.1f} Mops/s  "
                  f"bits={r['bits']} mem={r['memory_bytes']:.0f}B")

    # 每个位宽档的读写均值（看位宽对性能的影响，尤其是 2 的幂位宽不跨 u64 字）
    print("\n按位宽汇总（同档位所有 k 的算术平均）:")
    by_bits: dict[int, list[tuple[float, float]]] = {}
    for kinds, r in read.items():
        w = write.get(kinds)
        if w:
            by_bits.setdefault(r["bits"], []).append((r["mops"], w["mops"]))
    for bits in sorted(by_bits):
        pairs = by_bits[bits]
        avg_r = sum(p[0] for p in pairs) / len(pairs)
        avg_w = sum(p[1] for p in pairs) / len(pairs)
        print(f"  bits={bits:>2} ({len(pairs):>2} 档): read {avg_r:8.1f} Mops/s  write {avg_w:8.1f} Mops/s")


def render(data: dict, output: Path) -> None:
    read, write = data["series"]["read"], data["series"]["write"]
    kinds_all = sorted(set(read) | set(write))
    font = pick_cjk_font()
    zh = font is not None
    if font:
        plt.rcParams["font.sans-serif"] = [font]
        plt.rcParams["axes.unicode_minus"] = False

    def t(zh_text: str, en_text: str) -> str:
        return zh_text if zh else en_text

    fig, ax = plt.subplots(figsize=(12.5, 7.0), dpi=150)

    read_k = [k for k in kinds_all if k in read]
    write_k = [k for k in kinds_all if k in write]
    ax.plot(read_k, [read[k]["mops"] for k in read_k], marker="o", markersize=5, linewidth=1.9,
            color="#1f77b4", label=t("随机读 get()", "random read get()"))
    ax.plot(write_k, [write[k]["mops"] for k in write_k], marker="s", markersize=5, linewidth=1.9,
            color="#d94801", label=t("随机写 set()", "random write set()"))

    ax.set_xscale("log", base=2)
    # 纵轴线性且从 0 开始（不使用指数缩放）
    ax.set_yscale("linear")
    peak_mops = max([v["mops"] for v in read.values()] + [v["mops"] for v in write.values()])
    ax.set_ylim(0.0, peak_mops * 1.08)
    ax.set_xlabel(t("元素种类数量（唯一值个数 k）", "distinct element kinds (k)"), fontsize=12)
    ax.set_ylabel(t("随机读写性能（Mops/s，越高越好）", "random access throughput (Mops/s)"), fontsize=12)
    ax.set_title(t("PalettedContainer 随机读写性能 vs 位宽（每次迭代 4×4096 次随机访问，已预热、已剔除预热阶段）",
                   "PalettedContainer random access vs bit width (4x4096 random ops/iteration, warmed up)"),
                 fontsize=13)
    ax.grid(True, which="both", linestyle=":", linewidth=0.6, alpha=0.55)
    ax.legend(fontsize=11, loc="lower left")

    # 主要 x 刻度：以 2 的幂标注
    ticks = [k for k in kinds_all if k in (1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096)]
    ax.set_xticks(ticks)
    ax.get_xaxis().set_major_formatter(FuncFormatter(lambda value, _pos: f"{int(value):d}"))
    ax.xaxis.set_minor_locator(LogLocator(base=2.0, subs="auto", numticks=40))

    # 位宽档分界（只有一种工作模式，k 决定位宽）：均匀态 | 1 | 2 | 3 | 4 | 5..12 位
    ymin, ymax = ax.get_ylim()
    for _lo, hi, _label in BIT_WIDTH_REGIONS[:-1]:
        ax.axvline(hi + 0.5, color="#888888", linestyle=":", linewidth=0.9, alpha=0.8)
    for lo, hi, label in BIT_WIDTH_REGIONS:
        center = math.sqrt(lo * hi) if lo != hi else lo
        ax.text(center, ymax * 0.62, t(label.replace("bits", "位").replace("uniform", "均匀态"),
                                      label),
                fontsize=9, color="#555555", ha="center")
    ax.text(0.995, 0.02,
            t(f"注：位宽 = max({MIN_BITS}, ceil(log2 k))；k=1 为均匀态（bits=0、无 storage）；"
              "非 2 的幂位宽（3/5/6/7/9..12）条目跨 u64 字",
              f"note: bits = max({MIN_BITS}, ceil(log2 k)); k=1 is uniform (no storage)"),
            transform=ax.transAxes, ha="right", fontsize=9, color="#666666")

    ctx = data["context"]
    caption = (f"{ctx.get('host_name', '?')} | {ctx.get('num_cpus', '?')} CPUs | "
               f"{float(ctx.get('mhz_per_cpu', 0)) / 1000:.2f} GHz | {ctx.get('date', '?')} | "
               f"{Path(data['path']).parent.name}")
    fig.text(0.01, 0.005, caption, fontsize=8, color="#777777")

    fig.tight_layout(rect=(0, 0.02, 1, 1))
    output.parent.mkdir(parents=True, exist_ok=True)
    fig.savefig(output)
    print(f"\n已写出折线图: {output}")


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--results", type=Path, default=None, help="mc_benchmark 的 results.json（默认取最新）")
    parser.add_argument("--output", type=Path, default=DEFAULT_OUTPUT, help=f"输出 PNG（默认 {DEFAULT_OUTPUT}）")
    args = parser.parse_args()

    results_path = args.results or find_latest_results()
    data = load_series(results_path)
    print(f"结果文件: {results_path}")
    if not data["series"]["read"]:
        sys.exit("results.json 中没有 PalettedContainerRandomRead/Write 结果；请先跑该用例")
    print_tables(data)
    render(data, args.output)


if __name__ == "__main__":
    main()
