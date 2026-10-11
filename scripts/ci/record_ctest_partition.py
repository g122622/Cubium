#!/usr/bin/env python3
"""归档真实 CTest 查询得到的全量与分片用例清单。"""

import argparse
import json
from pathlib import Path


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ("all", "selected", "out"):
        parser.add_argument("--" + name, type=Path, required=True)
    parser.add_argument("--index", type=int, required=True)
    parser.add_argument("--count", type=int, required=True)
    options = parser.parse_args()
    inventory = [record["name"] for record in json.loads(options.all.read_text())["tests"]]
    selected = [record["name"] for record in json.loads(options.selected.read_text())["tests"]]
    options.out.write_text(json.dumps({"shard_index": options.index, "shard_count": options.count,
                                      "all_test_ids": sorted(inventory), "selected_test_ids": sorted(selected)}, indent=2) + "\n")


if __name__ == "__main__":
    main()
