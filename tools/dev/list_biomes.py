"""临时调研脚本：列出数据包群系名与其在 BiomeIds.hpp 中的覆盖情况。

不属于项目构建，调研结束后删除。
"""
import glob
import io
import os
import re

BIOME_DIR = os.path.expanduser(r"~/minecraft_reborn/datapacks/Vanilla/data/minecraft/worldgen/biome")
IDS_HPP = r"E:\dev\minecraft-reborn-branch-1\src\common\world\biome\BiomeIds.hpp"
MAPPER_CPP = r"E:\dev\minecraft-reborn-branch-1\src\server\world\storage\reader\java\JavaBiomeMapper.cpp"

names = sorted(os.path.basename(p)[:-5] for p in glob.glob(os.path.join(BIOME_DIR, "*.json")))
ids_src = io.open(IDS_HPP, encoding="utf-8").read()
mapper_src = io.open(MAPPER_CPP, encoding="utf-8").read()

# BiomeIds.hpp: constexpr BiomeId Name = N;
id_consts = dict(re.findall(r"constexpr BiomeId (\w+) = (\d+);", ids_src))
mapped_names = set(re.findall(r'm_nameToId\["minecraft:([a-z_]+)"\]', mapper_src))
mapped_targets = set(re.findall(r'm_nameToId\["minecraft:[a-z_]+"\] = Biomes::(\w+);', mapper_src))

print("BiomeIds.hpp 常量数:", len(id_consts))
print("JavaBiomeMapper 已映射名称数:", len(mapped_names))
print("数据包群系数:", len(names))
print()
missing = [n for n in names if n not in mapped_names]
print("== 未映射的群系（%d 个）==" % len(missing))
for n in missing:
    camel = "".join(w.capitalize() for w in n.split("_"))
    hit = id_consts.get(camel)
    print("   minecraft:%-28s -> BiomeIds 候选常量 %s=%s" % (n, camel, hit if hit else "【无同名常量】"))

print()
print("== 映射到 TheEnd 的名字（可疑的粗粒度近似）==")
for m in re.findall(r'm_nameToId\["minecraft:([a-z_]+)"\] = Biomes::TheEnd', mapper_src):
    print("   ", m)
print()
print("== 映射目标里出现过但可能可疑的常量 ==")
for t in sorted(mapped_targets):
    print("   ", t, "=", id_consts.get(t, "?"))
