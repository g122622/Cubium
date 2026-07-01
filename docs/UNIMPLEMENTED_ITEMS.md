# 未实现物品清单（ItemTagLoader 警告来源）


> 本文档由脚本 `scripts/build_unimplemented_items_doc.py` 自动生成，列出 Cubium 当前注册物品集合与
> 原版数据包（`minecraft_reborn/datapacks/Vanilla`）物品标签（item tags）所引用物品之间的差集。
> 这些物品在标签加载阶段触发 `ItemTagLoader: unknown item 'X' (required), skipping (tag: Y)` 警告，
> 属于「数据包引用了尚未实现的物品」，并非注册顺序 bug（物品注册早于标签加载，见 
`src/server/application/MinecraftServer.cpp` 的 `initializeRegistries()`）。


## 摘要


- 数据包物品标签文件数：**198**
- 标签引用的去重叶子物品数：**882**
- Cubium 已注册物品数（源码扫描）：**858**
- 标签引用但未实现的物品数：**363**
- 完全无法解析（标签内全部叶子物品均未实现）的标签数：**21**
- 部分缺失（至少一个叶子物品未实现）的标签数：**50**

## 注册物品来源


| 来源文件 | 注册方式 | 说明 |
| --- | --- | --- |
| `src/common/item/Items.cpp` | `registerItem(ResourceLocation("minecraft:X"), ...)` | 直接注册物品（含工具/食物/材料等） |
| `src/common/item/Items.cpp` | `registerBlockBackedItem(registry, VanillaBlocks::X, "name", ...)` | 以方块为底注册 BlockItem，物品 id 为 `minecraft:name` |
| `src/common/item/items/block/BlockItemRegistry.cpp` | `registerSimpleBlock(VanillaBlocks::X, "name")` | 注册简单方块物品，物品 id 取自方块的 `blockLocation()`（与 `name` 一致） |
| `src/common/item/items/block/BlockItemRegistry.cpp` | `registerItem<...>(ResourceLocation("minecraft:X"), ...)` | 显式注册特殊 BlockItem |

## 完全无法解析的标签（所有叶子物品均未实现）


这些标签会输出 `ItemTagLoader: tag 'X' resolved no valid items` 信息级日志，且其中每一项都触发一条警告。


| 标签 | 未实现的叶子物品数 |
| --- | --- |
| `minecraft:anvil` | 3 |
| `minecraft:bamboo_blocks` | 2 |
| `minecraft:beds` | 16 |
| `minecraft:bundles` | 17 |
| `minecraft:candles` | 17 |
| `minecraft:chains` | 9 |
| `minecraft:cherry_logs` | 4 |
| `minecraft:copper` | 8 |
| `minecraft:copper_chests` | 8 |
| `minecraft:copper_golem_statues` | 8 |
| `minecraft:decorated_pot_ingredients` | 1 |
| `minecraft:duplicates_allays` | 1 |
| `minecraft:harnesses` | 16 |
| `minecraft:lightning_rods` | 8 |
| `minecraft:mangrove_logs` | 4 |
| `minecraft:pale_oak_logs` | 4 |
| `minecraft:repairs_turtle_helmet` | 1 |
| `minecraft:repairs_wolf_armor` | 1 |
| `minecraft:sniffer_food` | 1 |
| `minecraft:spears` | 7 |
| `minecraft:wooden_shelves` | 12 |

## 部分缺失的标签（至少一个叶子物品未实现）


| 标签 | 未实现叶子物品数 |
| --- | --- |
| `minecraft:axes` | 1 |
| `minecraft:bars` | 8 |
| `minecraft:bee_food` | 11 |
| `minecraft:bookshelf_books` | 1 |
| `minecraft:chicken_food` | 2 |
| `minecraft:cluster_max_harvestables` | 1 |
| `minecraft:coal_ores` | 1 |
| `minecraft:compasses` | 1 |
| `minecraft:copper_ores` | 1 |
| `minecraft:diamond_ores` | 1 |
| `minecraft:dirt` | 5 |
| `minecraft:doors` | 8 |
| `minecraft:dyeable` | 1 |
| `minecraft:eggs` | 2 |
| `minecraft:emerald_ores` | 1 |
| `minecraft:enchantable/durability` | 1 |
| `minecraft:flowers` | 9 |
| `minecraft:gold_ores` | 1 |
| `minecraft:hoes` | 1 |
| `minecraft:iron_ores` | 1 |
| `minecraft:lanterns` | 8 |
| `minecraft:lapis_ores` | 1 |
| `minecraft:leaves` | 5 |
| `minecraft:non_flammable_wood` | 14 |
| `minecraft:parrot_food` | 2 |
| `minecraft:pickaxes` | 1 |
| `minecraft:piglin_loved` | 6 |
| `minecraft:piglin_preferred_weapons` | 1 |
| `minecraft:planks` | 6 |
| `minecraft:redstone_ores` | 1 |
| `minecraft:sand` | 1 |
| `minecraft:saplings` | 5 |
| `minecraft:shovels` | 1 |
| `minecraft:shulker_boxes` | 16 |
| `minecraft:slabs` | 40 |
| `minecraft:small_flowers` | 3 |
| `minecraft:stairs` | 36 |
| `minecraft:stone_buttons` | 1 |
| `minecraft:stone_crafting_materials` | 1 |
| `minecraft:stone_tool_materials` | 1 |
| `minecraft:swords` | 1 |
| `minecraft:trapdoors` | 8 |
| `minecraft:trim_materials` | 2 |
| `minecraft:villager_plantable_seeds` | 2 |
| `minecraft:walls` | 23 |
| `minecraft:wooden_doors` | 2 |
| `minecraft:wooden_fences` | 2 |
| `minecraft:wooden_slabs` | 2 |
| `minecraft:wooden_stairs` | 2 |
| `minecraft:wooden_trapdoors` | 2 |

## 未实现物品完整清单（按分类）


共 **364** 项。物品 id 形如 `minecraft:anvil`。


### Armor（2）


