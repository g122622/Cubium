#!/usr/bin/env python3
"""容量紧张时按最近访问时间清理 Nightly 缓存，为即将保存的缓存留出空间。"""

import argparse
import json
import subprocess
import sys

from prune_caches import _gh_api_delete, _gh_api_list


PREFIXES = ("ccache-linux-nightly-", "ccache-linux-release-", "ccache-linux-fuzz-", "vcpkg-linux-")


def eviction_plan(entries, max_bytes, reserve_bytes, protected_run, protected_keys):
    total = sum(int(entry.get("size_in_bytes", 0)) for entry in entries)
    candidates = [entry for entry in entries
                  if str(entry.get("key", "")).startswith(PREFIXES)
                  and entry.get("key") not in protected_keys
                  and not (protected_run and str(entry.get("key", "")).endswith("-" + protected_run))]
    candidates.sort(key=lambda entry: (entry.get("last_accessed_at") or entry.get("created_at", ""),
                                       int(entry["id"])))
    victims = []
    for entry in candidates:
        if total + reserve_bytes <= max_bytes:
            break
        victims.append(entry)
        total -= int(entry.get("size_in_bytes", 0))
    return victims, total


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--repo", required=True)
    parser.add_argument("--max-bytes", type=int, required=True)
    parser.add_argument("--reserve-bytes", type=int, required=True)
    parser.add_argument("--protected-run", required=True)
    parser.add_argument("--protected-key", action="append", default=[])
    parser.add_argument("--dry-run", action="store_true")
    args = parser.parse_args()
    if args.max_bytes <= 0 or args.reserve_bytes < 0:
        parser.error("Cache capacity must be positive and reservation nonnegative")
    try:
        entries = _gh_api_list(args.repo)
        # tar 元数据和上传格式也占空间，给即将保存的缓存增加 64 MiB 余量。
        reserve = args.reserve_bytes + (64 * 2**20 if args.reserve_bytes else 0)
        victims, remaining = eviction_plan(entries, args.max_bytes, reserve, args.protected_run,
                                           set(args.protected_key))
        print(f"Cache pressure: {len(victims)} eviction candidates, {reserve} bytes reserved")
        for entry in victims:
            print(f"{'Would evict' if args.dry_run else 'Evicting'} {entry['key']} ({entry['size_in_bytes']} bytes)")
            if not args.dry_run:
                _gh_api_delete(args.repo, int(entry["id"]))
        if remaining + reserve > args.max_bytes:
            print("::warning::Protected or unrelated caches still exceed the cache capacity target")
    except (OSError, RuntimeError, ValueError, subprocess.SubprocessError, json.JSONDecodeError) as error:
        print(f"::warning::Cache capacity cleanup failed: {error}", file=sys.stderr)
    return 0


if __name__ == "__main__":
    sys.exit(main())
