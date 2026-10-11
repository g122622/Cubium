#!/usr/bin/env python3
"""采集独立 pococc 编译池的 CPU、传输和并行证据，并生成最终报告。"""

from __future__ import annotations

import argparse
from collections import Counter, defaultdict
import fcntl
import importlib.util
import json
import os
from pathlib import Path
import re
import resource
import shlex
import subprocess
import sys
import time


def write_json(path, value):
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(value, indent=2) + "\n", encoding="utf-8")


def measured_command(command):
    """独占子进程树的资源差值包含其所有已回收后代，不使用 distcc 的零值 rusage。"""
    before = resource.getrusage(resource.RUSAGE_CHILDREN)
    start = time.time_ns()
    monotonic_start = time.monotonic()
    result = subprocess.run(command, check=False)
    elapsed = time.monotonic() - monotonic_start
    after = resource.getrusage(resource.RUSAGE_CHILDREN)
    return {"pid": os.getpid(), "start_ns": start, "end_ns": time.time_ns(),
            "wall_seconds": elapsed, "user_seconds": after.ru_utime - before.ru_utime,
            "system_seconds": after.ru_stime - before.ru_stime,
            "max_rss_kib": after.ru_maxrss, "exit_code": result.returncode}


def compiler(options):
    record = measured_command(options.command)
    with options.out.open("a", encoding="utf-8") as stream:
        fcntl.flock(stream, fcntl.LOCK_EX)
        stream.write(json.dumps(record) + "\n")
    return record["exit_code"]


def start_worker(options):
    """使用 distccd 官方命令白名单重映射到计时驱动，不修改安装的 Clang。"""
    spec = importlib.util.spec_from_file_location("pococc_bootstrap", options.bootstrap)
    bootstrap = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(bootstrap)
    original = bootstrap.configure_compiler_commands

    def instrument(state, selected):
        commands = original(state, selected)
        directory = state / "timed-compilers"
        directory.mkdir()
        wrappers = []
        for command in commands:
            wrapper = directory / command.name
            arguments = [sys.executable, str(Path(__file__).resolve()), "compiler",
                         "--out", str(state / "compiler-times.jsonl"), "--", str(command)]
            wrapper.write_text("#!/bin/sh\nexec " + shlex.join(arguments) + ' "$@"\n')
            wrapper.chmod(0o700)
            wrappers.append(wrapper)
        Path(os.environ["DISTCC_CMDLIST"]).write_text("".join(str(path) + "\n" for path in wrappers))
        return commands

    bootstrap.configure_compiler_commands = instrument
    bootstrap.worker(options)
    return 0


def build(options):
    before = ccache_counters()
    record = measured_command(options.command)
    after = ccache_counters()
    record["ccache"] = {name: after[name] - before[name] for name in before}
    if any(value < 0 for value in record["ccache"].values()):
        raise RuntimeError("ccache counters were reset during the build")
    record["ccache_restore_key"] = os.environ.get("POCOCC_CCACHE_RESTORE_KEY", "")
    record.update(kind="coordinator", pool=options.pool, workers=options.workers,
                  slots_per_worker=options.slots, ninja_jobs=options.jobs,
                  preprocess_jobs=options.preprocess_jobs,
                  run_id=os.environ.get("GITHUB_RUN_ID", "local"), distcc_submissions=0,
                  submitted_by_source={}, local_invocations=0, failures=0, summed_task_seconds=0,
                  remote_attempts=0, remote_retries=0, attempted_by_source={}, delivered_by_source={},
                  output_receive_seconds=0)
    if options.journal.exists():
        rows = [line.split("\t") for line in options.journal.read_text().splitlines()]
        if any(len(row) != 7 for row in rows):
            raise RuntimeError("Malformed compilation journal")
        record["distcc_submissions"] = sum(row[1] == "distcc" for row in rows)
        record["submitted_by_source"] = dict(Counter(row[4] for row in rows if row[1] == "distcc"))
        record["local_invocations"] = sum(row[1] == "local" for row in rows)
        record["failures"] = sum(row[2] != "0" for row in rows)
        record["summed_task_seconds"] = sum(int(row[3]) for row in rows) / 1e6
    if options.client_log.exists():
        record.update(client_metrics(options.client_log.read_text()))
        record["output_receive_seconds"] = sum(
            float(value) for value in re.findall(r"\d+ bytes received in ([\d.]+)s", options.client_log.read_text()))
    write_json(options.out, record)
    return record["exit_code"]


