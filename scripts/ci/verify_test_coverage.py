#!/usr/bin/env python3
"""核对 GameTest 或 CTest 分片清单，确保全量测试恰好覆盖一次。"""

import argparse
from collections import Counter
import json
from pathlib import Path
import sys
from xml.sax.saxutils import escape


def verify(directory, count, pattern):
    records = [json.loads(path.read_text()) for path in directory.rglob(pattern)]
    if len(records) != count or sorted(r["shard_index"] for r in records) != list(range(count)):
        raise RuntimeError("Missing or duplicated test shard manifests")
    inventory = records[0]["all_test_ids"]
    if not inventory or len(inventory) != len(set(inventory)):
        raise RuntimeError("Test inventory is empty or contains duplicate identifiers")
    if any(r["shard_count"] != count or r["all_test_ids"] != inventory for r in records):
        raise RuntimeError("Test shards disagree on the complete inventory")
    selected = Counter(identifier for r in records for identifier in r["selected_test_ids"])
    if set(selected) != set(inventory) or any(value != 1 for value in selected.values()):
        raise RuntimeError("Test partition coverage is incomplete or duplicated")
    return {"shards": count, "registered_tests": len(inventory), "exactly_once": True,
            "tests_per_shard": [len(r["selected_test_ids"]) for r in sorted(records, key=lambda r: r["shard_index"])]}


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
        diagnostic.write_text('<testsuites><testsuite name="GameTestCoverage"><testcase name="partition-coverage">'
                              '<error message="' + escape(str(error), {'"': '&quot;'}) + '"/>'
                              '</testcase></testsuite></testsuites>')
        print(f"Test coverage verification failed: {error}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
