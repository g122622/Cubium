#!/usr/bin/env python3
"""汇总 nightly 各测试 job 的结果，生成一个 Markdown 报告，用作 GitHub issue 正文。

设计要点：
- **永不抛异常**：任何解析失败都降级为「该项未能解析」，因为本脚本运行在 `if: always()`
  的 report job 里，它自身的失败会让 nightly 连 issue 都建不出来，是最坏的结果。
- **输出限长**：单元测试有数万用例，失败清单可能极长。所有清单都放进 `<details>` 折叠块，
  并对条数设上限（超出部分注明「另有 N 条未列出」），避免 issue 正文超出 GitHub 的
  65536 字符上限而被拒绝创建。
- 只读 artifacts 目录，不依赖任何第三方库。

用法：
    python3 scripts/ci/summarize_failures.py \
        --artifacts-dir artifacts \
        --job-status '{"build":"success","unit-tests":"failure"}' \
        --run-url https://github.com/... \
        --branch main --commit abc123 \
        --out issue-body.md
"""

from __future__ import annotations

import argparse
import json
import re
import sys
import xml.etree.ElementTree as ET
from pathlib import Path

# GitHub issue 正文上限是 65536 字符；留出安全余量。
_MAX_BODY_CHARS = 60_000
# 单个清单最多列出的条目数。
_MAX_ITEMS_PER_SECTION = 200

# ctest 的失败清单行形如：  "	  1 - TestSuite.Case (Failed)"
_CTEST_RESULT_LINE = re.compile(r"^\s*\d+\s*-\s*(.+?)\s*\(([^)]+)\)\s*$")
# e2e runner 的失败行格式：  "  [cubium] containers/barrel ... ✗ 快照不一致 (1234ms)"
# （见 tests/e2e/bot/src/runner.ts：先 write 前缀，再按结果补 ✓/✗。）
_E2E_FAIL_LINE = re.compile(r"^\s*\[(\w+)\]\s+(\S+)\s+\.\.\.\s+✗\s*(.*)$")
_E2E_SKIP_LINE = re.compile(r"^\s*\[跳过\]\s+(\S+)\s*$")
# e2e 的汇总行：  "合计：48 条，通过 45，失败 3，跳过 0（见上方原因）"
_E2E_SUMMARY = re.compile(r"^\s*合计：.*失败\s*(\d+)")

# 集成测试的崩溃/流水线错误标记。
#
# 【为什么必须单独检测】`run-gametests.ts` 在服务端崩溃时以退出码 2（流水线错误）结束，
# 且**不产出 JUnit XML**（崩溃发生在写报告之前）。而 integrated-tests job 按既定口径
# 用 `exit 0` 吞掉了退出码，于是：job 恒为 success、无 XML 可解析 —— 整条失败在汇总
# 报告里完全不可见。必须直接从日志里认这三类标记。
_INTEGRATED_CRASH_MARKERS = (
    "FATAL CRASH DETECTED",
    "Segmentation fault",
    "SIGSEGV",
    "SIGILL",
    "SIGABRT",
    "Stack trace:",
)
# `run-gametests exit code: 2` / `Round 1 exit code: -1`（崩溃导致非零退出）
_INTEGRATED_EXIT_LINE = re.compile(r"(?:run-gametests|Round \d+) exit code:\s*(-?\d+)")
# GitHub Actions 下载的日志每行带 `2026-10-09T05:03:28.4838952Z ` 前缀；
# 若日志来自网页复制则还带 `Build (Linux...)	UNKNOWN STEP	` 之类前缀。
_GH_LOG_PREFIX = re.compile(r"^.*?\d{4}-\d{2}-\d{2}T[\d:.]+Z\s?")

_JOB_LABELS = {
    "build": "构建（linux-relwithdebinfo）",
    "unit-tests": "单元测试（ctest）",
    "integrated-tests": "集成测试（GameTest）",
    "e2e-tests": "端到端 bot 测试",
    "fuzz": "模糊测试（libFuzzer）",
    "benchmark": "性能基准（仅记录 + 告警）",
    "report": "结果汇总",
}

