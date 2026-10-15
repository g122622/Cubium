#!/usr/bin/env python3
"""Emit the added-line ranges of a commit range as a TSV for clang-tidy filtering.

Output format (one record per range, tab-separated):

    <repo-relative-path>\\t<first-line>\\t<last-line>

Only files that survive ``--diff-filter=ACMR`` are considered, so deleted files
never contribute ranges. The ranges are the line numbers **in the new file**, i.e.
exactly the lines a reviewer sees as added in the pull request.

``scripts/tidy/tidy.sh`` consumes this file to drop diagnostics that fall outside
the changed lines, which keeps the pull-request gate focused on the diff instead of
re-reporting the whole historical backlog of a touched file.
"""

from __future__ import annotations

import argparse
import re
import subprocess
import sys
from pathlib import Path

# A hunk header: "@@ -<old_start>[,<old_count>] +<new_start>[,<new_count>] @@".
_HUNK_RE = re.compile(r"^@@ -\d+(?:,\d+)? \+(\d+)(?:,(\d+))? @@")

# C++ translation units and headers. Kept in sync with the file selection in ci.yml.
_SOURCE_SUFFIXES = (".cpp", ".cc", ".cxx", ".c++", ".hpp", ".h", ".hh", ".hxx")


def _is_cpp(path: str) -> bool:
    return path.lower().endswith(_SOURCE_SUFFIXES)


def collect_ranges(base: str, head: str) -> list[tuple[str, int, int]]:
    """Return the added-line ranges between ``base`` and ``head``."""
    # -U0 keeps hunks minimal so each range is exactly the added block; --no-color and
    # --no-ext-diff keep the output machine-readable regardless of local git config.
    completed = subprocess.run(
        ["git", "diff", "-U0", "--no-color", "--no-ext-diff", "--diff-filter=ACMR",
         f"{base}...{head}"],
        capture_output=True,
        check=False,
    )
    if completed.returncode != 0:
        raise RuntimeError(
            f"git diff failed ({completed.returncode}): "
            f"{completed.stderr.decode('utf-8', 'replace').strip()}"
        )

    text = completed.stdout.decode("utf-8", "replace")
    ranges: list[tuple[str, int, int]] = []
    current: str | None = None

    for line in text.splitlines():
        if line.startswith("+++ "):
            # "+++ b/path/to/file.cpp" — strip the a/ b/ prefix; /dev/null marks a deletion.
            target = line[4:].strip()
            if target == "/dev/null":
                current = None
                continue
            if target.startswith("b/"):
                target = target[2:]
            current = target if _is_cpp(target) else None
            continue

        if current is None:
            continue

        match = _HUNK_RE.match(line)
        if match is None:
            continue

        start = int(match.group(1))
        count = int(match.group(2) or "1")
        # A zero-count hunk means a pure deletion: no added line to report.
        if count > 0:
            ranges.append((current, start, start + count - 1))

    return ranges


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--base", required=True, help="base commit (exclusive)")
    parser.add_argument("--head", required=True, help="head commit (inclusive)")
    parser.add_argument("--output", type=Path, required=True,
                        help="destination TSV; also printed to stdout when '-' is given")
    args = parser.parse_args()

    try:
        ranges = collect_ranges(args.base, args.head)
    except RuntimeError as exc:
        print(f"ERROR: {exc}", file=sys.stderr)
        return 1

    payload = "".join(f"{path}\t{start}\t{end}\n" for path, start, end in ranges)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    # newline="\n" keeps the TSV LF-terminated on Windows too; a stray CR would end up
    # inside the numeric field and break the arithmetic comparison in tidy.sh.
    with args.output.open("w", encoding="utf-8", newline="\n") as handle:
        handle.write(payload)
    print(f"Recorded {len(ranges)} changed line range(s) across "
          f"{len({path for path, _, _ in ranges})} file(s).")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
