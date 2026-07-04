#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Build the complete list of datapack-referenced items NOT registered in Cubium.

Sources of registered items:
  1. Items.cpp            -> registerItem(ResourceLocation("minecraft:X"), ...) (incl. registerItem<Template>(...))
                            registerBlockBackedItem(registry, VanillaBlocks::X, "name", ...)
  2. BlockItemRegistry.cpp-> registerSimpleBlock(..., "name")  (name is the item id path)
                            registerItem<...>(ResourceLocation("minecraft:X"), ...)
                            registerAnvilBlock(VanillaBlocks::X, "name")  (local lambda; name is the item id path)

Datapack tag JSON files: every leaf entry (non-# references) under "values" across
all tag files is a referenced item id. # tag references are expanded recursively
but the loader already warns only on leaf items, so we collect leaf item ids only.

Output: a Markdown doc written to docs/UNIMPLEMENTED_ITEMS.md
"""
import json
import os
import re
import sys
from pathlib import Path

REPO = Path("E:/dev/minecraft-reborn-branch-1")
DATAPACK = Path("C:/Users/Administrator/minecraft_reborn/datapacks/Vanilla")

ITEMS_CPP = REPO / "src/common/item/Items.cpp"
BLOCK_ITEM_CPP = REPO / "src/common/item/items/block/BlockItemRegistry.cpp"

# ----------------------------------------------------------------------------
# 1. Collect registered item ids from source
# ----------------------------------------------------------------------------

registered = set()

# 1a. Items.cpp: registerItem(ResourceLocation("minecraft:X"), ...) and registerItem<T>(ResourceLocation("minecraft:X"), ...)
#     Also registerBlockBackedItem(registry, VanillaBlocks::X, "name", ...) -> item id is minecraft:name (helper at Items.cpp:94
#     calls registry.registerItem<mc::BlockItem>(id, ...) with id = ResourceLocation("minecraft:" + name)).
text = ITEMS_CPP.read_text(encoding="utf-8")

# registerItem calls with explicit ResourceLocation literal
for m in re.finditer(r'registerItem(?:<[^>]*>)?\s*\(\s*ResourceLocation\s*\(\s*"((?:[a-z0-9_]+:)?[a-z0-9_/]+)"\s*\)', text):
    registered.add(m.group(1))

# registerBlockBackedItem(registry, VanillaBlocks::X, "name", ...) -> id minecraft:name
#    Match the helper call sites (not the definition line, which has no name literal in that position).
for m in re.finditer(r'registerBlockBackedItem\s*\(\s*[^,]+,\s*[^,]+,\s*"([a-z0-9_/]+)"', text):
    registered.add("minecraft:" + m.group(1))

# 1b. BlockItemRegistry.cpp: registerSimpleBlock(..., "name") and registerItem<...>(ResourceLocation("minecraft:X"), ...)
text2 = BLOCK_ITEM_CPP.read_text(encoding="utf-8")

for m in re.finditer(r'registerSimpleBlock\s*\(\s*[^,]+,\s*"([a-z0-9_/]+)"', text2):
    registered.add("minecraft:" + m.group(1))

for m in re.finditer(r'registerItem(?:<[^>]*>)?\s*\(\s*ResourceLocation\s*\(\s*"((?:[a-z0-9_]+:)?[a-z0-9_/]+)"\s*\)', text2):
    registered.add(m.group(1))

# BlockItemRegistry.cpp: registerAnvilBlock(VanillaBlocks::X, "name") -> id minecraft:name
#  registerAnvilBlock 是一个本地 lambda（行 343-371），物品 id 来自方块的 blockLocation()，
#  与传入的 name 参数一致。其他类似的本地 lambda（registerWallBanner / registerWallSign 等）
#  同样以 name 参数作为物品 id 路径。
for m in re.finditer(r'registerAnvilBlock\s*\(\s*[^,]+,\s*"([a-z0-9_/]+)"', text2):
    registered.add("minecraft:" + m.group(1))

# Normalize: ensure every id has a namespace. Bare paths become minecraft:path.
registered = {("minecraft:" + i) if ":" not in i else i for i in registered}

# ----------------------------------------------------------------------------
# 2. Collect referenced item ids from datapack tag JSON files
#    Tag files live under */tags/item/**/*.json . Leaf entries are item ids.
# ----------------------------------------------------------------------------

referenced_items = set()   # leaf item ids (no leading #)
referenced_tags = set()    # #tag references encountered

def collect_from_tag_file(path):
    try:
        data = json.loads(path.read_text(encoding="utf-8"))
    except Exception as e:
        print(f"WARN: failed to parse {path}: {e}", file=sys.stderr)
        return
    values = data.get("values")
    if not isinstance(values, list):
        return
    for v in values:
        if isinstance(v, str):
            if v.startswith("#"):
                referenced_tags.add(v[1:])
            else:
                referenced_items.add(v)
        elif isinstance(v, dict):
            vid = v.get("id")
            if isinstance(vid, str):
                if vid.startswith("#"):
                    referenced_tags.add(vid[1:])
                else:
                    referenced_items.add(vid)

tag_files = []
for root, dirs, files in os.walk(DATAPACK):
    # only item tag directories
    if os.sep + "tags" + os.sep + "item" not in root.replace("\\", os.sep) + os.sep:
        # also accept /tags/item at end
        parts = Path(root).parts
        if not (len(parts) >= 2 and parts[-2] == "tags" and parts[-1] == "item"):
            continue
    for f in files:
        if f.endswith(".json"):
            tag_files.append(Path(root) / f)

# Fallback: simpler glob if the above filtered nothing
if not tag_files:
    tag_files = list(DATAPACK.rglob("*/tags/item/**/*.json"))

for tf in tag_files:
    collect_from_tag_file(tf)

print(f"Found {len(tag_files)} item tag files")
print(f"Registered items: {len(registered)}")
print(f"Referenced leaf items: {len(referenced_items)}")
print(f"Referenced #tag refs: {len(referenced_tags)}")

# ----------------------------------------------------------------------------
# 3. Compute unimplemented = referenced - registered
# ----------------------------------------------------------------------------

unimplemented = sorted(referenced_items - registered)
implemented_referenced = sorted(referenced_items & registered)

# Also report referenced #tags that themselves resolve to no/empty (best-effort: just list them)
print(f"Unimplemented items: {len(unimplemented)}")

# ----------------------------------------------------------------------------
# 4. Group unimplemented items by tag membership for the doc
# ----------------------------------------------------------------------------

# Build tag -> set(unimplemented leaf items in that tag)
tag_to_unimpl = {}
# Also need tag -> all leaf items, to flag tags that resolve to no valid items
tag_to_all_items = {}

def leaf_items_of_tag(path):
    try:
        data = json.loads(path.read_text(encoding="utf-8"))
    except Exception:
        return None, None
    values = data.get("values")
    if not isinstance(values, list):
        return None, None
    leaf = set()
    refs = set()
    for v in values:
        if isinstance(v, str):
            if v.startswith("#"):
                refs.add(v[1:])
            else:
                leaf.add(v)
        elif isinstance(v, dict):
            vid = v.get("id")
            if isinstance(vid, str):
                if vid.startswith("#"):
                    refs.add(vid[1:])
                else:
                    leaf.add(vid)
    return leaf, refs

# tag id (namespace:path) -> path
tag_id_to_path = {}
for tf in tag_files:
    # derive tag id from path: <ns>/tags/item/<rel>.json  (datapack root has data/<ns>/tags/item/...)
    # find the "tags/item" segment
    p = Path(tf)
    try:
        idx = p.parts.index("tags")
        # parts: .../<namespace>/tags/item/<rel...>.json
        ns = p.parts[idx - 1]
        rel = Path(*p.parts[idx + 2:]).as_posix()
        # strip .json
        if rel.endswith(".json"):
            rel = rel[:-5]
        tag_id = f"{ns}:{rel}"
    except ValueError:
        continue
    tag_id_to_path[tag_id] = tf

empty_resolving_tags = []  # tags whose leaf items are ALL unimplemented
partial_tags = []          # tags with at least one unimplemented leaf item (and at least one implemented)
for tag_id, tf in tag_id_to_path.items():
    leaf, refs = leaf_items_of_tag(tf)
    if leaf is None:
        continue
    unimpl_in_tag = sorted(leaf - registered)
    if not unimpl_in_tag:
        continue
    if leaf.issubset(registered):
        # all implemented, skip
        continue
    if all(i not in registered for i in leaf):
        empty_resolving_tags.append((tag_id, unimpl_in_tag))
    else:
        partial_tags.append((tag_id, unimpl_in_tag))

empty_resolving_tags.sort(key=lambda x: x[0])
partial_tags.sort(key=lambda x: x[0])

# ----------------------------------------------------------------------------
# 5. Categorize unimplemented items by naming pattern (best-effort)
# ----------------------------------------------------------------------------

def categorize(item_id):
    name = item_id.split(":", 1)[-1]
    # order matters: most specific first
    if any(k in name for k in ["_shulker_box"]):
        return "Shulker Boxes"
    if "shulker_box" == name:
        return "Shulker Boxes"
    if name.endswith("_bed") or name == "bed":
        return "Beds"
    if name.endswith("_candle") or name == "candle":
        return "Candles"
    if name.endswith("_log") or name.endswith("_wood") or name.endswith("_hyphae") or name.endswith("_stem"):
        return "Logs / Wood / Stems"
    if name.endswith("_planks"):
        return "Planks"
    if name.endswith("_slab"):
        return "Slabs"
    if name.endswith("_stairs"):
        return "Stairs"
    if name.endswith("_fence") or name.endswith("_fence_gate"):
        return "Fences / Fence Gates"
    if name.endswith("_wall") or name == "wall":
        return "Walls"
    if name.endswith("_door") or name == "door":
        return "Doors"
    if name.endswith("_trapdoor") or name == "trapdoor":
        return "Trapdoors"
    if name.endswith("_button"):
        return "Buttons"
    if name.endswith("_pressure_plate"):
        return "Pressure Plates"
    if name.endswith("_sign") or name.endswith("_hanging_sign"):
        return "Signs"
    if name.endswith("_banner") or name == "banner":
        return "Banners"
    if name.endswith("_carpet"):
        return "Carpets"
    if name.endswith("_wool") or name == "wool":
        return "Wool"
    if name.endswith("_terracotta") or name == "terracotta" or name.endswith("_glazed_terracotta"):
        return "Terracotta"
    if name.endswith("_concrete") or name.endswith("_concrete_powder"):
        return "Concrete"
    if "copper" in name:
        return "Copper"
    if name.endswith("_leaves"):
        return "Leaves"
    if name.endswith("_sapling"):
        return "Saplings"
    if name.endswith("_flower") or name in ("dandelion", "poppy", "blue_orchid", "allium", "azure_bluet",
                                            "red_tulip", "orange_tulip", "white_tulip", "pink_tulip",
                                            "oxeye_daisy", "cornflower", "lily_of_the_valley",
                                            "wither_rose", "torchflower", "pitcher_plant",
                                            "open_eyeblossom", "closed_eyeblossom"):
        return "Flowers / Small Flowers"
    if name.endswith("_mushroom") or name in ("mushroom", "mushroom_stem"):
        return "Mushrooms"
    if name.endswith("_boat") or name.endswith("_chest_boat") or name == "boat":
        return "Boats"
    if name.endswith("_minecart") or name == "minecart":
        return "Minecarts / Rails"
    if name.endswith("_rail"):
        return "Minecarts / Rails"
    if name.endswith("_bucket"):
        return "Buckets"
    if name.endswith("_spawn_egg"):
        return "Spawn Eggs"
    if name.endswith("_armor") or name in ("turtle_helmet",):
        return "Armor"
    if name.endswith("_chestplate") or name.endswith("_leggings") or name.endswith("_helmet") or name.endswith("_boots"):
        return "Armor"
    if name.endswith("_sword") or name.endswith("_pickaxe") or name.endswith("_axe") or name.endswith("_shovel") or name.endswith("_hoe") or name.endswith("_shears"):
        return "Tools / Weapons"
    if "spear" in name:
        return "Tools / Weapons"
    if name.endswith("_goat_horn") or name == "goat_horn":
        return "Music / Instruments"
    if name in ("music_disc_13",) or name.startswith("music_disc_"):
        return "Music / Instruments"
    if name.endswith("_pottery_sherd"):
        return "Pottery Sherds"
    if name.endswith("_banner_pattern") or name == "banner_pattern":
        return "Banner Patterns"
    if "armor_trim" in name or name.endswith("_armor_trim_smithing_template"):
        return "Smithing Templates"
    if "smithing_template" in name:
        return "Smithing Templates"
    if name.endswith("_bottle") or name == "bottle" or name == "experience_bottle":
        return "Potions / Bottles"
    if name.endswith("_potion") or name.endswith("_splash_potion") or name.endswith("_lingering_potion") or name.endswith("_tipped_arrow"):
        return "Potions / Bottles"
    return "Other"

from collections import defaultdict
cat_map = defaultdict(list)
for iid in unimplemented:
    cat_map[categorize(iid)].append(iid)
for k in cat_map:
    cat_map[k].sort()

# ----------------------------------------------------------------------------
# 6. Write the doc
# ----------------------------------------------------------------------------

out = []
out.append("# 未实现物品清单（ItemTagLoader 警告来源）\n")
out.append("")
out.append("> 本文档由脚本 `scripts/build_unimplemented_items_doc.py` 自动生成，列出 Cubium 当前注册物品集合与")
out.append("> 原版数据包（`minecraft_reborn/datapacks/Vanilla`）物品标签（item tags）所引用物品之间的差集。")
out.append("> 这些物品在标签加载阶段触发 `ItemTagLoader: unknown item 'X' (required), skipping (tag: Y)` 警告，")
out.append("> 属于「数据包引用了尚未实现的物品」，并非注册顺序 bug（物品注册早于标签加载，见 ")
out.append("`src/server/application/MinecraftServer.cpp` 的 `initializeRegistries()`）。\n")
out.append("")
out.append("## 摘要\n")
out.append("")
out.append(f"- 数据包物品标签文件数：**{len(tag_files)}**")
out.append(f"- 标签引用的去重叶子物品数：**{len(referenced_items)}**")
out.append(f"- Cubium 已注册物品数（源码扫描）：**{len(registered)}**")
out.append(f"- 标签引用但未实现的物品数：**{len(unimplemented)}**")
out.append(f"- 完全无法解析（标签内全部叶子物品均未实现）的标签数：**{len(empty_resolving_tags)}**")
out.append(f"- 部分缺失（至少一个叶子物品未实现）的标签数：**{len(partial_tags)}**")
out.append("")
out.append("## 注册物品来源\n")
out.append("")
out.append("| 来源文件 | 注册方式 | 说明 |")
out.append("| --- | --- | --- |")
out.append("| `src/common/item/Items.cpp` | `registerItem(ResourceLocation(\"minecraft:X\"), ...)` | 直接注册物品（含工具/食物/材料等） |")
out.append("| `src/common/item/Items.cpp` | `registerBlockBackedItem(registry, VanillaBlocks::X, \"name\", ...)` | 以方块为底注册 BlockItem，物品 id 为 `minecraft:name` |")
out.append("| `src/common/item/items/block/BlockItemRegistry.cpp` | `registerSimpleBlock(VanillaBlocks::X, \"name\")` | 注册简单方块物品，物品 id 取自方块的 `blockLocation()`（与 `name` 一致） |")
out.append("| `src/common/item/items/block/BlockItemRegistry.cpp` | `registerItem<...>(ResourceLocation(\"minecraft:X\"), ...)` | 显式注册特殊 BlockItem |")
out.append("| `src/common/item/items/block/BlockItemRegistry.cpp` | `registerAnvilBlock(VanillaBlocks::X, \"name\")` | 铁砧专用本地 lambda，物品 id 取自 `blockLocation()`（与 `name` 一致），仅 maxStackSize 不同（=1） |")
out.append("")
out.append("## 完全无法解析的标签（所有叶子物品均未实现）\n")
out.append("")
out.append("这些标签会输出 `ItemTagLoader: tag 'X' resolved no valid items` 信息级日志，且其中每一项都触发一条警告。\n")
out.append("")
out.append("| 标签 | 未实现的叶子物品数 |")
out.append("| --- | --- |")
for tag_id, items in empty_resolving_tags:
    out.append(f"| `{tag_id}` | {len(items)} |")
out.append("")
out.append("## 部分缺失的标签（至少一个叶子物品未实现）\n")
out.append("")
out.append("| 标签 | 未实现叶子物品数 |")
out.append("| --- | --- |")
for tag_id, items in partial_tags:
    out.append(f"| `{tag_id}` | {len(items)} |")
out.append("")
out.append("## 未实现物品完整清单（按分类）\n")
out.append("")
out.append("共 **{}** 项。物品 id 形如 `minecraft:anvil`。\n".format(len(unimplemented)))
out.append("")
# sort categories alphabetically except Other last
cats = sorted(k for k in cat_map if k != "Other")
for cat in cats:
    items = cat_map[cat]
    out.append(f"### {cat}（{len(items)}）\n")
    out.append("")
    for iid in items:
        out.append(f"- `{iid}`")
    out.append("")
if "Other" in cat_map:
    items = cat_map["Other"]
    out.append(f"### Other（{len(items)}）\n")
    out.append("")
    for iid in items:
        out.append(f"- `{iid}`")
    out.append("")
out.append("## 附录：完全无法解析标签的逐项明细\n")
out.append("")
out.append("下表列出每个「完全无法解析」标签内的全部未实现物品，便于逐个实现时核对。\n")
out.append("")
for tag_id, items in empty_resolving_tags:
    out.append(f"### `{tag_id}`\n")
    out.append("")
    for iid in items:
        out.append(f"- `{iid}`")
    out.append("")
out.append("## 附录：部分缺失标签的逐项明细\n")
out.append("")
for tag_id, items in partial_tags:
    out.append(f"### `{tag_id}`\n")
    out.append("")
    out.append("未实现项：\n")
    out.append("")
    for iid in items:
        out.append(f"- `{iid}`")
    out.append("")

doc_path = REPO / "docs" / "UNIMPLEMENTED_ITEMS.md"
doc_path.write_text("\n".join(out), encoding="utf-8")
print(f"\nWrote {doc_path}")
print(f"  unimplemented items: {len(unimplemented)}")
print(f"  empty tags: {len(empty_resolving_tags)}")
print(f"  partial tags: {len(partial_tags)}")
