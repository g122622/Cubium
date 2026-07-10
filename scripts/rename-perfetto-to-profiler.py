#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
rename-perfetto-to-profiler.py

把 src/common/perfetto 改名为 profiler，引入 Tracy 双轨前的纯机械改名。
执行顺序：先 git mv 目录/文件，再做文本替换（先长后短，避免子串误伤）。

排除目录：third_party/、build/、.git/、vcpkg_installed/、third_party/tracy/
作用目录：src/、tests/、benchmark/、docs/、.claude/、.github/、根 CMakeLists.txt

【绝不改动】：
  ::perfetto::        （第三方 SDK 命名空间）
  <perfetto.h>        （第三方 SDK 头文件）
  perfetto_sdk        （CMake target）
  PERFETTO_           （第三方 SDK 宏前缀，如 PERFETTO_DEFINE_CATEGORIES）
  third_party/perfetto
  mc::trace           （TraceEvents 枚举树命名空间，保持不变）
  *.perfetto-trace    （输出文件后缀）
  自然语言 "Perfetto"  （文档/注释里描述后端的，保留）

使用：python scripts/rename-perfetto-to-profiler.py
在项目根目录执行。可重复运行的幂等性不保证——设计为一次性执行。
"""

import os
import re
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
os.chdir(ROOT)

# ----------------------------------------------------------------------------
# 工具函数
# ----------------------------------------------------------------------------

def git(*args):
    """执行 git 命令，失败则报错退出。"""
    result = subprocess.run(["git", *args], capture_output=True, text=True)
    if result.returncode != 0:
        print(f"[git ERROR] git {' '.join(args)}\n{result.stderr}", file=sys.stderr)
        sys.exit(1)
    return result.stdout


def git_mv(src, dst):
    """git mv，源不存在时跳过。"""
    if os.path.exists(src):
        print(f"[git mv] {src} -> {dst}")
        git("mv", src, dst)
    else:
        print(f"[skip mv] 源不存在: {src}")


# ----------------------------------------------------------------------------
# 文本替换规则
# ----------------------------------------------------------------------------
# 顺序极其重要：先长后短。每条 (pattern, replacement, is_regex)。
# 用 regex 配合词边界，避免误伤 ::perfetto:: / perfetto_sdk / PERFETTO_ 等。
#
# 关键设计：
# - mc::perfetto::XXX 规则用字面量精确匹配（带 mc:: 前缀，天然不碰 ::perfetto::）
# - 裸 PerfettoManager 规则前，先处理更长的 PerfettoManagerTest
# - perfetto_sdk / PERFETTO_ / ::perfetto:: / <perfetto.h> / .perfetto-trace
#   不出现在替换规则里，故不会被改动（它们不含被匹配的模式，或被更长的规则保护）

REPLACEMENTS = [
    # ===== 1. include 路径（先长后短）=====
    # 违规相对路径（RocksDBDatabase.cpp）—— 规范化为绝对式 include
    (r'"\.\./\.\./\.\./perfetto/TraceEvents\.hpp"', '"common/profiler/TraceEvents.hpp"', True),
    # 短路径 perfetto/TraceEvents.hpp -> common/profiler/TraceEvents.hpp
    #    （仅在引号内、且不是 common/perfetto/ 已被上面处理的形式）
    (r'"perfetto/TraceEvents\.hpp"', '"common/profiler/TraceEvents.hpp"', True),
    # 标准 common/perfetto/ -> common/profiler/
    (r'common/perfetto/', 'common/profiler/', True),

    # ===== 2. 命名空间限定（先长后短，mc:: 前缀天然隔离 ::perfetto::）=====
    (r'mc::perfetto::PerfettoBackend', 'mc::profiler::PerfettoBackend', True),
    (r'mc::perfetto::PerfettoManager', 'mc::profiler::ProfilerManager', True),
    (r'mc::perfetto::TraceConfig', 'mc::profiler::TraceConfig', True),
    (r'mc::perfetto::initTraceCategories', 'mc::profiler::initTraceCategories', True),
    # 兜底：剩余 mc::perfetto:: -> mc::profiler::
    (r'mc::perfetto::', 'mc::profiler::', True),

    # ===== 3. 测试类/文件名（长串优先于 PerfettoManager）=====
    (r'PerfettoManagerTest', 'ProfilerManagerTest', True),

    # ===== 4. 裸标识符 PerfettoManager -> ProfilerManager =====
    #    此时 mc::perfetto::PerfettoManager 已被规则2处理，剩余为 class 定义/变量名等。
    #    变量名 perfettoManager -> profilerManager（camelCase 变量）
    (r'perfettoManager', 'profilerManager', True),
    #    类名 PerfettoManager -> ProfilerManager
    (r'PerfettoManager', 'ProfilerManager', True),

    # ===== 5. PerfettoConfig.hpp 文件引用（include / @file）=====
    (r'PerfettoConfig\.hpp', 'ProfilerConfig.hpp', True),

    # ===== 6. CMake target / subdirectory（仅影响 CMakeLists，但统一在所有文件跑无副作用）=====
    (r'\bmc_perfetto\b', 'mc_profiler', True),
    # mc::perfetto 别名（注意：mc::perfetto:: 已在规则2处理，此处只处理不带 :: 的 mc::perfetto 行尾/空格）
    (r'\bmc::perfetto\b(?!:)', 'mc::profiler', True),
    (r'add_subdirectory\(perfetto\)', 'add_subdirectory(profiler)', True),

    # ===== 7. doxygen @file 标签（PerfettoManager.hpp / PerfettoConfig.hpp 已被上面覆盖，
    #         此处兜底处理 @file 后跟文件名的形式）=====
    # （已被规则4/5覆盖，无需额外规则）
]

# 绝对不碰的子串（防御性日志：若替换结果意外触及，报警）—— 实际靠规则设计规避
NEVER_TOUCH = [
    '::perfetto::',
    '<perfetto.h>',
    'perfetto_sdk',
    'PERFETTO_',
    'mc::trace',
]


def apply_replacements(text):
    """对一段文本依次应用所有替换规则，返回新文本。"""
    for pattern, repl, is_regex in REPLACEMENTS:
        if is_regex:
            text = re.sub(pattern, repl, text)
        else:
            text = text.replace(pattern, repl)
    return text


# ----------------------------------------------------------------------------
# 文件遍历
# ----------------------------------------------------------------------------

# 处理的文件扩展名（只改文本文件；CMakeLists.txt 无扩展名单独处理）
TEXT_EXT = {
    '.hpp', '.cpp', '.h', '.cc', '.cxx', '.inl',
    '.cmake', '.txt', '.md', '.json', '.yml', '.yaml',
    '.sh', '.py', '.bat', '.ps1',
}

# 排除的目录（相对 ROOT）
# - third_party/: 第三方（含 perfetto SDK、tracy submodule）
# - build/ 等: 构建产物
# - scripts/: 工具脚本自身（本脚本含大量 perfetto 字样，避免自我修改）
# - .claude/plans, .claude/projects: 工作产物/记忆，不碰
EXCLUDE_DIRS = {
    'third_party', 'build', '.git', 'vcpkg_installed', 'out', 'cmake-build-debug',
    'build-clang', 'node_modules', '__pycache__', 'scripts',
    'plans', 'projects',  # .claude/plans, .claude/projects
}

# 排除的单个文件/路径片段
EXCLUDE_PATH_PARTS = {'third_party', 'vcpkg_installed'}


def should_process(path):
    """判断文件是否应被处理。"""
    norm = path.replace('\\', '/')
    # 排除目录
    parts = norm.split('/')
    for p in parts:
        if p in EXCLUDE_DIRS:
            return False
    # 仅处理已知文本扩展名 + CMakeLists.txt
    base = os.path.basename(path)
    if base == 'CMakeLists.txt':
        return True
    ext = os.path.splitext(path)[1].lower()
    return ext in TEXT_EXT


def collect_files():
    """收集所有待处理文件。"""
    files = []
    for dirpath, dirnames, filenames in os.walk(ROOT):
        # 原地修改 dirnames 实现 prune
        dirnames[:] = [d for d in dirnames if d not in EXCLUDE_DIRS]
        # 跳过 .claude 下的某些大目录？保留 .claude（SKILL.md 需改）
        for fn in filenames:
            full = os.path.join(dirpath, fn)
            rel = os.path.relpath(full, ROOT)
            if should_process(rel):
                files.append(full)
    return files


def process_files():
    """遍历所有文件，应用替换。返回被修改的文件列表。"""
    files = collect_files()
    changed = []
    for f in files:
        try:
            with open(f, 'r', encoding='utf-8') as fh:
                original = fh.read()
        except (UnicodeDecodeError, OSError):
            # 非 UTF-8 或无法读取，跳过
            continue
        new = apply_replacements(original)
        if new != original:
            with open(f, 'w', encoding='utf-8', newline='') as fh:
                fh.write(new)
            changed.append(f)
            print(f"[edit] {os.path.relpath(f, ROOT)}")
    return changed


# ----------------------------------------------------------------------------
# 主流程
# ----------------------------------------------------------------------------

def main():
    print("=" * 70)
    print("rename-perfetto-to-profiler")
    print("=" * 70)

    # ---- Step 1: git mv 目录 ----
    print("\n--- Step 1: 目录改名 ---")
    git_mv("src/common/perfetto", "src/common/profiler")
    git_mv("tests/common/perfetto", "tests/common/profiler")

    # ---- Step 2: git mv 文件 ----
    print("\n--- Step 2: 文件改名 ---")
    # src/common/profiler 下（已改名目录）
    git_mv("src/common/profiler/PerfettoManager.hpp", "src/common/profiler/ProfilerManager.hpp")
    git_mv("src/common/profiler/PerfettoManager.cpp", "src/common/profiler/ProfilerManager.cpp")
    git_mv("src/common/profiler/PerfettoConfig.hpp", "src/common/profiler/ProfilerConfig.hpp")
    # tests 下
    git_mv("tests/common/profiler/PerfettoManagerTest.cpp", "tests/common/profiler/ProfilerManagerTest.cpp")
    # PerfettoTest.cpp / TraceEventsTest.cpp / TraceCategories.* / TraceEvents.hpp / CMakeLists.txt / README.md 保留文件名

    # ---- Step 3: 文本替换 ----
    print("\n--- Step 3: 文本替换 ---")
    changed = process_files()

    print(f"\n=== 完成：共修改 {len(changed)} 个文件 ===")
    print("\n后续需手动检查：")
    print("  - docs/ 下自然语言 Perfetto 描述（按需保留/调整）")
    print("  - README.md 目录结构树中的 perfetto/ 条目")
    print("  - 引入tracy.md 任务文档")


if __name__ == "__main__":
    main()
