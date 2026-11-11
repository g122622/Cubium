#!/usr/bin/env python3
"""按批次执行单元测试分片，替代逐用例起进程的 ctest 调用。

## 为什么

GoogleTest 用例由 `gtest_discover_tests` 逐条注册为 CTest 用例，ctest 因此**为每个用例起一个
独立进程**。而 `mc_tests` 每个进程都要重建全部原版方块、标签表与世界生成注册表：

| 构建 | 单进程初始化耗时 |
|---|---|
| Release（无 sanitizer） | 0.76 s |
| RelWithDebInfo + ASan/UBSan（单测构建） | 12.6 s |

CI runner 上单个测试进程的端到端开销（ELF 装载 + sanitizer 运行库 + 上述初始化 + 退出）
还要更大：按分片进程数反推约 38 秒。

于是单分片 1390 个用例要付出约 1390 × 13 s ÷ 并发 的纯初始化开销——CI 上单分片实测 5900~10600 秒，
而用例体本身通常只有几毫秒。判定逻辑没有问题，纯粹是「初始化次数 × 进程数」。

本脚本把同一分片内的用例按 `--batch-size` 合并到少量进程里执行，初始化次数降为批次数；
某个批次崩溃或超时（没有产出完整 XML）时，从完整 gtest 输出恢复已结束的用例，
只将中断用例单跑复核，未开始的用例继续成批执行；没有进度时才二分或展开为单例。

同进程连跑也带来了逐用例进程没有的「批内污染」：前一个用例对注册表等全局单例的改动会
泄漏给后一个用例。因此批次内失败的用例会再单跑复核一次——单跑通过说明是该批次的污染
（日志中以 `[BatchPollution]` 标出，留待后续修复），单跑仍失败才是真实回归。

另有一处登记名差异：`gtest_discover_tests` 给参数化用例的登记名是 `GetParam()` 的可读
描述，真正能传给 `--gtest_filter` 的是 `Suite.Test/<序号>`，脚本按 `--gtest_list_tests`
的注释建立对应关系后，对外一律用 ctest 的登记名。

## 产出（与既有报告链路保持一致）

- `<out-log>`：含每条的 `Start <全局编号>: <用例名>` 与结果行，供 `verify_test_coverage.py`
  核验"每个登记项恰好执行一次"。
- `<out-junit>`：JUnit XML，每个 gtest 用例一条 `<testcase name="Suite.Case">`，供
  `summarize_failures.py` 汇总失败/跳过。
"""

from __future__ import annotations

import argparse
import concurrent.futures
import json
import re
import subprocess
import sys
import tempfile
import threading
import time
import uuid
import xml.etree.ElementTree as ET
from pathlib import Path


def filter_pattern(name: str) -> str:
    """把用例全名转成能原样命中它的 gtest 过滤模式。

    gtest 的过滤串里 `-` 是「排除」分隔符、`:` 是「或」分隔符，而参数化用例的自动命名会
    带进这两种字符（本仓库实测有 32 个名字含 `1-byte object <00>` 这类片段）。把 `-` 换成
    `?`（匹配任意单字符）即可精确表达同一个名字；若某个名字本身含 `:`，则无法用过滤串表达，
    直接报错而不是静默漏跑。
    """
    if ":" in name:
        raise ValueError(f"test name cannot be expressed as a gtest filter: {name!r}")
    return name.replace("-", "?")


