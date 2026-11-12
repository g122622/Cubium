#!/usr/bin/env python3
"""校验 `scripts/diag/orphan_sources.ts` 的扫描结论，并判定本次扫描是否可采信。

设计要点：
- **永不因“发现游离源文件”而让 job 失败**。游离扫描是提示性信号，不是门禁：它一旦失败
  会让整条 nightly 变红，而项目已有大量历史遗留项，红灯会让信号失效。脚本只负责把结论
  整理成可读的摘要与判定，退出码留给“扫描本身无法完成”这一种情况。
- **区分“有游离”与“结论不可信”**。静态解析出现的告警意味着结论可能不完整（既可能虚高
  也可能虚低），此时不能当作“零游离”上报，必须显式标记为不可采信。
- **交叉核对 `compile_commands.json`**。扫描器三路交叉里，编译数据库是最终真值。若本次
  没有编译数据库，结论降级为 degraded，仍可上报，但要说明原因。

用法：
    python3 scripts/ci/verify_source_collection.py \\
        --report scan/source-collection.json \\
        --step-summary "$GITHUB_STEP_SUMMARY" \\
        --markdown-out scan/source-collection.md
"""

from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path


# 报告 schema 版本；扫描器与校验器必须同时升级，避免读到结构不相容的旧报告。
_SCHEMA_VERSION = 1

# 结论等级 -> (图标文案, 是否需要人工跟进)。
_VERDICTS = {
    "complete": ("✅ 完整", False),
    "degraded": ("⚠️ 降级", False),
    "unverifiable": ("❌ 不可采信", True),
}

# Step Summary 明细里最多列出的游离文件数，超出部分折叠为一行计数，避免刷屏。
_MAX_LISTED_ORPHANS = 100


class ReportError(ValueError):
    """报告结构不合法。"""


def load_report(path: Path) -> dict:
    """读取并校验扫描报告，字段缺失或类型不符一律视为报告损坏。"""
    try:
        data = json.loads(path.read_text(encoding="utf-8"))
    except OSError as error:
        raise ReportError(f"Cannot read scan report {path}: {error}") from error
    except json.JSONDecodeError as error:
        raise ReportError(f"Scan report {path} is not valid JSON: {error}") from error
    if not isinstance(data, dict):
        raise ReportError("Scan report root must be an object")
    if data.get("schema_version") != _SCHEMA_VERSION:
        raise ReportError(f"Unsupported scan report schema: {data.get('schema_version')!r}")
    if data.get("verdict") not in _VERDICTS:
        raise ReportError(f"Unknown verdict: {data.get('verdict')!r}")
    counts = data.get("counts")
    if not isinstance(counts, dict):
        raise ReportError("Scan report has no counts object")
    for key in ("compiled", "declared", "aggregated", "orphan"):
        if not isinstance(counts.get(key), int) or counts.get(key) < 0:
            raise ReportError(f"Invalid count for {key!r}")
    orphans = data.get("orphans")
    if not isinstance(orphans, list) or any(not isinstance(item, str) for item in orphans):
        raise ReportError("Scan report has no orphan list")
    if len(orphans) != counts["orphan"]:
        # 计数与清单不一致说明报告被写坏，宁可报错也不要展示自相矛盾的数字。
        raise ReportError(f"Orphan count {counts['orphan']} does not match listed orphans {len(orphans)}")
    warnings = data.get("warnings")
    if not isinstance(warnings, list) or any(not isinstance(item, str) for item in warnings):
        raise ReportError("Scan report has no warnings list")
    return data


def needs_attention(report: dict) -> bool:
    """本次扫描是否需要人工跟进：存在游离文件，或结论不可采信。"""
    return report["counts"]["orphan"] > 0 or _VERDICTS[report["verdict"]][1]


