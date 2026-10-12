"""生成 CI 报告的论文风格图表；原始数据单独保留为 JSON。"""

from __future__ import annotations

import math
import re
import textwrap
from collections import Counter
from pathlib import Path

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt
import numpy as np
from matplotlib import font_manager
from matplotlib.ticker import MaxNLocator


COLORS = ["#0072B2", "#009E73", "#E69F00", "#CC79A7", "#56B4E9", "#D55E00", "#757575"]
JOB_NAMES = {"prepare": "工具准备", "workers": "编译 Worker", "build": "测试构建", "release-build": "发布构建", "unit-tests": "单元测试",
             "integrated-tests": "GameTest", "e2e-tests": "端到端", "fuzz": "Fuzz",
             "benchmark": "Benchmark", "report": "汇总"}
JOB_COLORS = ["#4E79A7", "#F28E2B", "#59A14F", "#E15759", "#76B7B2",
              "#B07AA1", "#EDC948", "#9C755F", "#FF9DA7", "#79706E"]


def _style() -> None:
    """统一无衬线字体、色盲友好配色与适合静态报告的留白。"""
    fonts = ["Noto Sans CJK SC", "Noto Sans CJK JP", "Microsoft YaHei", "DejaVu Sans"]
    available = {font.name for font in font_manager.fontManager.ttflist}
    selected = next((font for font in fonts if font in available), "DejaVu Sans")
    plt.rcParams.update({"font.family": selected, "font.size": 9, "axes.titlesize": 11,
                         "axes.labelsize": 9, "axes.spines.top": False, "axes.spines.right": False,
                         "axes.edgecolor": "#AAB3BD", "axes.linewidth": .6,
                         "axes.unicode_minus": False, "grid.color": "#DEE3E8", "grid.linewidth": .6,
                         "figure.facecolor": "white", "savefig.facecolor": "white",
                         "svg.fonttype": "none", "pdf.fonttype": 42})


def _save(fig, directory: Path, stem: str) -> str:
    """同时生成报告用 PNG 与便于导出的矢量 SVG。"""
    directory.mkdir(parents=True, exist_ok=True)
    fig.savefig(directory / f"{stem}.png", dpi=220, bbox_inches="tight", pad_inches=.18)
    fig.savefig(directory / f"{stem}.svg", bbox_inches="tight", pad_inches=.18)
    plt.close(fig)
    return f"{stem}.png"


def image_links(names: list[str], base_url: str) -> list[str]:
    """输出可直接渲染图片的引用，不生成文字表格。"""
    return [f"![CI 图表]({base_url.rstrip('/')}/{name})" if base_url else
            f"![CI 图表]({name})" for name in names]


def _axis(ax, title: str, axis: str) -> None:
    """设置子图标题与轻量网格。"""
    ax.set_title(title, loc="left", pad=12, fontweight="bold")
    ax.grid(axis=axis, alpha=.8)
    ax.set_axisbelow(True)


def _empty(ax, message: str) -> None:
    """缺少数据时明确表示不可比较，不伪造零值。"""
    ax.text(.5, .5, message, ha="center", va="center", transform=ax.transAxes, color="#657180")
    ax.set_axis_off()


