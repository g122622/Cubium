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
import shutil
import subprocess
import sys
import time
from pathlib import Path

NINJA_FILE = Path("build/CMakeFiles/impl-{config}.ninja")
COMPILE_DB = Path("build/compile_commands.json")
PROBE_BIN_DIR = Path("/tmp/teststart-probe")
REPO_ROOT = Path(__file__).resolve().parents[2]
# 显式指定工具链时的兜底路径：runner 上的 clang 通常经 update-alternatives 装在 /usr/bin。
FALLBACK_COMPILERS = ["/usr/bin/clang++", "/usr/bin/clang++-22", "clang++"]

PROBE_TIME_RE = re.compile(r"^PROBE_TIME (\w+) (-?\d+)$", re.MULTILINE)
PROBE_DONE_RE = re.compile(r"^PROBE_DONE (\S+) (-?\d+) (-?\d+)$", re.MULTILINE)


class ProbeError(RuntimeError):
    pass


def run(command: list[str], **kwargs) -> subprocess.CompletedProcess:
    return subprocess.run(command, check=False, text=True, **kwargs)


def find_entry(compile_db: Path, suffix: str) -> dict:
    """取某个 mc_tests 源文件的编译参数（探针源文件不在数据库里，借用同目标的编译参数）。"""
    for entry in json.loads(compile_db.read_text()):
        if entry.get("file", "").endswith(suffix) and "mc_tests.dir" in entry.get("output", ""):
            return entry
    raise ProbeError(f"compile_commands.json 中找不到 {suffix}")


def compile_variant(compile_db: Path, suffix: str, output: Path, extra_flags: list[str], compiler: str) -> None:
    """按 mc_tests 的编译参数编译一个变体目标文件（只替换 -o 与追加宏定义）。"""
    entry = find_entry(compile_db, "tests/unit/main.cpp")
    tokens = shlex.split(entry["command"])
    # 编译数据库里的编译器是构建 worker 的工具链专用封装；本作业没有该工具链。
    tokens[0] = compiler
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
    absolute = str(Path(suffix).resolve())
    tokens_source = next(i for i, token in enumerate(result) if token.endswith("tests/unit/main.cpp"))
    result[tokens_source] = absolute
    result = result[:1] + extra_flags + result[1:]
    completed = run(result, cwd=entry["directory"], capture_output=True)
    if completed.returncode != 0:
        raise ProbeError(f"编译 {suffix} 失败：\n{completed.stderr[-4000:]}")


def link_inputs(ninja_file: Path, config: str) -> tuple[list[str], list[str], list[str], str]:
    """从 ninja 的 mc_tests 链接语句解析 (对象文件, 库, FLAGS+LINK_FLAGS, 链接编译器命令)。"""
    lines = ninja_file.read_text().splitlines()

    def variables(start: int) -> dict[str, str]:
        """收集某条 build 语句之后的缩进变量（FLAGS/LINK_FLAGS/...）。"""
        result: dict[str, str] = {}
        for line in lines[start + 1:]:
            if not line.startswith("  "):
                break
            key, _, value = line.strip().partition(" = ")
            result[key] = value
        return result

    start = next(i for i, line in enumerate(lines) if line.startswith(f"build bin/{config}/mc_tests "))
    entry = variables(start)
    # "build <targets>: <rule> <inputs>"；目标路径里本身可能带 ":"，故从 " : " 之后找规则名。
    rule = re.search(r" \S+: (\S+)", lines[start]).group(1)
    inputs = lines[start].split(":", 1)[1].split()
    inputs = inputs[: inputs.index("|")]
    libraries = [token for token in shlex.split(entry["LINK_LIBRARIES"]) if not token.startswith("-Wl,-rpath")]
    objects = [token for token in inputs if token.endswith(".o")]

    flags = shlex.split(entry.get("FLAGS", "")) + shlex.split(entry.get("LINK_FLAGS", ""))
    return objects, libraries, flags, _linker_from_rules(ninja_file, rule)


def _linker_from_rules(ninja_file: Path, rule: str) -> str:
    """链接规则（command = $PRE_LINK && <compiler> $FLAGS $LINK_FLAGS @$RSP_FILE ...）在
    CMakeFiles/rules.ninja；取其编译器名，避免写死工具链版本。规则文件缺失时返回默认名，
    由调用方在 PATH 中解析。"""
    rules_file = ninja_file.with_name("rules.ninja")
    if not rules_file.exists():
        return "clang++"
    body = re.search(rf"^rule {re.escape(rule)}\r?\n((?:  [^\r\n]*\r?\n)*)", rules_file.read_text(), re.MULTILINE)
    if body is None:
        return "clang++"
    command = re.search(r"^  command = (.*)$", body.group(1), re.MULTILINE).group(1)
    compiler = re.search(r"clang\+\+|g\+\+", command)
    return compiler.group(0) if compiler else "clang++"


def resolve_compiler(preferred: str) -> str:
    """解析可执行的 clang++：先用给定名字查 PATH，找不到再按兜底列表逐个确认。"""
    found = shutil.which(preferred)
    if found:
        return found
    for candidate in FALLBACK_COMPILERS:
        if shutil.which(candidate):
            return shutil.which(candidate)
    raise ProbeError(f"找不到可用的 clang++（偏好 {preferred}）")