def ccache_counters():
    result = subprocess.run(["ccache", "--print-stats"], check=True, capture_output=True, text=True)
    values = dict(line.split() for line in result.stdout.splitlines())
    uncacheable = ("autoconf_test", "bad_compiler_arguments", "bad_input_file", "bad_output_file",
                  "called_for_link", "called_for_preprocessing", "compile_failed", "compiler_check_failed",
                  "compiler_produced_empty_output", "compiler_produced_no_output", "compiler_produced_stdout",
                  "could_not_find_compiler", "could_not_use_modules", "could_not_use_precompiled_header",
                  "disabled", "error_hashing_extra_file", "internal_error", "missing_cache_file",
                  "modified_input_file", "multiple_source_files", "no_input_file", "output_to_stdout",
                  "preprocessor_error", "unsupported_code_directive", "unsupported_compiler_option",
                  "unsupported_environment_variable", "unsupported_source_language")
    return {"direct_hits": int(values["direct_cache_hit"]),
            "preprocessed_hits": int(values["preprocessed_cache_hit"]),
            "misses": int(values["cache_miss"]),
            "uncacheable": sum(int(values.get(name, 0)) for name in uncacheable)}


def client_metrics(log):
    """一次调用可因隧道中断发往多台 worker；按客户端进程统计尝试和已收到的结果。"""
    sources = {}
    attempts = Counter()
    deliveries = Counter()
    calls = Counter()
    for line in log.splitlines():
        match = re.match(r"distcc\[(\d+)\] (.*)", line)
        if not match:
            continue
        pid, message = int(match[1]), match[2]
        source = re.fullmatch(r'\(dcc_scan_args\) found input file "(.*)"', message)
        if source:
            sources[pid] = source[1]
        if message.startswith("exec on "):
            if pid not in sources:
                raise RuntimeError("Remote attempt has no input source evidence")
            attempts[sources[pid]] += 1
            calls[pid] += 1
        if message.startswith("compile ") and message.endswith(" completed ok"):
            if pid not in calls:
                raise RuntimeError("Remote result has no attempt evidence")
            deliveries[sources[pid]] += 1
    return {"remote_attempts": sum(attempts.values()), "remote_retries": sum(calls.values()) - len(calls),
            "attempted_by_source": dict(attempts), "delivered_by_source": dict(deliveries)}


def worker_metrics(log, times):
    """按 distccd 请求进程归组，排除 doctor，传输长度使用压缩后的协议 token。"""
    tasks = defaultdict(dict)
    completed = []
    probes = 0
    for line in log.splitlines():
        match = re.match(r"distccd\[(\d+)\]", line)
        if not match:
            continue
        pid = int(match[1])
        task = tasks[pid]
        child = re.search(r"cc child (\d+) terminated", line)
        if child:
            task["compiler_pid"] = int(child[1])
        token = re.search(r"(?:got|send) (DOTI|DOTO|SERR|SOUT)([0-9a-fA-F]{8})", line)
        if token:
            task[token[1]] = int(token[2], 16)
        received = re.search(r"bytes received in ([\d.]+)s", line)
        if received:
            task["input_receive_seconds"] = float(received[1])
        if "(dcc_job_summary)" in line:
            summary = re.search(r"(COMPILE_OK|COMPILE_ERROR).*time:(\d+)ms .*? (\S+)\s*$", line)
            if summary and "/pococc-probe-" in summary[3] and summary[1] == "COMPILE_OK":
                probes += 1
            if summary and "/pococc-probe-" not in summary[3]:
                task.update(success=summary[1] == "COMPILE_OK", wall_seconds=int(summary[2]) / 1000,
                            source=summary[3])
                completed.append(task.copy())
            tasks[pid] = {}
    compiler_times = {record["pid"]: record for record in times}
    intervals = []
    for task in completed:
        timing = compiler_times.get(task.get("compiler_pid"))
        if timing is None:
            raise RuntimeError(f"Missing compiler CPU evidence for {task['source']}")
        task["cpu_seconds"] = timing["user_seconds"] + timing["system_seconds"]
        task["compiler_wall_seconds"] = timing["wall_seconds"]
        intervals.append([timing["start_ns"], timing["end_ns"]])
    return {"remote_compiles": sum(task["success"] for task in completed),
            "probe_compiles": probes,
            "completed_by_source": dict(Counter(task["source"] for task in completed if task["success"])),
            "failed_compiles": sum(not task["success"] for task in completed),
            "compiler_cpu_seconds": sum(task["cpu_seconds"] for task in completed),
            "compiler_wall_seconds": sum(task["compiler_wall_seconds"] for task in completed),
            "input_bytes": sum(task.get("DOTI", 0) for task in completed),
            "output_bytes": sum(sum(task.get(token, 0) for token in ("DOTO", "SERR", "SOUT")) for task in completed),
            "input_receive_seconds": sum(task.get("input_receive_seconds", 0) for task in completed),
            "intervals": intervals}