def plot_benchmark(data: dict, directory: Path) -> list[str]:
    """呈现同配置跨轮次比较、纳秒耗时与内存变化。"""
    _style()
    rows = data["rows"]
    names = []
    if data["status"] == "error":
        fig, ax = plt.subplots(figsize=(12, max(4, len(data["errors"]) * .5)))
        ax.set_title("Benchmark 数据读取或测量失败", loc="left", fontsize=16, color=COLORS[5], pad=20)
        _empty(ax, "\n\n".join(textwrap.fill(error, 85) for error in data["errors"]))
        fig.tight_layout()
        names.append(_save(fig, directory, "benchmark-error"))
        if not rows:
            return names
    comparable = [row for row in rows if row["comparable"]]
    fig, axes = plt.subplots(1, 3, figsize=(14, 4.3), gridspec_kw={"width_ratios": [1, 1.15, 1]})
    fig.suptitle("Benchmark 跨日对比", fontsize=16, fontweight="bold", y=1.05)
    _axis(axes[0], "(a) 实际耗时与等速参考线", "both")
    points = [row for row in comparable if row["previous_ns"] > 0 and row["current_ns"] > 0]
    if points:
        previous = [row["previous_ns"] for row in points]
        current = [row["current_ns"] for row in points]
        lower, upper = min(previous + current) / 1.8, max(previous + current) * 1.8
        axes[0].plot([lower, upper], [lower, upper], color="#657180", lw=1, ls="--")
        tolerance = data["time_threshold_pct"] / 100
        axes[0].fill_between([lower, upper], [lower * max(0, 1 - tolerance), upper * max(0, 1 - tolerance)],
                             [lower * (1 + tolerance), upper * (1 + tolerance)],
                             color=COLORS[0], alpha=.07)
        axes[0].scatter(previous, current, s=24, alpha=.85, linewidths=.5, edgecolors="white",
                        c=[COLORS[2] if row["change_pct"] > data["time_threshold_pct"] else COLORS[0]
                           for row in points])
        axes[0].set(xscale="log", yscale="log", xlim=(lower, upper), ylim=(lower, upper),
                    xlabel="上一轮 real_time [ns]", ylabel="本轮 real_time [ns]")
    else:
        _empty(axes[0], {"profile_mismatch": "构建配置不同，重新建立基线", "missing": "尚无同配置基线"}
               .get(data["baseline_reason"], "无可比较的正耗时数据"))
    _axis(axes[1], "(b) 耗时变化分布", "y")
    changes = [row["change_pct"] for row in comparable if row["change_pct"] is not None]
    if changes:
        axes[1].hist(changes, bins=min(16, max(4, int(math.sqrt(len(changes))))),
                     color=COLORS[0], edgecolor="white", linewidth=1)
        axes[1].axvline(0, color="#657180", lw=1)
        for threshold in (-data["time_threshold_pct"], data["time_threshold_pct"]):
            axes[1].axvline(threshold, color=COLORS[2], lw=1, ls="--")
        axes[1].set(xlabel="real_time 变化 [%]", ylabel="用例数")
        axes[1].yaxis.set_major_locator(MaxNLocator(integer=True))
    else:
        _empty(axes[1], "基线缺失或为零，变化百分比未定义")
    _axis(axes[2], "(c) 内存指标变化", "x")
    counters = sorted({entry["counter"] for row in rows for entry in row["memory"]
                       if entry["change_pct"] is not None})
    if counters:
        values = [[entry["change_pct"] for row in rows for entry in row["memory"]
                   if entry["counter"] == counter and entry["change_pct"] is not None] for counter in counters]
        axes[2].boxplot(values, vert=False, labels=counters, patch_artist=True, widths=.5,
                        boxprops={"facecolor": COLORS[1], "alpha": .25},
                        medianprops={"color": COLORS[1], "linewidth": 1.5},
                        flierprops={"marker": ".", "markersize": 3, "markeredgecolor": COLORS[1]})
        axes[2].axvline(0, color="#657180", lw=1)
        axes[2].set_xlabel("指标变化 [%]")
        axes[2].tick_params(axis="y", labelsize=7)
    else:
        _empty(axes[2], "未记录可比较的内存计数器")
    current_date = str(data["current_context"].get("date", "日期未记录"))[:19]
    previous_date = str((data["previous_context"] or {}).get("date", "无基线"))[:19]
    fig.text(.5, -.08, f"上一轮：{previous_date} → 本轮：{current_date}", ha="center", fontsize=8, color="#657180")
    fig.text(.5, -.01, f"代表值：中位数优先；{len(comparable)} 个同配置用例可比较。"
             f" 阈值：耗时 {data['time_threshold_pct']:g}% / 内存 {data['memory_threshold_pct']:g}%。"
             " 单轮差异包含托管机器噪声。", ha="center", fontsize=8, color="#657180")
    fig.tight_layout(w_pad=3)
    names.append(_save(fig, directory, "benchmark-overview"))
    for page, offset in enumerate(range(0, len(rows), 18), 1):
        part = rows[offset:offset + 18]
        fig, (ax, delta) = plt.subplots(1, 2, figsize=(14, max(3.2, len(part) * .36 + 1.8)),
                                      gridspec_kw={"width_ratios": [2.5, 1]})
        y = np.arange(len(part))
        for key, color, shift, label in [("previous_ns", "#8D99A6", -.16, "上一轮"),
                                         ("current_ns", COLORS[0], .16, "本轮")]:
            times = np.array([row[key] if row[key] is not None else np.nan for row in part])
            ax.barh(y + shift, times, .3, color=color, label=label, zorder=3)
        ax.set_yticks(y, [_label(row["name"]) for row in part], fontsize=8)
        ax.invert_yaxis()
        present = [row[key] for row in part for key in ("current_ns", "previous_ns") if row[key] is not None]
        ax.set_xscale("log" if present and min(present) > 0 else "symlog")
        ax.set_xlabel("real_time [ns] · 已统一时间单位")
        ax.legend(loc="upper right", frameon=False, ncol=2)
        _axis(ax, f"实际耗时对比 · 第 {page} 页", "x")
        changes = [row["change_pct"] if row["change_pct"] is not None else np.nan for row in part]
        delta.barh(y, changes, .5, color=[COLORS[2] if value > 0 else COLORS[1] for value in changes])
        delta.axvspan(-data["time_threshold_pct"], data["time_threshold_pct"], color=COLORS[0], alpha=.06)
        delta.axvline(0, color="#657180", lw=.8)
        delta.set_yticks([])
        delta.set_ylim(ax.get_ylim())
        for i, row in enumerate(part):
            value = row["change_pct"]
            text = f"{value:+.1f}%" if value is not None else (
                "统计口径不同" if row["current_statistic"] and row["previous_statistic"] and row["current_statistic"] != row["previous_statistic"] else
                "基线为零" if row["previous_ns"] == 0 else "本轮新增" if row["previous_ns"] is None else "本轮缺失")
            delta.annotate(text, (0 if value is None else value, i), xytext=(4 if value is None or value >= 0 else -4, 0),
                           textcoords="offset points", ha="left" if value is None or value >= 0 else "right",
                           va="center", fontsize=7, color="#475463")
        delta.margins(x=.35)
        delta.set_xlabel("变化 [%] · 越低越快")
        _axis(delta, "相对变化", "x")
        fig.tight_layout(w_pad=2)
        names.append(_save(fig, directory, f"benchmark-times-{page:02d}"))
    return names


