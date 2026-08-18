#!/usr/bin/env python3
# coding: utf-8
"""
生成 GameTest 用基岩版 .mcstructure 结构文件（little-endian NBT，未压缩）。

reinforce_arena：81×7×81 露天草地大场地，专供僵尸增援（ZombieEntity::trySummonReinforcements）测试。
- y=0：满铺 grass_block 草地地板（增援 OnGround 放置检查需脚底 solid 支撑）。
- y=1..6：全 air（无封顶露天，skyLight=15）。
- 四壁 glass（x=0/x=80, z=0/z=80，y=1..6）透光围栏防僵尸寻路走出。

关键设计：露天亮处（skyLight=15）。
- 增援不查光照（canMonsterSpawnInLightPredicate 为空 no-op，ZombieEntity.cpp:318-321），
  在亮处仍生成。
- NaturalSpawner 自然生成查光照（_checkLightLevel 用原始 skyLight，露天=15>7 拒绝），
  故露天亮处自然生成不触发——隔离自然生成污染，只增援生成。
- night batch 避免亡灵（僵尸+增援僵尸）白天燃烧死亡（night batch 设 dayTime=18000，
  不影响 NaturalSpawner 光照判定，仍 skyLight=15）。

尺寸 81×81：增援偏移各轴 0~39 格（nextInt(7,40)*nextInt(-1,1)），中心 (40,2,40) 放僵尸+玩家，
偏移范围 [1,79] 落在结构内，OnGround 脚下 grass_block 支撑。

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
SX, SY, SZ = 81, 7, 81  # reinforce_arena: 81×7×81

# 调色板：索引 → {方块名, states}
# 0=grass_block, 1=glass, 2=air
PALETTE = [
    {"name": "minecraft:grass_block", "states": {}},
    {"name": "minecraft:glass", "states": {}},
    {"name": "minecraft:air", "states": {}},
]


def build_block(x, y, z):
    """返回 (x,y,z) 处的调色板索引。reinforce_arena 露天草地围栏布局。"""
    # y=0：满铺 grass_block 草地地板
    if y == 0:
        return 0  # grass_block
    # y=1..6：四壁 glass 墙 + 内部 air（无封顶露天）
    if x == 0 or x == SX - 1 or z == 0 or z == SZ - 1:
        return 1  # glass 墙
    return 2  # air 内部


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
    """写空 List：元素类型=TAG_End(0) + count=0。基岩 NBT 空列表标准写法。
    用于 entities（结构内无预置实体，写空 list）。对齐 _rebuild_creeper_pit.ts:94-96。"""
    buf += bytes([0])  # 元素类型 TAG_End（空列表占位）
    buf += struct.pack("<i", 0)


def w_list_compound(buf, compounds):
    buf += bytes([10])  # 元素类型 compound
    buf += struct.pack("<i", len(compounds))
    for c in compounds:
        w_compound_body(buf, c)


def w_compound_body(buf, compound):
    """compound: dict[name -> (tag_id, value_or_callback)]。
    value: string 用 str；int 用 int；compound 用 dict；list 用 callback。"""
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
    # block_position_data：基岩 .mcstructure 的 required 字段（每个方块位置的可选元数据，如方块实体）。
    # reinforce_arena 无方块实体，写空 compound {}。缺此字段基岩解析报 "block_position_data field, a required
    # field, is missing"（虽容错继续，但为正确性补齐，对齐 _rebuild_creeper_pit.ts:119 的写法）。
    default_palette = {
        "block_palette": (9, lambda b: w_list_compound(b, palette_compounds)),
        "block_position_data": (10, {}),
    }
    # block_indices = List<List<Int>>：主层(方块索引) + 次层(全 -1，水层空，基岩标准 2 层)
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
    # 基岩 .mcstructure 根 compound 的 root name 恒为空（NBT 协议约定），
    # 但根 compound 的 body 直接就是内容（format_version/size/structure），
    # 不再套一层空键 "" 嵌套。对齐 C++ _unwrapRootCompound：根若无单空键则原样使用。
    out = bytearray()
    out += bytes([10])  # root compound tag
    w_str(out, "")  # 空 root name（NBT 协议约定）
    w_compound_body(out, inner)  # 根 body 直接写内容，不套空键

    fn = "tests/integrated/mob_behavior/structures/gametests/reinforce_arena.mcstructure"
    if len(sys.argv) > 1:
        fn = sys.argv[1]
    with open(fn, "wb") as f:
        f.write(out)
    print("写入 %s, %d 字节" % (fn, len(out)))


if __name__ == "__main__":
    main()
