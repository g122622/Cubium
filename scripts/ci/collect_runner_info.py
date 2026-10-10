#!/usr/bin/env python3
"""采集 nightly 单个 job 的机器配置与实际工具版本，只读取明确列出的系统信息。"""

from __future__ import annotations

import argparse
import json
import os
import platform
import re
import shutil
import subprocess
import sys
from datetime import datetime, timezone
from pathlib import Path


# 仅列出每个任务使用的工具，避免把测试机器预装的编译器误当作产物编译器。
_JOB_TOOLS = {
    "build": ("compiler", "cmake", "ninja", "linker", "ccache", "node", "npm"),
    "fuzz": ("compiler", "cmake", "ninja", "linker", "ccache"),
    "unit-tests": ("ctest",),
    "integrated-tests": ("node", "npm"),
    "e2e-tests": ("node", "npm"),
    "benchmark": ("gh",),
    "report": ("gh",),
}
_COMMANDS = {
    "compiler": "clang++", "linker": "ld.lld", "cmake": "cmake", "ninja": "ninja",
    "ccache": "ccache", "ctest": "ctest", "node": "node", "npm": "npm", "gh": "gh",
}
_BUILD_KEYS = (
    "CMAKE_CXX_COMPILER", "CMAKE_LINKER", "CMAKE_GENERATOR", "CMAKE_BUILD_TYPE",
    "CMAKE_CONFIGURATION_TYPES", "MC_ENABLE_NATIVE_ARCH", "MC_ENABLE_SANITIZERS", "MC_FUZZ_ASAN",
    "CMAKE_CXX_FLAGS_RELWITHDEBINFO", "VCPKG_TARGET_TRIPLET",
)


def _read_text(path: Path) -> str:
    try:
        return path.read_text(encoding="utf-8", errors="replace")
    except OSError:
        return ""


def read_build_config(build_dir: Path | None) -> dict[str, str]:
    """只读取 CMakeCache 中与构建配置有关的字段。"""
    if build_dir is None:
        return {}
    config = {}
    for line in _read_text(build_dir / "CMakeCache.txt").splitlines():
        match = re.match(r"([^:#]+):[^=]+=(.*)", line)
        if match and match.group(1) in _BUILD_KEYS:
            config[match.group(1)] = match.group(2)
    return config


def tool_version(command: str) -> dict[str, str]:
    """用有界子进程读取版本；缺失或失败的工具保留状态，不能使采集卡住。"""
    executable = shutil.which(command)
    if executable is None:
        return {"status": "unavailable"}
    try:
        result = subprocess.run([executable, "--version"], capture_output=True, text=True,
                                errors="replace", timeout=3, check=False)
    except (OSError, subprocess.TimeoutExpired):
        return {"path": executable, "status": "error"}
    if result.returncode != 0:
        return {"path": executable, "status": "error"}
    first_line = next((line.strip() for line in result.stdout.splitlines() if line.strip()), "")
    version = re.search(r"\d+(?:\.\d+)+(?:[-+][\w.]+)?", first_line)
    return {"name": Path(executable).name, "path": executable, "status": "ok", "version": version.group() if version else first_line,
            "description": first_line}


def collect(job: str, build_dir: Path | None) -> dict:
    """采集当前 runner 的硬件、系统、工具版本和可用的 CMake 配置。"""
    cpu_info = _read_text(Path("/proc/cpuinfo"))
    cpu_match = re.search(r"^model name\s*:\s*(.+)$", cpu_info, re.MULTILINE)
    logical_cpus = len(os.sched_getaffinity(0)) if hasattr(os, "sched_getaffinity") else os.cpu_count()
    memory_match = re.search(r"^MemTotal:\s*(\d+)\s+kB", _read_text(Path("/proc/meminfo")), re.MULTILINE)
    try:
        os_release = platform.freedesktop_os_release()
    except OSError:
        os_release = {}
    disk = shutil.disk_usage(Path.cwd())
    build_config = read_build_config(build_dir)
    tools = {}
    for tool in _JOB_TOOLS[job]:
        command = _COMMANDS[tool]
        if tool == "compiler":
            command = build_config.get("CMAKE_CXX_COMPILER", command)
        elif tool == "linker":
            command = build_config.get("CMAKE_LINKER", command)
        tools[tool] = tool_version(command)
    tools["python"] = {"path": sys.executable, "status": "ok", "version": platform.python_version()}
    return {
        "schema_version": 1, "job": job,
        "captured_at": datetime.now(timezone.utc).isoformat(),
        "runner": {"environment": os.environ.get("RUNNER_ENVIRONMENT", ""),
                   "image_os": os.environ.get("ImageOS", ""),
                   "image_version": os.environ.get("ImageVersion", "")},
        "cpu": {"model": cpu_match.group(1).strip() if cpu_match else platform.processor(),
                "logical_cpus": logical_cpus},
        "memory_bytes": int(memory_match.group(1)) * 1024 if memory_match else None,
        "disk": {"total_bytes": disk.total, "free_bytes": disk.free},
        "system": {"os": os_release.get("PRETTY_NAME", platform.system()),
                   "kernel": platform.release(), "architecture": platform.machine(),
                   "libc": " ".join(platform.libc_ver()).strip()},
        "tools": tools, "build": build_config,
        "vcpkg_baseline": os.environ.get("VCPKG_COMMIT", "") if job in ("build", "fuzz") else "",
    }


def main(argv: list[str]) -> int:
    parser = argparse.ArgumentParser(description="Record CI runner hardware and toolchain information")
    parser.add_argument("--job", required=True, choices=_JOB_TOOLS)
    parser.add_argument("--out", type=Path, required=True)
    parser.add_argument("--build-dir", type=Path, help="Read the configured compiler and options from CMakeCache.txt")
    args = parser.parse_args(argv)
    info = collect(args.job, args.build_dir)
    args.out.parent.mkdir(parents=True, exist_ok=True)
    args.out.write_text(json.dumps(info, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    print(f"Runner configuration recorded: {args.job} -> {args.out}")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
