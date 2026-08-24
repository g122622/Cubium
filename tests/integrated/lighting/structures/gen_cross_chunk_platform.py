#!/usr/bin/env python3
# coding: utf-8
"""
生成光照集成测试用跨区块大结构 .mcstructure（little-endian NBT，未压缩）。

权威索引公式（基岩官方 X-Y-Z 扁平化顺序，见
docs/minecraft-wiki-source/minecraft_wiki/tech_基岩版结构文件.txt）：
    index = x * sizeY * sizeZ + y * sizeZ + z   （X 外层、Y 中层、Z 内层）

C++ 解析权威源：src/common/world/gen/feature/template/TemplateLoader.cpp
    const size_t totalXZ = sizeY * sizeZ;
    x = i / totalXZ;  y = (i % totalXZ) / sizeZ;  z = (i % totalXZ) % sizeZ;
（与上面公式互逆，一致。）

cross_chunk_platform：33×7×33 跨区块平台。
  - X/Z 方向跨度 33 格，覆盖 3 个区块边界（chunk 边界在相对坐标 16、32 处）：
      相对坐标 [0,15]  → chunk A（worldgen chunk (0,0)）
      相对坐标 [16,31] → chunk B（worldgen chunk (1,0)）
      相对坐标 32      → chunk C 边界（worldgen chunk (2,0)）
    光源/探针跨 chunk 边界放置，验证 StarLight 5×5 区块缓存的跨区块传播无断链。
  - y=0：满铺 stone 地板（实心，隔绝下方 worldgen 地形干扰光照断言）
  - y=1..6：全部 air（6 层空气空间，足够方块光衰减传播与天空光垂直列）
  - 不封顶：配合 RegistrationBuilder.skyAccess(true) 清空 footprint 正上方至世界顶，
    制造露天列使天空光垂直直达 skyLight=15。y=6 air 即露天层。

  用于跨区块方块光传播 / 跨区块天空光列 / 大规模光源阵列 / 跨区块方块变更重算测试。
  搭配 RegistrationBuilder.loadSpawnChunks(true) 强制加载结构中心周围半径 3 区块
  （7×7=49 区块，远超 StarLight writeRadius=2 邻居需求），确保跨区块光照传播所需
  邻居区块均已加载且光照已计算。

用法：直接编辑下方 build_block(x,y,z) 与 size/palette，运行即覆盖输出文件。
"""
import struct
import sys
from collections import Counter

# ===================== 结构定义 =====================
SX, SY, SZ = 33, 7, 33  # cross_chunk_platform: 33×7×33（跨 3 个区块边界）

# 调色板：索引 → {方块名, states}
# 0=stone, 1=air
PALETTE = [
    {"name": "minecraft:stone", "states": {}},
    {"name": "minecraft:air", "states": {}},
]


def build_block(x, y, z):
    """返回 (x,y,z) 处的调色板索引。cross_chunk_platform 平台布局。"""
    # y=0：满铺 stone 地板（隔绝下方 worldgen）
    if y == 0:
        return 0  # stone
    # y=1..6：全部 air（6 层空气空间，不封顶配合 skyAccess 露天）
    return 1  # air


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


def w_empty_list(buf):
    """写空 List：元素类型=TAG_End(0) + count=0。基岩 NBT 空列表标准写法。"""
    buf += bytes([0])  # 元素类型 TAG_End（空列表占位）
    buf += struct.pack("<i", 0)


def w_list_compound(buf, compounds):
    buf += bytes([10])  # 元素类型 compound
    buf += struct.pack("<i", len(compounds))
    for c in compounds:
        w_compound_body(buf, c)


def w_compound_body(buf, compound):
    """compound: dict[name -> (tag_id, value_or_callback)]。"""
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
    default_palette = {
        "block_palette": (9, lambda b: w_list_compound(b, palette_compounds)),
        "block_position_data": (10, {}),
    }
    # block_indices = List<List<Int>>：主层(方块索引) + 次层(全 -1)
    secondary_layer = [-1] * (SX * SY * SZ)
    structure = {
        "block_indices": (9, lambda b: w_list_list_int(b, [indices, secondary_layer])),
        "entities": (9, lambda b: w_empty_list(b)),
        "palette": (10, {"default": (10, default_palette)}),
    }
    inner = {
        "format_version": (3, 1),
        "size": (9, lambda b: w_list_int(b, [SX, SY, SZ])),
        "structure_world_origin": (9, lambda b: w_list_int(b, [0, 0, 0])),
        "structure": (10, structure),
    }
    out = bytearray()
    out += bytes([10])  # root compound tag
    w_str(out, "")  # 空 root name（NBT 协议约定）
    w_compound_body(out, inner)  # 根 body 直接写内容，不套空键

    fn = "tests/integrated/lighting/structures/gametests/cross_chunk_platform.mcstructure"
    if len(sys.argv) > 1:
        fn = sys.argv[1]
    with open(fn, "wb") as f:
        f.write(out)
    print("写入 %s, %d 字节" % (fn, len(out)))


if __name__ == "__main__":
    main()