_STATUS_ICON = {
    "success": "✅ 完成",
    "failure": "❌ 失败",
    "cancelled": "⚪ 取消",
    "skipped": "⏭️ 跳过",
    "neutral": "⚪ 中性",
}


def _dedupe(items: list[str]) -> list[str]:
    """去重并保持首次出现的顺序。"""
    seen: set[str] = set()
    ordered: list[str] = []
    for item in items:
        if item not in seen:
            seen.add(item)
            ordered.append(item)
    return ordered


def _read_text(path: Path) -> str:
    try:
        # 输入不得截断：失败清单和退出码通常在长日志末尾；仅限制最终 issue 正文。
        return path.read_text(encoding="utf-8", errors="replace")
    except OSError:
        return ""


def _truncate_items(items: list[str]) -> list[str]:
    if len(items) <= _MAX_ITEMS_PER_SECTION:
        return items
    shown = items[:_MAX_ITEMS_PER_SECTION]
    shown.append(f"…另有 {len(items) - _MAX_ITEMS_PER_SECTION} 条未列出（完整清单见 artifact）")
    return shown


def _details_block(title: str, items: list[str], limit_output: bool) -> str:
    if not items:
        return ""
    lines = [f"<details><summary>{title}（{len(items)}）</summary>", ""]
    shown = _truncate_items(items) if limit_output else items
    lines.extend(f"- `{item}`" if not item.startswith("…") else f"- {item}" for item in shown)
    lines.extend(["", "</details>", ""])
    return "\n".join(lines)


def _read_junit_results(xml_paths: list[Path], include_classname: bool) -> dict[str, list[str]]:
    """流式解析完整 JUnit，分别保留通过、失败、跳过及报告读取错误。"""
    results: dict[str, list[str]] = {key: [] for key in ("passed", "failures", "skipped", "errors")}
    for xml in xml_paths:
        try:
            for _, case in ET.iterparse(xml, events=("end",)):
                if case.tag != "testcase":
                    continue
                name = case.get("name", "<unnamed testcase>")
                classname = case.get("classname", "")
                if include_classname and classname:
                    name = f"{classname}:{name}"
                outcome = case.find("failure")
                if outcome is None:
                    outcome = case.find("error")
                key = "failures"
                if outcome is None:
                    outcome = case.find("skipped")
                    key = "skipped" if outcome is not None else "passed"
                if outcome is not None:
                    reason = outcome.get("message", "") or (outcome.text or "").strip()
                    if key == "skipped" and reason == "SKIP_REGULAR_EXPRESSION_MATCHED":
                        # CTest 的通用标记不说明原因，补充 GTest 在输出中给出的跳过说明。
                        skip_reason = re.search(r": Skipped\s*\n([^\n]+)", case.findtext("system-out", ""))
                        if skip_reason:
                            reason = skip_reason.group(1).strip()
                    if reason:
                        name += f" — {' '.join(reason.split())}"
                results[key].append(name)
                # 释放每个用例的大段 system-out，避免整份数十 MB 报告常驻内存。
                case.clear()
        except (OSError, ET.ParseError) as exc:
            results["errors"].append(f"Cannot parse JUnit report {xml.name}: {exc}")
    # CTest 可能有同名注册项；统计须按 testcase 记录计数，不能按名称去重。
    return results


def collect_ctest_results(artifacts_dir: Path) -> dict[str, list[str]]:
    """读取单元测试全部结果；报告缺失或损坏时保留错误并从完整日志补充清单。"""
    xml_paths = sorted(artifacts_dir.rglob("ctest-results.xml"))
    results = _read_junit_results(xml_paths, False)
    if not xml_paths:
        results["errors"].append("CTest JUnit report is missing; unit test results cannot be verified")
    elif not any(results[key] for key in ("passed", "failures", "skipped")):
        results["errors"].append("CTest JUnit report contains no testcases")
    if results["errors"]:
        for log in artifacts_dir.rglob("*ctest*.log"):
            for line in _read_text(log).splitlines():
                match = _CTEST_RESULT_LINE.match(line)
                if match:
                    name, outcome = match.groups()
                    key = "skipped" if outcome == "Skipped" else "failures"
                    results[key].append(f"{name} — {outcome}")
        for key in ("failures", "skipped"):
            results[key] = _dedupe(results[key])
    return results