- `minecraft:golden_nautilus_armor`
- `minecraft:wolf_armor`

### Beds（16）


- `minecraft:black_bed`
- `minecraft:blue_bed`
- `minecraft:brown_bed`
- `minecraft:cyan_bed`
- `minecraft:gray_bed`
- `minecraft:green_bed`
- `minecraft:light_blue_bed`
- `minecraft:light_gray_bed`
- `minecraft:lime_bed`
- `minecraft:magenta_bed`
- `minecraft:orange_bed`
- `minecraft:pink_bed`
- `minecraft:purple_bed`
- `minecraft:red_bed`
- `minecraft:white_bed`
- `minecraft:yellow_bed`

### Buttons（1）


- `minecraft:polished_blackstone_button ✅ 已完成`

### Candles（17）


- `minecraft:black_candle`
- `minecraft:blue_candle`
- `minecraft:brown_candle`
- `minecraft:candle`
- `minecraft:cyan_candle`
- `minecraft:gray_candle`
- `minecraft:green_candle`
- `minecraft:light_blue_candle`
- `minecraft:light_gray_candle`
- `minecraft:lime_candle`
- `minecraft:magenta_candle`
- `minecraft:orange_candle`
- `minecraft:pink_candle`
- `minecraft:purple_candle`
- `minecraft:red_candle`
- `minecraft:white_candle`
- `minecraft:yellow_candle`

### Copper（55）


- `minecraft:copper_axe`
- `minecraft:copper_bars`
- `minecraft:copper_block`
- `minecraft:copper_chain`
- `minecraft:copper_chest`
- `minecraft:copper_golem_statue`
- `minecraft:copper_hoe`
- `minecraft:copper_lantern`
- `minecraft:copper_pickaxe`
- `minecraft:copper_shovel`
- `minecraft:copper_spear`
- `minecraft:copper_sword`
- `minecraft:deepslate_copper_ore ✅ 已完成`
- `minecraft:exposed_copper`
- `minecraft:exposed_copper_bars`
- `minecraft:exposed_copper_chain`
- `minecraft:exposed_copper_chest`
- `minecraft:exposed_copper_golem_statue`
- `minecraft:exposed_copper_lantern`
- `minecraft:oxidized_copper`
- `minecraft:oxidized_copper_bars`
- `minecraft:oxidized_copper_chain`
- `minecraft:oxidized_copper_chest`
- `minecraft:oxidized_copper_golem_statue`
- `minecraft:oxidized_copper_lantern`
- `minecraft:waxed_copper_bars`
- `minecraft:waxed_copper_block`
- `minecraft:waxed_copper_chain`
- `minecraft:waxed_copper_chest`
- `minecraft:waxed_copper_golem_statue`
- `minecraft:waxed_copper_lantern`
- `minecraft:waxed_exposed_copper`
- `minecraft:waxed_exposed_copper_bars`
- `minecraft:waxed_exposed_copper_chain`
- `minecraft:waxed_exposed_copper_chest`
- `minecraft:waxed_exposed_copper_golem_statue`
- `minecraft:waxed_exposed_copper_lantern`
- `minecraft:waxed_oxidized_copper`
- `minecraft:waxed_oxidized_copper_bars`
- `minecraft:waxed_oxidized_copper_chain`
- `minecraft:waxed_oxidized_copper_chest`
- `minecraft:waxed_oxidized_copper_golem_statue`
- `minecraft:waxed_oxidized_copper_lantern`
- `minecraft:waxed_weathered_copper`
- `minecraft:waxed_weathered_copper_bars`
- `minecraft:waxed_weathered_copper_chain`
- `minecraft:waxed_weathered_copper_chest`
- `minecraft:waxed_weathered_copper_golem_statue`
- `minecraft:waxed_weathered_copper_lantern`
- `minecraft:weathered_copper`
- `minecraft:weathered_copper_bars`
- `minecraft:weathered_copper_chain`
- `minecraft:weathered_copper_chest`
- `minecraft:weathered_copper_golem_statue`
- `minecraft:weathered_copper_lantern`

### Doors（10）


- `minecraft:copper_door`
- `minecraft:crimson_door`
- `minecraft:exposed_copper_door`
- `minecraft:oxidized_copper_door`
- `minecraft:warped_door`
- `minecraft:waxed_copper_door`
- `minecraft:waxed_exposed_copper_door`
- `minecraft:waxed_oxidized_copper_door`
- `minecraft:waxed_weathered_copper_door`
- `minecraft:weathered_copper_door`

### Fences / Fence Gates（3）


- `minecraft:crimson_fence`
- `minecraft:nether_brick_fence ✅ 已完成`
- `minecraft:warped_fence`

### Flowers / Small Flowers（5）


- `minecraft:cactus_flower`
- `minecraft:closed_eyeblossom`
- `minecraft:open_eyeblossom`
- `minecraft:pitcher_plant`
- `minecraft:torchflower`

### Leaves（5）


- `minecraft:azalea_leaves`
- `minecraft:cherry_leaves`
- `minecraft:flowering_azalea_leaves`
- `minecraft:mangrove_leaves`
- `minecraft:pale_oak_leaves ✅ 已完成`

### Logs / Wood / Stems（12）


- `minecraft:cherry_log`
- `minecraft:cherry_wood`
- `minecraft:mangrove_log`
- `minecraft:mangrove_wood`
- `minecraft:pale_oak_log ✅ 已完成`
- `minecraft:pale_oak_wood ✅ 已完成`
- `minecraft:stripped_cherry_log`
- `minecraft:stripped_cherry_wood`
- `minecraft:stripped_mangrove_log`
- `minecraft:stripped_mangrove_wood`
- `minecraft:stripped_pale_oak_log ✅ 已完成`
- `minecraft:stripped_pale_oak_wood ✅ 已完成`

### Planks（6）


- `minecraft:bamboo_planks`
- `minecraft:cherry_planks`
- `minecraft:crimson_planks`
- `minecraft:mangrove_planks`
- `minecraft:pale_oak_planks ✅ 已完成`
- `minecraft:warped_planks`

