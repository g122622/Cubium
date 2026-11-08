"""把 block_map_color 差分导出的逐状态快照压缩为「默认色 + 属性例外」表。

用法：python3 scripts/diff/compact_block_map_color.py <原始快照.cpp> <输出.cpp>

输入是从原版逐状态实测导出的 {"minecraft:xxx[attr=...]", color} 列表（按方块连续排列）；
输出是快照文件：方块名、默认色，以及变色方块的「非变色属性默认值 → 变色属性值 → 颜色」表。

为什么要压缩：原表 2.8 MB、3 万条字符串字面量，单独编译 1.5 s、峰值内存 170 MB，
在分布式编译的近期超时窗口里必然被判失败，Release/Fuzz/Test 三条链因此全红；压缩后
只剩 1165 个方块名和 29 个方块的例外。

变色规律：原版只有 29 个方块的默认地图颜色随状态变化，且每次都只由**一个**属性决定
（原木按 axis、床按 part、barrier/light 按 waterlogged、小麦按 age）。脚本按「把其他
属性固定在默认值、只改一个属性时颜色是否变化」归纳出这个属性；查表时非变色属性的值
必须与默认值相同，变色属性按值查颜色。原木的 axis=x 与 axis=z 同色、axis=y 不同，
这种属性值多于两个的情形因此也能精确表示。
"""
import re
import sys

src = sys.argv[1]
out_path = sys.argv[2]
text = open(src, encoding="utf-8").read()
header = text[:text.index('#include "common/world/block/registry/vanilla/BlockMapColorTable.hpp"')]
body = text[text.index("u8 lookupVanillaMapColor"):]


pat = re.compile(r'^    \{"(minecraft:[a-z0-9_]+)(\[.*\])?", (\d+)\},$')
entries = []
for line in text.split("\n"):
    m = pat.match(line)
    if m:
        entries.append((m.group(1), m.group(2)[1:-1] if m.group(2) else "", int(m.group(3))))

# 按方块分组（表已按方块连续排列）。块的默认色取该块的首个状态：
# 部分块没有不带属性的裸状态（如 acacia_button），首个状态即全部状态同色的代表。
blocks = []
by_block = {}
for name, attrs, color in entries:
    if not blocks or blocks[-1][0] != name:
        blocks.append((name, color))
        by_block[name] = []
    if attrs:
        by_block[name].append((attrs, color))


def split_attrs(attrs):
    return dict(item.split("=", 1) for item in attrs.split(",")) if attrs else {}



def varies_by(attrs, key, default):
    """该状态是否只在 key 上偏离默认值。"""
    state = split_attrs(attrs)
    return all(state.get(other) == value for other, value in default.items() if other != key)


def variants_of(data):
    """找出决定颜色的那一个属性及其「取值 → 颜色」表。

    颜色应当只由一个属性决定。但床的 part=head 与含水 light 的状态里，别的属性也被置成
    非默认值（如 facing=east），因此不能要求其余属性全部等于默认值；改为检查「该属性的
    同一取值总对应同一颜色」，并要求两种颜色下都存在只改这一个属性的状态。
    """
    default = split_attrs(data[0][0])
    best = None
    for key in default:
        by_value = {}
        pure = set()
        consistent = True
        for attrs, color in data:
            state = split_attrs(attrs)
            if by_value.setdefault(state[key], color) != color:
                consistent = False
                break
            if varies_by(attrs, key, default):
                pure.add(color)
        if not consistent or len(set(by_value.values())) < 2 or len(pure) < 2:
            continue
        if best is None or len(set(by_value.values())) > len(set(best[1].values())):
            best = (key, by_value)
    return best


variants = {}
for name, color in blocks:
    data = by_block[name]
    found = variants_of(data) if data else None
    if found is not None:
        variants[name] = found

records = []
for name, (key, value_colors) in variants.items():
    for value, color in value_colors.items():
        records.append((name, key, value, color))

dimensions = sorted({key for key, _ in variants.values()})
registers = dimensions
if [name for name, _ in blocks] != sorted(name for name, _ in blocks):
    raise SystemExit("block table must be sorted by name")
if registers != ["age", "axis", "part", "waterlogged"]:
    raise SystemExit("unexpected colour dimensions: " + repr(dimensions))

header = header.replace(
    "// **按状态而非按方块**：有 29 个方块的颜色随状态变化——\n"
    "//   - 床按 part：foot 用染色色，head 固定 WOOL；\n"
    "//   - 原木按 axis：y 用内皮色，x/z 用树皮色；\n"
    "//   - barrier 等含水方块按 waterlogged：含水时取 WATER。\n"
    "// 按方块记录会把这些状态全部抹平。",
    "// 有 29 个方块的颜色随单个属性变化（见 kMapColorExceptions）：\n"
    "//   - 床按 part：foot 用染色色，head 固定 WOOL；\n"
    "//   - 原木/竹块按 axis：y 用内皮色，x/z 用树皮色；\n"
    "//   - barrier/light 按 waterlogged：含水时取 WATER；\n"
    "//   - 小麦按 age：成熟后取 WHEAT。")
assert "kMapColorExceptions" in header, "header comment not rewritten"
parts = [header.rstrip() + "\n"]
parts.append('#include "common/world/block/registry/vanilla/BlockMapColorTable.hpp"')
parts.append("")
for include in ("algorithm", "array", "cstddef", "string", "string_view", "utility", "vector"):
    parts.append("#include <{}>".format(include))
