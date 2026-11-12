/**
 * `dispense_behavior` 能力的已知缺口：Cubium 尚未注册的发射器行为。
 *
 * 每一条都是「原版有专用行为、Cubium 走默认投掷」的真实实现缺口。清单必须随实现
 * 推进逐条删除——否则会出现「豁免了原版也有的项」的假一致。
 *
 * ## 分组与修法
 *
 * 1. **潜影盒 17 项**：需新增 `ShulkerBoxDispenseBehavior`，逻辑见
 *    `net/minecraft/core/dispenser/ShulkerBoxDispenseBehavior.java`：
 *    取 `FACING` 方向的前一格，若其下方为空则沿 FACING 放置、否则朝 UP 放置
 *    （`DirectionalPlaceContext`），放置失败时 `setSuccess(false)` 以避免消耗物品与播放音效。
 *
 * 2. **矿车 6 项**：需新增 `MinecartDispenseItemBehavior`，沿 FACING 方向在轨道上放置矿车。
 *
 * 3. **剪刀 1 项**：需新增 `ShearsDispenseItemBehavior`，对前方可剪实体（羊、哞菇）执行剪切。
 *
 * 4. **船 10 项**：`BoatDispenseBehavior` 已存在且已注册部分木材，缺的是
 *    `*_chest_boat` 这些**带箱子的变体**——注册时需传入对应的 chest boat 实体类型。
 *
 * 5. **刷怪蛋等 97 项 `custom`**：原版为每类物品写了独立的匿名行为（如刷怪蛋在发射位置
 *    生成对应生物）。这部分需要逐个物品补齐，工作量最大。
 *
 * 6. **`successDefault` 不一致 10 项**：Cubium 用了非 `Optional` 的基类，导致
 *    `isSuccess()` 恒为 true —— 放置失败时仍会消耗物品并播放音效。
 *
 * TODO: 按上述分组补齐实现后，逐条删除本清单中的对应项。
 */