### Saplings（2）


- `minecraft:cherry_sapling`
- `minecraft:pale_oak_sapling ✅ 已完成`

### Shulker Boxes（16）


- `minecraft:black_shulker_box`
- `minecraft:blue_shulker_box`
- `minecraft:brown_shulker_box`
- `minecraft:cyan_shulker_box`
- `minecraft:gray_shulker_box`
- `minecraft:green_shulker_box`
- `minecraft:light_blue_shulker_box`
- `minecraft:light_gray_shulker_box`
- `minecraft:lime_shulker_box`
- `minecraft:magenta_shulker_box`
- `minecraft:orange_shulker_box`
- `minecraft:pink_shulker_box`
- `minecraft:purple_shulker_box`
- `minecraft:red_shulker_box`
- `minecraft:white_shulker_box`
- `minecraft:yellow_shulker_box`

### Slabs（42）


- `minecraft:andesite_slab ✅ 已完成`
- `minecraft:blackstone_slab ✅ 已完成`
- `minecraft:brick_slab ✅ 已完成`
- `minecraft:cobbled_deepslate_slab ✅ 已完成`
- `minecraft:crimson_slab`
- `minecraft:cut_copper_slab ✅ 已完成`
- `minecraft:cut_red_sandstone_slab ✅ 已完成`
- `minecraft:cut_sandstone_slab ✅ 已完成`
- `minecraft:deepslate_brick_slab ✅ 已完成`
- `minecraft:deepslate_tile_slab ✅ 已完成`
- `minecraft:diorite_slab ✅ 已完成`
- `minecraft:end_stone_brick_slab ✅ 已完成`
- `minecraft:exposed_cut_copper_slab ✅ 已完成`
- `minecraft:granite_slab ✅ 已完成`
- `minecraft:mossy_cobblestone_slab ✅ 已完成`
- `minecraft:mud_brick_slab ✅ 已完成`
- `minecraft:nether_brick_slab ✅ 已完成`
- `minecraft:oxidized_cut_copper_slab ✅ 已完成`
- `minecraft:petrified_oak_slab ✅ 已完成`
- `minecraft:polished_andesite_slab ✅ 已完成`
- `minecraft:polished_blackstone_brick_slab ✅ 已完成`
- `minecraft:polished_blackstone_slab ✅ 已完成`
- `minecraft:polished_deepslate_slab ✅ 已完成`
- `minecraft:polished_diorite_slab ✅ 已完成`
- `minecraft:polished_granite_slab ✅ 已完成`
- `minecraft:polished_tuff_slab ✅ 已完成`
- `minecraft:purpur_slab ✅ 已完成`
- `minecraft:quartz_slab ✅ 已完成`
- `minecraft:red_nether_brick_slab ✅ 已完成`
- `minecraft:red_sandstone_slab ✅ 已完成`
- `minecraft:resin_brick_slab ✅ 已完成`
- `minecraft:smooth_quartz_slab ✅ 已完成`
- `minecraft:smooth_red_sandstone_slab ✅ 已完成`
- `minecraft:smooth_stone_slab ✅ 已完成`
- `minecraft:tuff_brick_slab ✅ 已完成`
- `minecraft:tuff_slab ✅ 已完成`
- `minecraft:warped_slab`
- `minecraft:waxed_cut_copper_slab ✅ 已完成`
- `minecraft:waxed_exposed_cut_copper_slab ✅ 已完成`
- `minecraft:waxed_oxidized_cut_copper_slab ✅ 已完成`
- `minecraft:waxed_weathered_cut_copper_slab ✅ 已完成`
- `minecraft:weathered_cut_copper_slab ✅ 已完成`

### Stairs（38）


- `minecraft:andesite_stairs ✅ 已完成`
- `minecraft:blackstone_stairs ✅ 已完成`
- `minecraft:brick_stairs ✅ 已完成`
- `minecraft:cobbled_deepslate_stairs ✅ 已完成`
- `minecraft:crimson_stairs`
- `minecraft:cut_copper_stairs ✅ 已完成`
- `minecraft:deepslate_brick_stairs ✅ 已完成`
- `minecraft:deepslate_tile_stairs ✅ 已完成`
- `minecraft:diorite_stairs ✅ 已完成`
- `minecraft:end_stone_brick_stairs ✅ 已完成`
- `minecraft:exposed_cut_copper_stairs ✅ 已完成`
- `minecraft:granite_stairs ✅ 已完成`
- `minecraft:mossy_cobblestone_stairs ✅ 已完成`
- `minecraft:mud_brick_stairs ✅ 已完成`
- `minecraft:nether_brick_stairs ✅ 已完成`
- `minecraft:oxidized_cut_copper_stairs ✅ 已完成`
- `minecraft:polished_andesite_stairs ✅ 已完成`
- `minecraft:polished_blackstone_brick_stairs ✅ 已完成`
- `minecraft:polished_blackstone_stairs ✅ 已完成`
- `minecraft:polished_deepslate_stairs ✅ 已完成`
- `minecraft:polished_diorite_stairs ✅ 已完成`
- `minecraft:polished_granite_stairs ✅ 已完成`
- `minecraft:polished_tuff_stairs ✅ 已完成`
- `minecraft:purpur_stairs ✅ 已完成`
- `minecraft:quartz_stairs ✅ 已完成`
- `minecraft:red_nether_brick_stairs ✅ 已完成`
- `minecraft:red_sandstone_stairs ✅ 已完成`
- `minecraft:resin_brick_stairs ✅ 已完成`
- `minecraft:smooth_quartz_stairs ✅ 已完成`
- `minecraft:smooth_red_sandstone_stairs ✅ 已完成`
- `minecraft:tuff_brick_stairs ✅ 已完成`
- `minecraft:tuff_stairs ✅ 已完成`
- `minecraft:warped_stairs`
- `minecraft:waxed_cut_copper_stairs ✅ 已完成`
- `minecraft:waxed_exposed_cut_copper_stairs ✅ 已完成`
- `minecraft:waxed_oxidized_cut_copper_stairs ✅ 已完成`
- `minecraft:waxed_weathered_cut_copper_stairs ✅ 已完成`
- `minecraft:weathered_cut_copper_stairs ✅ 已完成`

