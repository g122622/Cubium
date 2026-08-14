#!/usr/bin/env python3
# coding: utf-8
"""
生成 GameTest 用基岩版 .mcstructure 结构文件（little-endian NBT，未压缩）。

权威索引公式（基岩官方 X-Y-Z 扁平化顺序，见
docs/minecraft-wiki-source/minecraft_wiki/tech_基岩版结构文件.txt）：
    index = x * sizeY * sizeZ + y * sizeZ + z   （X 外层、Y 中层、Z 内层）

C++ 解析权威源：src/common/world/gen/feature/template/TemplateLoader.cpp
    const size_t totalXZ = sizeY * sizeZ;
    x = i / totalXZ;  y = (i % totalXZ) / sizeZ;  z = (i % totalXZ) % sizeZ;
（与上面公式互逆，一致。）

用法：直接编辑下方 build_block(x,y,z) 与 size/palette，运行即覆盖输出文件。
"""
import struct
import sys
from collections import Counter

# ===================== 结构定义 =====================
SX, SY, SZ = 9, 5, 9  # grass_pen: 9×5×9

# 调色板：索引 → {方块名, states}
# 0=grass_block, 1=glass, 2=air
PALETTE = [
    {"name": "minecraft:grass_block", "states": {}},
    {"name": "minecraft:glass", "states": {}},
    {"name": "minecraft:air", "states": {}},
]


def build_block(x, y, z):
    """返回 (x,y,z) 处的调色板索引。grass_pen 规整围栏布局。"""
    # y=0：满铺 grass_block 地板
    if y == 0:
        return 0  # grass_block
    # y=1,2,3：外圈玻璃墙 + 内部空气
    if y in (1, 2, 3):
        if x == 0 or x == SX - 1 or z == 0 or z == SZ - 1:
            return 1  # glass 墙
        return 2  # air 内部
    # y=4：全 air 露天（无封顶）
    return 2  # air


# ===================== LE NBT 写入器 =====================
def w_str(buf, s):
    b = s.encode("utf-8")
    buf += struct.pack("<H", len(b))
    buf += b


def w_int_payload(buf, v):
    buf += struct.pack("<i", v)


def w_string_payload(buf, s):
    w_str(buf, s)


def w_list_int(buf, ints):
    """写 List<Int>：元素类型=3(int)。用于 size=[x,y,z]。"""
    buf += bytes([3])  # 元素类型 int
    buf += struct.pack("<i", len(ints))
    for v in ints:
        w_int_payload(buf, v)


def w_list_list_int(buf, list_of_ints):
    """写 List<List<Int>>：外层元素类型=9(list)，每个元素是 List<Int>。
    用于 block_indices（主层+次层，次层全 -1）。"""
    buf += bytes([9])  # 元素类型 list
    buf += struct.pack("<i", len(list_of_ints))
    for ints in list_of_ints:
        w_list_int(buf, ints)


def w_list_compound(buf, compounds):
    buf += bytes([10])  # 元素类型 compound
    buf += struct.pack("<i", len(compounds))
    for c in compounds:
        w_compound_body(buf, c)


def w_compound_body(buf, compound):
    """compound: dict[name -> (tag_id, value_or_callback)]。
    value: string 用 str；int 用 int；compound 用 dict；list 用 callback(buf)。"""
    for key, (tag_id, val) in compound.items():
        buf += bytes([tag_id])
        w_str(buf, key)
        if tag_id == 8:  # string
            w_string_payload(buf, val)
        elif tag_id == 3:  # int
            w_int_payload(buf, val)
        elif tag_id == 10:  # compound
            w_compound_body(buf, val)
        elif tag_id == 9:  # list
            val(buf)  # list writer callback
        else:
            raise Exception("unsupported tag " + str(tag_id))
    buf += bytes([0])  # end tag


def build_palette_entry(name, states):
    entry = {"name": (8, name)}
    if states:
        entry["states"] = (10, dict(states))
    entry["version"] = (3, 17959425)  # 基岩方块版本号
    return entry


# ===================== 生成 =====================
def main():
    # 按 X-Y-Z 扁平化：index = x*SY*SZ + y*SZ + z
    indices = []
    for x in range(SX):
        for y in range(SY):
            for z in range(SZ):
                indices.append(build_block(x, y, z))

    # 验证每层分布
    names = [p["name"].replace("minecraft:", "") for p in PALETTE]
    print("布局验证 (size [%d,%d,%d]):" % (SX, SY, SZ))
    for y in range(SY):
        c = Counter()
        for x in range(SX):
            for z in range(SZ):
                c[names[indices[x * SY * SZ + y * SZ + z]]] += 1
        print("  y=%d: %s" % (y, dict(c)))

    # 构建 NBT
    palette_compounds = [build_palette_entry(p["name"], p["states"]) for p in PALETTE]
    default_palette = {"block_palette": (9, lambda b: w_list_compound(b, palette_compounds))}
    # block_indices = List<List<Int>>：主层(方块索引) + 次层(全 -1，水层空，基岩标准 2 层)
    secondary_layer = [-1] * (SX * SY * SZ)
    structure = {
        "block_indices": (9, lambda b: w_list_list_int(b, [indices, secondary_layer])),
        "palette": (10, {"default": (10, default_palette)}),
    }
    inner = {
        "format_version": (3, 1),
        "size": (9, lambda b: w_list_int(b, [SX, SY, SZ])),
        "structure": (10, structure),
    }
    # 基岩 .mcstructure 根 compound 的 root name 恒为空（NBT 协议约定），
    # 但根 compound 的 body 直接就是内容（format_version/size/structure），
    # 不再套一层空键 "" 嵌套。对齐 C++ _unwrapRootCompound：根若无单空键则原样使用。
    out = bytearray()
    out += bytes([10])  # root compound tag
    w_str(out, "")  # 空 root name（NBT 协议约定）
    w_compound_body(out, inner)  # 根 body 直接写内容，不套空键

    fn = "tests/integrated/mob_behavior/structures/gametests/grass_pen.mcstructure"
    if len(sys.argv) > 1:
        fn = sys.argv[1]
    with open(fn, "wb") as f:
        f.write(out)
    print("写入 %s, %d 字节" % (fn, len(out)))


if __name__ == "__main__":
    main()
