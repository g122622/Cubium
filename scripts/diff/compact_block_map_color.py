"""把 block_map_color 差分导出的逐状态快照重排为按方块分组的紧凑表。

用法：python3 scripts/diff/compact_block_map_color.py <原始快照.cpp> <输出.cpp>

输入是从原版逐状态实测导出的 {"minecraft:xxx[attr=...]", color} 列表（按方块连续排列），
输出是按方块分组的 MapColorBlock{名字, 默认色, 例外状态列表} 数组。

为什么要压缩：逐状态表有近 3 万条，生成的编译单元达到 2.8 MB。分布式编译下这个文件是
唯一被所有 worker 池判为失败的任务（四次提交全部超时/失败），随后的 Release、Fuzz、Test
三条构建链因此全红。压缩后只剩 1165 个方块条目，其余状态由「默认色 + 例外」推出。
"""
import re
import sys

src = sys.argv[1]
out_path = sys.argv[2]
text = open(src, encoding='utf-8').read()
header = text[:text.index('#include "common/world/block/registry/BlockMapColorTable.hpp"')]
body = text[text.index('u8 lookupVanillaMapColor'):]
canon = body[body.index('std::string canonicalStateName'):body.index('i32 vanillaMapColorTableSize')].rstrip() + '\n'

pat = re.compile(r'^    \{"(minecraft:[a-z0-9_]+)(\[.*\])?", (\d+)\},$')
entries = []
for line in text.split('\n'):
    m = pat.match(line)
    if m:
        entries.append((m.group(1), m.group(2)[1:-1] if m.group(2) else '', int(m.group(3))))

# 按方块分组（表已按方块连续排列）。块的默认色取该块的首个状态：
# 部分块没有不带属性的裸状态（如 acacia_button），首个状态即全部状态同色的代表。
blocks = []
for name, attrs, color in entries:
    if not blocks or blocks[-1][0] != name:
        blocks.append([name, color, []])
        continue
    if not attrs or color == blocks[-1][1]:
        continue
    if blocks[-1][2] and color == blocks[-1][2][-1][0]:
        blocks[-1][2][-1][1].append(attrs)
    else:
        blocks[-1][2].append([color, [attrs]])

# 用双重花括号占位，最后统一替换成 C++ 的花括号
def render_entry(name, color, overrides):
    parts = ['{MapColorBlock{"' + name + '", static_cast<u8>(' + str(color) + '), {']
    for i, (c, attrs) in enumerate(overrides):
        attrs.sort()
        parts.append('{static_cast<u8>(' + str(c) + '),')
        parts.extend('"' + a + '",' for a in attrs)
        parts.append('},' if i + 1 < len(overrides) else '}}')
    if not overrides:
        parts.append('}')
    parts.append('}},')
    return parts

lines = ['    {MapColorBlock{"minecraft:air", static_cast<u8>(0), {}}},']
line = '    '
for name, color, overrides in blocks:
    for token in render_entry(name, color, overrides):
        if len(line) + len(token) + 1 > 116 and line.strip():
            lines.append(line.rstrip())
            line = '    '
        line += token + ' '
lines.append(line.rstrip())

parts = [header.rstrip() + '\n']
parts.append('#include "common/world/block/registry/BlockMapColorTable.hpp"')
parts.append('')
parts.append('#include <array>')
parts.append('#include <cstddef>')
parts.append('#include <string>')
parts.append('#include <string_view>')
parts.append('#include <utility>')
parts.append('#include <vector>')
parts.append('')
parts.append('namespace mc::world::block {')
parts.append('namespace {')
parts.append('')
parts.append('/// 原版每个方块的地图颜色。')
parts.append('///')
parts.append('/// 按方块着色、状态例外单独记录：快照逐状态导出的 29196 条里绝大多数状态与所属方块同色，')
parts.append('/// 真正随状态变色的只有 29 个方块（床按 part、原木按 axis、barrier/light 按 waterlogged），')
parts.append('/// 它们的例外状态记在 stateOverrides。')
parts.append('constexpr std::array<MapColorBlock, {}> kMapColorBlocks = {{'.format(len(blocks)))
parts.extend(lines)
parts.append('};')
parts.append('')
parts.append('} // namespace')
parts.append('')
parts.append('''u8 lookupVanillaMapColor(std::string_view rawStateName)
{
    // 快照键是规范名（属性按名字排序），而状态字符串按属性注册顺序输出，因此先规范化再查。
    const std::string stateName = canonicalStateName(std::string(rawStateName));
    for (const MapColorBlock& block : kMapColorBlocks) {
        if (stateName.compare(0, block.name.size(), block.name) != 0) {
            continue;
        }
        if (stateName.size() != block.name.size() && stateName[block.name.size()] != '[') {
            continue;
        }
        for (const MapColorStateOverride& stateOverride : block.stateOverrides) {
            if (stateOverride.stateName == stateName) {
                return stateOverride.color;
            }
        }
        return block.color;
    }
    return 0;
}
''')
parts.append(canon.rstrip())
parts.append('')
parts.append('''i32 vanillaMapColorTableSize()
{
    return static_cast<i32>(kMapColorBlocks.size());
}
''')
out = '\n'.join(parts) + '\n'
assert '\\\\' not in out
open(out_path, 'w', encoding='utf-8').write(out)
print('states', len(entries), 'blocks', len(blocks), 'override states', sum(len(a) for b in blocks for c, a in b[2]))