### Tools / Weapons（6）


- `minecraft:diamond_spear`
- `minecraft:golden_spear`
- `minecraft:iron_spear`
- `minecraft:netherite_spear`
- `minecraft:stone_spear`
- `minecraft:wooden_spear`

### Trapdoors（10）


- `minecraft:copper_trapdoor`
- `minecraft:crimson_trapdoor`
- `minecraft:exposed_copper_trapdoor`
- `minecraft:oxidized_copper_trapdoor`
- `minecraft:warped_trapdoor`
- `minecraft:waxed_copper_trapdoor`
- `minecraft:waxed_exposed_copper_trapdoor`
- `minecraft:waxed_oxidized_copper_trapdoor`
- `minecraft:waxed_weathered_copper_trapdoor`
- `minecraft:weathered_copper_trapdoor`

### Walls（23）


- `minecraft:andesite_wall ✅ 已完成`
- `minecraft:blackstone_wall ✅ 已完成`
- `minecraft:brick_wall ✅ 已完成`
- `minecraft:cobbled_deepslate_wall ✅ 已完成`
- `minecraft:deepslate_brick_wall ✅ 已完成`
- `minecraft:deepslate_tile_wall ✅ 已完成`
- `minecraft:diorite_wall ✅ 已完成`
- `minecraft:end_stone_brick_wall ✅ 已完成`
- `minecraft:granite_wall ✅ 已完成`
- `minecraft:mossy_cobblestone_wall ✅ 已完成`
- `minecraft:mud_brick_wall ✅ 已完成`
- `minecraft:nether_brick_wall ✅ 已完成`
- `minecraft:polished_blackstone_brick_wall ✅ 已完成`
- `minecraft:polished_blackstone_wall ✅ 已完成`
- `minecraft:polished_deepslate_wall ✅ 已完成`
- `minecraft:polished_tuff_wall ✅ 已完成`
- `minecraft:prismarine_wall ✅ 已完成`
- `minecraft:red_nether_brick_wall ✅ 已完成`
- `minecraft:red_sandstone_wall ✅ 已完成`
- `minecraft:resin_brick_wall ✅ 已完成`
- `minecraft:sandstone_wall ✅ 已完成`
- `minecraft:tuff_brick_wall ✅ 已完成`
- `minecraft:tuff_wall ✅ 已完成`

### Other（95）


- `minecraft:acacia_shelf`
- `minecraft:amethyst_shard ✅ 已完成`
- `minecraft:anvil`
- `minecraft:armadillo_scute ✅ 已完成`
- `minecraft:azalea`
- `minecraft:bamboo_block`
- `minecraft:bamboo_shelf`
- `minecraft:bell`
- `minecraft:birch_shelf`
- `minecraft:black_bundle`
- `minecraft:black_harness`
- `minecraft:blue_bundle`
- `minecraft:blue_egg`
- `minecraft:blue_harness`
- `minecraft:brick ✅ 已完成`
- `minecraft:brown_bundle`
- `minecraft:brown_egg`
- `minecraft:brown_harness`
- `minecraft:brush ✅ 已完成`
- `minecraft:bundle`
- `minecraft:cherry_shelf`
- `minecraft:chipped_anvil`
- `minecraft:cobbled_deepslate ✅ 已完成`
- `minecraft:crimson_shelf`
- `minecraft:cyan_bundle`
- `minecraft:cyan_harness`
- `minecraft:damaged_anvil`
- `minecraft:dark_oak_shelf`
- `minecraft:deepslate_coal_ore ✅ 已完成`
- `minecraft:deepslate_diamond_ore ✅ 已完成`
- `minecraft:deepslate_emerald_ore ✅ 已完成`
- `minecraft:deepslate_gold_ore ✅ 已完成`
- `minecraft:deepslate_iron_ore ✅ 已完成`
- `minecraft:deepslate_lapis_ore ✅ 已完成`
- `minecraft:deepslate_redstone_ore ✅ 已完成`
- `minecraft:exposed_lightning_rod`
- `minecraft:flowering_azalea`
- `minecraft:gilded_blackstone ✅ 已完成`
- `minecraft:gray_bundle`
- `minecraft:gray_harness`
- `minecraft:green_bundle`
- `minecraft:green_harness`
- `minecraft:iron_chain`
- `minecraft:jungle_shelf`
- `minecraft:knowledge_book`
- `minecraft:light_blue_bundle`
- `minecraft:light_blue_harness`
- `minecraft:light_gray_bundle`
- `minecraft:light_gray_harness`
- `minecraft:lightning_rod`
- `minecraft:lime_bundle`
- `minecraft:lime_harness`
- `minecraft:magenta_bundle`
- `minecraft:magenta_harness`
- `minecraft:mangrove_propagule`
- `minecraft:mangrove_shelf`
- `minecraft:moss_block`
- `minecraft:mud`
- `minecraft:muddy_mangrove_roots`
- `minecraft:oak_shelf`
- `minecraft:orange_bundle`
- `minecraft:orange_harness`
- `minecraft:oxidized_lightning_rod`
- `minecraft:pale_moss_block ✅ 已完成`
- `minecraft:pale_oak_shelf`
- `minecraft:pink_bundle`
- `minecraft:pink_harness`
- `minecraft:pink_petals`
- `minecraft:pitcher_pod`
- `minecraft:purple_bundle`
- `minecraft:purple_harness`
- `minecraft:raw_gold`
- `minecraft:raw_gold_block`
- `minecraft:recovery_compass`
- `minecraft:red_bundle`
- `minecraft:red_harness`
- `minecraft:resin_brick ✅ 已完成`
- `minecraft:rooted_dirt`
- `minecraft:spore_blossom`
- `minecraft:spruce_shelf`
- `minecraft:stripped_bamboo_block`
- `minecraft:suspicious_sand`
- `minecraft:torchflower_seeds`
- `minecraft:turtle_scute`
- `minecraft:warped_shelf`
- `minecraft:waxed_exposed_lightning_rod`
- `minecraft:waxed_lightning_rod`
- `minecraft:waxed_oxidized_lightning_rod`
- `minecraft:waxed_weathered_lightning_rod`
- `minecraft:weathered_lightning_rod`
- `minecraft:white_bundle`
- `minecraft:white_harness`
- `minecraft:wildflowers`
- `minecraft:yellow_bundle`
- `minecraft:yellow_harness`

