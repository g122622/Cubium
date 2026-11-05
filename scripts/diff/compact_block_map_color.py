"""把 block_map_color 差分导出的逐状态快照重排为按方块分组的紧凑表。

用法：python3 scripts/diff/compact_block_map_color.py <原始快照.cpp> <输出.cpp>

输入是从原版逐状态实测导出的 {"minecraft:xxx[attr=...]", color} 列表（按方块连续排列），
输出是快照文件：每个方块一条默认色，外加「例外状态名 → 颜色」的扁平并行数组。

为什么要压缩：逐状态表有近 3 万条，生成的编译单元达到 2.8 MB。分布式编译下这个文件是
唯一被所有 worker 池判为失败的任务（Release/Fuzz/Test 四条链全部因它而红）。
压缩后只剩 1165 个方块条目和 157 条例外状态，其余状态由「默认色 + 例外」推出。

为什么例外状态不放在方块的聚合体里：聚合体成员是 std::vector 时，clang-format 会把
多行字符串拆成构造函数的多个实参（`vector(a, b, c)`），编译直接失败。因此两张表平行存放、
各自只有一个「颜色 + 名字」的标量友元结构，格式化怎么排都是合法初始化器。
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
overrides = []
for name, attrs, color in entries:
    if not blocks or blocks[-1][0] != name:
        blocks.append((name, color))
        continue
    if not attrs or color == blocks[-1][1]:
        continue
    overrides.append((name + '[' + attrs + ']', color))

# 原版 BlockState 的字符串按属性注册顺序排列，属性名以 facing/occupied/part 这类字母序出现，
# 但 part 在 occupied 之后，这里需要按属性名重新排序，保证规范名与查表函数的输入一致。
def canonical(key):
    open_ = key.find('[')
    head, body_text = key[:open_], key[open_ + 1:-1]
    attrs = sorted(body_text.split(','))
    return head + '[' + ','.join(attrs) + ']'

overrides = sorted(((canonical(name), color) for name, color in overrides), key=lambda item: item[0])

parts = [header.rstrip() + '\n']
parts.append('#include "common/world/block/registry/BlockMapColorTable.hpp"')
parts.append('')
parts.append('#include <array>')
parts.append('#include <cstddef>')
parts.append('#include <string>')
parts.append('#include <string_view>')
parts.append('#include <unordered_map>')
parts.append('#include <utility>')
parts.append('#include <vector>')
parts.append('')
parts.append('namespace mc::world::block {')
parts.append('namespace {')
parts.append('')
parts.append('/// 原版每个方块的地图颜色。')
parts.append('///')
parts.append('/// 快照逐状态导出 29196 条，其中绝大多数状态与所属方块同色：真正随状态变色的只有 29 个')
parts.append('/// 方块（床按 part、原木按 axis、barrier/light 按 waterlogged），其余状态与默认色相同，')
parts.append('/// 例外状态记在 kMapColorStateOverrides。')
parts.append('constexpr std::array<MapColorBlock, ' + str(len(blocks)) + '> kMapColorBlocks = {{')
parts.extend('    {"' + name + '", static_cast<u8>(' + str(color) + ')},' for name, color in blocks)
parts.append('}};')
parts.append('')
parts.append('/// 与原版方块默认色不同的状态（规范名）。')
parts.append('constexpr std::array<MapColorStateOverride, ' + str(len(overrides)) + '> kMapColorStateOverrides = {{')
parts.extend('    {static_cast<u8>(' + str(color) + '), "' + name + '"},' for name, color in overrides)
parts.append('}};')
parts.append('')
parts.append('} // namespace')
parts.append('')
parts.append('''u8 lookupVanillaMapColor(std::string_view rawStateName)
{
    // 快照键是规范名（属性按名字排序），而状态字符串按属性注册顺序输出，因此先规范化再查。
    const std::string stateName = canonicalStateName(std::string(rawStateName));

    // 例外状态只占全部状态的一小部分，用哈希表避免对每个状态都做一次前缀扫描。
    static const std::unordered_map<std::string, u8> overrideTable = [] {
        std::unordered_map<std::string, u8> table;
        table.reserve(kMapColorStateOverrides.size() * 2);
        for (const MapColorStateOverride& stateOverride : kMapColorStateOverrides) {
            table.emplace(std::string(stateOverride.stateName), stateOverride.color);
        }
        return table;
    }();

    const auto it = overrideTable.find(stateName);
    if (it != overrideTable.end()) {
        return it->second;
    }

    for (const MapColorBlock& block : kMapColorBlocks) {
        if (std::string_view(stateName).starts_with(block.name)) {
            const std::size_t nameSize = block.name.size();
            if (stateName.size() == nameSize || stateName[nameSize] == '[') {
                return block.color;
            }
        }
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

} // namespace mc::world::block
''')
out = '\n'.join(parts) + '\n'
assert '\\\\' not in out
open(out_path, 'w', encoding='utf-8').write(out)
print('states', len(entries), 'blocks', len(blocks), 'override states', len(overrides))