parts.append("")
parts.append("namespace mc::world::block {")
parts.append("namespace {")
parts.append("")
parts.append("/// 原版每个方块的地图颜色（不含状态例外）。")
parts.append("///")
parts.append("/// 快照逐状态导出 29196 条，其中 {} 个方块的默认色随一个属性变化（见 kMapColorExceptions），".format(len(variants)))
parts.append("/// 其余状态与方块默认色相同。")
parts.append("constexpr std::array<MapColorBlock, " + str(len(blocks)) + "> kMapColorBlocks = {{")
for name, color in blocks:
    parts.append('    {"' + name + '", static_cast<u8>(' + str(color) + ')},')
parts.append("}};")
parts.append("")

parts.append("/// 一个例外状态：方块取 color，条件是状态里 attribute 取 value、其余属性为默认值。")
parts.append("constexpr std::array<MapColorStateOverride, " + str(len(records)) + "> kMapColorExceptions = {{")
for name, key, value, color in sorted(records):
    parts.append('    {"' + name + '", "' + value + '", "' + key + '", static_cast<u8>(' + str(color) + ')},')
parts.append("}};")
parts.append("")
parts.append("/// 哪个属性决定方块颜色。只有这 29 个方块的默认色随状态变化，其余方块恒定。")
parts.append("constexpr std::string_view kMapColorDimensionNames[] = {")
for dimension in dimensions:
    parts.append('    "' + dimension + '",')
parts.append("};")
parts.append("")
parts.append("} // namespace")
parts.append("")
parts.append("/**")
parts.append(" * @brief 查原版方块状态的地图颜色")
parts.append(" *")
parts.append(" * 先按方块名取默认色，再看该状态在变色属性上的取值是否覆盖了默认色：")
parts.append(" * 只有方块默认状态里出现的属性才可能变色（原木的 axis、床的 part、含水方块的")
parts.append(" * waterlogged、小麦的 age），其余属性无论取什么值都不影响颜色。")
parts.append(" */")
parts.append("u8 lookupVanillaMapColor(std::string_view rawStateName)")
parts.append("{")
parts.append("    const std::string stateName = canonicalStateName(std::string(rawStateName));")
parts.append("    const std::size_t open = stateName.find('[');")
parts.append("    const std::size_t close = stateName.rfind(']');")
parts.append("")
parts.append("    u8 color = 0;")
parts.append("    bool found = false;")
parts.append("    // 表按方块名升序排列，查到名字前缀之后即可停止。")
parts.append("    for (const MapColorBlock& block : kMapColorBlocks) {")
parts.append("        if (stateName.compare(0, block.name.size(), block.name) == 0 &&")
parts.append("            (stateName.size() == block.name.size() || stateName[block.name.size()] == '[')) {")
parts.append("            color = block.color;")
parts.append("            found = true;")
parts.append("            break;")
parts.append("        }")
parts.append("    }")
parts.append("    if (!found || open == std::string::npos) {")
parts.append("        return color;")
parts.append("    }")
parts.append("")
parts.append("    // 逐个属性切出 `name=value`，变色属性以 name 开头，取 value 与例外表比对。")
parts.append("    std::string_view name(stateName.data(), open);")
parts.append("    for (std::size_t begin = open + 1; begin < close;) {")
parts.append("        const std::size_t comma = stateName.find(',', begin);")
parts.append("        const std::size_t end = comma == std::string::npos || comma > close ? close : comma;")
parts.append("        const std::string_view entry(stateName.data() + begin, end - begin);")
parts.append("        begin = end + 1;")
parts.append("        const std::size_t equals = entry.find('=');")
parts.append("        if (equals == std::string_view::npos) {")
parts.append("            continue;")
parts.append("        }")
parts.append("        const std::string_view attribute = entry.substr(0, equals);")
parts.append("        if (std::find(std::begin(kMapColorDimensionNames), std::end(kMapColorDimensionNames), attribute) ==")
parts.append("            std::end(kMapColorDimensionNames)) {")
parts.append("            continue;")
parts.append("        }")
parts.append("        const std::string_view value = entry.substr(equals + 1);")
parts.append("        for (const MapColorStateOverride& stateOverride : kMapColorExceptions) {")
parts.append("            if (stateOverride.blockName == name && stateOverride.attribute == attribute &&")
parts.append("                stateOverride.value == value) {")
parts.append("                return stateOverride.color;")
parts.append("            }")
parts.append("        }")
parts.append("    }")
parts.append("    return color;")
parts.append("}")
parts.append("")
parts.append("""std::string canonicalStateName(const std::string& raw)
{
    const std::size_t open = raw.find('[');
    if (open == std::string::npos) {
        return raw;
    }
    const std::size_t close = raw.rfind(']');
    if (close == std::string::npos || close <= open) {
        return raw;
    }

    std::vector<std::string> attrs;
    std::string current;
    for (std::size_t i = open + 1; i < close; ++i) {
        if (raw[i] == ',') {
            attrs.push_back(std::move(current));
            current.clear();
        } else {
            current.push_back(raw[i]);
        }
    }
    if (!current.empty()) {
        attrs.push_back(std::move(current));
    }
    std::sort(attrs.begin(), attrs.end());

    std::string result = raw.substr(0, open) + "[";
    for (std::size_t i = 0; i < attrs.size(); ++i) {
        if (i > 0) {
            result += ",";
        }
        result += attrs[i];
    }
    result += "]";
    return result;
}

""")
parts.append("""i32 vanillaMapColorTableSize()
{
    return static_cast<i32>(kMapColorBlocks.size());
}

} // namespace mc::world::block
""")
open(out_path, "w", encoding="utf-8").write("\n".join(parts) + "\n")
print("states", len(entries), "blocks", len(blocks), "varying", len(variants),
      "records", len(records), "dimensions", dimensions)