## 附录：完全无法解析标签的逐项明细


下表列出每个「完全无法解析」标签内的全部未实现物品，便于逐个实现时核对。


### `minecraft:anvil`


- `minecraft:anvil`
- `minecraft:chipped_anvil`
- `minecraft:damaged_anvil`

### `minecraft:bamboo_blocks`


- `minecraft:bamboo_block`
- `minecraft:stripped_bamboo_block`

### `minecraft:beds`


- `minecraft:black_bed`
- `minecraft:blue_bed`
- `minecraft:brown_bed`
- `minecraft:cyan_bed`
- `minecraft:gray_bed`
- `minecraft:green_bed`
- `minecraft:light_blue_bed`
- `minecraft:light_gray_bed`
- `minecraft:lime_bed`
- `minecraft:magenta_bed`
- `minecraft:orange_bed`
- `minecraft:pink_bed`
- `minecraft:purple_bed`
- `minecraft:red_bed`
- `minecraft:white_bed`
- `minecraft:yellow_bed`

### `minecraft:bundles`


- `minecraft:black_bundle`
- `minecraft:blue_bundle`
- `minecraft:brown_bundle`
- `minecraft:bundle`
- `minecraft:cyan_bundle`
- `minecraft:gray_bundle`
- `minecraft:green_bundle`
- `minecraft:light_blue_bundle`
- `minecraft:light_gray_bundle`
- `minecraft:lime_bundle`
- `minecraft:magenta_bundle`
- `minecraft:orange_bundle`
- `minecraft:pink_bundle`
- `minecraft:purple_bundle`
- `minecraft:red_bundle`
- `minecraft:white_bundle`
- `minecraft:yellow_bundle`

### `minecraft:candles`


- `minecraft:black_candle`
- `minecraft:blue_candle`
- `minecraft:brown_candle`
- `minecraft:candle`
- `minecraft:cyan_candle`
- `minecraft:gray_candle`
- `minecraft:green_candle`
- `minecraft:light_blue_candle`
- `minecraft:light_gray_candle`
- `minecraft:lime_candle`
- `minecraft:magenta_candle`
- `minecraft:orange_candle`
- `minecraft:pink_candle`
- `minecraft:purple_candle`
- `minecraft:red_candle`
- `minecraft:white_candle`
- `minecraft:yellow_candle`

### `minecraft:chains`


- `minecraft:copper_chain`
- `minecraft:exposed_copper_chain`
- `minecraft:iron_chain`
- `minecraft:oxidized_copper_chain`
- `minecraft:waxed_copper_chain`
- `minecraft:waxed_exposed_copper_chain`
- `minecraft:waxed_oxidized_copper_chain`
- `minecraft:waxed_weathered_copper_chain`
- `minecraft:weathered_copper_chain`

### `minecraft:cherry_logs`


- `minecraft:cherry_log`
- `minecraft:cherry_wood`
- `minecraft:stripped_cherry_log`
- `minecraft:stripped_cherry_wood`

### `minecraft:copper`


- `minecraft:copper_block`
- `minecraft:exposed_copper`
- `minecraft:oxidized_copper`
- `minecraft:waxed_copper_block`
- `minecraft:waxed_exposed_copper`
- `minecraft:waxed_oxidized_copper`
- `minecraft:waxed_weathered_copper`
- `minecraft:weathered_copper`

### `minecraft:copper_chests`


- `minecraft:copper_chest`
- `minecraft:exposed_copper_chest`
- `minecraft:oxidized_copper_chest`
- `minecraft:waxed_copper_chest`
- `minecraft:waxed_exposed_copper_chest`
- `minecraft:waxed_oxidized_copper_chest`
- `minecraft:waxed_weathered_copper_chest`
- `minecraft:weathered_copper_chest`

### `minecraft:copper_golem_statues`


- `minecraft:copper_golem_statue`
- `minecraft:exposed_copper_golem_statue`
- `minecraft:oxidized_copper_golem_statue`
- `minecraft:waxed_copper_golem_statue`
- `minecraft:waxed_exposed_copper_golem_statue`
- `minecraft:waxed_oxidized_copper_golem_statue`
- `minecraft:waxed_weathered_copper_golem_statue`
- `minecraft:weathered_copper_golem_statue`

### `minecraft:decorated_pot_ingredients`


- `minecraft:brick ✅ 已完成`

### `minecraft:duplicates_allays`


- `minecraft:amethyst_shard ✅ 已完成`

### `minecraft:fences`


- `minecraft:nether_brick_fence ✅ 已完成`

### `minecraft:harnesses`


- `minecraft:black_harness`
- `minecraft:blue_harness`
- `minecraft:brown_harness`
- `minecraft:cyan_harness`
- `minecraft:gray_harness`
- `minecraft:green_harness`
- `minecraft:light_blue_harness`
- `minecraft:light_gray_harness`
- `minecraft:lime_harness`
- `minecraft:magenta_harness`
- `minecraft:orange_harness`
- `minecraft:pink_harness`
- `minecraft:purple_harness`
- `minecraft:red_harness`
- `minecraft:white_harness`
- `minecraft:yellow_harness`

### `minecraft:lightning_rods`


