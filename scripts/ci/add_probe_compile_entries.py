#!/usr/bin/env python3
"""为 tests/ci-probes 的对照入口补充编译条目（临时工具，见 tests/ci-probes/README.md）。

对照入口与 tests/unit/main.cpp 属于同一个编译目标（宏定义、包含路径一致），但探针源文件
不参与构建，compile_commands.json 中没有它们的条目。本脚本把 main.cpp 的条目克隆给各探针源
文件，并把路径从构建机路径重写为当前工作区路径，供 teststart_probe.py 直接调用编译。
"""

import argparse
import json
import os
from pathlib import Path

PROBES = [
    "tests/ci-probes/src/launcher/probe_main.cpp",
    "tests/ci-probes/src/teststart/test_main.cpp",
]
BUILD_ROOT = "/home/runner/work/Cubium/Cubium"


def rewrite(value: str, root: Path) -> str:
    return value.replace(BUILD_ROOT, str(root))


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("database", type=Path)
    options = parser.parse_args()

    entries = json.loads(options.database.read_text())
    template = next(
        entry for entry in entries
        if entry["file"].endswith("tests/unit/main.cpp") and "mc_tests.dir" in entry.get("output", "")
    )
    root = Path(os.getcwd())
    for probe in PROBES:
        cloned = dict(template)
        cloned["file"] = str((root / probe).resolve())
        cloned["command"] = rewrite(template["command"], root)
        cloned["directory"] = rewrite(template["directory"], root)
        entries.append(cloned)
    options.database.write_text(json.dumps(entries))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())