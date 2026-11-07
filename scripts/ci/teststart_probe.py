#!/usr/bin/env python3
"""夜间 CI 单测耗时的进程启动对照实验（临时工具，见 tests/ci-probes/README.md）。

CI 上每个 ctest 用例都是一个独立进程，单进程固定开销 8~14 秒且 CPU 时间与墙钟基本相同；
ctest 只给出每个用例的总时长，没有任何归因。本脚本在同一工件、同一次作业内背靠背执行四组对照：

  launcher  只在 main 里做最基本的工作，逐级链接 fmt / spdlog / CrashHandler，
            回答"进程启动本身"与"重型静态库初始化"各占多少；
  teststart 用真实测试进程的目标文件，逐级装配 tests/unit/main.cpp 的入口组件，
            回答"测试环境初始化 / 用例体 / 退出"各占多少；
  ctest     与真实 unit-tests 作业完全相同的 ctest 调用（含 -j2 并发），作为基线；
  direct    与 ctest 等价的串行 gtest 调用（不经 ctest 的调度与日志包装），
            回答 ctest 自身是否在推高单用例耗时。

所有对照都只链接构建作业已产出的目标文件，不做任何编译（唯一例外是等级与层级无关的
main 之外的目标文件缺失时的重新编译，正常情况下不会发生）。
"""

from __future__ import annotations

import argparse
import json
import os
import re
import shlex
import subprocess
import sys
import time
from pathlib import Path

NINJA_FILE = Path("build/CMakeFiles/impl-{config}.ninja")
COMPILE_DB = Path("build/compile_commands.json")
PROBE_BIN_DIR = Path("/tmp/teststart-probe")

PROBE_TIME_RE = re.compile(r"^PROBE_TIME (\w+) (-?\d+)$", re.MULTILINE)
PROBE_DONE_RE = re.compile(r"^PROBE_DONE (\S+) (-?\d+) (-?\d+)$", re.MULTILINE)


class ProbeError(RuntimeError):
    pass


def run(command: list[str], **kwargs) -> subprocess.CompletedProcess:
    return subprocess.run(command, check=False, text=True, **kwargs)


def find_entry(compile_db: Path, suffix: str) -> dict:
    for entry in json.loads(compile_db.read_text()):
        file_name = entry.get("file", "")
        if file_name.endswith(suffix) and "mc_tests.dir" in entry.get("output", ""):
            return entry
    raise ProbeError(f"compile_commands.json 中找不到 {suffix}")


def compile_variant(compile_db: Path, suffix: str, output: Path, extra_flags: list[str]) -> None:
    """按 mc_tests 的编译参数编译一个变体目标文件（只替换 -o 与追加宏定义）。"""
    entry = find_entry(compile_db, suffix)
    tokens = shlex.split(entry["command"])
    result = []
    index = 0
    while index < len(tokens):
        token = tokens[index]
        if token == "-o":
            result.extend(["-o", str(output)])
            index += 2
            continue
        result.append(token)
        index += 1
    result = result[:1] + extra_flags + result[1:]
    completed = run(result, cwd=entry["directory"], capture_output=True)
    if completed.returncode != 0:
        raise ProbeError(f"编译 {suffix} 失败：\n{completed.stderr[-4000:]}")


def link_inputs(ninja_file: Path, config: str) -> tuple[list[str], list[str], list[str]]:
    """从 ninja 的 mc_tests 链接语句解析 (对象文件, 库, 其它 FLAGS/LINK_FLAGS)。"""
    lines = ninja_file.read_text().splitlines()
    start = next(i for i, line in enumerate(lines) if line.startswith(f"build bin/{config}/mc_tests "))
    inputs = lines[start].split(":", 1)[1].split()
    inputs = inputs[: inputs.index("|")]
    flags = re.search(r"^\s*FLAGS = (.*)$", lines[start + 1]).group(1)
    link_flags = re.search(r"^\s*LINK_FLAGS = (.*)$", lines[start + 2]).group(1)
    libraries = re.search(r"^\s*LINK_LIBRARIES = (.*)$", lines[start + 3]).group(1)
    libraries = [token for token in shlex.split(libraries) if not token.startswith("-Wl,-rpath")]
    objects = [token for token in inputs if token.endswith(".o")]
    return objects, libraries, shlex.split(flags) + shlex.split(link_flags)


