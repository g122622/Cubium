#!/usr/bin/env bash
#
# 单文件语法检查：复用 build/compile_commands.json 里该文件的真实编译参数，
# 去掉 -o/-c 与 PCH 参数后加 -fsyntax-only 编译。
#
# 用途：不触发整项目构建（不产出 .o、不链接）就能拿到真实的类型错误，
# 适合「重构数量大、但不希望每次都跑全量构建」的场景。
#
# 用法：
#   ./scripts/diag/syntax-check/syntax_check.sh src/common/command/CommandNode.cpp
#
# 前提：已经用任意 preset 配置过 build/（compile_commands.json 存在）。
set -u

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../.." && pwd)"
target="${1:?用法: syntax_check.sh <源文件相对仓库根的路径>}"

cd "$repo_root" || exit 1

python3 - "$target" <<'PY' > /tmp/dsh_syntax_check_cmd.sh
import json
import os
import shlex
import sys

target = os.path.abspath(sys.argv[1])
database = json.load(open('build/compile_commands.json'))

command = None
for entry in database:
    if os.path.abspath(entry['file']) == target:
        command = entry['command']
        break

if command is None:
    sys.stderr.write('compile_commands.json 中没有该文件的编译条目: %s\n' % target)
    print('exit 9')
    raise SystemExit

parts = shlex.split(command)
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

result.append('-fsyntax-only')
result.append(target)
print(' '.join(shlex.quote(token) for token in result))
PY

bash /tmp/dsh_syntax_check_cmd.sh
