#!/usr/bin/env python3
"""核验独立 pococc 测试构建的配置、无 PCH 编译命令和严格分发记录。"""

from __future__ import annotations

import argparse
from collections import Counter
import json
import re
import shlex
import sys
from pathlib import Path

from prepare_build_artifacts import verify_configuration


def verify_configuration_without_pch(build_dir: Path) -> int:
    """检查真实 cache 和编译命令，不能仅相信 configure 的命令行。"""
    config = {}
    for line in (build_dir / "CMakeCache.txt").read_text(encoding="utf-8").splitlines():
        match = re.match(r"([^:#]+):[^=]+=(.*)", line)
        if match:
            config[match.group(1)] = match.group(2)
    verify_configuration(config, "test")
    for key, expected in {
        "CMAKE_DISABLE_PRECOMPILE_HEADERS": "ON",
        "CMAKE_CXX_SCAN_FOR_MODULES": "OFF",
    }.items():
        if config.get(key) != expected:
            raise RuntimeError(f"Incorrect pococc configuration: {key}={config.get(key)!r}")
    commands = json.loads((build_dir / "compile_commands.json").read_text(encoding="utf-8"))
    if not commands:
        raise RuntimeError("Compile database is empty")
    for entry in commands:
        arguments = entry["arguments"] if "arguments" in entry else shlex.split(entry["command"])
        if any(argument in ("-include-pch", "-emit-pch", "-fpch-preprocess") or "cmake_pch" in argument
               for argument in arguments):
            raise RuntimeError(f"PCH is still enabled for {entry['file']}")
    return len(commands)


def verify_journal(path: Path) -> dict[str, object]:
    """允许已记录的本地路由和缓存命中，但不允许失败或通信错误回退。"""
    routes: Counter[str] = Counter()
    reasons: Counter[str] = Counter()
    for number, line in enumerate(path.read_text(encoding="utf-8").splitlines(), 1):
        fields = line.split("\t")
        if len(fields) != 7 or fields[1] not in ("local", "distcc"):
            raise RuntimeError(f"Malformed pococc journal record at line {number}")
        if fields[2] != "0" or fields[6] != "strict":
            raise RuntimeError(f"Failed or non-strict pococc compilation at line {number}")
        routes[fields[1]] += 1
        reasons[fields[5]] += 1
    if routes["distcc"] == 0:
        raise RuntimeError("Cubium did not submit any actual compilation to distcc")
    return {
        "distcc_submissions": routes["distcc"],
        "local_invocations": routes["local"],
        "local_or_remote_reasons": dict(reasons),
        "strict": True,
        "pch": False,
    }


def main(argv: list[str]) -> int:
    """输出独立构建报告，配置预检阶段不要求对象编译已经发生。"""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build-dir", type=Path, required=True)
    parser.add_argument("--configuration-only", action="store_true")
    args = parser.parse_args(argv)
    try:
        count = verify_configuration_without_pch(args.build_dir)
        if args.configuration_only:
            print(f"Verified no-PCH sanitizer configuration: {count} compile commands across all configurations")
            return 0
        report = verify_journal(args.build_dir / "pococc.tsv")
        report["compile_database_entries_all_configurations"] = count
        report["cache_hits_bypass_pococc"] = True
        (args.build_dir / "pococc-build.json").write_text(
            json.dumps(report, indent=2) + "\n", encoding="utf-8")
        print(json.dumps(report, indent=2))
    except (OSError, RuntimeError, ValueError, KeyError) as error:
        print(f"pococc build verification failed: {error}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