class Runner:
    def __init__(self, options: argparse.Namespace):
        self.options = options
        self.build_dir = options.build_dir.resolve()
        self.binary = self.build_dir / "bin" / options.config / "mc_tests"
        self.work_dir = self.build_dir / "tests" / "unit"
        self.work_dir.mkdir(parents=True, exist_ok=True)
        # ctest 登记名 → gtest 过滤名，以及反向映射（多数用例两者相同；参数化用例不同，见
        # _resolve_names）。
        self.filter_of: dict[str, str] = {}
        self.ctest_name_of: dict[str, str] = {}
        self.process_issues: list[dict] = []
        self._issue_lock = threading.Lock()
        self.process_log_dir = options.out_log.parent / "unit-test-process-logs"
        self.process_log_dir.mkdir(parents=True, exist_ok=True)

    # ---------------------------------------------------------------- 用例清单

    def inventory(self) -> list[str]:
        """按 ctest 的编号顺序返回全部用例名（顺序与 `ctest -N` 一致）。"""
        if self.options.inventory is not None:
            data = json.loads(self.options.inventory.read_text())
        else:
            completed = subprocess.run(
                ["ctest", "--test-dir", str(self.build_dir), "--build-config", self.options.config,
                 "--show-only=json-v1"],
                capture_output=True, text=True, check=True,
            )
            data = json.loads(completed.stdout)
        tests = data["tests"]
        # 本脚本用固定路径的 mc_tests 承载整批用例；若清单里混入了别的测试可执行文件，
        # 那些用例会被"跑不起来"而误报，故此处直接拦下。
        programs = {test["command"][0] for test in tests if test.get("command")}
        if programs != {str(self.binary)}:
            raise SystemExit(f"ctest inventory does not match the expected single test binary: "
                             f"expected {str(self.binary)}, got {sorted(programs)}")
        names = [test["name"] for test in tests]
        self._resolve_names(names)
        return names

    def _resolve_names(self, names: list[str]) -> None:
        """建立 ctest 登记名与 gtest 实际用例名的对应关系。

        `gtest_discover_tests` 对参数化用例的登记名用的是 `GetParam()` 的可读描述
        （如 `Suite.Test/(1-byte object <00>, 0, false)`），而真正能传给 `--gtest_filter`
        的是 `Suite.Test/0`。两者混用会导致这批用例永远"跑不到"却被当成失败，故按
        `--gtest_list_tests` 输出的 `# GetParam() = ...` 注释逐一对应。
        """
        listing = subprocess.run(
            [str(self.binary), "--gtest_list_tests"], cwd=self.work_dir,
            capture_output=True, text=True, check=True,
        ).stdout
        suite = ""
        for line in listing.splitlines():
            if not line.strip():
                continue
            if not line.startswith(" "):
                suite = line.strip()  # 形如 "SuiteName."
                continue
            body, _, comment = line.partition("# ")
            runtime_name = f"{suite}{body.strip()}"
            self.filter_of.setdefault(runtime_name, runtime_name)
            self.ctest_name_of[runtime_name] = runtime_name
            if comment.startswith("GetParam() = "):
                pretty = comment[len("GetParam() = "):].strip()
                registered = f"{suite}{body.strip().split('/')[0]}/{pretty}"
                self.filter_of[registered] = runtime_name
                self.ctest_name_of[runtime_name] = registered
        unresolved = [name for name in names if name not in self.filter_of]
        if unresolved:
            raise SystemExit("cannot map registered tests to gtest names, e.g. "
                             + ", ".join(repr(name) for name in unresolved[:3]))

    def selection(self, names: list[str]) -> list[tuple[int, str]]:
        """按 `-I <shard+1>,,<count>` 的语义选取本分片的用例（全局编号从 1 开始）。"""
        return [
            (index + 1, name)
            for index, name in enumerate(names)
            if index % self.options.shard_count == self.options.shard_index
        ]

    # ------------------------------------------------------------------ 执行

    def _run_gtest(self, names: list[str], xml_path: Path, timeout: float) -> tuple[int, bool, str]:
        """在单个进程里执行若干用例；返回 (退出码, 是否因超时被杀, 进程输出)。"""
        command = [
            str(self.binary),
            "--gtest_filter=" + ":".join(filter_pattern(self.filter_of[name]) for name in names),
            "--gtest_also_run_disabled_tests",
            "--gtest_color=no",
            "--gtest_brief=0",
            "--gtest_print_time=1",
            "--gtest_output=xml:" + str(xml_path),
        ]
        try:
            completed = subprocess.run(
                command, cwd=self.work_dir, capture_output=True, text=True, timeout=timeout, check=False
            )
            # 进程没写出结果时（崩溃、超时），这段输出就是唯一的现场证据（ASan/UBSan 报告等）。
            result = completed.returncode, False, completed.stdout + completed.stderr
        except subprocess.TimeoutExpired as expired:
            output = expired.stdout or b""
            errors = expired.stderr or b""
            if isinstance(output, bytes):
                output = output.decode("utf-8", "replace")
            if isinstance(errors, bytes):
                errors = errors.decode("utf-8", "replace")
            result = -1, True, output + errors
        log_path = self.process_log_dir / f"process-{uuid.uuid4().hex}.log"
        log_path.write_text(result[2], encoding="utf-8")
        return result

    def _record_process_issue(self, names: list[str], returncode: int, timed_out: bool,
                              output: str, results: list[dict]) -> None:
        """进程异常独立于用例结果归档，隔离复跑不能清除 sanitizer 或退出阶段错误。"""
        markers = ("ERROR: AddressSanitizer:", "ERROR: LeakSanitizer:",
                   "AddressSanitizer:DEADLYSIGNAL", "UndefinedBehaviorSanitizer:", ": runtime error:")
        has_failure = any(result["status"] in ("failure", "error") for result in results)
        if not (timed_out or returncode not in (0, 1)
                or (returncode == 1 and not has_failure)
                or any(marker in output for marker in markers)):
            return
        message = f"Test process {'timed out' if timed_out else 'exited'} with code {returncode}"
        message += f"; selected tests: {', '.join(names)}\n{output}"
        with self._issue_lock:
            self.process_issues.append({"name": f"process-{len(self.process_issues) + 1}",
                                        "message": message})

    def _parse_progress(self, output: str, names: list[str]) -> tuple[list[dict], str | None]:
        """从完整的 gtest 逐用例输出恢复完成事件，以及中断时正在运行的用例。

        XML 只在进程收尾时写出；控制台的成对 RUN/结束事件已确认用例完成，不能因为后面的
        用例卡死而丢弃。只接受本批次的实际过滤名，并忽略末尾汇总中的 FAILED/SKIPPED 行。
        """
        expected = {self.filter_of[name]: name for name in names}
        results = []
        active = None
        details = []
        for line in output.splitlines():
            start = re.fullmatch(r"\[ RUN      \] (.+)", line)
            if start and start[1] in expected:
                active = start[1]
                details = []
                continue
            end = re.fullmatch(r"\[\s*(OK|FAILED|SKIPPED)\s*\] (.+) \((\d+) ms\)", line)
            if end and active is not None and (end[2] == active or end[2].startswith(active + ", where ")):
                status = {"OK": "passed", "FAILED": "failure", "SKIPPED": "skipped"}[end[1]]
                results.append({"name": expected[active], "status": status,
                                "time": int(end[3]) / 1000,
                                "message": "\n".join(details) if status != "passed" else ""})
                active = None
            elif active is not None:
                details.append(line)
        return results, expected[active] if active is not None else None

    def _execute_subset(self, names: list[str], xml_path: Path) -> tuple[list[dict], list[list[str]]]:
        """恢复中断前的完成事件，只重试未完成的子集；没有进度时保留原有定位方式。

        返回 `(结果, 待执行的子集)`，子集重新提交到线程池，避免恢复工作串行推进。
        """
        if len(names) == 1:
            return [self._run_single(names[0])], []

        part_xml = xml_path.with_name(f"{xml_path.stem}_{uuid.uuid4().hex[:8]}.xml")
        part_xml.unlink(missing_ok=True)
        returncode, timed_out, output = self._run_gtest(names, part_xml, self.options.batch_timeout)
        results = self._parse_batch(part_xml) if part_xml.exists() else []
        part_xml.unlink(missing_ok=True)

        wanted = set(names)
        results = [result for result in results if result["name"] in wanted]
        reported = {result["name"] for result in results}
        progress, interrupted = self._parse_progress(output, names)
        if returncode != 0 or timed_out:
            for result in progress:
                if result["name"] not in reported:
                    results.append(result)
                    reported.add(result["name"])
        missing = [name for name in names if name not in reported]
        self._record_process_issue(names, returncode, timed_out, output, results)
        if not missing:
            return results, []

        if interrupted in missing:
            # gtest 明确给出了中断位置：只单跑该用例，尚未开始的用例继续成批执行。
            # 这避免反复执行已完成的前缀，也避免一次卡死展开为数百个初始化进程。
            remaining = [name for name in missing if name != interrupted]
            splits = [[interrupted]]
            if remaining:
                splits.append(remaining)
            print(f"batch interrupted in {interrupted}; recovered {len(results)} result(s), "
                  f"retrying {len(remaining)} unstarted test(s) as a batch", flush=True)
            return results, splits
        if len(missing) == 1:
            results.append(self._run_single(missing[0], batch_output=output))
            return results, []

        if timed_out:
            # 卡死与崩溃要分开处理：崩溃是快速的信号终止，二分每层只花一次进程启动；而卡死
            # 会让包含它的每个子集都跑满超时，二分反而成倍放大代价。超时直接展开为单用例，
            # 让卡死用例各自在 `--test-timeout` 上暴露，其余用例正常完成。
            print(f"batch of {len(names)} timed out; running {len(missing)} test(s) individually",
                  flush=True)
            return results, [[name] for name in missing]

        print(f"batch of {len(names)} exited with {returncode}; "
              f"bisecting {len(missing)} unreported test(s)", flush=True)
        middle = len(missing) // 2
        return results, [missing[:middle], missing[middle:]]

    def _run_single(self, name: str, batch_output: str = "") -> dict:
        """在独立进程里跑一个用例，返回它的结果。"""
        with tempfile.TemporaryDirectory(prefix="mc-single-") as scratch:
            xml_path = Path(scratch) / "single.xml"
            returncode, single_timed_out, single_output = self._run_gtest(
                [name], xml_path, self.options.test_timeout)
            parsed = self._parse_batch(xml_path) if xml_path.exists() else []
        self._record_process_issue([name], returncode, single_timed_out, single_output, parsed)
        if parsed:
            return {**parsed[0], "isolated": True}

        detail = "timed out" if single_timed_out else "exited before reporting a result"
        message = f"test process {detail}"
        # 单跑也没产出结果，说明进程在报告前就终止了；把批次/单跑的输出末尾带上，
        # 让崩溃原因（通常是 sanitizer 报告）能从失败清单直接看到。
        evidence = (single_output or batch_output).strip()
        if evidence:
            message += "\n" + evidence
        return {"name": name, "status": "error", "time": 0.0, "timed_out": single_timed_out,
                "isolated": True,
                "message": message}

    def _recheck_single(self, result: dict) -> dict:
        """对批次内失败的用例单独复核，区分「真实失败」与「批内污染」。

        同一进程连续跑多个用例时，前一个用例对全局单例（注册表、标签表等）的改动会泄漏给
        后一个用例；本仓库实测 `MerchantOfferTest.PriceAdjustment` 会把 `minecraft:bread`
        注册成普通 Item，之后同批用例的 SetUp 直接抛 "Item id already registered with
        different type"。逐用例起进程的 ctest 里看不到这类失败，批处理必须把它和真实回归
        区分开：单跑通过即按通过上报，并把污染事件写进日志留待修复。
        """
        if result["status"] not in ("failure", "error"):
            return result
        if result.get("isolated"):
            # 已单跑的失败不再重复复核，避免崩溃和超时各支付两次进程成本。
            return result
        single = self._run_single(result["name"])
        if single["status"] == "passed":
            return {**result, "status": "passed", "polluted_from_batch": True,
                    "message": "failed inside a batch but passed when run alone"}
        return single

    def _parse_batch(self, xml_path: Path) -> list[dict]:
        """解析 gtest 的 XML 输出，归一化为 {name,status,time,message}。"""
        results: list[dict] = []
        try:
            root = ET.parse(xml_path).getroot()
        except ET.ParseError:
            return results
        for case in root.iter("testcase"):
            suite = case.get("classname", "")
            name = case.get("name", "")
            full = f"{suite}.{name}" if suite else name
            # 结果统一用 ctest 的登记名回报，参数化用例的 gtest 名（`.../0`）在这里换回去。
            full = self.ctest_name_of.get(full, full)
            failure = case.find("failure")
            skipped = case.find("skipped")
            if failure is not None:
                status = "failure"
                message = (failure.get("message") or failure.text or "").strip()
            elif skipped is not None or case.get("result") == "skipped" or case.get("status") == "notrun":
                status = "skipped"
                message = (skipped.get("message") if skipped is not None else "") or ""
            else:
                status = "passed"
                message = ""
            results.append({"name": full, "status": status,
                            "time": float(case.get("time") or 0.0), "message": message})
        return results

    def run(self, selected: list[tuple[int, str]]) -> list[dict]:
        """按批次并发执行本分片的全部用例，返回每条用例的结果。"""
        wanted = [name for _, name in selected]
        batches = [wanted[index:index + self.options.batch_size]
                   for index in range(0, len(wanted), self.options.batch_size)]

        results: list[dict] = []
        with tempfile.TemporaryDirectory(prefix="mc-batch-") as scratch, \
                concurrent.futures.ThreadPoolExecutor(max_workers=self.options.jobs) as pool:
            futures = {
                pool.submit(self._execute_subset, batch, Path(scratch) / f"batch-{index}.xml")
                for index, batch in enumerate(batches)
            }
            while futures:
                done, futures = concurrent.futures.wait(
                    futures, return_when=concurrent.futures.FIRST_COMPLETED)
                for future in done:
                    part_results, splits = future.result()
                    results.extend(part_results)
                    # 崩溃后的子集重新入队，让多次二分在线程池里并发推进。
                    for split in splits:
                        futures.add(pool.submit(
                            self._execute_subset, split,
                            Path(scratch) / f"split-{uuid.uuid4().hex[:8]}.xml"))

        # 过滤模式里的 `?` 可能顺带命中同分片的相邻用例（重复执行一次，结果按名字去重），
        # 这里只保留本分片真正要的用例，避免把别人的结果写进本分片报告。
        selected_names = set(wanted)
        by_name: dict[str, dict] = {}
        for result in results:
            if result["name"] in selected_names:
                by_name.setdefault(result["name"], result)

        # 批次内失败的用例统一单跑复核（并发执行）：区分批内全局状态污染与真实回归。
        failed = [result for result in by_name.values() if result["status"] in ("failure", "error")]
        if failed:
            with concurrent.futures.ThreadPoolExecutor(max_workers=self.options.jobs) as pool:
                for rechecked in pool.map(self._recheck_single, failed):
                    by_name[rechecked["name"]] = rechecked
        return list(by_name.values())

    # ------------------------------------------------------------------ 产出

    def write_outputs(self, selected: list[tuple[int, str]], results: list[dict]) -> dict:
        by_name: dict[str, dict] = {}
        for result in results:
            by_name.setdefault(result["name"], result)

        log_lines = []
        polluted: list[str] = []
        counts = {"passed": 0, "failure": 0, "skipped": 0, "error": 0}
        for ordinal, (number, name) in enumerate(selected, start=1):
            result = by_name.get(name, {"status": "error", "time": 0.0,
                                        "message": "test did not run"})
            counts[result["status"]] = counts.get(result["status"], 0) + 1
            label = {"passed": "Passed", "failure": "***Failed",
                     "skipped": "Skipped", "error": "***Exception"}.get(result["status"], "Failed")
            if result.get("polluted_from_batch"):
                label = "Passed (batch pollution)"
                polluted.append(name)
            log_lines.append(f"          Start {number}: {name}")
            log_lines.append(
                f"{ordinal:4d}/{len(selected)} Test #{number}: {name} ... {label} "
                f"{result['time']:7.2f} sec")
            if result["status"] in ("failure", "error") and result["message"]:
                for line in result["message"].splitlines():
                    log_lines.append(f"    {line}")

        total = len(selected)
        reported_total = total + len(self.process_issues)
        passed = counts["passed"]
        log_lines.append("")
        log_lines.append(f"{100.0 * passed / reported_total if reported_total else 0:.0f}% tests passed, "
                         f"{counts['failure'] + counts['error'] + len(self.process_issues)} "
                         f"tests failed out of {reported_total}")
        if polluted:
            # 批内污染不是本次改动引入的缺陷，但会掩盖真实的用例顺序依赖，必须留在日志里。
            log_lines.append("")
            log_lines.append(f"[BatchPollution] {len(polluted)} test(s) failed inside a batch but passed alone; "
                             f"they depend on process-global state mutated by earlier tests in the same batch:")
            log_lines.extend(f"    {name}" for name in polluted)
        for issue in self.process_issues:
            log_lines.extend(["", f"[ProcessError] {issue['name']}", issue["message"]])
        self.options.out_log.write_text("\n".join(log_lines) + "\n", encoding="utf-8")
        for name in polluted:
            print(f"[BatchPollution] {name}", flush=True)

        # JUnit：每个用例一条 testcase；名字取完整 gtest 名（报告脚本按 name 直接展示）。
        root = ET.Element("testsuites")
        suite_element = ET.SubElement(root, "testsuite", name="mc_tests",
                                      tests=str(total), failures=str(counts["failure"]),
                                      errors=str(counts["error"]), skipped=str(counts["skipped"]),
                                      time=f"{sum(result['time'] for result in by_name.values()):.3f}")
        for _, name in selected:
            result = by_name.get(name, {"status": "error", "time": 0.0, "message": "test did not run"})
            case = ET.SubElement(suite_element, "testcase", name=name, classname="mc_tests",
                                 time=f"{result['time']:.3f}")
            if result["status"] == "failure":
                failure = ET.SubElement(case, "failure", message=result["message"][:4000] or "assertion failed")
                failure.text = result["message"]
            elif result["status"] == "error":
                error = ET.SubElement(case, "error", message=result["message"][:4000] or "process error")
                error.text = result["message"]
            elif result["status"] == "skipped":
                ET.SubElement(case, "skipped", message=result["message"] or "skipped")
        if self.process_issues:
            process_suite = ET.SubElement(root, "testsuite", name="TestProcess",
                                          tests=str(len(self.process_issues)), failures="0",
                                          errors=str(len(self.process_issues)), skipped="0", time="0")
            for issue in self.process_issues:
                case = ET.SubElement(process_suite, "testcase", name=issue["name"],
                                     classname="TestProcess", time="0")
                error = ET.SubElement(case, "error", message=issue["message"].splitlines()[0])
                error.text = issue["message"]
        ET.ElementTree(root).write(self.options.out_junit, encoding="utf-8", xml_declaration=True)

        return {"total": reported_total, "passed": counts["passed"], "failed": counts["failure"],
                "errors": counts["error"] + len(self.process_issues), "skipped": counts["skipped"]}


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--build-dir", type=Path, required=True)
    parser.add_argument("--inventory", type=Path,
                        help="预先由 `ctest --show-only=json-v1` 产出的用例清单；省略则现场生成")
    parser.add_argument("--config", default="RelWithDebInfo")
    parser.add_argument("--shard-index", type=int, required=True)
    parser.add_argument("--shard-count", type=int, required=True)
    parser.add_argument("--batch-size", type=int, default=200,
                        help="每个进程执行的用例数；越大初始化摊薄得越彻底，但崩溃影响面越大")
    parser.add_argument("--jobs", type=int, default=4, help="并发进程数")
    parser.add_argument("--batch-timeout", type=float, default=300.0,
                        help="单个批次的墙钟上限；正常批次在 CI 上约 40~90 秒，留足余量即可，"
                             "设得过大只会让卡死的批次更晚暴露")
    parser.add_argument("--test-timeout", type=float, default=300.0)
    parser.add_argument("--out-log", type=Path, required=True)
    parser.add_argument("--out-junit", type=Path, required=True)
    options = parser.parse_args()

    runner = Runner(options)
    names = runner.inventory()
    selected = runner.selection(names)
    if not selected:
        print(f"::error::shard {options.shard_index}/{options.shard_count} selected no tests")
        return 1

    started = time.monotonic()
    results = runner.run(selected)
    summary = runner.write_outputs(selected, results)
    print(f"shard {options.shard_index}/{options.shard_count}: {summary} "
          f"in {time.monotonic() - started:.1f}s "
          f"(batch={options.batch_size}, jobs={options.jobs})")
    return 2 if runner.process_issues else 1 if summary["failed"] or summary["errors"] else 0


if __name__ == "__main__":
    sys.exit(main())