def worker_report(options):
    state = options.state
    times = [json.loads(line) for line in (state / "compiler-times.jsonl").read_text().splitlines()]
    record = worker_metrics((state / "worker.log").read_text(), times)
    endpoint = json.loads((state / "endpoint.json").read_text())
    record.update(kind="worker", pool=options.pool, worker=endpoint["worker"], run_id=endpoint["run_id"])
    write_json(state / "pococc-performance.json", record)
    if record["failed_compiles"] or not record["probe_compiles"]:
        raise RuntimeError("Worker compilation or compiler probe failed")
    return 0


def peak_parallelism(intervals):
    events = sorted([(start, 1) for start, _ in intervals] + [(end, -1) for _, end in intervals])
    active = peak = 0
    for _, delta in events:
        active += delta
        peak = max(peak, active)
    return peak


def performance_report(directory, run_id):
    records = [json.loads(path.read_text()) for path in directory.rglob("pococc-performance.json")]
    if any(record["run_id"] != run_id for record in records):
        raise RuntimeError("Performance evidence belongs to a different run")
    lines = ["## pococc 分布式编译性能", "",
             "| 编译池 | Worker / 远端槽位 | Ninja / distcc 预处理上限 | 实测远端峰值 | 构建耗时 | 缓存命中 / 未命中（命中率） | 远端调用 / 尝试 / 完成 / 本地编译 | 协调节点 CPU | 远端编译 CPU | 上行 / 下行 | 收取及解压累计时间 |",
             "|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|"]
    errors = []
    for pool in ("test", "release", "fuzz"):
        coordinators = [record for record in records if record["pool"] == pool and record["kind"] == "coordinator"]
        workers = [record for record in records if record["pool"] == pool and record["kind"] == "worker"]
        if len(coordinators) != 1:
            errors.append(f"{pool}: missing unique coordinator evidence")
            continue
        coordinator = coordinators[0]
        cache = coordinator["ccache"]
        hits = cache["direct_hits"] + cache["preprocessed_hits"]
        misses = cache["misses"]
        calls = hits + misses
        count = coordinator["workers"]
        completed = sum(worker["remote_compiles"] for worker in workers)
        if sorted(worker["worker"] for worker in workers) != list(range(1, count + 1)):
            errors.append(f"{pool}: missing or duplicated worker evidence")
        submitted = Counter(coordinator["submitted_by_source"])
        attempted = Counter(coordinator["attempted_by_source"])
        delivered = Counter(coordinator["delivered_by_source"])
        actual = Counter()
        for worker in workers:
            actual.update(worker["completed_by_source"])
        if delivered != submitted or sum(submitted.values()) != coordinator["distcc_submissions"]:
            errors.append(f"{pool}: delivered results do not match compilation invocations")
        if submitted - actual or actual - attempted or sum(actual.values()) != completed:
            errors.append(f"{pool}: worker completions do not match submitted sources and remote attempts")
        if sum(attempted.values()) != coordinator["remote_attempts"]:
            errors.append(f"{pool}: remote attempt count does not match source evidence")
        if coordinator["exit_code"] or coordinator.get("failures") or any(w["failed_compiles"] for w in workers):
            errors.append(f"{pool}: build or compilation failed")
        if any(not worker["probe_compiles"] for worker in workers):
            errors.append(f"{pool}: worker compiler probe is missing")
        if not coordinator["distcc_submissions"] and not hits:
            errors.append(f"{pool}: no compilation or cache hit evidence")
        if misses + cache["uncacheable"] != coordinator["distcc_submissions"] + coordinator["local_invocations"]:
            errors.append(f"{pool}: uncached compiler calls do not match strict routing evidence")
        cpu = sum(worker["compiler_cpu_seconds"] for worker in workers)
        intervals = [interval for worker in workers for interval in worker["intervals"]]
        upload = sum(worker["input_bytes"] for worker in workers) / 2**30
        download = sum(worker["output_bytes"] for worker in workers) / 2**30
        receive = sum(worker["input_receive_seconds"] for worker in workers) + coordinator.get("output_receive_seconds", 0)
        coordinator_cpu = coordinator["user_seconds"] + coordinator["system_seconds"]
        lines.append(f"| {pool} | {count} / {count * coordinator['slots_per_worker']} | "
                     f"{coordinator['ninja_jobs']} / {coordinator['preprocess_jobs']} | {peak_parallelism(intervals)} | "
                     f"{coordinator['wall_seconds'] / 60:.1f} min | {hits} / {misses} ({hits / calls * 100 if calls else 0:.1f}%) | "
                     f"{coordinator['distcc_submissions']} / "
                     f"{coordinator['remote_attempts']} / {completed} / {coordinator.get('local_invocations', 0)} | "
                     f"{coordinator_cpu / 60:.1f} CPU-min | {cpu / 60:.1f} CPU-min | "
                     f"{upload:.3f} / {download:.3f} GiB | {receive:.1f} s |")
    configured_workers = sum(record["workers"] for record in records if record["kind"] == "coordinator")
    retries = sum(record["remote_retries"] for record in records if record["kind"] == "coordinator")
    lines += ["", f"- 本次配置 {configured_workers} 个 worker，分为 test / release / fuzz 三个独立池；禁用 PCH，严格模式禁止通信失败后本地回退。",
              f"- 本次发生 {retries} 次远端重试。一次调用可以产生多个远端尝试；响应丢失后重复完成的编译也计入 CPU 与传输开销。报告逐源码核对调用、已收取结果、worker 完成数及尝试上限。",
              "- 构建耗时与 CPU 计量覆盖编译、链接阶段，不包含依赖配置安装、节点会合及 artifact 传输。预处理上限是 distcc 的设置，不限制 ccache 自身的哈希预处理。",
              "- CPU 时间来自进程树资源计量：协调节点含预处理、本地编译、链接、ccache 和 pococc；远端只计实际项目编译，排除 doctor 探针。背景 SSH/cloudflared 服务的 CPU 不在这两个计量范围内。",
              "- 上下行是 distcc 经 LZO 压缩后的文件载荷（包含对象文件和诊断），不含 SSH/TLS/WebSocket 头、重传或 GitHub artifact 上传。",
              "- 收取及解压时间是两端接收文件的累计计时，包含等待和解压，多个任务可以重叠；不能与构建墙钟时间直接相减，也不等于纯网络延迟。",
              "- 峰值由各 runner 的 UTC 编译区间合并计算，跨机器时钟误差会影响边界；累计任务时间含排队和通信，不作为 CPU 开销。",
              "- ccache 使用 Nightly 相同的持久缓存策略；命中率按本轮构建前后计数差计算，排除恢复的历史统计。暖缓存允许 worker 空闲，逐节点 doctor 探针仍必须通过。",
              "- 没有同配置单机基线，不推算加速比。", ""]
    for record in records:
        if record["kind"] == "coordinator":
            lines.append(f"- {record['pool']} ccache 恢复来源：`{record['ccache_restore_key'] or '未恢复'}`")
    if errors:
        lines += ["### 性能证据错误", ""] + ["- " + error for error in errors] + [""]
    return "\n".join(lines), records, errors