def collect_ctest_failures(artifacts_dir: Path) -> list[str]:
    """提取单元测试失败及报告错误，供 issue 触发判定使用。"""
    results = collect_ctest_results(artifacts_dir)
    return results["failures"] + results["errors"]


def collect_junit_results(artifacts_dir: Path) -> dict[str, list[str]]:
    """提取各轮集成测试的失败和跳过；非必需用例的 skipped 也必须公开列出。"""
    xml_paths = sorted(xml for xml in artifacts_dir.rglob("*.xml") if xml.name != "ctest-results.xml")
    results = _read_junit_results(xml_paths, True)
    return {key: _dedupe(items) for key, items in results.items()}


def collect_junit_failures(artifacts_dir: Path) -> list[str]:
    """提取集成测试失败及报告错误，跳过不计为失败。"""
    results = collect_junit_results(artifacts_dir)
    return results["failures"] + results["errors"]


def collect_e2e_failures(artifacts_dir: Path) -> list[str]:
    """从 e2e runner 的 stdout 日志中提取失败用例。

    日志行形如：`  [cubium] containers/barrel ... ✗ 快照不一致 (1234ms)`。
    """
    failures: list[str] = []
    for log in artifacts_dir.rglob("*e2e*.log"):
        for line in _read_text(log).splitlines():
            match = _E2E_FAIL_LINE.match(line)
            if match:
                server_kind, case_id, reason = match.group(1), match.group(2), match.group(3).strip()
                failures.append(f"[{server_kind}] {case_id} — {reason}")
    return _dedupe(failures)


def collect_e2e_skipped(artifacts_dir: Path) -> list[str]:
    """提取 e2e 主动跳过的用例及下一行给出的原因。"""
    skipped: list[str] = []
    for log in artifacts_dir.rglob("*e2e*.log"):
        lines = _read_text(log).splitlines()
        for index, line in enumerate(lines):
            match = _E2E_SKIP_LINE.match(line)
            if match:
                reason = lines[index + 1].strip() if index + 1 < len(lines) else ""
                skipped.append(f"{match.group(1)} — {reason}" if reason else match.group(1))
    return _dedupe(skipped)


def collect_integrated_failures(artifacts_dir: Path) -> list[str]:
    """从集成测试日志中提取**崩溃/流水线错误**（JUnit XML 覆盖不到的那一类失败）。

    背景：`run-gametests.ts` 在服务端崩溃时以退出码 2 结束，且**不产出 JUnit XML**
    （崩溃发生在写报告之前）。integrated-tests job 按既定口径 `exit 0` 吞掉了退出码，
    于是 job 恒为 success、无 XML 可解析 —— 整条失败在汇总报告里完全不可见。
    这里直接从日志里认崩溃标记与非零退出码。
    """
    findings: list[str] = []
    for log in artifacts_dir.rglob("*integrated*.log"):
        text = _read_text(log)
        for marker in _INTEGRATED_CRASH_MARKERS:
            if marker in text:
                findings.append(f"日志出现崩溃标记：{marker}")
        for match in _INTEGRATED_EXIT_LINE.finditer(text):
            code = int(match.group(1))
            if code != 0:
                findings.append(f"run-gametests 以非零退出码结束：{code}"
                                "（2 = 流水线错误，通常是服务端崩溃导致 JUnit XML 未产出）")
    return _dedupe(findings)


