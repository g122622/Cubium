#!/usr/bin/env python3
"""核对 GameTest 或 CTest 分片清单，确保全量测试恰好覆盖一次。"""

import argparse
from collections import Counter
import json
from pathlib import Path
import re
import sys
from xml.sax.saxutils import escape


def verify(directory, count, pattern):
    paths = list(directory.rglob(pattern))
    records = [json.loads(path.read_text()) for path in paths]
    if len(records) != count or sorted(r["shard_index"] for r in records) != list(range(count)):
        raise RuntimeError("Missing or duplicated test shard manifests")
    inventory = records[0]["all_test_ids"]
    is_ctest = pattern == "ctest-partition.json"
    if not inventory or (not is_ctest and len(inventory) != len(set(inventory))):
        raise RuntimeError("Test inventory is empty or contains duplicate identifiers")
    if any(r["shard_count"] != count or r["all_test_ids"] != inventory for r in records):
        raise RuntimeError("Test shards disagree on the complete inventory")
    selected = Counter(identifier for r in records for identifier in r["selected_test_ids"])
    if selected != Counter(inventory):
        raise RuntimeError("Test partition coverage is incomplete or duplicated")
    report = {"shards": count, "registered_tests": len(inventory), "exactly_once": True,
              "tests_per_shard": [len(r["selected_test_ids"]) for r in sorted(records, key=lambda r: r["shard_index"])]}
    if is_ctest:
        # CTest 名称可以重复；实际全局编号才标识独立的登记项。核对每个 stride 的执行日志，
        # 避免只比较名称计数而漏掉“同名项重复执行、另一项未执行”的错误。
        for path, record in zip(paths, records):
            starts = re.findall(r"^\s*Start\s+(\d+):\s+(.*?)\s*$",
                                (path.parent / "ctest-output.log").read_text(), re.MULTILINE)
            expected = list(range(record["shard_index"] + 1, len(inventory) + 1, count))
            if sorted(int(number) for number, _ in starts) != expected:
                raise RuntimeError(f"CTest shard {record['shard_index']}: execution indices are missing or duplicated")
            if Counter(name for _, name in starts) != Counter(record["selected_test_ids"]):
                raise RuntimeError(f"CTest shard {record['shard_index']}: execution names disagree with selection")
        report["execution_indices_verified"] = True
        report["duplicate_names"] = len(inventory) - len(set(inventory))
    return report


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--directory", type=Path, required=True)
    parser.add_argument("--shards", type=int, required=True)
    parser.add_argument("--out", type=Path, required=True)
    parser.add_argument("--kind", choices=("gametest", "ctest"), required=True)
    options = parser.parse_args()
    try:
        pattern = "*.partition.json" if options.kind == "gametest" else "ctest-partition.json"
        report = verify(options.directory, options.shards, pattern)
        options.out.write_text(json.dumps(report, indent=2) + "\n")
        print(json.dumps(report))
    except (OSError, ValueError, RuntimeError, KeyError) as error:
        diagnostic = options.directory / (options.kind + "-coverage-error.xml")
        diagnostic.parent.mkdir(parents=True, exist_ok=True)
        suite = "CTestCoverage" if options.kind == "ctest" else "GameTestCoverage"
        diagnostic.write_text(f'<testsuites><testsuite name="{suite}"><testcase name="partition-coverage">'
                              '<error message="' + escape(str(error), {'"': '&quot;'}) + '"/>'
                              '</testcase></testsuite></testsuites>')
        print(f"Test coverage verification failed: {error}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
