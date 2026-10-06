#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
生成 docs/UNIMPLEMENTED_CONTENT.md —— 原版方块/物品中尚未在 Cubium 注册的清单。

判定口径（权威）：
  - 原版全集：assets/data/blocks_1.21.11.json（1166 个 block）、
              assets/data/items_1.21.11.json（1505 个 item）。
  - 已注册集合：扫描 src/common/world/block/registry/**.cpp 与 src/common/item/**.cpp
              中出现的 "minecraft:<name>" 字符串字面量（含 ResourceLocation("minecraft:x")、
              ResourceLocation("minecraft","x")、registerSimpleBlock(..., "x")、
              registerPotted(registry,"x") 等全部注册范式）。

用法：python scripts/build_unimplemented_content_doc.py
"""
import json
import re
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parent.parent
DATA = REPO / "assets" / "data"
OUT = REPO / "docs" / "UNIMPLEMENTED_CONTENT.md"

SCAN_DIRS = [
    REPO / "src" / "common" / "world" / "block" / "registry",
    REPO / "src" / "common" / "item",
]
# 注意：不要把 BlockTags.cpp / 注释里提到名字的文件纳入扫描——它们只"提及"而非"注册"，
# 纳入会造成漏报（把未实现的方块误判为已实现）。
SCAN_FILES = []


def _scan_register_calls(text: str) -> set:
    """找出所有 `registerXxx(...)` 调用，收集其括号内出现的裸路径字符串（即注册的 id）。

    用括号配平扫描而非正则——注册调用内部常嵌套其它调用（如 ItemProperties().maxStackSize(64)），
    且辅助函数的 id 名可能是第 2、3 个参数（如 registerBlockBackedItem(registry, Block, "name", ...)），
    正则难以稳健覆盖。
    """
    names = set()
    str_pat = re.compile(r'"([a-z0-9_/]+)"')
    i = 0
    n = len(text)
    while True:
        m = re.search(r'\bregister[A-Z][A-Za-z]*\s*\(', text[i:])
        if not m:
            break
        open_pos = i + m.end() - 1  # 指向 '('
        depth = 0
        j = open_pos
        while j < n:
            c = text[j]
            if c == '(':
                depth += 1
            elif c == ')':
                depth -= 1
                if depth == 0:
                    break
            j += 1
        call = text[open_pos + 1 : j]
        for sm in str_pat.finditer(call):
            names.add(sm.group(1))
        i = j + 1
    return names


def collect_registered() -> set:
    """扫描注册源码，收集所有实际注册调用的方块/物品名。

    仅匹配真实注册范式（ResourceLocation(...) 字面量 + register* 辅助函数的字符串名），
    不匹配裸 `minecraft:` 提及（注释、标签成员等），以免把"未实现"误判为"已实现"。
    """
    reg = set()
    # ResourceLocation("minecraft:X") 单参形式
    pat_single = re.compile(r'ResourceLocation\(\s*"minecraft:([a-z0-9_/]+)"')
    # ResourceLocation("minecraft", "X") 双参形式
    pat_double = re.compile(r'ResourceLocation\(\s*"minecraft"\s*,\s*"([a-z0-9_/]+)"')
    files = []
    for d in SCAN_DIRS:
        files.extend(sorted(d.rglob("*.cpp")))
    files.extend(SCAN_FILES)
    for f in files:
        if not f.exists():
            continue
        text = f.read_text(encoding="utf-8", errors="replace")
        for m in pat_single.finditer(text):
            reg.add(m.group(1))
        for m in pat_double.finditer(text):
            reg.add(m.group(1))
        reg |= _scan_register_calls(text)
    return reg


def load_vanilla_blocks() -> dict:
    d = json.loads((DATA / "blocks_1.21.11.json").read_text(encoding="utf-8"))
    return {k.split(":", 1)[1]: v for k, v in d.items()}


def load_vanilla_items() -> list:
    d = json.loads((DATA / "items_1.21.11.json").read_text(encoding="utf-8"))
    return [x["name"] for x in d]


# 分类规则：按名称前缀/模式归类，便于"以后做"时分批认领。
CATEGORY_RULES = [
    ("珊瑚（活/死 珊瑚体与珊瑚块变种）", r"^(dead_)?(tube|brain|bubble|fire|horn)_coral$"),
    ("生物头颅 / 头（Skull / Head，含墙挂变体）",
     r"^(skeleton|wither_skeleton|zombie|creeper|dragon|piglin|player)_(wall_)?(skull|head)$"),
    ("铜火把", r"^copper_(wall_)?torch$"),
    ("光源方块 / 测试方块（调试/管理员）",
     r"^(light|test_block|test_instance_block|structure_block|jigsaw|command_block|"
     r"chain_command_block|repeating_command_block|barrier|debug_stick|debug_stick)$"),
    ("干燥恶魂", r"^dried_ghast$"),
    ("蜜脾块", r"^honeycomb_block$"),
    ("石英 / 平滑石（建筑基础方块）", r"^(quartz_bricks|smooth_stone)$"),
    ("旗帜图案物品（Banner Pattern Item）", r"_banner_pattern$"),
    ("染料（Dye）", r"^(black|blue|brown|white|orange|magenta|light_blue|yellow|lime|pink|"
                    r"gray|light_gray|cyan|purple|green|red)_dye$"),
    ("矿物 / 材料（锭、粒、碎片、球）",
     r"^(clay_ball|copper_nugget|gold_nugget|iron_nugget|nether_brick|prismarine_crystals|"
     r"prismarine_shard|shulker_shell|echo_shard|glow_ink_sac|disc_fragment_5|"
     r"popped_chorus_fruit)$"),
    ("工具 / 特殊物品",
     r"^(spyglass|totem_of_undying|end_crystal|glow_item_frame|goat_horn|armor_stand|"
     r"tadpole_bucket|bundle|white_bundle)$"),
    ("铁砧变种", r"^(chipped_anvil|damaged_anvil)$"),
]


def categorize(name: str) -> str:
    for label, rx in CATEGORY_RULES:
        if re.match(rx, name):
            return label
    return "其他"


def main() -> int:
    registered = collect_registered()
    vblocks = load_vanilla_blocks()
    vitems = load_vanilla_items()

    missing_blocks = sorted(n for n in vblocks if n not in registered)
    missing_items = sorted(n for n in vitems if n not in registered)

    lines = []
    lines.append("# 未实现的原版方块 / 物品清单")
    lines.append("")
    lines.append("> 本文件由 `scripts/build_unimplemented_content_doc.py` 自动生成，勿手动编辑。")
    lines.append("")
    lines.append("## 判定口径")
    lines.append("")
    lines.append(f"- 原版全集：`assets/data/blocks_1.21.11.json`（{len(vblocks)} 个 block）、"
                 f"`assets/data/items_1.21.11.json`（{len(vitems)} 个 item）。")
    lines.append("- 已注册集合：扫描 `src/common/world/block/registry/**.cpp`、`src/common/item/**.cpp` "
                 "中出现的真实注册范式（`ResourceLocation(\"minecraft:X\")` 字面量、"
                 "`register*()` 调用内的字符串名）。")
    lines.append("- 该口径为**近似**：仅在注释里出现名字不计入已注册（避免漏报），"
                 "但个别间接注册（id 由变量拼接而非字面量）可能被计入缺失（可能轻微高报）。")
    lines.append("")
    lines.append(f"## 汇总：未实现方块 {len(missing_blocks)} 个，未实现物品 {len(missing_items)} 个")
    lines.append("")

    def emit(title, names):
        lines.append(f"### {title}")
        lines.append("")
        groups = {}
        for n in names:
            groups.setdefault(categorize(n), []).append(n)
        for cat in sorted(groups):
            lst = groups[cat]
            lines.append(f"**{cat}**（{len(lst)}）")
            lines.append("")
            for n in lst:
                lines.append(f"- `minecraft:{n}`")
            lines.append("")

    emit("未实现方块", missing_blocks)
    emit("未实现物品", missing_items)

    OUT.write_text("\n".join(lines) + "\n", encoding="utf-8")
    print(f"未实现方块: {len(missing_blocks)}")
    print(f"未实现物品: {len(missing_items)}")
    print(f"Wrote {OUT}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