# 崩溃块在日志中的起点标记。其上方一行是 `====` 分隔线，下方是 Reason / Stack trace。
_CRASH_TITLE = "FATAL CRASH DETECTED"
# 崩溃块结束标记（栈之后紧跟的内容）。
_CRASH_TAIL_MARKERS = ("Perfetto tracing stopped", "run-gametests exit code:", "exit code:")
# 崩溃前保留的上下文行数（用户明确要求包含崩溃前 20 条日志）。
_CRASH_CONTEXT_LINES = 20
# 单个崩溃报告最多保留的行数（防止异常日志把 issue 正文撑爆）。
_CRASH_MAX_LINES = 160
# 最多提取几份崩溃报告（一次运行通常只崩一次；重跑轮次可能多份）。
_CRASH_MAX_REPORTS = 3


def extract_crash_reports(artifacts_dir: Path) -> list[str]:
    """提取完整的崩溃报告：崩溃前若干行上下文 + Reason + 完整调用栈 + 收尾行。

    为什么要连上下文一起抓：崩溃日志里真正定位问题的信息有两部分——**调用栈**
    （哪个函数炸的）与**崩溃前的日志**（当时在跑哪个测试、传送到哪个维度）。
    只给「日志出现崩溃标记：SIGSEGV」等于什么都没说，排查者仍要去下载 artifact
    才能看到栈；而 CI 的 artifact 有 30 天保留期且需要登录，把关键信息直接放进
    issue 正文才能让人一眼看到。

    返回若干个可直接贴进 Markdown 代码块的文本块。
    """
    reports: list[str] = []
    for log in sorted(artifacts_dir.rglob("*integrated*.log")):
        lines = _read_text(log).splitlines()
        index = 0
        while index < len(lines) and len(reports) < _CRASH_MAX_REPORTS:
            if _CRASH_TITLE not in lines[index]:
                index += 1
                continue

            # 标题上方一行是分隔线，上下文再往上数 _CRASH_CONTEXT_LINES 行。
            start = max(0, index - 1 - _CRASH_CONTEXT_LINES)

            # 从标题往后找收尾标记（Perfetto / 退出码），再多带 2 行。
            end = index
            cursor = index
            while cursor < len(lines) and cursor - index < _CRASH_MAX_LINES:
                if any(marker in lines[cursor] for marker in _CRASH_TAIL_MARKERS):
                    end = cursor + 2
                    break
                cursor += 1
            else:
                end = min(len(lines), index + _CRASH_MAX_LINES)

            block = lines[start:min(end, len(lines))]
            # 去掉 GitHub Actions 日志行首的时间戳前缀（下载的原始日志没有，
            # 但手工从网页复制的会有，去掉更整洁）。
            block = [_GH_LOG_PREFIX.sub("", line) for line in block]
            reports.append("\n".join(block).strip())
            index = end
    return reports


def collect_fuzz_artifacts(artifacts_dir: Path) -> list[str]:
    """列出 fuzz 产出的崩溃/OOM/超时用例（这些是可复现的最小输入）。"""
    found: list[str] = []
    for pattern in ("crash-*", "oom-*", "timeout-*", "leak-*", "fuzz-crash-*"):
        for artifact in artifacts_dir.rglob(pattern):
            if artifact.is_file():
                found.append(f"{artifact.name}（{artifact.stat().st_size} 字节）")
    return sorted(set(found))

def read_benchmark_report(artifacts_dir: Path) -> str:
    for report in artifacts_dir.rglob("benchmark-compare.md"):
        return _read_text(report)[:20_000]
    return ""


def has_failures(job_status: dict[str, str], artifacts_dir: Path) -> bool:
    """判定本次 nightly 是否存在失败项。

    两类来源：
      1. job 层面的失败（build / e2e / fuzz / benchmark 任一非 success）；
      2. 非阻塞 job（unit-tests / integrated-tests）产物中的失败用例——这两个 job 内部
         吞掉了失败码、恒为 success，必须从产物里查。
    """
    if any(status == "failure" for status in job_status.values()):
        return True
    if collect_ctest_failures(artifacts_dir):
        return True
    if collect_junit_failures(artifacts_dir):
        return True
    if collect_integrated_failures(artifacts_dir):
        return True
    if collect_e2e_failures(artifacts_dir):
        return True
    if collect_fuzz_artifacts(artifacts_dir):
        return True
    return False