def report(options):
    markdown, records, errors = performance_report(options.directory, os.environ["GITHUB_RUN_ID"])
    options.out.write_text(markdown, encoding="utf-8")
    write_json(options.json_out, {"records": records, "errors": errors})
    print(markdown)
    return int(bool(errors))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    commands = parser.add_subparsers(dest="mode", required=True)
    timed = commands.add_parser("compiler")
    timed.add_argument("--out", type=Path, required=True)
    timed.add_argument("command", nargs=argparse.REMAINDER)
    worker = commands.add_parser("start-worker")
    worker.add_argument("--bootstrap", type=Path, required=True)
    worker.add_argument("--worker", type=int, required=True)
    worker.add_argument("--state", type=Path, required=True)
    measured = commands.add_parser("build")
    for name in ("out", "journal", "client-log"):
        measured.add_argument("--" + name, type=Path, required=True)
    measured.add_argument("--pool", required=True)
    for name in ("workers", "slots", "jobs", "preprocess-jobs"):
        measured.add_argument("--" + name, type=int, required=True)
    measured.add_argument("command", nargs=argparse.REMAINDER)
    evidence = commands.add_parser("worker-report")
    evidence.add_argument("--state", type=Path, required=True)
    evidence.add_argument("--pool", required=True)
    final = commands.add_parser("report")
    for name in ("directory", "out", "json-out"):
        final.add_argument("--" + name, type=Path, required=True)
    options = parser.parse_args()
    if hasattr(options, "command") and options.command[:1] == ["--"]:
        options.command = options.command[1:]
    try:
        return {"compiler": compiler, "start-worker": start_worker, "build": build,
                "worker-report": worker_report, "report": report}[options.mode](options)
    except (OSError, RuntimeError, ValueError, KeyError, subprocess.SubprocessError) as error:
        print(f"pococc performance error: {error}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    sys.exit(main())
