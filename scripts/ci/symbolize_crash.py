#!/usr/bin/env python3
"""把崩溃日志里的 `binary(+0xOFFSET)` 栈帧符号化为「函数名 + 源文件:行号」。

## 为什么需要这个脚本

`CrashHandler` 在 Linux 上用 `backtrace_symbols()` 输出调用栈，而该函数只查**动态
符号表**（`.dynsym`）。可执行文件的函数默认只进 `.symtab`，于是 CI 上的崩溃日志长这样：

    Reason: SIGSEGV - Segmentation fault at address 0x38
    Stack trace:
      [ 0] /lib/x86_64-linux-gnu/libc.so.6(+0x45330) [0x7fadb5845330]
      [ 1] __dynamic_cast
      [ 2] /home/runner/.../minecraft-server(+0xdcb55c) [0x55ba8c22755c]
      [ 3] /home/runner/.../minecraft-server(+0xc297ef) [0x55ba8c0857ef]
      ...

只有裸偏移，必须事后拿二进制逐个换算才能定位，排查成本极高。

`CMakeLists.txt` 已加 `-rdynamic` 让全局符号进入 `.dynsym`（多数帧因此直接带名字），
但**匿名 namespace / static 函数与内联帧**仍只会显示偏移。本脚本用带调试信息的二进制
（CI 的 `build-artifacts` 里那份，未 strip）做**事后符号化**，把这些偏移还原成
「函数名 + 源文件:行号」，彻底解决可读性问题。

## 用法

    python3 scripts/ci/symbolize_crash.py \
        --log integrated-tests.log \
        --binary build/bin/RelWithDebInfo/minecraft-server

默认把符号化结果打到 stdout；`--in-place` 可原地改写日志文件（便于 artifact 里带的就是
可读栈）。找不到符号化工具或二进制时**不报错**（降级为原样输出），因为本脚本运行在 CI 的
收尾步骤，自身失败不应掩盖真正的崩溃信息。
"""

from __future__ import annotations

import argparse
import re
import shutil
import subprocess
import sys
from pathlib import Path

# 崩溃栈行形如：
#   [ 2] /path/to/minecraft-server(+0xdcb55c) [0x55ba8c22755c]
# 也兼容不带地址的形式：  [ 2] /path/to/bin(+0xdcb55c)
_FRAME_LINE = re.compile(
    r"^(?P<indent>\s*)\[(?P<idx>\s*\d+)\]\s+"
    r"(?P<binary>[^\s(]+)\(\+0x(?P<offset>[0-9a-fA-F]+)\)"
    r"(?:\s+\[(?P<addr>0x[0-9a-fA-F]+)\])?\s*$"
)

# 崩溃块的起点：其后到空行/结束为止是调用栈。
_CRASH_MARKER = "Stack trace:"


def _find_symbolizer() -> list[str] | None:
    """挑一个可用的符号化工具，返回调用它的命令前缀（不含参数）。

    优先 llvm-symbolizer：它能展开**内联帧**（一个地址对应多层源码位置），
    输出形如
        mc::CombatTracker::getBestAttacker() const
        /path/CombatTracker.cpp:176:14
        mc::CombatTracker::getBestAttackerLiving() const
        /path/CombatTracker.cpp:208:24
    这正是定位模板/内联密集的 C++ 崩溃所需要的。

    退化到 addr2line（binutils，ubuntu runner 自带），它只能给出最内层位置，
    但对「哪个函数崩了」这个首要问题已经够用。
    """
    for name in ("llvm-symbolizer", "llvm-symbolizer-22", "llvm-symbolizer-18"):
        path = shutil.which(name)
        if path:
            return [path]
    # llvm 的 apt 包把工具装在 /usr/lib/llvm-*/bin 下，未必在 PATH 里。
    for pattern in ("/usr/lib/llvm-*/bin/llvm-symbolizer", "/usr/bin/llvm-symbolizer-*"):
        for candidate in sorted(Path("/").glob(pattern.lstrip("/"))):
            if candidate.is_file():
                return [str(candidate)]
    path = shutil.which("addr2line")
    if path:
        return [path, "-f", "-C", "-i"]  # -f 输出函数名，-C demangle，-i 展开内联
    return None