- `minecraft:exposed_lightning_rod`
- `minecraft:lightning_rod`
- `minecraft:oxidized_lightning_rod`
- `minecraft:waxed_exposed_lightning_rod`
- `minecraft:waxed_lightning_rod`
- `minecraft:waxed_oxidized_lightning_rod`
- `minecraft:waxed_weathered_lightning_rod`
- `minecraft:weathered_lightning_rod`

### `minecraft:mangrove_logs`


- `minecraft:mangrove_log`
- `minecraft:mangrove_wood`
- `minecraft:stripped_mangrove_log`
- `minecraft:stripped_mangrove_wood`

### `minecraft:pale_oak_logs`


- `minecraft:pale_oak_log ✅ 已完成`
- `minecraft:pale_oak_wood ✅ 已完成`
- `minecraft:stripped_pale_oak_log ✅ 已完成`
- `minecraft:stripped_pale_oak_wood ✅ 已完成`

### `minecraft:repairs_turtle_helmet`


- `minecraft:turtle_scute`

### `minecraft:repairs_wolf_armor`


- `minecraft:armadillo_scute ✅ 已完成`

### `minecraft:sniffer_food`


- `minecraft:torchflower_seeds`

### `minecraft:spears`


- `minecraft:copper_spear`
- `minecraft:diamond_spear`
- `minecraft:golden_spear`
- `minecraft:iron_spear`
- `minecraft:netherite_spear`
- `minecraft:stone_spear`
- `minecraft:wooden_spear`

### `minecraft:wooden_shelves`


- `minecraft:acacia_shelf`
- `minecraft:bamboo_shelf`
- `minecraft:birch_shelf`
- `minecraft:cherry_shelf`
- `minecraft:crimson_shelf`
- `minecraft:dark_oak_shelf`
- `minecraft:jungle_shelf`
- `minecraft:mangrove_shelf`
- `minecraft:oak_shelf`
- `minecraft:pale_oak_shelf`
- `minecraft:spruce_shelf`
- `minecraft:warped_shelf`

## 附录：部分缺失标签的逐项明细


### `minecraft:axes`


未实现项：


- `minecraft:copper_axe`

### `minecraft:bars`


未实现项：


- `minecraft:copper_bars`
- `minecraft:exposed_copper_bars`
- `minecraft:oxidized_copper_bars`
- `minecraft:waxed_copper_bars`
- `minecraft:waxed_exposed_copper_bars`
- `minecraft:waxed_oxidized_copper_bars`
- `minecraft:waxed_weathered_copper_bars`
- `minecraft:weathered_copper_bars`

### `minecraft:bee_food`


未实现项：


- `minecraft:cactus_flower`
- `minecraft:cherry_leaves`
- `minecraft:flowering_azalea`
- `minecraft:flowering_azalea_leaves`
- `minecraft:mangrove_propagule`
- `minecraft:open_eyeblossom`
- `minecraft:pink_petals`
- `minecraft:pitcher_plant`
- `minecraft:spore_blossom`
- `minecraft:torchflower`
- `minecraft:wildflowers`

### `minecraft:bookshelf_books`


未实现项：


- `minecraft:knowledge_book`

### `minecraft:chicken_food`


未实现项：


- `minecraft:pitcher_pod`
- `minecraft:torchflower_seeds`

### `minecraft:cluster_max_harvestables`


未实现项：


- `minecraft:copper_pickaxe`

### `minecraft:coal_ores`


未实现项：


- `minecraft:deepslate_coal_ore ✅ 已完成`

### `minecraft:compasses`


未实现项：


- `minecraft:recovery_compass`

### `minecraft:copper_ores`


未实现项：


- `minecraft:deepslate_copper_ore ✅ 已完成`

### `minecraft:diamond_ores`


未实现项：


- `minecraft:deepslate_diamond_ore ✅ 已完成`

### `minecraft:dirt`


未实现项：


- `minecraft:moss_block`
- `minecraft:mud`
- `minecraft:muddy_mangrove_roots`
- `minecraft:pale_moss_block ✅ 已完成`
- `minecraft:rooted_dirt`

### `minecraft:doors`


未实现项：


- `minecraft:copper_door`
- `minecraft:exposed_copper_door`
- `minecraft:oxidized_copper_door`
- `minecraft:waxed_copper_door`
- `minecraft:waxed_exposed_copper_door`
- `minecraft:waxed_oxidized_copper_door`
- `minecraft:waxed_weathered_copper_door`
- `minecraft:weathered_copper_door`

### `minecraft:dyeable`


未实现项：


- `minecraft:wolf_armor`

### `minecraft:eggs`


未实现项：


- `minecraft:blue_egg`
- `minecraft:brown_egg`

### `minecraft:emerald_ores`


未实现项：


- `minecraft:deepslate_emerald_ore ✅ 已完成`

### `minecraft:enchantable/durability`


未实现项：


- `minecraft:brush ✅ 已完成`

### `minecraft:flowers`


未实现项：


- `minecraft:cactus_flower`
- `minecraft:cherry_leaves`
- `minecraft:flowering_azalea`
- `minecraft:flowering_azalea_leaves`
- `minecraft:mangrove_propagule`
- `minecraft:pink_petals`
- `minecraft:pitcher_plant`
- `minecraft:spore_blossom`
- `minecraft:wildflowers`

### `minecraft:gold_ores`


未实现项：


- `minecraft:deepslate_gold_ore ✅ 已完成`

### `minecraft:hoes`


未实现项：


- `minecraft:copper_hoe`

### `minecraft:iron_ores`


未实现项：


- `minecraft:deepslate_iron_ore ✅ 已完成`

### `minecraft:lanterns`


未实现项：


- `minecraft:copper_lantern`
- `minecraft:exposed_copper_lantern`
- `minecraft:oxidized_copper_lantern`
- `minecraft:waxed_copper_lantern`
- `minecraft:waxed_exposed_copper_lantern`
- `minecraft:waxed_oxidized_copper_lantern`
- `minecraft:waxed_weathered_copper_lantern`
- `minecraft:weathered_copper_lantern`

### `minecraft:lapis_ores`


未实现项：