def runner_plot_data(runners: dict, job_status: dict, errors: list[str]) -> dict:
    """保留缺失配置；同一非矩阵任务的多个快照只取最新一次，避免重复计数。"""
    selected = {}
    for key, info in runners.items():
        identity = key if info["job"] in ("unit-tests", "integrated-tests", "workers") else info["job"]
        if identity not in selected or info.get("captured_at", "") >= selected[identity][1].get("captured_at", ""):
            selected[identity] = (key, info)
    rows = []
    for identity, (key, info) in sorted(selected.items()):
        shard = re.search(r"test-results-(\d+)", info.get("artifact", ""))
        label = JOB_NAMES.get(info["job"], info["job"]) + (f" [{shard.group(1)}]" if shard else "")
        if info["job"] == "workers":
            label = f"{info['runner'].get('pool', '未知池')} worker {info['runner'].get('worker', '?')}"
        rows.append({"key": key, "label": label, "info": info})
    return {"rows": rows, "snapshots": len(runners), "errors": errors,
            "missing": {job: "未运行" if job_status.get(job) == "skipped" else "未采集"
                        for job in JOB_NAMES if not any(row["info"]["job"] == job for row in rows)}}


def _number(value):
    """只接受有效非负测量值，缺失量保持为空。"""
    return float(value) if isinstance(value, (int, float)) and not isinstance(value, bool) and math.isfinite(value) and value >= 0 else np.nan


def _label(value) -> str:
    """把配置标签作为普通文字，禁止用美元符号触发数学表达式解析。"""
    return " ".join(str(value).split()).replace("$", r"\$")


def _model(row: dict) -> str:
    """统一 CPU 型号中的商标符号，保留实际型号。"""
    return _label(row["info"]["cpu"].get("model") or "未采集型号").replace("(R)", "").replace("(TM)", "").replace("64-Core Processor", "").strip()


