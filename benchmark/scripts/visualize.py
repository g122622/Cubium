#!/usr/bin/env python3
"""
Generate per-case benchmark charts from the unified benchmark CSV file.

Copyright (c) 2026 Guo Yi

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

"""

from __future__ import annotations

import sys
from pathlib import Path

import matplotlib.pyplot as plt
import pandas as pd


def load_results(csv_path: Path) -> pd.DataFrame:
    if not csv_path.exists():
        raise FileNotFoundError(f"benchmark csv not found: {csv_path}")

    data = pd.read_csv(csv_path)
    if "durationMs" not in data.columns or "caseName" not in data.columns:
        raise ValueError("benchmark csv is missing required columns")
    return data


def sanitize_file_name(name: str) -> str:
    return "".join(ch if ch.isalnum() or ch in ("-", "_") else "_" for ch in name)


def plot_case(case_name: str, case_data: pd.DataFrame, output_dir: Path) -> None:
    measured_data = case_data.dropna(subset=["iteration", "durationMs"]).copy()
    if measured_data.empty:
        return

    measured_data["iteration"] = measured_data["iteration"].astype(int)
    measured_data["durationMs"] = measured_data["durationMs"].astype(float)

    average_ms = measured_data["durationMs"].mean()
    min_ms = measured_data["durationMs"].min()
    max_ms = measured_data["durationMs"].max()

    fig, ax = plt.subplots(figsize=(12, 6))
    ax.plot(
        measured_data["iteration"],
        measured_data["durationMs"],
        color="#1f77b4",
        linewidth=1.5,
        alpha=0.85,
    )
    ax.axhline(average_ms, color="#d62728", linestyle="--", linewidth=1.2, label=f"Average: {average_ms:.4f} ms")
    ax.set_title(f"{case_name} Iteration Duration")
    ax.set_xlabel("Iteration")
    ax.set_ylabel("Duration (ms)")
    ax.set_ylim(bottom=0.0)
    ax.grid(True, alpha=0.3)
    ax.legend()

    stats_text = f"min={min_ms:.4f} ms\nmax={max_ms:.4f} ms\ncount={len(measured_data)}"
    ax.text(
        0.98,
        0.98,
        stats_text,
        transform=ax.transAxes,
        ha="right",
        va="top",
        fontsize=9,
        bbox={"boxstyle": "round", "facecolor": "white", "alpha": 0.85, "edgecolor": "#bbbbbb"},
    )

    fig.tight_layout()
    output_path = output_dir / f"{sanitize_file_name(case_name)}.png"
    fig.savefig(output_path, dpi=150)
    plt.close(fig)


def main() -> int:
    if len(sys.argv) != 3:
        raise ValueError("usage: visualize.py <result_dir> <benchmark_csv>")

    result_dir = Path(sys.argv[1])
    csv_path = Path(sys.argv[2])
    result_dir.mkdir(parents=True, exist_ok=True)

    data = load_results(csv_path)
    for case_name, case_data in data.groupby("caseName"):
        plot_case(case_name, case_data, result_dir)

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
