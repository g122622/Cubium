#!/usr/bin/env bash
#
# 单文件语法检查：复用 build/compile_commands.json 里的真实编译参数，
# 去掉 -o/-c 与 PCH 参数后加 -fsyntax-only 编译。
#
# 用途：不触发整项目构建（不产出 .o、不链接）就能拿到真实的类型错误。
#
# 用法：
#   ./scripts/diag/syntax-check/syntax_check.sh src/common/command/CommandNode.cpp
#
# 退出码：
#   0  编译通过
#   1  编译失败（有 error）
#   3  compile_commands.json 里没有该文件，且找不到同目录可借用的参数
#      —— 这通常意味着「文件是新加的，但 CMake 还没重新 configure」，
#         此时本脚本**不能**给出「通过」的结论，必须显式失败。
#
# 【重要】本脚本曾经在「找不到编译条目」时只打印一行提示就退出，
# 被批量扫描用 `grep -c "error:"` 判 0 当成通过，导致一个根本编译不过的
# 新文件（Result<const json&> 在 std::optional<T&> 上非法）一路提交。
# 现在找不到条目会明确失败并给出可操作提示，杜绝这种静默放过。
set -u

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../.." && pwd)"
target="${1:?用法: syntax_check.sh <源文件相对仓库根的路径>}"

cd "$repo_root" || exit 1

if [[ ! -f build/compile_commands.json ]]; then
    echo "错误: build/compile_commands.json 不存在，请先执行 cmake --preset <preset> 配置。" >&2
    exit 3
fi

python3 - "$target" <<'PY' > /tmp/dsh_syntax_check_cmd.sh
import json
import os
import shlex
import sys

target = os.path.abspath(sys.argv[1])
database = json.load(open('build/compile_commands.json'))

entry = next((item for item in database if os.path.abspath(item['file']) == target), None)

borrowed_from = None
if entry is None:
    # 新文件可能还没进 compile_commands.json（CMake 未重新 configure）。
    # 退而求其次：借用同目录下任一 .cpp 的参数，只替换输入文件路径。
    target_dir = os.path.dirname(target)
    candidates = [item for item in database
                  if os.path.dirname(os.path.abspath(item['file'])) == target_dir]
    if not candidates:
        sys.stderr.write(
            '错误: compile_commands.json 中没有 %s，且同目录无可借用参数。\n'
            '      请先重新 configure（cmake --preset macos-relwithdebinfo-server），'
            '并确认该文件已加入 CMakeLists。\n' % target)
        print('exit 3')
        raise SystemExit
    entry = candidates[0]
    borrowed_from = os.path.abspath(entry['file'])

parts = shlex.split(entry['command'])
result = []
index = 0
while index < len(parts):
    part = parts[index]
    if part == '-o' or part == '-c':
        index += 2
        continue
    # PCH 相关参数整组跳过。注意 `-Xclang -include -Xclang <pch>` 是 4 个 token，
    # 只跳 3 个会把 pch 路径留下当成输入文件。
    if part == '-Xclang' and index + 1 < len(parts) and parts[index + 1] in ('-include-pch', '-include'):
        index += 4
        continue
    if part == '-Winvalid-pch':
        index += 1
        continue
    result.append(part)
    index += 1

# 借用参数时，把原条目里的源文件路径替换成目标文件
if borrowed_from is not None:
    result = [target if os.path.abspath(token) == borrowed_from else token for token in result]

result.append('-fsyntax-only')
# 不限制错误条数：迁移类任务需要一次看到全部不匹配点
result.append('-ferror-limit=0')
result.append(target)
print(' '.join(shlex.quote(token) for token in result))
if borrowed_from is not None:
    sys.stderr.write('提示: %s 不在 compile_commands.json 中，借用 %s 的编译参数。\n'
                     % (target, borrowed_from))
PY

bash /tmp/dsh_syntax_check_cmd.sh
