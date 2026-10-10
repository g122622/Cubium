#!/usr/bin/env python3
"""核验测试/发布构建的 ELF 契约，并收集实际依赖的非系统动态库。"""

from __future__ import annotations

import argparse
import os
import re
import shutil
import subprocess
import sys
from pathlib import Path

from collect_runner_info import read_build_config


def _run(command: list[str]) -> str:
    result = subprocess.run(command, capture_output=True, text=True, timeout=60, check=False)
    if result.returncode != 0:
        raise RuntimeError(f"Command failed: {' '.join(command)}: {result.stderr.strip()}")
    return result.stdout


def verify_configuration(config: dict[str, str], variant: str) -> None:
    """核验实际配置，避免发布或 benchmark 混用测试构建。"""
    expected = {"MC_ENABLE_NATIVE_ARCH": "OFF"}
    if variant == "release":
        expected.update({"CMAKE_BUILD_TYPE": "Release", "MC_ENABLE_SANITIZERS": "OFF",
                         "MC_ENABLE_TRACING": "OFF", "MC_ENABLE_TRACY": "OFF", "MC_ENABLE_MEMORY": "OFF"})
        for key in ("CMAKE_C_FLAGS_RELEASE", "CMAKE_CXX_FLAGS_RELEASE"):
            if "-g0" not in config.get(key, "").split():
                raise RuntimeError(f"Release build must disable debug information: {key}")
    else:
        expected.update({"CMAKE_BUILD_TYPE": "RelWithDebInfo", "MC_ENABLE_SANITIZERS": "ON"})
        for key in ("CMAKE_C_FLAGS", "CMAKE_CXX_FLAGS"):
            flags = config.get(key, "").split()
            if not {"-fsanitize=address,undefined", "-fno-sanitize-recover=all", "-fno-omit-frame-pointer"}.issubset(flags):
                raise RuntimeError(f"Test build must instrument C and C++ with ASan and UBSan: {key}")
    for key, value in expected.items():
        if config.get(key) != value:
            raise RuntimeError(f"Incorrect {variant} configuration: {key}={config.get(key)!r}, expected {value}")


def verify_binary(binary: Path, variant: str) -> None:
    """独立检查二进制本身的调试节和 sanitizer 符号，不能只相信 preset。"""
    sections = _run(["readelf", "--sections", "--wide", str(binary)])
    symbols = _run(["readelf", "--dyn-syms", "--wide", str(binary)])
    debug = re.search(r"\.(?:z?debug_\w+|gnu_debug(?:link|altlink|data)|gdb_index)\b", sections)
    asan = re.search(r"\b__asan_init\b", symbols)
    ubsan = re.search(r"\b__ubsan_handle_\w+", symbols)
    if variant == "release":
        if debug or asan or ubsan:
            raise RuntimeError(f"Release binary contains debug information or sanitizer runtime: {binary}")
    elif not (debug and asan and ubsan):
        raise RuntimeError(f"Test binary must retain debug information and ASan/UBSan symbols: {binary}")


def collect_libraries(binary: Path, build_dir: Path, library_dir: Path, variant: str) -> str:
    """仅打包实际加载的 vcpkg/Clang 运行库；系统 libc 和加载器由目标系统提供。"""
    output = _run(["ldd", str(binary)])
    if "not found" in output:
        raise RuntimeError(f"Unresolved shared library dependencies for {binary}: {output}")
    roots = [build_dir.resolve() / "vcpkg_installed"]
    if os.environ.get("VCPKG_ROOT"):
        roots.append(Path(os.environ["VCPKG_ROOT"]).resolve() / "installed")
    library_dir.mkdir(parents=True, exist_ok=True)
    for line in output.splitlines():
        match = re.match(r"\s*(\S+)\s+=>\s+(\S+)\s+\(", line)
        if not match:
            continue
        name, source = match.group(1), Path(match.group(2)).resolve()
        if not any(source.is_relative_to(root) for root in roots) and not name.startswith("libclang_rt."):
            continue
        target = library_dir / name
        shutil.copy2(source, target)
        if variant == "release":
            _run(["llvm-strip", "--strip-unneeded", str(target)])
            verify_binary(target, "release")
    return output


def main(argv: list[str]) -> int:
    parser = argparse.ArgumentParser(description="Verify and prepare CI build artifacts")
    parser.add_argument("--variant", choices=("release", "test"), required=True)
    parser.add_argument("--build-dir", type=Path, required=True)
    parser.add_argument("--binary", type=Path, action="append", required=True)
    parser.add_argument("--library-dir", type=Path, required=True)
    parser.add_argument("--report", type=Path, required=True)
    args = parser.parse_args(argv)
    try:
        verify_configuration(read_build_config(args.build_dir), args.variant)
        lines = []
        for binary in args.binary:
            if args.variant == "release":
                _run(["llvm-strip", "--strip-all", str(binary)])
            verify_binary(binary, args.variant)
            lines.extend([f"Verified {args.variant} binary: {binary}",
                          collect_libraries(binary, args.build_dir, args.library_dir, args.variant)])
        args.report.write_text("\n".join(lines) + "\n", encoding="utf-8")
    except (OSError, RuntimeError, subprocess.TimeoutExpired) as exc:
        print(f"Build artifact verification failed: {exc}", file=sys.stderr)
        return 1
    print(f"Build artifact verification passed: {args.variant}")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