def plot_runners(data: dict, directory: Path) -> list[str]:
    """生成型号占比、容量柱状图和分布图；完整配置由 JSON 归档。"""
    _style()
    rows = data["rows"]
    fig, axes = plt.subplots(2, 3, figsize=(13.5, 8.3))
    fig.suptitle("各任务机器配置", fontsize=16, fontweight="bold", y=1.02)
    models = Counter(_model(row) for row in rows)
    model_colors = {model: COLORS[index % len(COLORS)] for index, model in enumerate(models)}
    _axis(axes[0, 0], "(a) CPU 型号占比", "y")
    axes[0, 1].set_axis_off()
    if models:
        wedges, _, _ = axes[0, 0].pie(list(models.values()), colors=COLORS, startangle=90,
                                     autopct=lambda value: f"{value:.0f}%" if value >= 8 else "",
                                     wedgeprops={"width": .38, "edgecolor": "white", "linewidth": 1.5},
                                     pctdistance=.81, textprops={"fontsize": 8})
        axes[0, 0].text(0, 0, f"{len(rows)}\n任务实例", ha="center", va="center", fontsize=13)
        axes[0, 1].legend(wedges, [f"{model} ({count})" for model, count in models.items()],
                          loc="center", frameon=False, fontsize=8)
    else:
        _empty(axes[0, 0], "尚无机器配置")
    cores = Counter(value for row in rows if math.isfinite(value := _number(row["info"]["cpu"].get("logical_cpus"))) and value > 0)
    _axis(axes[0, 2], "(b) 逻辑核心数分布", "y")
    if cores:
        keys = sorted(cores)
        axes[0, 2].bar([f"{key:g}" for key in keys], [cores[key] for key in keys], color=COLORS[0], width=.5)
        axes[0, 2].set(xlabel="逻辑核心数", ylabel="任务实例数")
        axes[0, 2].yaxis.set_major_locator(MaxNLocator(integer=True))
    else:
        _empty(axes[0, 2], "未采集逻辑核心数")
    free = [_number(row["info"]["disk"].get("free_bytes")) / 2**30 for row in rows]
    free = [value for value in free if math.isfinite(value)]
    _axis(axes[1, 0], "(c) 磁盘余量分布", "y")
    if free:
        axes[1, 0].hist(free, bins=min(10, max(3, int(math.sqrt(len(free))))), color=COLORS[1], edgecolor="white")
        axes[1, 0].set(xlabel="空闲磁盘 [GiB] · 采集时", ylabel="任务实例数")
        axes[1, 0].yaxis.set_major_locator(MaxNLocator(integer=True))
    else:
        _empty(axes[1, 0], "未采集磁盘容量")
    images = Counter(_label(row["info"]["runner"].get("image_version") or "未采集镜像") for row in rows)
    _axis(axes[1, 1], "(d) Runner 镜像版本", "x")
    if images:
        axes[1, 1].barh(list(images), list(images.values()), color=COLORS[4], height=.45)
        axes[1, 1].set_xlabel("任务实例数")
        axes[1, 1].xaxis.set_major_locator(MaxNLocator(integer=True))
    else:
        _empty(axes[1, 1], "未采集镜像版本")
    states = Counter("已采集" for _ in rows)
    states.update(data["missing"].values())
    states.update({"读取错误": len(data["errors"])})
    states = {key: value for key, value in states.items() if value}
    _axis(axes[1, 2], "(e) 配置采集状态", "x")
    state_colors = {"已采集": COLORS[0], "未运行": "#8D99A6", "未采集": COLORS[2], "读取错误": COLORS[5]}
    axes[1, 2].barh(list(states), list(states.values()), color=[state_colors[state] for state in states], height=.5)
    axes[1, 2].xaxis.set_major_locator(MaxNLocator(integer=True))
    axes[1, 2].set_xlabel("记录数")
    fig.text(.5, -.015, f"{data['snapshots']} 份配置快照；同一任务的重复快照不重复计数。"
             " 缺失配置不按零值处理，完整配置与工具版本另存 JSON。", ha="center", fontsize=8, color="#657180")
    fig.tight_layout(h_pad=3, w_pad=2.5)
    names = [_save(fig, directory, "runner-overview")]
    for page, offset in enumerate(range(0, len(rows), 18), 1):
        part = rows[offset:offset + 18]
        fig, axes = plt.subplots(1, 3, figsize=(13.5, max(3.4, len(part) * .32 + 1.6)),
                                 sharey=True, gridspec_kw={"width_ratios": [1, 1.2, 1.8]})
        y = np.arange(len(part))
        metrics = [("cpu", "logical_cpus", 1, "逻辑核心数", COLORS[0]),
                   (None, "memory_bytes", 2**30, "总内存 [GiB]", COLORS[1]),
                   ("disk", "free_bytes", 2**30, "磁盘 [GiB] · 采集时", COLORS[2])]
        for ax, (group, key, scale, title, color) in zip(axes, metrics):
            values = [_number((row["info"][group] if group else row["info"]).get(key)) / scale for row in part]
            if group == "disk":
                totals = [_number(row["info"]["disk"].get("total_bytes")) / scale for row in part]
                ax.barh(y, totals, height=.55, color="#E7ECF1", label="总容量")
            ax.barh(y, values, height=.55, color=[model_colors[_model(row)] for row in part] if group == "cpu" else color,
                    label="空闲" if group == "disk" else title)
            for i, value in enumerate(values):
                ax.text(value if math.isfinite(value) else 0, i,
                        f" {value:.1f}" if math.isfinite(value) and scale != 1 else
                        f" {value:g}" if math.isfinite(value) else " 未采集", va="center", fontsize=7)
            _axis(ax, title, "x")
            ax.margins(x=.2)
        axes[0].set_yticks(y, [row["label"] for row in part], fontsize=8)
        axes[0].invert_yaxis()
        axes[2].legend(loc="upper right", ncol=2, frameon=False, fontsize=8)
        fig.suptitle(f"各任务硬件容量 · 第 {page} 页", fontsize=14, fontweight="bold")
        from matplotlib.patches import Patch
        fig.legend([Patch(facecolor=color) for color in model_colors.values()], list(model_colors),
                   loc="lower center", ncol=2, frameon=False, fontsize=7)
        fig.tight_layout(w_pad=2, rect=(0, .14, 1, 1))
        names.append(_save(fig, directory, f"runner-capacity-{page:02d}"))
    if rows:
        tools = {}
        for row in rows:
            for key, tool in row["info"]["tools"].items():
                if not isinstance(tool, dict):
                    continue
                label = _label(f"{tool.get('name', key)} {tool.get('version') if tool.get('status') == 'ok' else '未读取'}")
                tools.setdefault(label, Counter())[row["info"]["job"]] += 1
        fig, (environment, versions) = plt.subplots(1, 2, figsize=(14, max(5, len(tools) * .3 + 1.8)),
                                                   gridspec_kw={"width_ratios": [1, 1.8]})
        systems = Counter(_label(f"{row['info']['system'].get('os', '未采集')} · {row['info']['system'].get('architecture', '未采集')}") for row in rows)
        kernels = Counter(_label(row["info"]["system"].get("kernel") or "未采集内核") for row in rows)
        libraries = Counter(_label(row["info"]["system"].get("libc") or "未采集 libc") for row in rows)
        builds = Counter(_label(f"native={row['info']['build'].get('MC_ENABLE_NATIVE_ARCH', '未配置')} · "
                                f"sanitizers={row['info']['build'].get('MC_ENABLE_SANITIZERS', '未配置')} · "
                                f"fuzz-ASan={row['info']['build'].get('MC_FUZZ_ASAN', '未配置')}")
                         for row in rows if row["info"]["build"])
        environments = {**{f"系统 · {key}": value for key, value in systems.items()},
                        **{f"内核 · {key}": value for key, value in kernels.items()},
                        **{f"运行库 · {key}": value for key, value in libraries.items()},
                        **{f"构建 · {key}": value for key, value in builds.items()}}
        environment.barh(list(environments), list(environments.values()), color=COLORS[1], height=.5)
        environment.set_xlabel("任务实例数")
        environment.xaxis.set_major_locator(MaxNLocator(integer=True))
        environment.tick_params(axis="y", labelsize=7)
        _axis(environment, "(a) 操作系统与架构", "x")
        labels = sorted(tools)
        y = np.arange(len(labels))
        left = np.zeros(len(labels))
        for index, job in enumerate(JOB_NAMES):
            counts = np.array([tools[label][job] for label in labels])
            if not counts.any():
                continue
            versions.barh(y, counts, left=left, color=JOB_COLORS[index], label=JOB_NAMES[job], height=.6)
            left += counts
        versions.set_yticks(y, labels, fontsize=7)
        versions.invert_yaxis()
        versions.set_xlabel("任务实例数 · 按任务类型分组")
        versions.xaxis.set_major_locator(MaxNLocator(integer=True))
        fig.legend(*versions.get_legend_handles_labels(), loc="lower center", ncol=5, frameon=False, fontsize=7)
        _axis(versions, "(b) 实际工具版本", "x")
        fig.suptitle("软件环境与工具链", fontsize=15, fontweight="bold")
        fig.tight_layout(w_pad=4, rect=(0, .1, 1, 1))
        names.append(_save(fig, directory, "runner-software"))
    return names