- `minecraft:deepslate_lapis_ore ✅ 已完成`

### `minecraft:leaves`


未实现项：


- `minecraft:azalea_leaves`
- `minecraft:cherry_leaves`
- `minecraft:flowering_azalea_leaves`
- `minecraft:mangrove_leaves`
- `minecraft:pale_oak_leaves ✅ 已完成`

### `minecraft:non_flammable_wood`


未实现项：


- `minecraft:crimson_door`
- `minecraft:crimson_fence`
- `minecraft:crimson_planks`
- `minecraft:crimson_shelf`
- `minecraft:crimson_slab`
- `minecraft:crimson_stairs`
- `minecraft:crimson_trapdoor`
- `minecraft:warped_door`
- `minecraft:warped_fence`
- `minecraft:warped_planks`
- `minecraft:warped_shelf`
- `minecraft:warped_slab`
- `minecraft:warped_stairs`
- `minecraft:warped_trapdoor`

### `minecraft:parrot_food`


未实现项：


- `minecraft:pitcher_pod`
- `minecraft:torchflower_seeds`

### `minecraft:pickaxes`


未实现项：


- `minecraft:copper_pickaxe`

### `minecraft:piglin_loved`


未实现项：


- `minecraft:bell`
- `minecraft:gilded_blackstone ✅ 已完成`
- `minecraft:golden_nautilus_armor`
- `minecraft:golden_spear`
- `minecraft:raw_gold`
- `minecraft:raw_gold_block`

### `minecraft:piglin_preferred_weapons`


未实现项：


- `minecraft:golden_spear`

### `minecraft:planks`


未实现项：


- `minecraft:bamboo_planks`
- `minecraft:cherry_planks`
- `minecraft:crimson_planks`
- `minecraft:mangrove_planks`
- `minecraft:pale_oak_planks ✅ 已完成`
- `minecraft:warped_planks`

### `minecraft:redstone_ores`


未实现项：


- `minecraft:deepslate_redstone_ore ✅ 已完成`

### `minecraft:sand`


未实现项：


- `minecraft:suspicious_sand`

### `minecraft:saplings`


未实现项：


- `minecraft:azalea`
- `minecraft:cherry_sapling`
- `minecraft:flowering_azalea`
- `minecraft:mangrove_propagule`
- `minecraft:pale_oak_sapling ✅ 已完成`

### `minecraft:shovels`


未实现项：


- `minecraft:copper_shovel`

### `minecraft:shulker_boxes`


未实现项：


- `minecraft:black_shulker_box`
- `minecraft:blue_shulker_box`
- `minecraft:brown_shulker_box`
- `minecraft:cyan_shulker_box`
- `minecraft:gray_shulker_box`
- `minecraft:green_shulker_box`
- `minecraft:light_blue_shulker_box`
- `minecraft:light_gray_shulker_box`
- `minecraft:lime_shulker_box`
- `minecraft:magenta_shulker_box`
- `minecraft:orange_shulker_box`
- `minecraft:pink_shulker_box`
- `minecraft:purple_shulker_box`
- `minecraft:red_shulker_box`
- `minecraft:white_shulker_box`
- `minecraft:yellow_shulker_box`

### `minecraft:slabs`


未实现项：


- `minecraft:andesite_slab ✅ 已完成`
- `minecraft:blackstone_slab ✅ 已完成`
- `minecraft:brick_slab ✅ 已完成`
- `minecraft:cobbled_deepslate_slab ✅ 已完成`
- `minecraft:cut_copper_slab ✅ 已完成`
- `minecraft:cut_red_sandstone_slab ✅ 已完成`
- `minecraft:cut_sandstone_slab ✅ 已完成`
- `minecraft:deepslate_brick_slab ✅ 已完成`
- `minecraft:deepslate_tile_slab ✅ 已完成`
- `minecraft:diorite_slab ✅ 已完成`
- `minecraft:end_stone_brick_slab ✅ 已完成`
- `minecraft:exposed_cut_copper_slab ✅ 已完成`
- `minecraft:granite_slab ✅ 已完成`
- `minecraft:mossy_cobblestone_slab ✅ 已完成`
- `minecraft:mud_brick_slab ✅ 已完成`
- `minecraft:nether_brick_slab ✅ 已完成`
- `minecraft:oxidized_cut_copper_slab ✅ 已完成`
- `minecraft:petrified_oak_slab ✅ 已完成`
- `minecraft:polished_andesite_slab ✅ 已完成`
- `minecraft:polished_blackstone_brick_slab ✅ 已完成`
- `minecraft:polished_blackstone_slab ✅ 已完成`
- `minecraft:polished_deepslate_slab ✅ 已完成`
- `minecraft:polished_diorite_slab ✅ 已完成`
- `minecraft:polished_granite_slab ✅ 已完成`
- `minecraft:polished_tuff_slab ✅ 已完成`
- `minecraft:purpur_slab ✅ 已完成`
- `minecraft:quartz_slab ✅ 已完成`
- `minecraft:red_nether_brick_slab ✅ 已完成`
- `minecraft:red_sandstone_slab ✅ 已完成`
- `minecraft:resin_brick_slab ✅ 已完成`
- `minecraft:smooth_quartz_slab ✅ 已完成`
- `minecraft:smooth_red_sandstone_slab ✅ 已完成`
- `minecraft:smooth_stone_slab ✅ 已完成`
- `minecraft:tuff_brick_slab ✅ 已完成`
- `minecraft:tuff_slab ✅ 已完成`
- `minecraft:waxed_cut_copper_slab ✅ 已完成`
- `minecraft:waxed_exposed_cut_copper_slab ✅ 已完成`
- `minecraft:waxed_oxidized_cut_copper_slab ✅ 已完成`
- `minecraft:waxed_weathered_cut_copper_slab ✅ 已完成`
- `minecraft:weathered_cut_copper_slab ✅ 已完成`

### `minecraft:small_flowers`


未实现项：


- `minecraft:closed_eyeblossom`
- `minecraft:open_eyeblossom`
- `minecraft:torchflower`