def build_body(
    job_status: dict[str, str],
    artifacts_dir: Path,
    run_url: str,
    branch: str,
    commit: str,
    limit_output: bool,
) -> str:
    ctest_results = collect_ctest_results(artifacts_dir)
    ctest_failures = ctest_results["failures"]
    junit_results = collect_junit_results(artifacts_dir)
    junit_failures = junit_results["failures"]
    integrated_failures = collect_integrated_failures(artifacts_dir)
    crash_reports = extract_crash_reports(artifacts_dir)
    e2e_failures = collect_e2e_failures(artifacts_dir)
    e2e_skipped = collect_e2e_skipped(artifacts_dir)
    fuzz_artifacts = collect_fuzz_artifacts(artifacts_dir)
    benchmark_report = read_benchmark_report(artifacts_dir)

    lines: list[str] = []
    lines.append("## Nightly CI 结果")
    lines.append("")
    lines.append(f"- **运行**：[{run_url.split('/')[-1]}]({run_url})")
    lines.append(f"- **分支**：`{branch}`")
    lines.append(f"- **提交**：`{commit}`")
    lines.append("")

    lines.append("### 各 job 结论")
    lines.append("")
    lines.append("> 执行结论表示 job 是否完成；非阻塞测试的 job 成功不代表所有用例通过。")
    lines.append("")
    lines.append("| Job | 执行结论 | 测试结果 |")
    lines.append("|---|---|---|")
    for job_id, label in _JOB_LABELS.items():
        status = job_status.get(job_id, "unknown")
        outcome = "—"
        if job_id == "unit-tests":
            outcome = (f"通过 {len(ctest_results['passed'])}；失败 {len(ctest_failures)}；"
                       f"跳过 {len(ctest_results['skipped'])}")
            if ctest_results["errors"]:
                outcome += "；报告不完整"
        elif job_id == "integrated-tests":
            outcome = f"各轮失败记录 {len(junit_failures)}；跳过记录 {len(junit_results['skipped'])}"
            if not any(junit_results[key] for key in ("passed", "failures", "skipped")):
                outcome = "未提供 JUnit 用例记录"
            if integrated_failures or junit_results["errors"]:
                outcome += "；崩溃或报告错误"
        elif job_id == "e2e-tests":
            outcome = f"失败 {len(e2e_failures)}；跳过 {len(e2e_skipped)}"
        lines.append(f"| {label} | {_STATUS_ICON.get(status, status)} | {outcome} |")
    lines.append("")

    if ctest_failures:
        lines.append("### 单元测试失败用例")
        lines.append("")
        lines.append("> 单元测试按既定口径**不阻塞 CI**（项目存在大量历史遗留失败）。"
                     "下列清单供排查参考。")
        lines.append("")
        lines.append(_details_block("失败用例", ctest_failures, limit_output))

    if ctest_results["skipped"]:
        lines.extend(["### 单元测试跳过用例", "",
                      _details_block("跳过用例", ctest_results["skipped"], limit_output)])

    report_errors = ctest_results["errors"] + junit_results["errors"]
    if report_errors:
        lines.extend(["### 测试报告读取错误", "",
                      _details_block("报告错误", report_errors, limit_output)])

    if junit_failures:
        lines.append("### 集成测试失败用例")
        lines.append("")
        lines.append(_details_block("失败 testcase（含各轮）", junit_failures, limit_output))

    if junit_results["skipped"]:
        lines.extend(["### 集成测试跳过用例", "",
                      _details_block("跳过 testcase（含各轮）", junit_results["skipped"], limit_output)])

    if integrated_failures:
        lines.append("### 集成测试崩溃 / 流水线错误")
        lines.append("")
        lines.append("> 这类失败**没有 JUnit XML**（服务端在写出报告前就崩了），"
                     "因此不会出现在上面的失败用例清单里。下方直接给出完整调用栈"
                     "与崩溃前若干行日志；原始日志见 artifact `integrated-test-results`。")
        lines.append("")
        lines.append(_details_block("崩溃标记", integrated_failures, limit_output))
        for i, report in enumerate(crash_reports, start=1):
            title = "崩溃调用栈" if len(crash_reports) == 1 else f"崩溃调用栈 #{i}"
            lines.append(f"<details><summary>{title}</summary>")
            lines.append("")
            lines.append("```")
            lines.append(report)
            lines.append("```")
            lines.append("")
            lines.append("</details>")
            lines.append("")

    if e2e_failures:
        lines.append("### 端到端 bot 测试失败")
        lines.append("")
        lines.append(_details_block("失败行", e2e_failures, limit_output))

    if e2e_skipped:
        lines.extend(["### 端到端 bot 测试跳过用例", "",
                      _details_block("跳过用例", e2e_skipped, limit_output)])

    if fuzz_artifacts:
        lines.append("### 模糊测试发现的问题")
        lines.append("")
        lines.append("> 下列产物是 libFuzzer 落盘的**可复现最小输入**，"
                     "可直接用 `-runs=1 <file>` 复现。")
        lines.append("")
        lines.append(_details_block("崩溃 / 超限产物", fuzz_artifacts, limit_output))

    if benchmark_report:
        lines.append("### 性能基准")
        lines.append("")
        lines.append(benchmark_report)
        lines.append("")

    lines.append("### 排查入口")
    lines.append("")
    lines.append(f"- 完整日志与结果见本次运行的 artifacts：{run_url}")
    lines.append("- 全部失败和跳过清单见 artifact `nightly-report` 中的 `nightly-report-full.md`。")
    lines.append("- 数据包由 `.github/actions/setup-datapack` 从 misode/mcmeta 安装到 "
                 "`~/minecraft_reborn/datapacks/Vanilla`。")
    lines.append("- 本地复现：先 `./scripts/configure.sh build`，再按 `docs/CI.md` 的对应命令执行。")
    lines.append("")

    body = "\n".join(lines)
    if limit_output and len(body) > _MAX_BODY_CHARS:
        body = body[:_MAX_BODY_CHARS] + "\n\n…（正文超长已截断，完整内容见 artifacts）\n"
    return body