def render_markdown(report: dict) -> str:
    """生成写入 Step Summary 与汇总报告的 Markdown 片段。"""
    counts = report["counts"]
    orphans = report["orphans"]
    icon, _ = _VERDICTS[report["verdict"]]
    headline = "❌ 存在游离源文件" if orphans else "✅ 无游离源文件"

    lines = [
        "### 游离源文件扫描",
        "",
        f"**{headline}**",
        "",
        "| 项目 | 数值 |",
        "|---|---|",
        f"| 判定结论 | {icon} |",
        f"| 源码总数 | {counts['compiled'] + counts['declared'] + counts['aggregated'] + counts['orphan']} |",
        f"| 编译器实际编译 | {counts['compiled']} |",
        f"| CMake 声明 | {counts['declared']} |",
        f"| 聚合单元成员 | {counts['aggregated']} |",
        f"| **游离** | **{counts['orphan']}** |",
        "",
    ]

    if report["verdict"] == "unverifiable":
        lines.extend([
            "> 静态解析出现告警，结论可能不完整：游离数量既可能虚高（误报），也可能虚低（漏报）。"
            "请先修 `scripts/diag/orphan_sources.ts`，再采信本结论。",
            "",
        ])
    elif report["verdict"] == "degraded":
        lines.extend([
            "> 本次没有可用的 `compile_commands.json`，结论仅来自静态解析；`file(GLOB)` 之外的"
            "收集手段可能漏判。",
            "",
        ])

    if orphans:
        lines.append("> 游离源文件不会被任何 CMake 目标编译，其中的代码与测试实际从未生效。")
        lines.append("")
        shown = orphans[:_MAX_LISTED_ORPHANS]
        lines.append(f"<details><summary>游离文件清单（{len(orphans)} 个）</summary>")
        lines.append("")
        for path in shown:
            lines.append(f"- `{path}`")
        if len(orphans) > len(shown):
            lines.append(f"- …… 另有 {len(orphans) - len(shown)} 个未列出，完整清单见 artifact")
        lines.append("")
        lines.append("</details>")
        lines.append("")

    if report["warnings"]:
        lines.append(f"<details><summary>解析告警（{len(report['warnings'])} 条）</summary>")
        lines.append("")
        for warning in report["warnings"]:
            lines.append(f"- `{warning}`")
        lines.append("")
        lines.append("</details>")
        lines.append("")

    lines.append("本地复现：`node --experimental-strip-types scripts/diag/orphan_sources.ts --all`")
    lines.append("")
    return "\n".join(lines)


def main(argv: list[str]) -> int:
    parser = argparse.ArgumentParser(description="校验游离源文件扫描结论并生成摘要")
    parser.add_argument("--report", required=True, type=Path, help="orphan_sources.ts 的 --report-json 产物")
    parser.add_argument("--markdown-out", type=Path, help="写出 Markdown 摘要，供汇总报告读取")
    parser.add_argument("--step-summary", type=Path, help="追加写入 GitHub Step Summary")
    parser.add_argument(
        "--fail-on-report-error",
        action="store_true",
        help="报告损坏时以非零码退出；默认只告警，避免扫描环节自身故障阻塞后续步骤",
    )
    args = parser.parse_args(argv)

    try:
        report = load_report(args.report)
    except ReportError as error:
        print(f"::error::Source collection scan report is unusable: {error}")
        if args.fail_on_report_error:
            return 1
        # 报告读不出来时不产出摘要，汇总报告会按“未采集”处理，不虚构结论。
        return 0

    markdown = render_markdown(report)
    if args.markdown_out:
        args.markdown_out.parent.mkdir(parents=True, exist_ok=True)
        args.markdown_out.write_text(markdown, encoding="utf-8")
    if args.step_summary:
        with args.step_summary.open("a", encoding="utf-8") as summary:
            summary.write(markdown)
            summary.write("\n")
    else:
        print(markdown)

    orphans = report["counts"]["orphan"]
    print(f"[source-collection] verdict={report['verdict']} orphans={orphans}")
    if needs_attention(report):
        # 用 warning 而非 error：坚持“发现游离不终止流水线”的口径。
        print(f"::warning::Source collection needs attention: {orphans} orphan file(s), verdict={report['verdict']}")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))