def link(compiler: str, rsp: Path, output: Path) -> None:
    completed = run([compiler, "-g", "-rdynamic", f"@{rsp}", "-o", str(output)], capture_output=True)
    if completed.returncode != 0:
        raise ProbeError(f"链接 {output} 失败：\n{completed.stderr[-4000:]}")


def launch_once(binary: Path, env: dict[str, str], cwd: Path, tag: str | None = None) -> dict:
    """运行一次探针并解析 PROBE_*：wall 取 Python 观测的进程生命周期（含退出收尾）。"""
    environment = dict(os.environ)
    environment.update(env)
    if tag is not None:
        environment["PROBE_TAG"] = tag
    began = time.monotonic()
    completed = run([str(binary)], cwd=cwd, env=environment, capture_output=True)
    wall_ms = (time.monotonic() - began) * 1000
    phases = {match.group(1): int(match.group(2)) for match in PROBE_TIME_RE.finditer(completed.stdout)}
    done = PROBE_DONE_RE.search(completed.stdout)
    return {
        "tag": tag,
        "returncode": completed.returncode,
        "wall_ms": round(wall_ms),
        "phases": phases,
        "internal": {"wall_ms": int(done.group(2)), "cpu_ms": int(done.group(3))} if done else None,
        "stderr_tail": completed.stderr[-800:] if completed.returncode != 0 else "",
    }


