# 未实现的原版方块 / 物品清单

> 本文件由 `scripts/build_unimplemented_content_doc.py` 自动生成，勿手动编辑。

## 判定口径

- 原版全集：`assets/data/blocks_1.21.11.json`（1166 个 block）、`assets/data/items_1.21.11.json`（1505 个 item）。
- 已注册集合：扫描 `src/common/world/block/registry/**.cpp`、`src/common/item/**.cpp` 中出现的真实注册范式（`ResourceLocation("minecraft:X")` 字面量、`register*()` 调用内的字符串名）。
- 该口径为**近似**：仅在注释里出现名字不计入已注册（避免漏报），但个别间接注册（id 由变量拼接而非字面量）可能被计入缺失（可能轻微高报）。

## 汇总：未实现方块 10 个，未实现物品 11 个

### 未实现方块

**光源方块 / 测试方块（调试/管理员）**（3）

- `minecraft:light`
- `minecraft:test_block`
- `minecraft:test_instance_block`

**生物头颅 / 头（Skull / Head，含墙挂变体）**（7）

- `minecraft:creeper_wall_head`
- `minecraft:dragon_wall_head`
- `minecraft:piglin_wall_head`
- `minecraft:player_wall_head`
- `minecraft:skeleton_wall_skull`
- `minecraft:wither_skeleton_wall_skull`
- `minecraft:zombie_wall_head`

### 未实现物品

**光源方块 / 测试方块（调试/管理员）**（4）

- `minecraft:debug_stick`
- `minecraft:light`
- `minecraft:test_block`
- `minecraft:test_instance_block`

**工具 / 特殊物品**（7）

- `minecraft:armor_stand`
- `minecraft:end_crystal`
- `minecraft:glow_item_frame`
- `minecraft:goat_horn`
- `minecraft:spyglass`
- `minecraft:tadpole_bucket`
- `minecraft:totem_of_undying`