def main(argv: list[str]) -> int:
    parser = argparse.ArgumentParser(description="汇总 nightly 结果生成 issue 正文")
    parser.add_argument("--artifacts-dir", required=True, type=Path)
    parser.add_argument("--job-status", default="{}", help="各 job 结论的 JSON")
    parser.add_argument("--run-url", default="")
    parser.add_argument("--branch", default="")
    parser.add_argument("--commit", default="")
    parser.add_argument("--out", type=Path, default=None)
    parser.add_argument("--full-out", type=Path, help="写出不截断失败和跳过清单的完整报告")
    parser.add_argument(
        "--status-out",
        type=Path,
        default=None,
        help="写出判定结果（failures / clean），供 workflow 决定是否建 issue",
    )
    args = parser.parse_args(argv)

    try:
        job_status = json.loads(args.job_status)
    except json.JSONDecodeError:
        job_status = {}

    body = build_body(job_status, args.artifacts_dir, args.run_url, args.branch, args.commit, True)

    if args.full_out:
        args.full_out.parent.mkdir(parents=True, exist_ok=True)
        full_body = build_body(job_status, args.artifacts_dir, args.run_url, args.branch, args.commit, False)
        args.full_out.write_text(full_body, encoding="utf-8")

    if args.out:
        args.out.parent.mkdir(parents=True, exist_ok=True)
        args.out.write_text(body, encoding="utf-8")
    else:
        print(body)

    # 判定是否有失败项。异常一律降级为 failures（宁可多建一个 issue，也不要漏报失败）。
    try:
        status = "failures" if has_failures(job_status, args.artifacts_dir) else "clean"
    except Exception:  # noqa: BLE001 - 汇总脚本绝不能因自身异常而漏报
        status = "failures"
    if args.status_out:
        args.status_out.write_text(status, encoding="utf-8")
    else:
        print(f"\n[status] {status}")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