### `minecraft:stairs`


未实现项：


- `minecraft:andesite_stairs ✅ 已完成`
- `minecraft:blackstone_stairs ✅ 已完成`
- `minecraft:brick_stairs ✅ 已完成`
- `minecraft:cobbled_deepslate_stairs ✅ 已完成`
- `minecraft:cut_copper_stairs ✅ 已完成`
- `minecraft:deepslate_brick_stairs ✅ 已完成`
- `minecraft:deepslate_tile_stairs ✅ 已完成`
- `minecraft:diorite_stairs ✅ 已完成`
- `minecraft:end_stone_brick_stairs ✅ 已完成`
- `minecraft:exposed_cut_copper_stairs ✅ 已完成`
- `minecraft:granite_stairs ✅ 已完成`
- `minecraft:mossy_cobblestone_stairs ✅ 已完成`
- `minecraft:mud_brick_stairs ✅ 已完成`
- `minecraft:nether_brick_stairs ✅ 已完成`
- `minecraft:oxidized_cut_copper_stairs ✅ 已完成`
- `minecraft:polished_andesite_stairs ✅ 已完成`
- `minecraft:polished_blackstone_brick_stairs ✅ 已完成`
- `minecraft:polished_blackstone_stairs ✅ 已完成`
- `minecraft:polished_deepslate_stairs ✅ 已完成`
- `minecraft:polished_diorite_stairs ✅ 已完成`
- `minecraft:polished_granite_stairs ✅ 已完成`
- `minecraft:polished_tuff_stairs ✅ 已完成`
- `minecraft:purpur_stairs ✅ 已完成`
- `minecraft:quartz_stairs ✅ 已完成`
- `minecraft:red_nether_brick_stairs ✅ 已完成`
- `minecraft:red_sandstone_stairs ✅ 已完成`
- `minecraft:resin_brick_stairs ✅ 已完成`
- `minecraft:smooth_quartz_stairs ✅ 已完成`
- `minecraft:smooth_red_sandstone_stairs ✅ 已完成`
- `minecraft:tuff_brick_stairs ✅ 已完成`
- `minecraft:tuff_stairs ✅ 已完成`
- `minecraft:waxed_cut_copper_stairs ✅ 已完成`
- `minecraft:waxed_exposed_cut_copper_stairs ✅ 已完成`
- `minecraft:waxed_oxidized_cut_copper_stairs ✅ 已完成`
- `minecraft:waxed_weathered_cut_copper_stairs ✅ 已完成`
- `minecraft:weathered_cut_copper_stairs ✅ 已完成`

### `minecraft:stone_buttons`


未实现项：


- `minecraft:polished_blackstone_button ✅ 已完成`

### `minecraft:stone_crafting_materials`


未实现项：


- `minecraft:cobbled_deepslate ✅ 已完成`

### `minecraft:stone_tool_materials`


未实现项：


- `minecraft:cobbled_deepslate ✅ 已完成`

### `minecraft:swords`


未实现项：


- `minecraft:copper_sword`

### `minecraft:trapdoors`


未实现项：


- `minecraft:copper_trapdoor`
- `minecraft:exposed_copper_trapdoor`
- `minecraft:oxidized_copper_trapdoor`
- `minecraft:waxed_copper_trapdoor`
- `minecraft:waxed_exposed_copper_trapdoor`
- `minecraft:waxed_oxidized_copper_trapdoor`
- `minecraft:waxed_weathered_copper_trapdoor`
- `minecraft:weathered_copper_trapdoor`

### `minecraft:trim_materials`


未实现项：


- `minecraft:amethyst_shard ✅ 已完成`
- `minecraft:resin_brick ✅ 已完成`

### `minecraft:villager_plantable_seeds`


未实现项：


- `minecraft:pitcher_pod`
- `minecraft:torchflower_seeds`

### `minecraft:walls`


未实现项：


- `minecraft:andesite_wall ✅ 已完成`
- `minecraft:blackstone_wall ✅ 已完成`
- `minecraft:brick_wall ✅ 已完成`
- `minecraft:cobbled_deepslate_wall ✅ 已完成`
- `minecraft:deepslate_brick_wall ✅ 已完成`
- `minecraft:deepslate_tile_wall ✅ 已完成`
- `minecraft:diorite_wall ✅ 已完成`
- `minecraft:end_stone_brick_wall ✅ 已完成`
- `minecraft:granite_wall ✅ 已完成`
- `minecraft:mossy_cobblestone_wall ✅ 已完成`
- `minecraft:mud_brick_wall ✅ 已完成`
- `minecraft:nether_brick_wall ✅ 已完成`
- `minecraft:polished_blackstone_brick_wall ✅ 已完成`
- `minecraft:polished_blackstone_wall ✅ 已完成`
- `minecraft:polished_deepslate_wall ✅ 已完成`
- `minecraft:polished_tuff_wall ✅ 已完成`
- `minecraft:prismarine_wall ✅ 已完成`
- `minecraft:red_nether_brick_wall ✅ 已完成`
- `minecraft:red_sandstone_wall ✅ 已完成`
- `minecraft:resin_brick_wall ✅ 已完成`
- `minecraft:sandstone_wall ✅ 已完成`
- `minecraft:tuff_brick_wall ✅ 已完成`
- `minecraft:tuff_wall ✅ 已完成`

### `minecraft:wooden_doors`


未实现项：


- `minecraft:crimson_door`
- `minecraft:warped_door`

### `minecraft:wooden_fences`


未实现项：


- `minecraft:crimson_fence`
- `minecraft:warped_fence`

### `minecraft:wooden_slabs`


未实现项：


- `minecraft:crimson_slab`
- `minecraft:warped_slab`

### `minecraft:wooden_stairs`


未实现项：


- `minecraft:crimson_stairs`
- `minecraft:warped_stairs`

### `minecraft:wooden_trapdoors`


未实现项：


- `minecraft:crimson_trapdoor`
- `minecraft:warped_trapdoor`
