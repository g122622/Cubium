#!/usr/bin/env python3
"""Create temporary compile commands for changed headers from nearby C++ TUs."""

from __future__ import annotations

import argparse
import json
import os
import shlex
import sys
from pathlib import Path

_SOURCE_SUFFIXES = {".cc", ".cpp", ".cxx", ".c++"}


def _absolute(path: str, directory: str) -> Path:
    value = Path(path)
    if not value.is_absolute():
        value = Path(directory) / value
    return value.resolve()


def _command_arguments(entry: dict) -> list[str]:
    if isinstance(entry.get("arguments"), list):
        return [str(argument) for argument in entry["arguments"]]
    command = entry.get("command")
    if not isinstance(command, str):
        raise ValueError("compile command has neither arguments nor command")
    return shlex.split(command)


def _header_command(entry: dict, header: Path) -> list[str]:
    directory = str(Path(entry["directory"]).resolve())
    source = _absolute(entry["file"], directory)
    source_key = os.path.normcase(str(source))
    original = _command_arguments(entry)
    arguments: list[str] = []
    index = 0
    while index < len(original):
        argument = original[index]
        if argument in {"-o", "-MF", "-MT", "-MQ"}:
            index += 2
            continue
        if argument in {"-c", "-MD", "-MMD", "-MP"}:
            index += 1
            continue
        if argument.startswith("-o") and len(argument) > 2:
            index += 1
            continue
        try:
            candidate = os.path.normcase(str(_absolute(argument, directory)))
        except (OSError, ValueError):
            candidate = ""
        if candidate == source_key:
            index += 1
            continue
        arguments.append(argument)
        index += 1

    arguments.extend(("-x", "c++", "-Wno-pragma-once-outside-header", str(header)))
    return arguments


def _common_parent_parts(left: Path, right: Path) -> int:
    left_parts = left.parts
    right_parts = right.parts
    count = 0
    for left_part, right_part in zip(left_parts, right_parts):
        if os.path.normcase(left_part) != os.path.normcase(right_part):
            break
        count += 1
    return count


def create_commands(compdb_path: Path, headers: list[Path], output_path: Path) -> None:
    entries = json.loads(compdb_path.read_text(encoding="utf-8"))
    candidates = []
    for entry in entries:
        directory = entry.get("directory")
        source = entry.get("file")
        if not isinstance(directory, str) or not isinstance(source, str):
            continue
        absolute_source = _absolute(source, directory)
        if absolute_source.suffix.lower() not in _SOURCE_SUFFIXES:
            continue
        candidates.append((absolute_source, entry))

    if not candidates:
        raise ValueError(f"No C++ translation units found in {compdb_path}")

    commands = []
    for header in headers:
        absolute_header = header.resolve()
        _, template = max(
            candidates,
            key=lambda candidate: _common_parent_parts(candidate[0].parent, absolute_header.parent),
        )
        commands.append({
            "directory": str(Path(template["directory"]).resolve()),
            "file": str(absolute_header),
            "arguments": _header_command(template, absolute_header),
        })

    output_path.parent.mkdir(parents=True, exist_ok=True)
    output_path.write_text(json.dumps(commands, indent=2) + "\n", encoding="utf-8")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--compile-commands", type=Path, required=True)
    parser.add_argument("--headers-list", type=Path, required=True,
                        help="NUL-separated list of changed headers")
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()

    try:
        raw_headers = args.headers_list.read_bytes().split(b"\0")
        headers = [Path(item.decode("utf-8")) for item in raw_headers if item]
        if not headers:
            raise ValueError("The changed-header list is empty")
        create_commands(args.compile_commands, headers, args.output)
    except (OSError, UnicodeDecodeError, json.JSONDecodeError, KeyError, ValueError) as exc:
        print(f"ERROR: cannot create header compile commands: {exc}", file=sys.stderr)
        return 1
    print(f"Created temporary compile commands for {len(headers)} changed header(s).")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
