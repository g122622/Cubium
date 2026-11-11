#!/usr/bin/env python3
"""在 CI 验证 Ninja 的真实 launcher，以及 ccache 对 pococc stderr 的重放。"""

from __future__ import annotations

import argparse
import json
import os
from pathlib import Path
import shlex
import subprocess
import tempfile

from pococc_performance import parse_ccache_stats


def verify_launcher(build_dir: Path, variant: str, compiler: Path) -> str:
    """检查 Ninja 生成的对象编译命令；compile_commands 本身不包含 launcher。"""
    configuration = "Release" if variant == "release" else "RelWithDebInfo"
    entries = json.loads((build_dir / "compile_commands.json").read_text())
    for entry in entries:
        if not entry["file"].endswith("/HangingRootsBlock.cpp"):
            continue
        arguments = entry.get("arguments") or shlex.split(entry["command"])
        target = arguments[arguments.index("-o") + 1]
        if f"/{configuration}/" in target:
            break
    else:
        raise RuntimeError("No representative object command found")
    result = subprocess.run(
        ["ninja", "-C", str(build_dir), "-f", f"build-{configuration}.ninja",
         "-t", "commands", target], check=True, capture_output=True, text=True,
    )
    command = result.stdout.strip().splitlines()[-1]
    arguments = shlex.split(command)
    cache_index = next((index for index, argument in enumerate(arguments)
                        if Path(argument).name == "ccache"), -1)
    if cache_index < 0 or cache_index + 1 >= len(arguments) \
            or Path(arguments[cache_index + 1]).resolve() != compiler.resolve():
        raise RuntimeError(f"Ninja does not invoke ccache before the real compiler: {command}")
    return command


def verify_replay(pococc: Path, compiler: Path, out: Path) -> dict:
    """隔离缓存和日志，冷跑一次、暖跑一次，以真实路由记录裁决 stderr 的含义。"""
    out.mkdir(parents=True, exist_ok=True)
    # worker 计量将 pococc-probe-* 路径归入探针，不掺入项目的编译及缓存统计。
    with tempfile.TemporaryDirectory(prefix="pococc-probe-ccache-") as temporary:
        root = Path(temporary)
        source = root / "probe.cpp"
        source.write_text("int ccache_replay_probe() { return 42; }\n")
        journal = out / "journal.tsv"
        journal.unlink(missing_ok=True)
        environment = dict(os.environ, CCACHE_DIR=str(root / "cache"),
                           CCACHE_CONFIGPATH=str(root / "ccache.conf"),
                           CCACHE_PREFIX=str(pococc.with_name("pococc-cc")),
                           DISTCC_LOG=str(out / "distcc.log"))
        (root / "ccache.conf").write_text("max_size = 10M\n")
        command = [str(pococc), "run", "--hosts", os.environ["POCOCC_CI_HOSTS"],
                   "--strict", "--journal", str(journal), "--", "ccache", str(compiler),
                   "-c", str(source), "-o", str(root / "probe.o")]
        evidence = {}
        previous_routes = 0
        for stage in ("cold", "warm"):
            environment["CCACHE_LOGFILE"] = str(out / f"{stage}-ccache.log")
            result = subprocess.run(command, env=environment, capture_output=True, text=True)
            (out / f"{stage}-stderr.txt").write_text(result.stderr)
            (out / f"{stage}-stdout.txt").write_text(result.stdout)
            if result.returncode:
                raise RuntimeError(f"{stage} ccache probe failed: {result.stderr}")
            stats = subprocess.run(["ccache", "--print-stats"], env=environment,
                                   check=True, capture_output=True, text=True)
            (out / f"{stage}-stats.txt").write_text(stats.stdout)
            counters = parse_ccache_stats(stats.stdout)
            rows = journal.read_text().splitlines()
            if any(row.split("\t")[1:3] != ["distcc", "0"] for row in rows):
                raise RuntimeError("Probe did not use successful remote compilation")
            evidence[stage] = {
                "hits": counters.get("direct_cache_hit", 0) + counters.get("preprocessed_cache_hit", 0),
                "misses": counters["cache_miss"],
                "new_remote_invocations": len(rows) - previous_routes,
                "submitting_lines": result.stderr.count("[pococc] info: Submitting"),
            }
            previous_routes = len(rows)
        if evidence["cold"] != {"hits": 0, "misses": 1, "new_remote_invocations": 1,
                                "submitting_lines": 1}:
            raise RuntimeError(f"Unexpected cold probe evidence: {evidence}")
        if evidence["warm"] != {"hits": 1, "misses": 1, "new_remote_invocations": 0,
                                "submitting_lines": 1}:
            raise RuntimeError(f"Unexpected warm probe evidence: {evidence}")
        evidence["stderr_replayed"] = (out / "cold-stderr.txt").read_bytes() == (out / "warm-stderr.txt").read_bytes()
        if not evidence["stderr_replayed"]:
            raise RuntimeError("Warm cache did not reproduce the cached compiler stderr")
    return evidence


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build-dir", required=True, type=Path)
    parser.add_argument("--variant", required=True, choices=("test", "release", "fuzz"))
    parser.add_argument("--pococc", required=True, type=Path)
    parser.add_argument("--compiler", required=True, type=Path)
    parser.add_argument("--out", required=True, type=Path)
    options = parser.parse_args()
    options.out = options.out.resolve()
    options.out.mkdir(parents=True, exist_ok=True)
    command = verify_launcher(options.build_dir, options.variant, options.compiler)
    (options.out / "ninja-command.txt").write_text(command + "\n")
    print(f"Verified Ninja launcher: {command}", flush=True)
    evidence = verify_replay(options.pococc, options.compiler, options.out)
    (options.out / "replay.json").write_text(json.dumps(evidence, indent=2) + "\n")
    print(json.dumps(evidence, indent=2), flush=True)
    print("ccache replays pococc stderr on hits; count real submissions in pococc.tsv.")


if __name__ == "__main__":
    main()