def link(compiler: str, rsp: Path, output: Path, build_dir: Path) -> None:
    # rsp 内的相对路径（lib/、vcpkg_installed/、各目标文件）都以构建目录为基准。
    completed = run([compiler, "-g", "-rdynamic", f"@{rsp}", "-o", str(output)], cwd=build_dir, capture_output=True)
    if completed.returncode != 0:
        raise ProbeError(
            f"链接 {output} 失败（cwd={build_dir}，rsp={rsp}）：\n{completed.stdout[-1000:]}\n{completed.stderr[-4000:]}"
        )


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
    parser.add_argument("--test-build-dir", default=None, help="含测试的构建目录（ctest 元数据与 mc_tests 所在）")
    parser.add_argument("--test-config", default="RelWithDebInfo")
    parser.add_argument("--compiler", default=None, help="链接/编译用的 clang++；缺省取 ninja 规则里的编译器名")
    parser.add_argument("--out", type=Path, required=True)
    parser.add_argument("--clean", action="store_true", help="删除 tests/ci-probes 后退出（调查结束后使用）")
    options = parser.parse_args()

    if options.clean:
        subprocess.run(["git", "rm", "-r", "-q", "--cached", "tests/ci-probes", "scripts/ci/teststart_probe.py"], check=False)
        subprocess.run(["rm", "-rf", "tests/ci-probes", "scripts/ci/teststart_probe.py"], check=False)
        return 0

    build_dir = options.build_dir.resolve()
    # ninja 描述文件与 compile_commands.json 写的是相对构建根（build/）的路径，故切到仓库根；
    # 之后需要相对构建根的步骤（链接）再显式指定 cwd=build_dir。
    os.chdir(build_dir.parent)
    config = options.config
    # 测试可执行文件与目标文件都来自 Test Build（RelWithDebInfo + ASan）；只有链接所需的
    # 静态库来自本目录的构建。两者配置不同，故测试二进制路径单独指定。
    test_build_dir = Path(options.test_build_dir).resolve() if options.test_build_dir else build_dir
    test_binary = test_build_dir / "bin" / options.test_config / "mc_tests"
    work_dir = test_build_dir / "tests" / "unit"
    PROBE_BIN_DIR.mkdir(parents=True, exist_ok=True)

    objects, libraries, flags, compiler = link_inputs(NINJA_FILE.with_name(NINJA_FILE.name.format(config=config)), config)
    main_object = next(
        path for path in objects
        if path.startswith("tests/unit/CMakeFiles/mc_tests.dir/") and path.endswith("/main.cpp.o")
    )
    ordered_objects = [main_object] + [path for path in objects if path != main_object]

    compiler = resolve_compiler(options.compiler or compiler)
    report: dict = {
        "config": config,
        "compiler": compiler,
        "compiler_version": run([compiler, "--version"], capture_output=True).stdout.splitlines()[:1],
        "mc_tests_bytes": test_binary.stat().st_size,
        "launcher": [],
        "teststart": [],
        "ctest": [],
        "direct": [],
    }

    # ---- 1. launcher：进程启动与静态库初始化 ----
    include_flags = [f"-I{REPO_ROOT / 'tests/ci-probes/src/launcher'}", f"-I{REPO_ROOT / 'tests/ci-probes/src/teststart'}"]
    for mode, tag in ((0, "triv"), (1, "fmt"), (2, "spdlog"), (3, "crasher")):
        variant = PROBE_BIN_DIR / f"main_{tag}.o"
        compile_variant(
            COMPILE_DB, "tests/ci-probes/src/launcher/probe_main.cpp", variant,
            include_flags + [f"-DPROBE_MODE={mode}"], compiler,
        )
        rsp = PROBE_BIN_DIR / f"launcher_{tag}.rsp"
        rsp.write_text("\n".join([str(variant)] + libraries) + "\n")
        binary = PROBE_BIN_DIR / f"launcher_{tag}"
        link(compiler, rsp, binary, build_dir)
        report["launcher"].append(
            {"mode": tag, "binary_bytes": binary.stat().st_size, **launch_once(binary, {}, Path("/tmp"), tag)}
        )

    # ---- 2. teststart：测试入口的分阶段装配 ----
    for level in range(5):
        variant = PROBE_BIN_DIR / f"main_level{level}.o"
        compile_variant(COMPILE_DB, "tests/ci-probes/src/teststart/test_main.cpp", variant, include_flags, compiler)
        rsp = PROBE_BIN_DIR / f"teststart_level{level}.rsp"
        members = [str(variant)] + ordered_objects[1:]
        rsp.write_text("\n".join(members + libraries) + "\n")
        binary = PROBE_BIN_DIR / f"teststart_level{level}"
        link(compiler, rsp, binary, build_dir)
        for repeat in range(3):
            sample = launch_once(binary, {"PROBE_LEVEL": str(level), "MC_TIMING_WORLDGEN": "1"}, work_dir, f"level{level}")
            report["teststart"].append({"level": level, "repeat": repeat, **sample})

    # ---- 3. ctest 基线与 4. 直跑对照 ----
    window = "1,,20"
    report["ctest"].append(ctest_step(test_build_dir, options.test_config, window, 2, PROBE_BIN_DIR / "ctest-j2.xml", PROBE_BIN_DIR / "ctest-j2.log"))
    report["ctest"].append(ctest_step(test_build_dir, options.test_config, window, 1, PROBE_BIN_DIR / "ctest-j1.xml", PROBE_BIN_DIR / "ctest-j1.log"))
    names = selection_names(test_build_dir, window, 60)
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