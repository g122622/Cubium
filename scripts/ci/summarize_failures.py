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
from pathlib import Path

# GitHub issue 正文上限是 65536 字符；留出安全余量。
_MAX_BODY_CHARS = 60_000
# 单个清单最多列出的条目数。
_MAX_ITEMS_PER_SECTION = 200

# ctest 的失败清单行形如：  "	  1 - TestSuite.Case (Failed)"
_CTEST_FAILED_LINE = re.compile(r"^\s*\d+\s*-\s*(.+?)\s*\((Failed|Timeout|Subprocess aborted|Exception)\)\s*$")
# JUnit XML 的 testcase 起始标签。
_JUNIT_TESTCASE = re.compile(r"<testcase\b([^>]*?)(/?)>", re.DOTALL)
_JUNIT_ATTR = re.compile(r'(\w+)="([^"]*)"')
# e2e runner 的失败行格式：  "  [cubium] containers/barrel ... ✗ 快照不一致 (1234ms)"
# （见 tests/e2e/bot/src/runner.ts：先 write 前缀，再按结果补 ✓/✗。）
_E2E_FAIL_LINE = re.compile(r"^\s*\[(\w+)\]\s+(\S+)\s+\.\.\.\s+✗\s*(.*)$")
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
    "success": "✅ 通过",
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


def _read_text(path: Path, limit: int = 4_000_000) -> str:
    try:
        return path.read_text(encoding="utf-8", errors="replace")[:limit]
    except OSError:
        return ""


def _truncate_items(items: list[str]) -> list[str]:
    if len(items) <= _MAX_ITEMS_PER_SECTION:
        return items
    shown = items[:_MAX_ITEMS_PER_SECTION]
    shown.append(f"…另有 {len(items) - _MAX_ITEMS_PER_SECTION} 条未列出（完整清单见 artifact）")
    return shown


def _details_block(title: str, items: list[str]) -> str:
    if not items:
        return ""
    lines = [f"<details><summary>{title}（{len(items)}）</summary>", ""]
    lines.extend(f"- `{item}`" if not item.startswith("…") else f"- {item}" for item in _truncate_items(items))
    lines.extend(["", "</details>", ""])
    return "\n".join(lines)


def collect_ctest_failures(artifacts_dir: Path) -> list[str]:
    """从 ctest 输出日志中提取失败用例名。

    优先用 `ctest --output-junit` 产出的 XML（结构化、无歧义）；没有时退回解析
    ctest 的文本输出（形如 `	  1 - TestSuite.Case (Failed)`）。
    """
    from_xml = _ctest_junit_failures(artifacts_dir)
    if from_xml:
        return from_xml

    failures: list[str] = []
    for log in artifacts_dir.rglob("*ctest*.log"):
        for line in _read_text(log).splitlines():
            match = _CTEST_FAILED_LINE.match(line)
            if match:
                failures.append(match.group(1))
    return _dedupe(failures)


def _ctest_junit_failures(artifacts_dir: Path) -> list[str]:
    """从 ctest 的 JUnit XML 中提取失败用例（`<failure>` 子元素）。"""
    failures: list[str] = []
    for xml in artifacts_dir.rglob("ctest-results.xml"):
        text = _read_text(xml)
        for block in re.split(r"(?=<testcase\b)", text):
            match = _JUNIT_TESTCASE.match(block)
            if not match:
                continue
            attrs = dict(_JUNIT_ATTR.findall(match.group(1)))
            name = attrs.get("name")
            if name and "<failure" in block:
                failures.append(name)
    return _dedupe(failures)


def collect_junit_failures(artifacts_dir: Path) -> list[str]:
    """从集成测试的 JUnit XML 中提取失败的 testcase。

    注意：**不含** ctest 的 `ctest-results.xml`——那份由 `collect_ctest_failures()` 处理，
    这里跳过以免同一个失败在报告中重复出现两次。

    `JUnitTestReporter` 输出的失败标记是 `<failure>` 子元素；`required=false` 的失败写的是
    `<skipped>`（「非必需用例」），不算失败，不计入。
    """
    failures: list[str] = []
    for xml in artifacts_dir.rglob("*.xml"):
        if xml.name == "ctest-results.xml":
            continue
        text = _read_text(xml)
        if "<testcase" not in text:
            continue
        for block in re.split(r"(?=<testcase\b)", text):
            match = _JUNIT_TESTCASE.match(block)
            if not match:
                continue
            attrs = dict(_JUNIT_ATTR.findall(match.group(1)))
            name = attrs.get("name")
            if not name or "<failure" not in block:
                continue
            classname = attrs.get("classname", "")
            # GameTest 的 classname 存的是 structure 名，一并带上便于定位。
            failures.append(f"{classname}:{name}" if classname else name)
    return _dedupe(failures)


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
        return _read_text(report, limit=20_000)
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
) -> str:
    ctest_failures = collect_ctest_failures(artifacts_dir)
    junit_failures = collect_junit_failures(artifacts_dir)
    integrated_failures = collect_integrated_failures(artifacts_dir)
    e2e_failures = collect_e2e_failures(artifacts_dir)
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
    lines.append("| Job | 结论 |")
    lines.append("|---|---|")
    for job_id, label in _JOB_LABELS.items():
        status = job_status.get(job_id, "unknown")
        lines.append(f"| {label} | {_STATUS_ICON.get(status, status)} |")
    lines.append("")

    if ctest_failures:
        lines.append("### 单元测试失败用例")
        lines.append("")
        lines.append("> 单元测试按既定口径**不阻塞 CI**（项目存在大量历史遗留失败）。"
                     "下列清单供排查参考。")
        lines.append("")
        lines.append(_details_block("失败用例", ctest_failures))

    if junit_failures:
        lines.append("### 集成测试失败用例")
        lines.append("")
        lines.append(_details_block("失败 testcase", junit_failures))

    if integrated_failures:
        lines.append("### 集成测试崩溃 / 流水线错误")
        lines.append("")
        lines.append("> 这类失败**没有 JUnit XML**（服务端在写出报告前就崩了），"
                     "因此不会出现在上面的失败用例清单里。完整调用栈见 "
                     "artifact `integrated-test-results` 中的 `integrated-tests.log`。")
        lines.append("")
        lines.append(_details_block("崩溃标记", integrated_failures))

    if e2e_failures:
        lines.append("### 端到端 bot 测试失败")
        lines.append("")
        lines.append(_details_block("失败行", e2e_failures))

    if fuzz_artifacts:
        lines.append("### 模糊测试发现的问题")
        lines.append("")
        lines.append("> 下列产物是 libFuzzer 落盘的**可复现最小输入**，"
                     "可直接用 `-runs=1 <file>` 复现。")
        lines.append("")
        lines.append(_details_block("崩溃 / 超限产物", fuzz_artifacts))

    if benchmark_report:
        lines.append("### 性能基准")
        lines.append("")
        lines.append(benchmark_report)
        lines.append("")

    lines.append("### 排查入口")
    lines.append("")
    lines.append(f"- 完整日志与结果见本次运行的 artifacts：{run_url}")
    lines.append("- 数据包由 `.github/actions/setup-datapack` 从 misode/mcmeta 安装到 "
                 "`~/minecraft_reborn/datapacks/Vanilla`。")
    lines.append("- 本地复现：先 `./scripts/configure.sh build`，再按 `docs/CI.md` 的对应命令执行。")
    lines.append("")

    body = "\n".join(lines)
    if len(body) > _MAX_BODY_CHARS:
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

    body = build_body(job_status, args.artifacts_dir, args.run_url, args.branch, args.commit)

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