def ctest_step(build_dir: Path, config: str, window: str, jobs: int, out_xml: Path, log: Path) -> dict:
    """执行一段与真实 unit-tests 作业完全相同的 ctest 调用，返回各用例耗时。"""
    command = [
        "ctest", "--build-config", config, "-I", window,
        "--output-on-failure", "--output-junit", str(out_xml), "--timeout", "300", f"-j{jobs}",
    ]
    began = time.monotonic()
    completed = run(command, cwd=build_dir, capture_output=True)
    log.write_text(completed.stdout + completed.stderr)
    durations = [float(value) for value in re.findall(r"^\s*\d+/\d+ Test\s+#\d+:.*?([0-9.]+) sec$", log.read_text(), re.MULTILINE)]
    return {
        "command": " ".join(command[:6]),
        "wall_s": round(time.monotonic() - began, 1),
        "tests": len(durations),
        "median_s": round(sorted(durations)[len(durations) // 2], 2) if durations else None,
    }


def direct_step(binary: Path, cwd: Path, names: list[str]) -> dict:
    """串行直跑与 ctest 相同的用例清单，逐用例观测完整进程生命周期。"""
    durations = []
    for name in names:
        began = time.monotonic()
        run([str(binary), f"--gtest_filter={name}"], cwd=cwd, capture_output=True)
        durations.append(time.monotonic() - began)
    return {
        "tests": len(durations),
        "median_s": round(sorted(durations)[len(durations) // 2], 2) if durations else None,
    }


def selection_names(build_dir: Path, window: str, limit: int) -> list[str]:
    completed = run(["ctest", "--test-dir", str(build_dir), "-I", window, "--show-only=json-v1"], capture_output=True)
    tests = json.loads(completed.stdout)["tests"][:limit]
    return [test["name"] for test in tests]


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--build-dir", type=Path, default=Path("build"))
    parser.add_argument("--config", default="RelWithDebInfo")
    parser.add_argument("--compiler", default="/usr/lib/llvm-22/bin/clang++")
    parser.add_argument("--out", type=Path, required=True)
    parser.add_argument("--clean", action="store_true", help="删除 tests/ci-probes 后退出（调查结束后使用）")
    options = parser.parse_args()

    if options.clean:
        subprocess.run(["git", "rm", "-r", "-q", "--cached", "tests/ci-probes", "scripts/ci/teststart_probe.py"], check=False)
        subprocess.run(["rm", "-rf", "tests/ci-probes", "scripts/ci/teststart_probe.py"], check=False)
        return 0

    build_dir = options.build_dir.resolve()
    config = options.config
    os.chdir(build_dir.parent)
    test_binary = build_dir / "bin" / config / "mc_tests"
    work_dir = build_dir / "tests" / "unit"
    PROBE_BIN_DIR.mkdir(parents=True, exist_ok=True)

    objects, libraries, flags = link_inputs(NINJA_FILE.with_name(NINJA_FILE.name.format(config=config)), config)
    main_object = next(
        path for path in objects
        if path.startswith("tests/unit/CMakeFiles/mc_tests.dir/") and path.endswith("/main.cpp.o")
    )
    ordered_objects = [main_object] + [path for path in objects if path != main_object]

    report: dict = {
        "config": config,
        "compiler": run([options.compiler, "--version"], capture_output=True).stdout.splitlines()[:1],
        "mc_tests_bytes": test_binary.stat().st_size,
        "launcher": [],
        "teststart": [],
        "ctest": [],
        "direct": [],
    }

    # ---- 1. launcher：进程启动与静态库初始化 ----
    include_flags = ["-Itests/ci-probes/src/launcher", "-Itests/ci-probes/src/teststart"]
    for mode, tag in ((0, "triv"), (1, "fmt"), (2, "spdlog"), (3, "crasher")):
        variant = PROBE_BIN_DIR / f"main_{tag}.o"
        compile_variant(
            COMPILE_DB, "tests/ci-probes/src/launcher/probe_main.cpp", variant,
            include_flags + [f"-DPROBE_MODE={mode}"],
        )
        rsp = PROBE_BIN_DIR / f"launcher_{tag}.rsp"
        rsp.write_text("\n".join([str(variant)] + libraries) + "\n")
        binary = PROBE_BIN_DIR / f"launcher_{tag}"
        link(options.compiler, rsp, binary)
        report["launcher"].append(
            {"mode": tag, "binary_bytes": binary.stat().st_size, **launch_once(binary, {}, Path("/tmp"), tag)}
        )

    # ---- 2. teststart：测试入口的分阶段装配 ----
    for level in range(5):
        variant = PROBE_BIN_DIR / f"main_level{level}.o"
        compile_variant(COMPILE_DB, "tests/ci-probes/src/teststart/test_main.cpp", variant, include_flags)
        rsp = PROBE_BIN_DIR / f"teststart_level{level}.rsp"
        members = [str(variant)] + ordered_objects[1:]
        rsp.write_text("\n".join(members + libraries) + "\n")
        binary = PROBE_BIN_DIR / f"teststart_level{level}"
        link(options.compiler, rsp, binary)
        for repeat in range(3):
            sample = launch_once(binary, {"PROBE_LEVEL": str(level), "MC_TIMING_WORLDGEN": "1"}, work_dir, f"level{level}")
            report["teststart"].append({"level": level, "repeat": repeat, **sample})

    # ---- 3. ctest 基线与 4. 直跑对照 ----
    window = "1,,20"
    report["ctest"].append(ctest_step(build_dir, config, window, 2, PROBE_BIN_DIR / "ctest-j2.xml", PROBE_BIN_DIR / "ctest-j2.log"))
    report["ctest"].append(ctest_step(build_dir, config, window, 1, PROBE_BIN_DIR / "ctest-j1.xml", PROBE_BIN_DIR / "ctest-j1.log"))
    names = selection_names(build_dir, window, 60)
    report["direct"].append(direct_step(test_binary, work_dir, names))

    options.out.write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps(report, indent=2))
    return 0


if __name__ == "__main__":
    try:
        sys.exit(main())
    except ProbeError as error:
        print(f"::error::{error}", file=sys.stderr)
        sys.exit(1)