def _symbolize_one(cmd: list[str], binary: Path, offset: str) -> list[str]:
    """符号化单个偏移，返回若干行（llvm-symbolizer 可能因内联返回多组）。"""
    try:
        if Path(cmd[0]).name.startswith("addr2line"):
            proc = subprocess.run(
                [*cmd, "-e", str(binary), f"0x{offset}"],
                capture_output=True, text=True, timeout=30,
            )
            lines = [ln for ln in proc.stdout.splitlines() if ln and ln != "??"]
            # addr2line 每两行一组：函数名 / 文件:行号
            return lines
        proc = subprocess.run(
            [*cmd, "--obj", str(binary), "--functions=linkage", "--inlines", f"0x{offset}"],
            capture_output=True, text=True, timeout=30,
        )
        return [ln for ln in proc.stdout.splitlines() if ln]
    except (OSError, subprocess.SubprocessError):
        return []


def symbolize_log(text: str, binary: Path | None, cmd: list[str] | None) -> tuple[str, int]:
    """把日志里的崩溃栈符号化。

    返回 (新文本, 成功符号化的帧数)。未命中崩溃块或工具不可用时原样返回。
    """
    if cmd is None or binary is None or not binary.is_file():
        return text, 0

    lines = text.splitlines()
    out: list[str] = []
    in_stack = False
    resolved = 0

    for line in lines:
        if _CRASH_MARKER in line:
            in_stack = True
            out.append(line)
            continue

        if in_stack:
            match = _FRAME_LINE.match(line)
            if match is None:
                # 栈块里也可能出现非帧行（如 `[ 1] __dynamic_cast`——名字已在日志里、
                # 没有 (+0x...) 偏移）。这类行原样保留，且**不结束**栈块：结束只由
                # 空行或分隔线（=====）决定，否则一行未匹配就会截断后面所有帧。
                if line.strip() == "" or line.lstrip().startswith("===="):
                    in_stack = False
                out.append(line)
                continue

            frame_binary = Path(match.group("binary"))
            # 只符号化指向本项目二进制的帧：libc 等系统库的偏移与本项目二进制无关。
            if frame_binary.name != binary.name:
                out.append(line)
                continue

            syms = _symbolize_one(cmd, binary, match.group("offset"))
            if not syms:
                out.append(line)
                continue

            resolved += 1
            out.append(f"{match.group('indent')}[{match.group('idx')}] "
                       f"{syms[0]}  (was +0x{match.group('offset')})")
            # 内联展开与文件:行号作为缩进子行，保持与首行对齐。
            pad = match.group("indent") + " " * 6
            for extra in syms[1:]:
                out.append(f"{pad}{extra}")
            continue

        out.append(line)

    new_text = "\n".join(out)
    if text.endswith("\n"):
        new_text += "\n"
    return new_text, resolved


def main(argv: list[str]) -> int:
    parser = argparse.ArgumentParser(description="符号化崩溃日志中的调用栈")
    parser.add_argument("--log", required=True, type=Path, help="崩溃日志路径")
    parser.add_argument("--binary", type=Path, default=None,
                        help="带调试信息的可执行文件（CI 中即 build-artifacts 里的 minecraft-server）")
    parser.add_argument("--in-place", action="store_true",
                        help="原地改写日志文件，而不是打印到 stdout")
    parser.add_argument("--out", type=Path, default=None, help="把结果写到该文件")
    args = parser.parse_args(argv)

    if not args.log.is_file():
        print(f"[symbolize] 日志不存在：{args.log}", file=sys.stderr)
        return 0

    text = args.log.read_text(encoding="utf-8", errors="replace")

    if _CRASH_MARKER not in text:
        # 没有崩溃栈，无需符号化。这是正常路径（未崩溃的运行）。
        if args.out:
            args.out.write_text(text, encoding="utf-8")
        elif not args.in_place:
            print(text, end="")
        return 0

    cmd = _find_symbolizer()
    if cmd is None:
        print("[symbolize] 未找到 llvm-symbolizer / addr2line，跳过符号化（栈仍为裸偏移）",
              file=sys.stderr)
        if args.out:
            args.out.write_text(text, encoding="utf-8")
        elif not args.in_place:
            print(text, end="")
        return 0

    new_text, resolved = symbolize_log(text, args.binary, cmd)
    print(f"[symbolize] 符号化 {resolved} 个栈帧（工具：{cmd[0]}，二进制：{args.binary}）",
          file=sys.stderr)

    if args.in_place:
        args.log.write_text(new_text, encoding="utf-8")
    elif args.out:
        args.out.write_text(new_text, encoding="utf-8")
    else:
        print(new_text, end="")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