export const KNOWN_DISPENSE_GAPS: readonly string[] = [
    // 潜影盒（17）
    "minecraft:black_shulker_box",
    "minecraft:blue_shulker_box",
    "minecraft:brown_shulker_box",
    "minecraft:cyan_shulker_box",
    "minecraft:gray_shulker_box",
    "minecraft:green_shulker_box",
    "minecraft:light_blue_shulker_box",
    "minecraft:light_gray_shulker_box",
    "minecraft:lime_shulker_box",
    "minecraft:magenta_shulker_box",
    "minecraft:orange_shulker_box",
    "minecraft:pink_shulker_box",
    "minecraft:purple_shulker_box",
    "minecraft:red_shulker_box",
    "minecraft:shulker_box",
    "minecraft:white_shulker_box",
    "minecraft:yellow_shulker_box",
    // 矿车（6）
    "minecraft:chest_minecart",
    "minecraft:command_block_minecart",
    "minecraft:furnace_minecart",
    "minecraft:hopper_minecart",
    "minecraft:minecart",
    "minecraft:tnt_minecart",
    // 剪刀（1）
    "minecraft:shears",
    // 带箱子的船（10）
    "minecraft:acacia_chest_boat",
    "minecraft:birch_chest_boat",
    "minecraft:cherry_chest_boat",
    "minecraft:dark_oak_chest_boat",
    "minecraft:jungle_chest_boat",
    "minecraft:mangrove_chest_boat",
    "minecraft:oak_chest_boat",
    "minecraft:pale_oak_chest_boat",
    "minecraft:spruce_chest_boat",
    "minecraft:bamboo_chest_raft",
    // 刷怪蛋（87）：原版在发射位置生成对应生物，Cubium 尚未实现该行为
    "minecraft:allay_spawn_egg",
    "minecraft:armadillo_spawn_egg",
    "minecraft:axolotl_spawn_egg",
    "minecraft:bat_spawn_egg",
    "minecraft:bee_spawn_egg",
    "minecraft:blaze_spawn_egg",
    "minecraft:bogged_spawn_egg",
    "minecraft:breeze_spawn_egg",
    "minecraft:camel_husk_spawn_egg",
    "minecraft:camel_spawn_egg",
    "minecraft:cat_spawn_egg",
    "minecraft:cave_spider_spawn_egg",
    "minecraft:chicken_spawn_egg",
    "minecraft:cod_spawn_egg",
    "minecraft:copper_golem_spawn_egg",
    "minecraft:cow_spawn_egg",
    "minecraft:creaking_spawn_egg",
    "minecraft:creeper_spawn_egg",
    "minecraft:dolphin_spawn_egg",
    "minecraft:donkey_spawn_egg",
    "minecraft:drowned_spawn_egg",
    "minecraft:elder_guardian_spawn_egg",
    "minecraft:ender_dragon_spawn_egg",
    "minecraft:enderman_spawn_egg",
    "minecraft:endermite_spawn_egg",
    "minecraft:evoker_spawn_egg",
    "minecraft:fox_spawn_egg",
    "minecraft:frog_spawn_egg",
    "minecraft:ghast_spawn_egg",
    "minecraft:glow_squid_spawn_egg",
    "minecraft:goat_spawn_egg",
    "minecraft:guardian_spawn_egg",
    "minecraft:happy_ghast_spawn_egg",
    "minecraft:hoglin_spawn_egg",
    "minecraft:horse_spawn_egg",
    "minecraft:husk_spawn_egg",
    "minecraft:iron_golem_spawn_egg",
    "minecraft:llama_spawn_egg",
    "minecraft:magma_cube_spawn_egg",
    "minecraft:mooshroom_spawn_egg",
    "minecraft:mule_spawn_egg",
    "minecraft:nautilus_spawn_egg",
    "minecraft:ocelot_spawn_egg",
    "minecraft:panda_spawn_egg",
    "minecraft:parched_spawn_egg",
    "minecraft:parrot_spawn_egg",
    "minecraft:phantom_spawn_egg",
    "minecraft:pig_spawn_egg",
    "minecraft:piglin_brute_spawn_egg",
    "minecraft:piglin_spawn_egg",
    "minecraft:pillager_spawn_egg",
    "minecraft:polar_bear_spawn_egg",
    "minecraft:pufferfish_spawn_egg",
    "minecraft:rabbit_spawn_egg",
    "minecraft:ravager_spawn_egg",
    "minecraft:salmon_spawn_egg",
    "minecraft:sheep_spawn_egg",
    "minecraft:shulker_spawn_egg",
    "minecraft:silverfish_spawn_egg",
    "minecraft:skeleton_horse_spawn_egg",
    "minecraft:skeleton_spawn_egg",
    "minecraft:slime_spawn_egg",
    "minecraft:sniffer_spawn_egg",
    "minecraft:snow_golem_spawn_egg",
    "minecraft:spider_spawn_egg",
    "minecraft:squid_spawn_egg",
    "minecraft:stray_spawn_egg",
    "minecraft:strider_spawn_egg",
    "minecraft:tadpole_spawn_egg",
    "minecraft:trader_llama_spawn_egg",
    "minecraft:tropical_fish_spawn_egg",
    "minecraft:turtle_spawn_egg",
    "minecraft:vex_spawn_egg",
    "minecraft:villager_spawn_egg",
    "minecraft:vindicator_spawn_egg",
    "minecraft:wandering_trader_spawn_egg",
    "minecraft:warden_spawn_egg",
    "minecraft:witch_spawn_egg",
    "minecraft:wither_skeleton_spawn_egg",
    "minecraft:wither_spawn_egg",
    "minecraft:wolf_spawn_egg",
    "minecraft:zoglin_spawn_egg",
    "minecraft:zombie_horse_spawn_egg",
    "minecraft:zombie_nautilus_spawn_egg",
    "minecraft:zombie_spawn_egg",
    "minecraft:zombie_villager_spawn_egg",
    "minecraft:zombified_piglin_spawn_egg",
    // 生物桶（8）：axolotl/cod/pufferfish/salmon/tadpole/tropical_fish/water/lava
    // 原版在发射位置放出对应生物或流体，Cubium 尚未实现
    "minecraft:armor_stand",
    "minecraft:axolotl_bucket",
    "minecraft:brush",
    "minecraft:chest",
    "minecraft:cod_bucket",
    "minecraft:glass_bottle",
    "minecraft:glowstone",
    "minecraft:honeycomb",
    "minecraft:lava_bucket",
    "minecraft:potion",
    "minecraft:powder_snow_bucket",
    "minecraft:pufferfish_bucket",
    "minecraft:salmon_bucket",
    "minecraft:tadpole_bucket",
    "minecraft:tropical_fish_bucket",
    "minecraft:water_bucket",
];
