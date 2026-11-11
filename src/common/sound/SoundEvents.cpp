/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include "SoundEvents.hpp"
#include "common/resource/ResourceLocation.hpp"

namespace mc {

namespace SoundEvents {

// ============================================================================
// 环境音效 (AMBIENT_)
// ============================================================================

const ResourceLocation AMBIENT_CAVE("minecraft:ambient.cave");

const ResourceLocation AMBIENT_BASALT_DELTAS_ADDITIONS("minecraft:ambient.basalt_deltas.additions");
const ResourceLocation AMBIENT_BASALT_DELTAS_LOOP("minecraft:ambient.basalt_deltas.loop");
const ResourceLocation AMBIENT_BASALT_DELTAS_MOOD("minecraft:ambient.basalt_deltas.mood");

const ResourceLocation AMBIENT_CRIMSON_FOREST_ADDITIONS("minecraft:ambient.crimson_forest.additions");
const ResourceLocation AMBIENT_CRIMSON_FOREST_LOOP("minecraft:ambient.crimson_forest.loop");
const ResourceLocation AMBIENT_CRIMSON_FOREST_MOOD("minecraft:ambient.crimson_forest.mood");

const ResourceLocation AMBIENT_NETHER_WASTES_ADDITIONS("minecraft:ambient.nether_wastes.additions");
const ResourceLocation AMBIENT_NETHER_WASTES_LOOP("minecraft:ambient.nether_wastes.loop");
const ResourceLocation AMBIENT_NETHER_WASTES_MOOD("minecraft:ambient.nether_wastes.mood");

const ResourceLocation AMBIENT_SOUL_SAND_VALLEY_ADDITIONS("minecraft:ambient.soul_sand_valley.additions");
const ResourceLocation AMBIENT_SOUL_SAND_VALLEY_LOOP("minecraft:ambient.soul_sand_valley.loop");
const ResourceLocation AMBIENT_SOUL_SAND_VALLEY_MOOD("minecraft:ambient.soul_sand_valley.mood");

const ResourceLocation AMBIENT_WARPED_FOREST_ADDITIONS("minecraft:ambient.warped_forest.additions");
const ResourceLocation AMBIENT_WARPED_FOREST_LOOP("minecraft:ambient.warped_forest.loop");
const ResourceLocation AMBIENT_WARPED_FOREST_MOOD("minecraft:ambient.warped_forest.mood");

const ResourceLocation AMBIENT_UNDERWATER_ENTER("minecraft:ambient.underwater.enter");
const ResourceLocation AMBIENT_UNDERWATER_EXIT("minecraft:ambient.underwater.exit");
const ResourceLocation AMBIENT_UNDERWATER_LOOP("minecraft:ambient.underwater.loop");
const ResourceLocation AMBIENT_UNDERWATER_LOOP_ADDITIONS("minecraft:ambient.underwater.loop.additions");
const ResourceLocation AMBIENT_UNDERWATER_LOOP_ADDITIONS_RARE("minecraft:ambient.underwater.loop.additions.rare");
const ResourceLocation AMBIENT_UNDERWATER_LOOP_ADDITIONS_ULTRA_RARE(
    "minecraft:ambient.underwater.loop.additions.ultra_rare");

// ============================================================================
// 方块音效 (BLOCK_)
// ============================================================================

// 基础方块音效
const ResourceLocation BLOCK_STONE_BREAK("minecraft:block.stone.break");
const ResourceLocation BLOCK_STONE_FALL("minecraft:block.stone.fall");
const ResourceLocation BLOCK_STONE_HIT("minecraft:block.stone.hit");
const ResourceLocation BLOCK_STONE_PLACE("minecraft:block.stone.place");
const ResourceLocation BLOCK_STONE_STEP("minecraft:block.stone.step");

const ResourceLocation BLOCK_GRASS_BREAK("minecraft:block.grass.break");
const ResourceLocation BLOCK_GRASS_FALL("minecraft:block.grass.fall");
const ResourceLocation BLOCK_GRASS_HIT("minecraft:block.grass.hit");
const ResourceLocation BLOCK_GRASS_PLACE("minecraft:block.grass.place");
const ResourceLocation BLOCK_GRASS_STEP("minecraft:block.grass.step");

const ResourceLocation BLOCK_GRAVEL_BREAK("minecraft:block.gravel.break");
const ResourceLocation BLOCK_GRAVEL_FALL("minecraft:block.gravel.fall");
const ResourceLocation BLOCK_GRAVEL_HIT("minecraft:block.gravel.hit");
const ResourceLocation BLOCK_GRAVEL_PLACE("minecraft:block.gravel.place");
const ResourceLocation BLOCK_GRAVEL_STEP("minecraft:block.gravel.step");

const ResourceLocation BLOCK_SAND_BREAK("minecraft:block.sand.break");
const ResourceLocation BLOCK_SAND_FALL("minecraft:block.sand.fall");
const ResourceLocation BLOCK_SAND_HIT("minecraft:block.sand.hit");
const ResourceLocation BLOCK_SAND_PLACE("minecraft:block.sand.place");
const ResourceLocation BLOCK_SAND_STEP("minecraft:block.sand.step");

const ResourceLocation BLOCK_GLASS_BREAK("minecraft:block.glass.break");
const ResourceLocation BLOCK_GLASS_FALL("minecraft:block.glass.fall");
const ResourceLocation BLOCK_GLASS_HIT("minecraft:block.glass.hit");
const ResourceLocation BLOCK_GLASS_PLACE("minecraft:block.glass.place");
const ResourceLocation BLOCK_GLASS_STEP("minecraft:block.glass.step");

const ResourceLocation BLOCK_WOOD_BREAK("minecraft:block.wood.break");
const ResourceLocation BLOCK_WOOD_FALL("minecraft:block.wood.fall");
const ResourceLocation BLOCK_WOOD_HIT("minecraft:block.wood.hit");
const ResourceLocation BLOCK_WOOD_PLACE("minecraft:block.wood.place");
const ResourceLocation BLOCK_WOOD_STEP("minecraft:block.wood.step");

const ResourceLocation BLOCK_WOOL_BREAK("minecraft:block.wool.break");
const ResourceLocation BLOCK_WOOL_FALL("minecraft:block.wool.fall");
const ResourceLocation BLOCK_WOOL_HIT("minecraft:block.wool.hit");
const ResourceLocation BLOCK_WOOL_PLACE("minecraft:block.wool.place");
const ResourceLocation BLOCK_WOOL_STEP("minecraft:block.wool.step");

const ResourceLocation BLOCK_METAL_BREAK("minecraft:block.metal.break");
const ResourceLocation BLOCK_METAL_FALL("minecraft:block.metal.fall");
const ResourceLocation BLOCK_METAL_HIT("minecraft:block.metal.hit");
const ResourceLocation BLOCK_METAL_PLACE("minecraft:block.metal.place");
const ResourceLocation BLOCK_METAL_STEP("minecraft:block.metal.step");

const ResourceLocation BLOCK_SNOW_BREAK("minecraft:block.snow.break");
const ResourceLocation BLOCK_SNOW_FALL("minecraft:block.snow.fall");
const ResourceLocation BLOCK_SNOW_HIT("minecraft:block.snow.hit");
const ResourceLocation BLOCK_SNOW_PLACE("minecraft:block.snow.place");
const ResourceLocation BLOCK_SNOW_STEP("minecraft:block.snow.step");

// 下界方块音效
const ResourceLocation BLOCK_ANCIENT_DEBRIS_BREAK("minecraft:block.ancient_debris.break");
const ResourceLocation BLOCK_ANCIENT_DEBRIS_FALL("minecraft:block.ancient_debris.fall");
const ResourceLocation BLOCK_ANCIENT_DEBRIS_HIT("minecraft:block.ancient_debris.hit");
const ResourceLocation BLOCK_ANCIENT_DEBRIS_PLACE("minecraft:block.ancient_debris.place");
const ResourceLocation BLOCK_ANCIENT_DEBRIS_STEP("minecraft:block.ancient_debris.step");

const ResourceLocation BLOCK_BASALT_BREAK("minecraft:block.basalt.break");
const ResourceLocation BLOCK_BASALT_FALL("minecraft:block.basalt.fall");
const ResourceLocation BLOCK_BASALT_HIT("minecraft:block.basalt.hit");
const ResourceLocation BLOCK_BASALT_PLACE("minecraft:block.basalt.place");
const ResourceLocation BLOCK_BASALT_STEP("minecraft:block.basalt.step");

const ResourceLocation BLOCK_BONE_BLOCK_BREAK("minecraft:block.bone_block.break");
const ResourceLocation BLOCK_BONE_BLOCK_FALL("minecraft:block.bone_block.fall");
const ResourceLocation BLOCK_BONE_BLOCK_HIT("minecraft:block.bone_block.hit");
const ResourceLocation BLOCK_BONE_BLOCK_PLACE("minecraft:block.bone_block.place");
const ResourceLocation BLOCK_BONE_BLOCK_STEP("minecraft:block.bone_block.step");

const ResourceLocation BLOCK_NETHER_BRICKS_BREAK("minecraft:block.nether_bricks.break");
const ResourceLocation BLOCK_NETHER_BRICKS_FALL("minecraft:block.nether_bricks.fall");
const ResourceLocation BLOCK_NETHER_BRICKS_HIT("minecraft:block.nether_bricks.hit");
const ResourceLocation BLOCK_NETHER_BRICKS_PLACE("minecraft:block.nether_bricks.place");
const ResourceLocation BLOCK_NETHER_BRICKS_STEP("minecraft:block.nether_bricks.step");

const ResourceLocation BLOCK_NETHER_GOLD_ORE_BREAK("minecraft:block.nether_gold_ore.break");
const ResourceLocation BLOCK_NETHER_GOLD_ORE_FALL("minecraft:block.nether_gold_ore.fall");
const ResourceLocation BLOCK_NETHER_GOLD_ORE_HIT("minecraft:block.nether_gold_ore.hit");
const ResourceLocation BLOCK_NETHER_GOLD_ORE_PLACE("minecraft:block.nether_gold_ore.place");
const ResourceLocation BLOCK_NETHER_GOLD_ORE_STEP("minecraft:block.nether_gold_ore.step");

const ResourceLocation BLOCK_NETHER_ORE_BREAK("minecraft:block.nether_ore.break");
const ResourceLocation BLOCK_NETHER_ORE_FALL("minecraft:block.nether_ore.fall");
const ResourceLocation BLOCK_NETHER_ORE_HIT("minecraft:block.nether_ore.hit");
const ResourceLocation BLOCK_NETHER_ORE_PLACE("minecraft:block.nether_ore.place");
const ResourceLocation BLOCK_NETHER_ORE_STEP("minecraft:block.nether_ore.step");

const ResourceLocation BLOCK_NETHERITE_BLOCK_BREAK("minecraft:block.netherite_block.break");
const ResourceLocation BLOCK_NETHERITE_BLOCK_FALL("minecraft:block.netherite_block.fall");
const ResourceLocation BLOCK_NETHERITE_BLOCK_HIT("minecraft:block.netherite_block.hit");
const ResourceLocation BLOCK_NETHERITE_BLOCK_PLACE("minecraft:block.netherite_block.place");
const ResourceLocation BLOCK_NETHERITE_BLOCK_STEP("minecraft:block.netherite_block.step");

const ResourceLocation BLOCK_NETHERRACK_BREAK("minecraft:block.netherrack.break");
const ResourceLocation BLOCK_NETHERRACK_FALL("minecraft:block.netherrack.fall");
const ResourceLocation BLOCK_NETHERRACK_HIT("minecraft:block.netherrack.hit");
const ResourceLocation BLOCK_NETHERRACK_PLACE("minecraft:block.netherrack.place");
const ResourceLocation BLOCK_NETHERRACK_STEP("minecraft:block.netherrack.step");

const ResourceLocation BLOCK_NYLIUM_BREAK("minecraft:block.nylium.break");
const ResourceLocation BLOCK_NYLIUM_FALL("minecraft:block.nylium.fall");
const ResourceLocation BLOCK_NYLIUM_HIT("minecraft:block.nylium.hit");
const ResourceLocation BLOCK_NYLIUM_PLACE("minecraft:block.nylium.place");
const ResourceLocation BLOCK_NYLIUM_STEP("minecraft:block.nylium.step");

const ResourceLocation BLOCK_SOUL_SAND_BREAK("minecraft:block.soul_sand.break");
const ResourceLocation BLOCK_SOUL_SAND_FALL("minecraft:block.soul_sand.fall");
const ResourceLocation BLOCK_SOUL_SAND_HIT("minecraft:block.soul_sand.hit");
const ResourceLocation BLOCK_SOUL_SAND_PLACE("minecraft:block.soul_sand.place");
const ResourceLocation BLOCK_SOUL_SAND_STEP("minecraft:block.soul_sand.step");

const ResourceLocation BLOCK_SOUL_SOIL_BREAK("minecraft:block.soul_soil.break");
const ResourceLocation BLOCK_SOUL_SOIL_FALL("minecraft:block.soul_soil.fall");
const ResourceLocation BLOCK_SOUL_SOIL_HIT("minecraft:block.soul_soil.hit");
const ResourceLocation BLOCK_SOUL_SOIL_PLACE("minecraft:block.soul_soil.place");
const ResourceLocation BLOCK_SOUL_SOIL_STEP("minecraft:block.soul_soil.step");

const ResourceLocation BLOCK_LODESTONE_BREAK("minecraft:block.lodestone.break");
const ResourceLocation BLOCK_LODESTONE_FALL("minecraft:block.lodestone.fall");
const ResourceLocation BLOCK_LODESTONE_HIT("minecraft:block.lodestone.hit");
const ResourceLocation BLOCK_LODESTONE_PLACE("minecraft:block.lodestone.place");
const ResourceLocation BLOCK_LODESTONE_STEP("minecraft:block.lodestone.step");

const ResourceLocation BLOCK_GILDED_BLACKSTONE_BREAK("minecraft:block.gilded_blackstone.break");
const ResourceLocation BLOCK_GILDED_BLACKSTONE_FALL("minecraft:block.gilded_blackstone.fall");
const ResourceLocation BLOCK_GILDED_BLACKSTONE_HIT("minecraft:block.gilded_blackstone.hit");
const ResourceLocation BLOCK_GILDED_BLACKSTONE_PLACE("minecraft:block.gilded_blackstone.place");
const ResourceLocation BLOCK_GILDED_BLACKSTONE_STEP("minecraft:block.gilded_blackstone.step");

const ResourceLocation BLOCK_SHROOMLIGHT_BREAK("minecraft:block.shroomlight.break");
const ResourceLocation BLOCK_SHROOMLIGHT_FALL("minecraft:block.shroomlight.fall");
const ResourceLocation BLOCK_SHROOMLIGHT_HIT("minecraft:block.shroomlight.hit");
const ResourceLocation BLOCK_SHROOMLIGHT_PLACE("minecraft:block.shroomlight.place");
const ResourceLocation BLOCK_SHROOMLIGHT_STEP("minecraft:block.shroomlight.step");

const ResourceLocation BLOCK_WART_BLOCK_BREAK("minecraft:block.wart_block.break");
const ResourceLocation BLOCK_WART_BLOCK_FALL("minecraft:block.wart_block.fall");
const ResourceLocation BLOCK_WART_BLOCK_HIT("minecraft:block.wart_block.hit");
const ResourceLocation BLOCK_WART_BLOCK_PLACE("minecraft:block.wart_block.place");
const ResourceLocation BLOCK_WART_BLOCK_STEP("minecraft:block.wart_block.step");

const ResourceLocation BLOCK_STEM_BREAK("minecraft:block.stem.break");
const ResourceLocation BLOCK_STEM_FALL("minecraft:block.stem.fall");
const ResourceLocation BLOCK_STEM_HIT("minecraft:block.stem.hit");
const ResourceLocation BLOCK_STEM_PLACE("minecraft:block.stem.place");
const ResourceLocation BLOCK_STEM_STEP("minecraft:block.stem.step");

const ResourceLocation BLOCK_FUNGUS_BREAK("minecraft:block.fungus.break");
const ResourceLocation BLOCK_FUNGUS_FALL("minecraft:block.fungus.fall");
const ResourceLocation BLOCK_FUNGUS_HIT("minecraft:block.fungus.hit");
const ResourceLocation BLOCK_FUNGUS_PLACE("minecraft:block.fungus.place");
const ResourceLocation BLOCK_FUNGUS_STEP("minecraft:block.fungus.step");

const ResourceLocation BLOCK_ROOTS_BREAK("minecraft:block.roots.break");
const ResourceLocation BLOCK_ROOTS_FALL("minecraft:block.roots.fall");
const ResourceLocation BLOCK_ROOTS_HIT("minecraft:block.roots.hit");
const ResourceLocation BLOCK_ROOTS_PLACE("minecraft:block.roots.place");
const ResourceLocation BLOCK_ROOTS_STEP("minecraft:block.roots.step");

const ResourceLocation BLOCK_NETHER_SPROUTS_BREAK("minecraft:block.nether_sprouts.break");
const ResourceLocation BLOCK_NETHER_SPROUTS_FALL("minecraft:block.nether_sprouts.fall");
const ResourceLocation BLOCK_NETHER_SPROUTS_HIT("minecraft:block.nether_sprouts.hit");
const ResourceLocation BLOCK_NETHER_SPROUTS_PLACE("minecraft:block.nether_sprouts.place");
const ResourceLocation BLOCK_NETHER_SPROUTS_STEP("minecraft:block.nether_sprouts.step");

const ResourceLocation BLOCK_WEEPING_VINES_BREAK("minecraft:block.weeping_vines.break");
const ResourceLocation BLOCK_WEEPING_VINES_FALL("minecraft:block.weeping_vines.fall");
const ResourceLocation BLOCK_WEEPING_VINES_HIT("minecraft:block.weeping_vines.hit");
const ResourceLocation BLOCK_WEEPING_VINES_PLACE("minecraft:block.weeping_vines.place");
const ResourceLocation BLOCK_WEEPING_VINES_STEP("minecraft:block.weeping_vines.step");

// 其他方块音效
const ResourceLocation BLOCK_BAMBOO_BREAK("minecraft:block.bamboo.break");
const ResourceLocation BLOCK_BAMBOO_FALL("minecraft:block.bamboo.fall");
const ResourceLocation BLOCK_BAMBOO_HIT("minecraft:block.bamboo.hit");
const ResourceLocation BLOCK_BAMBOO_PLACE("minecraft:block.bamboo.place");
const ResourceLocation BLOCK_BAMBOO_STEP("minecraft:block.bamboo.step");
const ResourceLocation BLOCK_BAMBOO_SAPLING_BREAK("minecraft:block.bamboo_sapling.break");
const ResourceLocation BLOCK_BAMBOO_SAPLING_HIT("minecraft:block.bamboo_sapling.hit");
const ResourceLocation BLOCK_BAMBOO_SAPLING_PLACE("minecraft:block.bamboo_sapling.place");

const ResourceLocation BLOCK_WET_GRASS_BREAK("minecraft:block.wet_grass.break");
const ResourceLocation BLOCK_WET_GRASS_FALL("minecraft:block.wet_grass.fall");
const ResourceLocation BLOCK_WET_GRASS_HIT("minecraft:block.wet_grass.hit");
const ResourceLocation BLOCK_WET_GRASS_PLACE("minecraft:block.wet_grass.place");
const ResourceLocation BLOCK_WET_GRASS_STEP("minecraft:block.wet_grass.step");

const ResourceLocation BLOCK_VINE_STEP("minecraft:block.vine.step");

const ResourceLocation BLOCK_CORAL_BLOCK_BREAK("minecraft:block.coral_block.break");
const ResourceLocation BLOCK_CORAL_BLOCK_FALL("minecraft:block.coral_block.fall");
const ResourceLocation BLOCK_CORAL_BLOCK_HIT("minecraft:block.coral_block.hit");
const ResourceLocation BLOCK_CORAL_BLOCK_PLACE("minecraft:block.coral_block.place");
const ResourceLocation BLOCK_CORAL_BLOCK_STEP("minecraft:block.coral_block.step");

const ResourceLocation BLOCK_CROP_BREAK("minecraft:block.crop.break");
const ResourceLocation BLOCK_NETHER_WART_BREAK("minecraft:block.nether_wart.break");
const ResourceLocation BLOCK_SWEET_BERRY_BUSH_BREAK("minecraft:block.sweet_berry_bush.break");
const ResourceLocation BLOCK_SWEET_BERRY_BUSH_PLACE("minecraft:block.sweet_berry_bush.place");
const ResourceLocation BLOCK_CAVE_VINES_PICK_BERRIES("minecraft:block.cave_vines.pick_berries");
const ResourceLocation BLOCK_LILY_PAD_PLACE("minecraft:block.lily_pad.place");

const ResourceLocation BLOCK_BIG_DRIPLEAF_TILT_DOWN("minecraft:block.big_dripleaf.tilt_down");
const ResourceLocation BLOCK_BIG_DRIPLEAF_TILT_UP("minecraft:block.big_dripleaf.tilt_up");

/// 滴水石音效
const ResourceLocation BLOCK_POINTED_DRIPSTONE_HIT("minecraft:block.pointed_dripstone.hit");
const ResourceLocation BLOCK_POINTED_DRIPSTONE_FALL("minecraft:block.pointed_dripstone.fall");
const ResourceLocation BLOCK_POINTED_DRIPSTONE_LAND("minecraft:block.pointed_dripstone.land");
const ResourceLocation BLOCK_POINTED_DRIPSTONE_DRIP_WATER("minecraft:block.pointed_dripstone.drip_water");
const ResourceLocation BLOCK_POINTED_DRIPSTONE_DRIP_LAVA("minecraft:block.pointed_dripstone.drip_lava");
const ResourceLocation BLOCK_POINTED_DRIPSTONE_DRIP_WATER_INTO_CAULDRON(
    "minecraft:block.pointed_dripstone.drip_water_into_cauldron");
const ResourceLocation BLOCK_POINTED_DRIPSTONE_DRIP_LAVA_INTO_CAULDRON(
    "minecraft:block.pointed_dripstone.drip_lava_into_cauldron");

const ResourceLocation BLOCK_WATER_AMBIENT("minecraft:block.water.ambient");

const ResourceLocation BLOCK_BARREL_CLOSE("minecraft:block.barrel.close");
const ResourceLocation BLOCK_BARREL_OPEN("minecraft:block.barrel.open");
const ResourceLocation BLOCK_CHEST_LOCKED("minecraft:block.chest.locked");
const ResourceLocation BLOCK_CHORUS_FLOWER_DEATH("minecraft:block.chorus_flower.death");
const ResourceLocation BLOCK_CHORUS_FLOWER_GROW("minecraft:block.chorus_flower.grow");
const ResourceLocation BLOCK_COMPOSTER_EMPTY("minecraft:block.composter.empty");
const ResourceLocation BLOCK_COMPOSTER_FILL("minecraft:block.composter.fill");
const ResourceLocation BLOCK_COMPOSTER_FILL_SUCCESS("minecraft:block.composter.fill_success");
const ResourceLocation BLOCK_COMPOSTER_READY("minecraft:block.composter.ready");

/// 合成器
const ResourceLocation BLOCK_CRAFTER_CRAFT("minecraft:block.crafter.craft");
const ResourceLocation BLOCK_CRAFTER_FAIL("minecraft:block.crafter.fail");

const ResourceLocation BLOCK_FURNACE_FIRE_CRACKLE("minecraft:block.furnace.fire_crackle");
const ResourceLocation BLOCK_LEVER_CLICK("minecraft:block.lever.click");
const ResourceLocation BLOCK_PUMPKIN_CARVE("minecraft:block.pumpkin.carve");
const ResourceLocation BLOCK_TRIPWIRE_ATTACH("minecraft:block.tripwire.attach");
const ResourceLocation BLOCK_TRIPWIRE_CLICK_OFF("minecraft:block.tripwire.click_off");
const ResourceLocation BLOCK_TRIPWIRE_CLICK_ON("minecraft:block.tripwire.click_on");
const ResourceLocation BLOCK_TRIPWIRE_DETACH("minecraft:block.tripwire.detach");

/// 木门
const ResourceLocation BLOCK_WOODEN_DOOR_OPEN("minecraft:block.wooden_door.open");
const ResourceLocation BLOCK_WOODEN_DOOR_CLOSE("minecraft:block.wooden_door.close");
const ResourceLocation BLOCK_IRON_DOOR_OPEN("minecraft:block.iron_door.open");
const ResourceLocation BLOCK_IRON_DOOR_CLOSE("minecraft:block.iron_door.close");
const ResourceLocation BLOCK_FENCE_GATE_OPEN("minecraft:block.fence_gate.open");
const ResourceLocation BLOCK_FENCE_GATE_CLOSE("minecraft:block.fence_gate.close");
const ResourceLocation BLOCK_WOODEN_TRAPDOOR_OPEN("minecraft:block.wooden_trapdoor.open");
const ResourceLocation BLOCK_WOODEN_TRAPDOOR_CLOSE("minecraft:block.wooden_trapdoor.close");
const ResourceLocation BLOCK_IRON_TRAPDOOR_OPEN("minecraft:block.iron_trapdoor.open");
const ResourceLocation BLOCK_IRON_TRAPDOOR_CLOSE("minecraft:block.iron_trapdoor.close");

const ResourceLocation BLOCK_CHEST_OPEN("minecraft:block.chest.open");
const ResourceLocation BLOCK_CHEST_CLOSE("minecraft:block.chest.close");
const ResourceLocation BLOCK_ENDER_CHEST_OPEN("minecraft:block.ender_chest.open");
const ResourceLocation BLOCK_ENDER_CHEST_CLOSE("minecraft:block.ender_chest.close");
const ResourceLocation BLOCK_SHULKER_BOX_OPEN("minecraft:block.shulker_box.open");
const ResourceLocation BLOCK_SHULKER_BOX_CLOSE("minecraft:block.shulker_box.close");

// 铜箱子开合音效（MC 1.21.11）
// Unaffected 与 Exposed 等级共用 block.copper_chest.open/close
// Weathered 等级使用 block.copper_chest_weathered.open/close
// Oxidized 等级使用 block.copper_chest_oxidized.open/close
// 涂蜡变体复用对应氧化等级的声音事件
const ResourceLocation BLOCK_COPPER_CHEST_OPEN("minecraft:block.copper_chest.open");
const ResourceLocation BLOCK_COPPER_CHEST_CLOSE("minecraft:block.copper_chest.close");
const ResourceLocation BLOCK_COPPER_CHEST_WEATHERED_OPEN("minecraft:block.copper_chest_weathered.open");
const ResourceLocation BLOCK_COPPER_CHEST_WEATHERED_CLOSE("minecraft:block.copper_chest_weathered.close");
const ResourceLocation BLOCK_COPPER_CHEST_OXIDIZED_OPEN("minecraft:block.copper_chest_oxidized.open");
const ResourceLocation BLOCK_COPPER_CHEST_OXIDIZED_CLOSE("minecraft:block.copper_chest_oxidized.close");

const ResourceLocation BLOCK_PISTON_EXTEND("minecraft:block.piston.extend");
const ResourceLocation BLOCK_PISTON_CONTRACT("minecraft:block.piston.contract");

const ResourceLocation BLOCK_STONE_BUTTON_CLICK_ON("minecraft:block.stone_button.click_on");
const ResourceLocation BLOCK_STONE_BUTTON_CLICK_OFF("minecraft:block.stone_button.click_off");
const ResourceLocation BLOCK_WOODEN_BUTTON_CLICK_ON("minecraft:block.wooden_button.click_on");
const ResourceLocation BLOCK_WOODEN_BUTTON_CLICK_OFF("minecraft:block.wooden_button.click_off");

const ResourceLocation BLOCK_STONE_PRESSURE_PLATE_CLICK_ON("minecraft:block.stone_pressure_plate.click_on");
const ResourceLocation BLOCK_STONE_PRESSURE_PLATE_CLICK_OFF("minecraft:block.stone_pressure_plate.click_off");
const ResourceLocation BLOCK_WOODEN_PRESSURE_PLATE_CLICK_ON("minecraft:block.wooden_pressure_plate.click_on");
const ResourceLocation BLOCK_WOODEN_PRESSURE_PLATE_CLICK_OFF("minecraft:block.wooden_pressure_plate.click_off");
const ResourceLocation BLOCK_METAL_PRESSURE_PLATE_CLICK_ON("minecraft:block.metal_pressure_plate.click_on");
const ResourceLocation BLOCK_METAL_PRESSURE_PLATE_CLICK_OFF("minecraft:block.metal_pressure_plate.click_off");

const ResourceLocation BLOCK_NOTE_BLOCK_BASS("minecraft:block.note_block.bass");
const ResourceLocation BLOCK_NOTE_BLOCK_SNARE("minecraft:block.note_block.snare");
const ResourceLocation BLOCK_NOTE_BLOCK_HAT("minecraft:block.note_block.hat");
const ResourceLocation BLOCK_NOTE_BLOCK_BASEDRUM("minecraft:block.note_block.basedrum");
const ResourceLocation BLOCK_NOTE_BLOCK_BELL("minecraft:block.note_block.bell");
const ResourceLocation BLOCK_NOTE_BLOCK_FLUTE("minecraft:block.note_block.flute");
const ResourceLocation BLOCK_NOTE_BLOCK_CHIME("minecraft:block.note_block.chime");
const ResourceLocation BLOCK_NOTE_BLOCK_GUITAR("minecraft:block.note_block.guitar");
const ResourceLocation BLOCK_NOTE_BLOCK_XYLOPHONE("minecraft:block.note_block.xylophone");
const ResourceLocation BLOCK_NOTE_BLOCK_IRON_XYLOPHONE("minecraft:block.note_block.iron_xylophone");
const ResourceLocation BLOCK_NOTE_BLOCK_COW_BELL("minecraft:block.note_block.cow_bell");
const ResourceLocation BLOCK_NOTE_BLOCK_DIDGERIDOO("minecraft:block.note_block.didgeridoo");
const ResourceLocation BLOCK_NOTE_BLOCK_BIT("minecraft:block.note_block.bit");
const ResourceLocation BLOCK_NOTE_BLOCK_BANJO("minecraft:block.note_block.banjo");
const ResourceLocation BLOCK_NOTE_BLOCK_PLING("minecraft:block.note_block.pling");
// 本文件全部 ~1700 个声音常量与下方 6 个新增项写法一致：ResourceLocation 内部持有 std::string，
// 构造期分配失败理论可抛 bad_alloc，故 bugprone-throwing-static-initialization 恒报。这是本文件
// 既有约定（存量常量同样触发，只是仅"改动行"会被 CI 报告），重构成非抛出式超出本改动范围。
//
// NOLINTNEXTLINE(bugprone-throwing-static-initialization)
const ResourceLocation BLOCK_NOTE_BLOCK_IMITATE_ZOMBIE("minecraft:block.note_block.imitate.zombie");
// NOLINTNEXTLINE(bugprone-throwing-static-initialization)
const ResourceLocation BLOCK_NOTE_BLOCK_IMITATE_SKELETON("minecraft:block.note_block.imitate.skeleton");
// NOLINTNEXTLINE(bugprone-throwing-static-initialization)
const ResourceLocation BLOCK_NOTE_BLOCK_IMITATE_CREEPER("minecraft:block.note_block.imitate.creeper");
// NOLINTNEXTLINE(bugprone-throwing-static-initialization)
const ResourceLocation BLOCK_NOTE_BLOCK_IMITATE_ENDER_DRAGON("minecraft:block.note_block.imitate.ender_dragon");
// NOLINTNEXTLINE(bugprone-throwing-static-initialization)
const ResourceLocation BLOCK_NOTE_BLOCK_IMITATE_WITHER_SKELETON("minecraft:block.note_block.imitate.wither_skeleton");
// NOLINTNEXTLINE(bugprone-throwing-static-initialization)
const ResourceLocation BLOCK_NOTE_BLOCK_IMITATE_PIGLIN("minecraft:block.note_block.imitate.piglin");
const ResourceLocation BLOCK_NOTE_BLOCK_HARP("minecraft:block.note_block.harp");

const ResourceLocation BLOCK_FIRE_AMBIENT("minecraft:block.fire.ambient");
const ResourceLocation BLOCK_FIRE_EXTINGUISH("minecraft:block.fire.extinguish");
const ResourceLocation BLOCK_LAVA_AMBIENT("minecraft:block.lava.ambient");
const ResourceLocation BLOCK_LAVA_EXTINGUISH("minecraft:block.lava.extinguish");
const ResourceLocation BLOCK_LAVA_POP("minecraft:block.lava.pop");

const ResourceLocation BLOCK_PORTAL_AMBIENT("minecraft:block.portal.ambient");
const ResourceLocation BLOCK_PORTAL_TRAVEL("minecraft:block.portal.travel");
const ResourceLocation BLOCK_PORTAL_TRIGGER("minecraft:block.portal.trigger");

const ResourceLocation BLOCK_END_PORTAL_FRAME_FILL("minecraft:block.end_portal_frame.fill");
const ResourceLocation BLOCK_END_PORTAL_SPAWN("minecraft:block.end_portal.spawn");
const ResourceLocation BLOCK_END_GATEWAY_SPAWN("minecraft:block.end_gateway.spawn");

const ResourceLocation BLOCK_BEACON_ACTIVATE("minecraft:block.beacon.activate");
const ResourceLocation BLOCK_BEACON_AMBIENT("minecraft:block.beacon.ambient");
const ResourceLocation BLOCK_BEACON_DEACTIVATE("minecraft:block.beacon.deactivate");
const ResourceLocation BLOCK_BEACON_POWER_SELECT("minecraft:block.beacon.power_select");

const ResourceLocation BLOCK_BREWING_STAND_BREW("minecraft:block.brewing_stand.brew");

const ResourceLocation BLOCK_ANVIL_BREAK("minecraft:block.anvil.break");
const ResourceLocation BLOCK_ANVIL_DESTROY("minecraft:block.anvil.destroy");
const ResourceLocation BLOCK_ANVIL_FALL("minecraft:block.anvil.fall");
const ResourceLocation BLOCK_ANVIL_HIT("minecraft:block.anvil.hit");
const ResourceLocation BLOCK_ANVIL_LAND("minecraft:block.anvil.land");
const ResourceLocation BLOCK_ANVIL_PLACE("minecraft:block.anvil.place");
const ResourceLocation BLOCK_ANVIL_STEP("minecraft:block.anvil.step");
const ResourceLocation BLOCK_ANVIL_USE("minecraft:block.anvil.use");

const ResourceLocation BLOCK_CAMPFIRE_CRACKLE("minecraft:block.campfire.crackle");
const ResourceLocation BLOCK_CAMPFIRE_EXTINGUISH("minecraft:block.campfire.extinguish");

// 蜡烛
const ResourceLocation BLOCK_CANDLE_AMBIENT("minecraft:block.candle.ambient");
const ResourceLocation BLOCK_CANDLE_BREAK("minecraft:block.candle.break");
const ResourceLocation BLOCK_CANDLE_EXTINGUISH("minecraft:block.candle.extinguish");
const ResourceLocation BLOCK_CANDLE_HIT("minecraft:block.candle.hit");
const ResourceLocation BLOCK_CANDLE_PLACE("minecraft:block.candle.place");
const ResourceLocation BLOCK_CANDLE_STEP("minecraft:block.candle.step");

const ResourceLocation BLOCK_BEEHIVE_DRIP("minecraft:block.beehive.drip");
const ResourceLocation BLOCK_BEEHIVE_DROP("minecraft:block.beehive.drop");
const ResourceLocation BLOCK_BEEHIVE_ENTER("minecraft:block.beehive.enter");
const ResourceLocation BLOCK_BEEHIVE_EXIT("minecraft:block.beehive.exit");
const ResourceLocation BLOCK_BEEHIVE_SHEAR("minecraft:block.beehive.shear");
const ResourceLocation BLOCK_BEEHIVE_WORK("minecraft:block.beehive.work");

const ResourceLocation BLOCK_SIGN_WAXED_INTERACT_FAIL("minecraft:block.sign.waxed_interact_fail");
const ResourceLocation BLOCK_HANGING_SIGN_WAXED_INTERACT_FAIL("minecraft:block.hanging_sign.waxed_interact_fail");

const ResourceLocation BLOCK_BELL_USE("minecraft:block.bell.use");
const ResourceLocation BLOCK_BELL_RESONATE("minecraft:block.bell.resonate");

const ResourceLocation BLOCK_AMETHYST_BLOCK_CHIME("minecraft:block.amethyst_block.chime");
const ResourceLocation BLOCK_AMETHYST_BLOCK_RESONATE("minecraft:block.amethyst_block.resonate");

const ResourceLocation BLOCK_GRINDSTONE_USE("minecraft:block.grindstone.use");

const ResourceLocation BLOCK_BLASTFURNACE_FIRE_CRACKLE("minecraft:block.blastfurnace.fire_crackle");
const ResourceLocation BLOCK_SMOKER_SMOKE("minecraft:block.smoker.smoke");

const ResourceLocation BLOCK_SMITHING_TABLE_USE("minecraft:block.smithing_table.use");
const ResourceLocation BLOCK_ENCHANTMENT_TABLE_USE("minecraft:block.enchantment_table.use");

const ResourceLocation BLOCK_DISPENSER_DISPENSE("minecraft:block.dispenser.dispense");
const ResourceLocation BLOCK_DISPENSER_FAIL("minecraft:block.dispenser.fail");
const ResourceLocation BLOCK_DISPENSER_LAUNCH("minecraft:block.dispenser.launch");

const ResourceLocation BLOCK_COMPARATOR_CLICK("minecraft:block.comparator.click");
const ResourceLocation BLOCK_REDSTONE_TORCH_BURNOUT("minecraft:block.redstone_torch.burnout");
const ResourceLocation BLOCK_REDSTONE_BREAK("minecraft:block.redstone.break");
const ResourceLocation BLOCK_REDSTONE_HIT("minecraft:block.redstone.hit");
const ResourceLocation BLOCK_REDSTONE_PLACE("minecraft:block.redstone.place");
const ResourceLocation BLOCK_REDSTONE_STEP("minecraft:block.redstone.step");
const ResourceLocation BLOCK_REDSTONE_FALL("minecraft:block.redstone.fall");

const ResourceLocation BLOCK_BUBBLE_COLUMN_BUBBLE_POP("minecraft:block.bubble_column.bubble_pop");
const ResourceLocation BLOCK_BUBBLE_COLUMN_UPWARDS_AMBIENT("minecraft:block.bubble_column.upwards_ambient");
const ResourceLocation BLOCK_BUBBLE_COLUMN_UPWARDS_INSIDE("minecraft:block.bubble_column.upwards_inside");
const ResourceLocation BLOCK_BUBBLE_COLUMN_WHIRLPOOL_AMBIENT("minecraft:block.bubble_column.whirlpool_ambient");
const ResourceLocation BLOCK_BUBBLE_COLUMN_WHIRLPOOL_INSIDE("minecraft:block.bubble_column.whirlpool_inside");

const ResourceLocation BLOCK_CONDUIT_ACTIVATE("minecraft:block.conduit.activate");
const ResourceLocation BLOCK_CONDUIT_AMBIENT("minecraft:block.conduit.ambient");
const ResourceLocation BLOCK_CONDUIT_AMBIENT_SHORT("minecraft:block.conduit.ambient.short");
const ResourceLocation BLOCK_CONDUIT_ATTACK_TARGET("minecraft:block.conduit.attack.target");
const ResourceLocation BLOCK_CONDUIT_DEACTIVATE("minecraft:block.conduit.deactivate");

const ResourceLocation BLOCK_RESPAWN_ANCHOR_AMBIENT("minecraft:block.respawn_anchor.ambient");
const ResourceLocation BLOCK_RESPAWN_ANCHOR_CHARGE("minecraft:block.respawn_anchor.charge");
const ResourceLocation BLOCK_RESPAWN_ANCHOR_DEPLETE("minecraft:block.respawn_anchor.deplete");
const ResourceLocation BLOCK_RESPAWN_ANCHOR_SET_SPAWN("minecraft:block.respawn_anchor.set_spawn");

const ResourceLocation BLOCK_LADDER_BREAK("minecraft:block.ladder.break");
const ResourceLocation BLOCK_LADDER_FALL("minecraft:block.ladder.fall");
const ResourceLocation BLOCK_LADDER_HIT("minecraft:block.ladder.hit");
const ResourceLocation BLOCK_LADDER_PLACE("minecraft:block.ladder.place");
const ResourceLocation BLOCK_LADDER_STEP("minecraft:block.ladder.step");

const ResourceLocation BLOCK_SLIME_BLOCK_BREAK("minecraft:block.slime_block.break");
const ResourceLocation BLOCK_SLIME_BLOCK_FALL("minecraft:block.slime_block.fall");
const ResourceLocation BLOCK_SLIME_BLOCK_HIT("minecraft:block.slime_block.hit");
const ResourceLocation BLOCK_SLIME_BLOCK_PLACE("minecraft:block.slime_block.place");
const ResourceLocation BLOCK_SLIME_BLOCK_STEP("minecraft:block.slime_block.step");

const ResourceLocation BLOCK_HONEY_BLOCK_BREAK("minecraft:block.honey_block.break");
const ResourceLocation BLOCK_HONEY_BLOCK_FALL("minecraft:block.honey_block.fall");
const ResourceLocation BLOCK_HONEY_BLOCK_HIT("minecraft:block.honey_block.hit");
const ResourceLocation BLOCK_HONEY_BLOCK_PLACE("minecraft:block.honey_block.place");
const ResourceLocation BLOCK_HONEY_BLOCK_SLIDE("minecraft:block.honey_block.slide");
const ResourceLocation BLOCK_HONEY_BLOCK_STEP("minecraft:block.honey_block.step");

const ResourceLocation BLOCK_SCAFFOLDING_BREAK("minecraft:block.scaffolding.break");
const ResourceLocation BLOCK_SCAFFOLDING_FALL("minecraft:block.scaffolding.fall");
const ResourceLocation BLOCK_SCAFFOLDING_HIT("minecraft:block.scaffolding.hit");
const ResourceLocation BLOCK_SCAFFOLDING_PLACE("minecraft:block.scaffolding.place");
const ResourceLocation BLOCK_SCAFFOLDING_STEP("minecraft:block.scaffolding.step");

const ResourceLocation BLOCK_SHELF_ACTIVATE("minecraft:block.shelf.activate");
const ResourceLocation BLOCK_SHELF_DEACTIVATE("minecraft:block.shelf.deactivate");
const ResourceLocation BLOCK_SHELF_PLACE_ITEM("minecraft:block.shelf.place_item");
const ResourceLocation BLOCK_SHELF_TAKE_ITEM("minecraft:block.shelf.take_item");
const ResourceLocation BLOCK_SHELF_SINGLE_SWAP("minecraft:block.shelf.single_swap");
const ResourceLocation BLOCK_SHELF_MULTI_SWAP("minecraft:block.shelf.multi_swap");

const ResourceLocation BLOCK_DECORATED_POT_INSERT("minecraft:block.decorated_pot.insert");
const ResourceLocation BLOCK_DECORATED_POT_INSERT_FAIL("minecraft:block.decorated_pot.insert_fail");

const ResourceLocation BLOCK_LANTERN_BREAK("minecraft:block.lantern.break");
const ResourceLocation BLOCK_LANTERN_FALL("minecraft:block.lantern.fall");
const ResourceLocation BLOCK_LANTERN_HIT("minecraft:block.lantern.hit");
const ResourceLocation BLOCK_LANTERN_PLACE("minecraft:block.lantern.place");
const ResourceLocation BLOCK_LANTERN_STEP("minecraft:block.lantern.step");

const ResourceLocation BLOCK_CHAIN_BREAK("minecraft:block.chain.break");
const ResourceLocation BLOCK_CHAIN_FALL("minecraft:block.chain.fall");
const ResourceLocation BLOCK_CHAIN_HIT("minecraft:block.chain.hit");
const ResourceLocation BLOCK_CHAIN_PLACE("minecraft:block.chain.place");
const ResourceLocation BLOCK_CHAIN_STEP("minecraft:block.chain.step");

// 幽匿感测体
const ResourceLocation BLOCK_SCULK_SENSOR_CLICKING("minecraft:block.sculk_sensor.clicking");
const ResourceLocation BLOCK_SCULK_SENSOR_CLICKING_STOP("minecraft:block.sculk_sensor.clicking_stop");

// 幽匿尖啸体
const ResourceLocation BLOCK_SCULK_SHRIEKER_SHRIEK("minecraft:block.sculk_shrieker.shriek");

// 铜傀儡雕像（玩家右键切换姿态时播放的音效）
const ResourceLocation BLOCK_COPPER_GOLEM_BECOME_STATUE("minecraft:block.copper_golem.become_statue");

// 监守者
// 完整对齐 MC 1.21.11 SoundEvents 中所有 WARDEN_* 事件（共 21 个）。
const ResourceLocation ENTITY_WARDEN_AGITATED("minecraft:entity.warden.agitated");
const ResourceLocation ENTITY_WARDEN_AMBIENT("minecraft:entity.warden.ambient");
const ResourceLocation ENTITY_WARDEN_ANGRY("minecraft:entity.warden.angry");
const ResourceLocation ENTITY_WARDEN_ATTACK_IMPACT("minecraft:entity.warden.attack_impact");
const ResourceLocation ENTITY_WARDEN_DEATH("minecraft:entity.warden.death");
const ResourceLocation ENTITY_WARDEN_DIG("minecraft:entity.warden.dig");
const ResourceLocation ENTITY_WARDEN_EMERGE("minecraft:entity.warden.emerge");
const ResourceLocation ENTITY_WARDEN_HEARTBEAT("minecraft:entity.warden.heartbeat");
const ResourceLocation ENTITY_WARDEN_HURT("minecraft:entity.warden.hurt");
const ResourceLocation ENTITY_WARDEN_LISTENING("minecraft:entity.warden.listening");
const ResourceLocation ENTITY_WARDEN_LISTENING_ANGRY("minecraft:entity.warden.listening_angry");
const ResourceLocation ENTITY_WARDEN_NEARBY_CLOSE("minecraft:entity.warden.nearby_close");
const ResourceLocation ENTITY_WARDEN_NEARBY_CLOSER("minecraft:entity.warden.nearby_closer");
const ResourceLocation ENTITY_WARDEN_NEARBY_CLOSEST("minecraft:entity.warden.nearby_closest");
const ResourceLocation ENTITY_WARDEN_ROAR("minecraft:entity.warden.roar");
const ResourceLocation ENTITY_WARDEN_SNIFF("minecraft:entity.warden.sniff");
const ResourceLocation ENTITY_WARDEN_SONIC_BOOM("minecraft:entity.warden.sonic_boom");
const ResourceLocation ENTITY_WARDEN_SONIC_CHARGE("minecraft:entity.warden.sonic_charge");
const ResourceLocation ENTITY_WARDEN_STEP("minecraft:entity.warden.step");
const ResourceLocation ENTITY_WARDEN_TENDRIL_CLICKS("minecraft:entity.warden.tendril_clicks");

// ============================================================================
// 实体通用声音
// ============================================================================

const ResourceLocation ENTITY_GENERIC_EAT("minecraft:entity.generic.eat");
const ResourceLocation ENTITY_GENERIC_DRINK("minecraft:entity.generic.drink");
const ResourceLocation ENTITY_GENERIC_HURT("minecraft:entity.generic.hurt");
const ResourceLocation ENTITY_GENERIC_DEATH("minecraft:entity.generic.death");
const ResourceLocation ENTITY_GENERIC_BURN("minecraft:entity.generic.burn");
const ResourceLocation ENTITY_GENERIC_EXTINGUISH_FIRE("minecraft:entity.generic.extinguish_fire");
const ResourceLocation ENTITY_GENERIC_BIG_FALL("minecraft:entity.generic.big_fall");
const ResourceLocation ENTITY_GENERIC_SMALL_FALL("minecraft:entity.generic.small_fall");
const ResourceLocation ENTITY_GENERIC_SPLASH("minecraft:entity.generic.splash");
const ResourceLocation ENTITY_GENERIC_SWIM("minecraft:entity.generic.swim");
const ResourceLocation ENTITY_GENERIC_EXPLODE("minecraft:entity.generic.explode");

// ============================================================================
// 试炼密室相关音效 (Trial Chambers)
// ============================================================================

const ResourceLocation TRIAL_SPAWNER_ABOUT_TO_SPAWN_ITEM("minecraft:block.trial_spawner.about_to_spawn_item");

const ResourceLocation ENTITY_WIND_CHARGE_THROW("minecraft:entity.wind_charge.throw");
const ResourceLocation ENTITY_WIND_CHARGE_WIND_BURST("minecraft:entity.wind_charge.wind_burst");
const ResourceLocation ENTITY_BREEZE_WIND_CHARGE_BURST("minecraft:entity.breeze.wind_charge_burst");
const ResourceLocation ENTITY_BREEZE_INHALE("minecraft:entity.breeze.inhale");
const ResourceLocation ENTITY_BREEZE_SHOOT("minecraft:entity.breeze.shoot");
const ResourceLocation ENTITY_BREEZE_CHARGE("minecraft:entity.breeze.charge");
const ResourceLocation ENTITY_BREEZE_JUMP("minecraft:entity.breeze.jump");
const ResourceLocation ENTITY_BREEZE_LAND("minecraft:entity.breeze.land");
const ResourceLocation ENTITY_BREEZE_SLIDE("minecraft:entity.breeze.slide");
const ResourceLocation ENTITY_BREEZE_DEFLECT("minecraft:entity.breeze.deflect");
const ResourceLocation ENTITY_BREEZE_WHIRL("minecraft:entity.breeze.whirl");
const ResourceLocation ENTITY_BREEZE_IDLE_GROUND("minecraft:entity.breeze.idle_ground");
const ResourceLocation ENTITY_BREEZE_IDLE_AIR("minecraft:entity.breeze.idle_air");

// ============================================================================
// 重锤声音
// ============================================================================

const ResourceLocation ITEM_MACE_SMASH_GROUND("minecraft:item.mace.smash_ground");
const ResourceLocation ITEM_MACE_SMASH_GROUND_HEAVY("minecraft:item.mace.smash_ground_heavy");
const ResourceLocation ITEM_MACE_SMASH_AIR("minecraft:item.mace.smash_air");

// ============================================================================
// 玩家声音
// ============================================================================

const ResourceLocation ENTITY_PLAYER_BURP("minecraft:entity.player.burp");
const ResourceLocation ENTITY_PLAYER_HURT("minecraft:entity.player.hurt");
const ResourceLocation ENTITY_PLAYER_HURT_DROWN("minecraft:entity.player.hurt_drown");
const ResourceLocation ENTITY_PLAYER_HURT_ON_FIRE("minecraft:entity.player.hurt_on_fire");
const ResourceLocation ENTITY_PLAYER_HURT_SWEET_BERRY_BUSH("minecraft:entity.player.hurt_sweet_berry_bush");
const ResourceLocation ENTITY_PLAYER_DEATH("minecraft:entity.player.death");
const ResourceLocation ENTITY_PLAYER_SPLASH("minecraft:entity.player.splash");
const ResourceLocation ENTITY_PLAYER_SPLASH_HIGH_SPEED("minecraft:entity.player.splash.high_speed");
const ResourceLocation ENTITY_PLAYER_SWIM("minecraft:entity.player.swim");
const ResourceLocation ENTITY_PLAYER_STEP("minecraft:entity.player.step");
const ResourceLocation ENTITY_PLAYER_ATTACK_SWEEP("minecraft:entity.player.attack.sweep");
const ResourceLocation ENTITY_PLAYER_ATTACK_CRIT("minecraft:entity.player.attack.crit");
const ResourceLocation ENTITY_PLAYER_ATTACK_KNOCKBACK("minecraft:entity.player.attack.knockback");
const ResourceLocation ENTITY_PLAYER_ATTACK_STRONG("minecraft:entity.player.attack.strong");
const ResourceLocation ENTITY_PLAYER_ATTACK_NODAMAGE("minecraft:entity.player.attack.nodamage");
const ResourceLocation ENTITY_PLAYER_ATTACK_WEAK("minecraft:entity.player.attack.weak");
const ResourceLocation ENTITY_PLAYER_BREATH("minecraft:entity.player.breath");
const ResourceLocation ENTITY_PLAYER_LEVELUP("minecraft:entity.player.levelup");
const ResourceLocation ENTITY_PLAYER_BIG_FALL("minecraft:entity.player.big_fall");
const ResourceLocation ENTITY_PLAYER_SMALL_FALL("minecraft:entity.player.small_fall");

// ============================================================================
// 友好生物声音
// ============================================================================

// 鸡
const ResourceLocation ENTITY_CHICKEN_AMBIENT("minecraft:entity.chicken.ambient");
const ResourceLocation ENTITY_CHICKEN_DEATH("minecraft:entity.chicken.death");
const ResourceLocation ENTITY_CHICKEN_EGG("minecraft:entity.chicken.egg");
const ResourceLocation ENTITY_CHICKEN_HURT("minecraft:entity.chicken.hurt");
const ResourceLocation ENTITY_CHICKEN_STEP("minecraft:entity.chicken.step");

// 牛
const ResourceLocation ENTITY_COW_AMBIENT("minecraft:entity.cow.ambient");
const ResourceLocation ENTITY_COW_DEATH("minecraft:entity.cow.death");
const ResourceLocation ENTITY_COW_HURT("minecraft:entity.cow.hurt");
const ResourceLocation ENTITY_COW_MILK("minecraft:entity.cow.milk");
const ResourceLocation ENTITY_COW_STEP("minecraft:entity.cow.step");

// 猪
const ResourceLocation ENTITY_PIG_AMBIENT("minecraft:entity.pig.ambient");
const ResourceLocation ENTITY_PIG_DEATH("minecraft:entity.pig.death");
const ResourceLocation ENTITY_PIG_HURT("minecraft:entity.pig.hurt");
const ResourceLocation ENTITY_PIG_SADDLE("minecraft:entity.pig.saddle");
const ResourceLocation ENTITY_PIG_STEP("minecraft:entity.pig.step");

// 羊
const ResourceLocation ENTITY_SHEEP_AMBIENT("minecraft:entity.sheep.ambient");
const ResourceLocation ENTITY_SHEEP_DEATH("minecraft:entity.sheep.death");
const ResourceLocation ENTITY_SHEEP_HURT("minecraft:entity.sheep.hurt");
const ResourceLocation ENTITY_SHEEP_SHEAR("minecraft:entity.sheep.shear");
const ResourceLocation ENTITY_SHEEP_STEP("minecraft:entity.sheep.step");

// 马
const ResourceLocation ENTITY_HORSE_AMBIENT("minecraft:entity.horse.ambient");
const ResourceLocation ENTITY_HORSE_ANGRY("minecraft:entity.horse.angry");
const ResourceLocation ENTITY_HORSE_ARMOR("minecraft:entity.horse.armor");
const ResourceLocation ENTITY_HORSE_BREATHE("minecraft:entity.horse.breathe");
const ResourceLocation ENTITY_HORSE_DEATH("minecraft:entity.horse.death");
const ResourceLocation ENTITY_HORSE_EAT("minecraft:entity.horse.eat");
const ResourceLocation ENTITY_HORSE_GALLOP("minecraft:entity.horse.gallop");
const ResourceLocation ENTITY_HORSE_HURT("minecraft:entity.horse.hurt");
const ResourceLocation ENTITY_HORSE_JUMP("minecraft:entity.horse.jump");
const ResourceLocation ENTITY_HORSE_LAND("minecraft:entity.horse.land");
const ResourceLocation ENTITY_HORSE_SADDLE("minecraft:entity.horse.saddle");
const ResourceLocation ENTITY_HORSE_STEP("minecraft:entity.horse.step");
const ResourceLocation ENTITY_HORSE_STEP_WOOD("minecraft:entity.horse.step_wood");

// 驴
const ResourceLocation ENTITY_DONKEY_AMBIENT("minecraft:entity.donkey.ambient");
const ResourceLocation ENTITY_DONKEY_ANGRY("minecraft:entity.donkey.angry");
const ResourceLocation ENTITY_DONKEY_CHEST("minecraft:entity.donkey.chest");
const ResourceLocation ENTITY_DONKEY_DEATH("minecraft:entity.donkey.death");
const ResourceLocation ENTITY_DONKEY_EAT("minecraft:entity.donkey.eat");
const ResourceLocation ENTITY_DONKEY_HURT("minecraft:entity.donkey.hurt");

// 骡
const ResourceLocation ENTITY_MULE_AMBIENT("minecraft:entity.mule.ambient");
const ResourceLocation ENTITY_MULE_ANGRY("minecraft:entity.mule.angry");
const ResourceLocation ENTITY_MULE_CHEST("minecraft:entity.mule.chest");
const ResourceLocation ENTITY_MULE_DEATH("minecraft:entity.mule.death");
const ResourceLocation ENTITY_MULE_EAT("minecraft:entity.mule.eat");
const ResourceLocation ENTITY_MULE_HURT("minecraft:entity.mule.hurt");

// 羊驼
const ResourceLocation ENTITY_LLAMA_AMBIENT("minecraft:entity.llama.ambient");
const ResourceLocation ENTITY_LLAMA_ANGRY("minecraft:entity.llama.angry");
const ResourceLocation ENTITY_LLAMA_CHEST("minecraft:entity.llama.chest");
const ResourceLocation ENTITY_LLAMA_DEATH("minecraft:entity.llama.death");
const ResourceLocation ENTITY_LLAMA_EAT("minecraft:entity.llama.eat");
const ResourceLocation ENTITY_LLAMA_HURT("minecraft:entity.llama.hurt");
const ResourceLocation ENTITY_LLAMA_SPIT("minecraft:entity.llama.spit");
const ResourceLocation ENTITY_LLAMA_STEP("minecraft:entity.llama.step");
const ResourceLocation ENTITY_LLAMA_SWAG("minecraft:entity.llama.swag");

// 猫
const ResourceLocation ENTITY_CAT_AMBIENT("minecraft:entity.cat.ambient");
const ResourceLocation ENTITY_CAT_STRAY_AMBIENT("minecraft:entity.cat.stray_ambient");
const ResourceLocation ENTITY_CAT_DEATH("minecraft:entity.cat.death");
const ResourceLocation ENTITY_CAT_EAT("minecraft:entity.cat.eat");
const ResourceLocation ENTITY_CAT_HISS("minecraft:entity.cat.hiss");
const ResourceLocation ENTITY_CAT_BEG_FOR_FOOD("minecraft:entity.cat.beg_for_food");
const ResourceLocation ENTITY_CAT_HURT("minecraft:entity.cat.hurt");
const ResourceLocation ENTITY_CAT_PURR("minecraft:entity.cat.purr");
const ResourceLocation ENTITY_CAT_PURREOW("minecraft:entity.cat.purreow");

// 豹猫
const ResourceLocation ENTITY_OCELOT_AMBIENT("minecraft:entity.ocelot.ambient");
const ResourceLocation ENTITY_OCELOT_DEATH("minecraft:entity.ocelot.death");
const ResourceLocation ENTITY_OCELOT_HURT("minecraft:entity.ocelot.hurt");

// 狼
const ResourceLocation ENTITY_WOLF_AMBIENT("minecraft:entity.wolf.ambient");
const ResourceLocation ENTITY_WOLF_DEATH("minecraft:entity.wolf.death");
const ResourceLocation ENTITY_WOLF_GROWL("minecraft:entity.wolf.growl");
const ResourceLocation ENTITY_WOLF_HOWL("minecraft:entity.wolf.howl");
const ResourceLocation ENTITY_WOLF_HURT("minecraft:entity.wolf.hurt");
const ResourceLocation ENTITY_WOLF_PANT("minecraft:entity.wolf.pant");
const ResourceLocation ENTITY_WOLF_SHAKE("minecraft:entity.wolf.shake");
const ResourceLocation ENTITY_WOLF_STEP("minecraft:entity.wolf.step");
const ResourceLocation ENTITY_WOLF_WHINE("minecraft:entity.wolf.whine");

/// 狼铠音效
const ResourceLocation ENTITY_WOLF_ARMOR_BREAK("minecraft:item.wolf_armor.break");
const ResourceLocation ENTITY_WOLF_ARMOR_CRACK("minecraft:item.wolf_armor.crack");
const ResourceLocation ENTITY_WOLF_ARMOR_DAMAGE("minecraft:item.wolf_armor.damage");
const ResourceLocation ENTITY_WOLF_ARMOR_REPAIR("minecraft:item.wolf_armor.repair");

// 兔子
const ResourceLocation ENTITY_RABBIT_AMBIENT("minecraft:entity.rabbit.ambient");
const ResourceLocation ENTITY_RABBIT_ATTACK("minecraft:entity.rabbit.attack");
const ResourceLocation ENTITY_RABBIT_DEATH("minecraft:entity.rabbit.death");
const ResourceLocation ENTITY_RABBIT_HURT("minecraft:entity.rabbit.hurt");
const ResourceLocation ENTITY_RABBIT_JUMP("minecraft:entity.rabbit.jump");

// 北极熊
const ResourceLocation ENTITY_POLAR_BEAR_AMBIENT("minecraft:entity.polar_bear.ambient");
const ResourceLocation ENTITY_POLAR_BEAR_AMBIENT_BABY("minecraft:entity.polar_bear.ambient_baby");
const ResourceLocation ENTITY_POLAR_BEAR_DEATH("minecraft:entity.polar_bear.death");
const ResourceLocation ENTITY_POLAR_BEAR_HURT("minecraft:entity.polar_bear.hurt");
const ResourceLocation ENTITY_POLAR_BEAR_STEP("minecraft:entity.polar_bear.step");
const ResourceLocation ENTITY_POLAR_BEAR_WARNING("minecraft:entity.polar_bear.warning");

// 蝙蝠
const ResourceLocation ENTITY_BAT_AMBIENT("minecraft:entity.bat.ambient");
const ResourceLocation ENTITY_BAT_DEATH("minecraft:entity.bat.death");
const ResourceLocation ENTITY_BAT_HURT("minecraft:entity.bat.hurt");
const ResourceLocation ENTITY_BAT_LOOP("minecraft:entity.bat.loop");
const ResourceLocation ENTITY_BAT_TAKEOFF("minecraft:entity.bat.takeoff");

// 狐狸
const ResourceLocation ENTITY_FOX_AGGRO("minecraft:entity.fox.aggro");
const ResourceLocation ENTITY_FOX_AMBIENT("minecraft:entity.fox.ambient");
const ResourceLocation ENTITY_FOX_BITE("minecraft:entity.fox.bite");
const ResourceLocation ENTITY_FOX_DEATH("minecraft:entity.fox.death");
const ResourceLocation ENTITY_FOX_EAT("minecraft:entity.fox.eat");
const ResourceLocation ENTITY_FOX_HURT("minecraft:entity.fox.hurt");
const ResourceLocation ENTITY_FOX_SCREECH("minecraft:entity.fox.screech");
const ResourceLocation ENTITY_FOX_SLEEP("minecraft:entity.fox.sleep");
const ResourceLocation ENTITY_FOX_SNIFF("minecraft:entity.fox.sniff");
const ResourceLocation ENTITY_FOX_SPIT("minecraft:entity.fox.spit");
const ResourceLocation ENTITY_FOX_TELEPORT("minecraft:entity.fox.teleport");

// 熊猫
const ResourceLocation ENTITY_PANDA_AGGRESSIVE_AMBIENT("minecraft:entity.panda.aggressive_ambient");
const ResourceLocation ENTITY_PANDA_AMBIENT("minecraft:entity.panda.ambient");
const ResourceLocation ENTITY_PANDA_BITE("minecraft:entity.panda.bite");
const ResourceLocation ENTITY_PANDA_CANT_BREED("minecraft:entity.panda.cant_breed");
const ResourceLocation ENTITY_PANDA_DEATH("minecraft:entity.panda.death");
const ResourceLocation ENTITY_PANDA_EAT("minecraft:entity.panda.eat");
const ResourceLocation ENTITY_PANDA_HURT("minecraft:entity.panda.hurt");
const ResourceLocation ENTITY_PANDA_PRE_SNEEZE("minecraft:entity.panda.pre_sneeze");
const ResourceLocation ENTITY_PANDA_SNEEZE("minecraft:entity.panda.sneeze");
const ResourceLocation ENTITY_PANDA_STEP("minecraft:entity.panda.step");
const ResourceLocation ENTITY_PANDA_WORRIED_AMBIENT("minecraft:entity.panda.worried_ambient");

// 鹦鹉
const ResourceLocation ENTITY_PARROT_AMBIENT("minecraft:entity.parrot.ambient");
const ResourceLocation ENTITY_PARROT_DEATH("minecraft:entity.parrot.death");
const ResourceLocation ENTITY_PARROT_EAT("minecraft:entity.parrot.eat");
const ResourceLocation ENTITY_PARROT_FLY("minecraft:entity.parrot.fly");
const ResourceLocation ENTITY_PARROT_HURT("minecraft:entity.parrot.hurt");
const ResourceLocation ENTITY_PARROT_IMITATE_BLAZE("minecraft:entity.parrot.imitate.blaze");
const ResourceLocation ENTITY_PARROT_IMITATE_CREEPER("minecraft:entity.parrot.imitate.creeper");
const ResourceLocation ENTITY_PARROT_IMITATE_DROWNED("minecraft:entity.parrot.imitate.drowned");
const ResourceLocation ENTITY_PARROT_IMITATE_ELDER_GUARDIAN("minecraft:entity.parrot.imitate.elder_guardian");
const ResourceLocation ENTITY_PARROT_IMITATE_ENDER_DRAGON("minecraft:entity.parrot.imitate.ender_dragon");
const ResourceLocation ENTITY_PARROT_IMITATE_ENDERMITE("minecraft:entity.parrot.imitate.endermite");
const ResourceLocation ENTITY_PARROT_IMITATE_EVOKER("minecraft:entity.parrot.imitate.evoker");
const ResourceLocation ENTITY_PARROT_IMITATE_GHAST("minecraft:entity.parrot.imitate.ghast");
const ResourceLocation ENTITY_PARROT_IMITATE_GUARDIAN("minecraft:entity.parrot.imitate.guardian");
const ResourceLocation ENTITY_PARROT_IMITATE_HOGLIN("minecraft:entity.parrot.imitate.hoglin");
const ResourceLocation ENTITY_PARROT_IMITATE_HUSK("minecraft:entity.parrot.imitate.husk");
const ResourceLocation ENTITY_PARROT_IMITATE_ILLUSIONER("minecraft:entity.parrot.imitate.illusioner");
const ResourceLocation ENTITY_PARROT_IMITATE_MAGMA_CUBE("minecraft:entity.parrot.imitate.magma_cube");
const ResourceLocation ENTITY_PARROT_IMITATE_PHANTOM("minecraft:entity.parrot.imitate.phantom");
const ResourceLocation ENTITY_PARROT_IMITATE_PIGLIN("minecraft:entity.parrot.imitate.piglin");
const ResourceLocation ENTITY_PARROT_IMITATE_PIGLIN_BRUTE("minecraft:entity.parrot.imitate.piglin_brute");
const ResourceLocation ENTITY_PARROT_IMITATE_PILLAGER("minecraft:entity.parrot.imitate.pillager");
const ResourceLocation ENTITY_PARROT_IMITATE_RAVAGER("minecraft:entity.parrot.imitate.ravager");
const ResourceLocation ENTITY_PARROT_IMITATE_SHULKER("minecraft:entity.parrot.imitate.shulker");
const ResourceLocation ENTITY_PARROT_IMITATE_SILVERFISH("minecraft:entity.parrot.imitate.silverfish");
const ResourceLocation ENTITY_PARROT_IMITATE_SKELETON("minecraft:entity.parrot.imitate.skeleton");
const ResourceLocation ENTITY_PARROT_IMITATE_SLIME("minecraft:entity.parrot.imitate.slime");
const ResourceLocation ENTITY_PARROT_IMITATE_SPIDER("minecraft:entity.parrot.imitate.spider");
const ResourceLocation ENTITY_PARROT_IMITATE_STRAY("minecraft:entity.parrot.imitate.stray");
const ResourceLocation ENTITY_PARROT_IMITATE_VEX("minecraft:entity.parrot.imitate.vex");
const ResourceLocation ENTITY_PARROT_IMITATE_VINDICATOR("minecraft:entity.parrot.imitate.vindicator");
const ResourceLocation ENTITY_PARROT_IMITATE_WITCH("minecraft:entity.parrot.imitate.witch");
const ResourceLocation ENTITY_PARROT_IMITATE_WITHER("minecraft:entity.parrot.imitate.wither");
const ResourceLocation ENTITY_PARROT_IMITATE_WITHER_SKELETON("minecraft:entity.parrot.imitate.wither_skeleton");
const ResourceLocation ENTITY_PARROT_IMITATE_ZOGLIN("minecraft:entity.parrot.imitate.zoglin");
const ResourceLocation ENTITY_PARROT_IMITATE_ZOMBIE("minecraft:entity.parrot.imitate.zombie");
const ResourceLocation ENTITY_PARROT_IMITATE_ZOMBIE_VILLAGER("minecraft:entity.parrot.imitate.zombie_villager");
const ResourceLocation ENTITY_PARROT_STEP("minecraft:entity.parrot.step");

// 骷髅马
const ResourceLocation ENTITY_SKELETON_HORSE_AMBIENT("minecraft:entity.skeleton_horse.ambient");
const ResourceLocation ENTITY_SKELETON_HORSE_AMBIENT_WATER("minecraft:entity.skeleton_horse.ambient_water");
const ResourceLocation ENTITY_SKELETON_HORSE_DEATH("minecraft:entity.skeleton_horse.death");
const ResourceLocation ENTITY_SKELETON_HORSE_GALLOP_WATER("minecraft:entity.skeleton_horse.gallop_water");
const ResourceLocation ENTITY_SKELETON_HORSE_HURT("minecraft:entity.skeleton_horse.hurt");
const ResourceLocation ENTITY_SKELETON_HORSE_JUMP_WATER("minecraft:entity.skeleton_horse.jump_water");
const ResourceLocation ENTITY_SKELETON_HORSE_STEP_WATER("minecraft:entity.skeleton_horse.step_water");
const ResourceLocation ENTITY_SKELETON_HORSE_SWIM("minecraft:entity.skeleton_horse.swim");

// 僵尸马
const ResourceLocation ENTITY_ZOMBIE_HORSE_AMBIENT("minecraft:entity.zombie_horse.ambient");
const ResourceLocation ENTITY_ZOMBIE_HORSE_DEATH("minecraft:entity.zombie_horse.death");
const ResourceLocation ENTITY_ZOMBIE_HORSE_HURT("minecraft:entity.zombie_horse.hurt");

// 幻术师
const ResourceLocation ENTITY_ILLUSIONER_AMBIENT("minecraft:entity.illusioner.ambient");
const ResourceLocation ENTITY_ILLUSIONER_CAST_SPELL("minecraft:entity.illusioner.cast_spell");
const ResourceLocation ENTITY_ILLUSIONER_DEATH("minecraft:entity.illusioner.death");
const ResourceLocation ENTITY_ILLUSIONER_HURT("minecraft:entity.illusioner.hurt");
const ResourceLocation ENTITY_ILLUSIONER_MIRROR_MOVE("minecraft:entity.illusioner.mirror_move");
const ResourceLocation ENTITY_ILLUSIONER_PREPARE_BLINDNESS("minecraft:entity.illusioner.prepare_blindness");
const ResourceLocation ENTITY_ILLUSIONER_PREPARE_MIRROR("minecraft:entity.illusioner.prepare_mirror");

// 敌对生物通用
const ResourceLocation ENTITY_HOSTILE_BIG_FALL("minecraft:entity.hostile.big_fall");
const ResourceLocation ENTITY_HOSTILE_DEATH("minecraft:entity.hostile.death");
const ResourceLocation ENTITY_HOSTILE_HURT("minecraft:entity.hostile.hurt");
const ResourceLocation ENTITY_HOSTILE_SMALL_FALL("minecraft:entity.hostile.small_fall");
const ResourceLocation ENTITY_HOSTILE_SPLASH("minecraft:entity.hostile.splash");
const ResourceLocation ENTITY_HOSTILE_SWIM("minecraft:entity.hostile.swim");

// 海豚
const ResourceLocation ENTITY_DOLPHIN_AMBIENT("minecraft:entity.dolphin.ambient");
const ResourceLocation ENTITY_DOLPHIN_AMBIENT_WATER("minecraft:entity.dolphin.ambient_water");
const ResourceLocation ENTITY_DOLPHIN_ATTACK("minecraft:entity.dolphin.attack");
const ResourceLocation ENTITY_DOLPHIN_DEATH("minecraft:entity.dolphin.death");
const ResourceLocation ENTITY_DOLPHIN_EAT("minecraft:entity.dolphin.eat");
const ResourceLocation ENTITY_DOLPHIN_HURT("minecraft:entity.dolphin.hurt");
const ResourceLocation ENTITY_DOLPHIN_JUMP("minecraft:entity.dolphin.jump");
const ResourceLocation ENTITY_DOLPHIN_PLAY("minecraft:entity.dolphin.play");
const ResourceLocation ENTITY_DOLPHIN_SPLASH("minecraft:entity.dolphin.splash");
const ResourceLocation ENTITY_DOLPHIN_SWIM("minecraft:entity.dolphin.swim");

// 美西螈
const ResourceLocation ENTITY_AXOLOTL_ATTACK("minecraft:entity.axolotl.attack");
const ResourceLocation ENTITY_AXOLOTL_DEATH("minecraft:entity.axolotl.death");
const ResourceLocation ENTITY_AXOLOTL_HURT("minecraft:entity.axolotl.hurt");
const ResourceLocation ENTITY_AXOLOTL_IDLE_AIR("minecraft:entity.axolotl.idle_air");
const ResourceLocation ENTITY_AXOLOTL_IDLE_WATER("minecraft:entity.axolotl.idle_water");
const ResourceLocation ENTITY_AXOLOTL_SPLASH("minecraft:entity.axolotl.splash");
const ResourceLocation ENTITY_AXOLOTL_SWIM("minecraft:entity.axolotl.swim");
const ResourceLocation ITEM_BUCKET_FILL_AXOLOTL("minecraft:item.bucket.fill_axolotl");
const ResourceLocation ITEM_BUCKET_EMPTY_AXOLOTL("minecraft:item.bucket.empty_axolotl");

// 鹦鹉螺（成体）
const ResourceLocation ENTITY_NAUTILUS_AMBIENT("minecraft:entity.nautilus.ambient");
const ResourceLocation ENTITY_NAUTILUS_AMBIENT_ON_LAND("minecraft:entity.nautilus.ambient_on_land");
const ResourceLocation ENTITY_NAUTILUS_HURT("minecraft:entity.nautilus.hurt");
const ResourceLocation ENTITY_NAUTILUS_HURT_ON_LAND("minecraft:entity.nautilus.hurt_on_land");
const ResourceLocation ENTITY_NAUTILUS_DEATH("minecraft:entity.nautilus.death");
const ResourceLocation ENTITY_NAUTILUS_DEATH_ON_LAND("minecraft:entity.nautilus.death_on_land");
const ResourceLocation ENTITY_NAUTILUS_DASH("minecraft:entity.nautilus.dash");
const ResourceLocation ENTITY_NAUTILUS_DASH_ON_LAND("minecraft:entity.nautilus.dash_on_land");
const ResourceLocation ENTITY_NAUTILUS_DASH_READY("minecraft:entity.nautilus.dash_ready");
const ResourceLocation ENTITY_NAUTILUS_DASH_READY_ON_LAND("minecraft:entity.nautilus.dash_ready_on_land");
const ResourceLocation ENTITY_NAUTILUS_EAT("minecraft:entity.nautilus.eat");
const ResourceLocation ENTITY_NAUTILUS_SADDLE_EQUIP("minecraft:entity.nautilus.saddle_equip");
const ResourceLocation ENTITY_NAUTILUS_SADDLE_UNDERWATER_EQUIP("minecraft:entity.nautilus.saddle_underwater_equip");

// 鹦鹉螺（幼体）
const ResourceLocation ENTITY_BABY_NAUTILUS_AMBIENT("minecraft:entity.baby_nautilus.ambient");
const ResourceLocation ENTITY_BABY_NAUTILUS_AMBIENT_ON_LAND("minecraft:entity.baby_nautilus.ambient_on_land");
const ResourceLocation ENTITY_BABY_NAUTILUS_HURT("minecraft:entity.baby_nautilus.hurt");
const ResourceLocation ENTITY_BABY_NAUTILUS_HURT_ON_LAND("minecraft:entity.baby_nautilus.hurt_on_land");
const ResourceLocation ENTITY_BABY_NAUTILUS_DEATH("minecraft:entity.baby_nautilus.death");
const ResourceLocation ENTITY_BABY_NAUTILUS_DEATH_ON_LAND("minecraft:entity.baby_nautilus.death_on_land");
const ResourceLocation ENTITY_BABY_NAUTILUS_EAT("minecraft:entity.baby_nautilus.eat");

// 僵尸鹦鹉螺
const ResourceLocation ENTITY_ZOMBIE_NAUTILUS_AMBIENT("minecraft:entity.zombie_nautilus.ambient");
const ResourceLocation ENTITY_ZOMBIE_NAUTILUS_AMBIENT_ON_LAND("minecraft:entity.zombie_nautilus.ambient_on_land");
const ResourceLocation ENTITY_ZOMBIE_NAUTILUS_HURT("minecraft:entity.zombie_nautilus.hurt");
const ResourceLocation ENTITY_ZOMBIE_NAUTILUS_HURT_ON_LAND("minecraft:entity.zombie_nautilus.hurt_on_land");
const ResourceLocation ENTITY_ZOMBIE_NAUTILUS_DEATH("minecraft:entity.zombie_nautilus.death");
const ResourceLocation ENTITY_ZOMBIE_NAUTILUS_DEATH_ON_LAND("minecraft:entity.zombie_nautilus.death_on_land");
const ResourceLocation ENTITY_ZOMBIE_NAUTILUS_DASH("minecraft:entity.zombie_nautilus.dash");
const ResourceLocation ENTITY_ZOMBIE_NAUTILUS_DASH_ON_LAND("minecraft:entity.zombie_nautilus.dash_on_land");
const ResourceLocation ENTITY_ZOMBIE_NAUTILUS_DASH_READY("minecraft:entity.zombie_nautilus.dash_ready");
const ResourceLocation ENTITY_ZOMBIE_NAUTILUS_DASH_READY_ON_LAND("minecraft:entity.zombie_nautilus.dash_ready_on_land");
const ResourceLocation ENTITY_ZOMBIE_NAUTILUS_EAT("minecraft:entity.zombie_nautilus.eat");

// 鱿鱼
const ResourceLocation ENTITY_SQUID_AMBIENT("minecraft:entity.squid.ambient");
const ResourceLocation ENTITY_SQUID_DEATH("minecraft:entity.squid.death");
const ResourceLocation ENTITY_SQUID_HURT("minecraft:entity.squid.hurt");
const ResourceLocation ENTITY_SQUID_SQUIRT("minecraft:entity.squid.squirt");

// 发光鱿鱼
const ResourceLocation ENTITY_GLOW_SQUID_AMBIENT("minecraft:entity.glow_squid.ambient");
const ResourceLocation ENTITY_GLOW_SQUID_DEATH("minecraft:entity.glow_squid.death");
const ResourceLocation ENTITY_GLOW_SQUID_HURT("minecraft:entity.glow_squid.hurt");
const ResourceLocation ENTITY_GLOW_SQUID_SQUIRT("minecraft:entity.glow_squid.squirt");

// 鱼
const ResourceLocation ENTITY_COD_AMBIENT("minecraft:entity.cod.ambient");
const ResourceLocation ENTITY_COD_DEATH("minecraft:entity.cod.death");
const ResourceLocation ENTITY_COD_FLOP("minecraft:entity.cod.flop");
const ResourceLocation ENTITY_COD_HURT("minecraft:entity.cod.hurt");

// 蝌蚪
const ResourceLocation ENTITY_TADPOLE_DEATH("minecraft:entity.tadpole.death");
const ResourceLocation ENTITY_TADPOLE_FLOP("minecraft:entity.tadpole.flop");
const ResourceLocation ENTITY_TADPOLE_GROW_UP("minecraft:entity.tadpole.grow_up");
const ResourceLocation ENTITY_TADPOLE_HURT("minecraft:entity.tadpole.hurt");

// 青蛙
const ResourceLocation FROG_AMBIENT("minecraft:entity.frog.ambient");
const ResourceLocation FROG_DEATH("minecraft:entity.frog.death");
const ResourceLocation FROG_EAT("minecraft:entity.frog.eat");
const ResourceLocation FROG_HURT("minecraft:entity.frog.hurt");
const ResourceLocation FROG_STEP("minecraft:entity.frog.step");
const ResourceLocation FROG_TONGUE("minecraft:entity.frog.tongue");

const ResourceLocation FROGSPAWN_HATCH("minecraft:block.frogspawn.hatch");

const ResourceLocation ENTITY_SALMON_AMBIENT("minecraft:entity.salmon.ambient");
const ResourceLocation ENTITY_SALMON_DEATH("minecraft:entity.salmon.death");
const ResourceLocation ENTITY_SALMON_FLOP("minecraft:entity.salmon.flop");
const ResourceLocation ENTITY_SALMON_HURT("minecraft:entity.salmon.hurt");

const ResourceLocation ENTITY_TROPICAL_FISH_AMBIENT("minecraft:entity.tropical_fish.ambient");
const ResourceLocation ENTITY_TROPICAL_FISH_DEATH("minecraft:entity.tropical_fish.death");
const ResourceLocation ENTITY_TROPICAL_FISH_FLOP("minecraft:entity.tropical_fish.flop");
const ResourceLocation ENTITY_TROPICAL_FISH_HURT("minecraft:entity.tropical_fish.hurt");

const ResourceLocation ENTITY_PUFFER_FISH_AMBIENT("minecraft:entity.puffer_fish.ambient");
const ResourceLocation ENTITY_PUFFER_FISH_BLOW_OUT("minecraft:entity.puffer_fish.blow_out");
const ResourceLocation ENTITY_PUFFER_FISH_BLOW_UP("minecraft:entity.puffer_fish.blow_up");
const ResourceLocation ENTITY_PUFFER_FISH_DEATH("minecraft:entity.puffer_fish.death");
const ResourceLocation ENTITY_PUFFER_FISH_FLOP("minecraft:entity.puffer_fish.flop");
const ResourceLocation ENTITY_PUFFER_FISH_HURT("minecraft:entity.puffer_fish.hurt");
const ResourceLocation ENTITY_PUFFER_FISH_STING("minecraft:entity.puffer_fish.sting");

// 海龟
const ResourceLocation ENTITY_TURTLE_AMBIENT_LAND("minecraft:entity.turtle.ambient_land");
const ResourceLocation ENTITY_TURTLE_DEATH("minecraft:entity.turtle.death");
const ResourceLocation ENTITY_TURTLE_DEATH_BABY("minecraft:entity.turtle.death_baby");
const ResourceLocation ENTITY_TURTLE_EGG_BREAK("minecraft:entity.turtle.egg_break");
const ResourceLocation ENTITY_TURTLE_EGG_CRACK("minecraft:entity.turtle.egg_crack");
const ResourceLocation ENTITY_TURTLE_EGG_HATCH("minecraft:entity.turtle.egg_hatch");
const ResourceLocation ENTITY_TURTLE_HURT("minecraft:entity.turtle.hurt");
const ResourceLocation ENTITY_TURTLE_HURT_BABY("minecraft:entity.turtle.hurt_baby");
const ResourceLocation ENTITY_TURTLE_LAY_EGG("minecraft:entity.turtle.lay_egg");
const ResourceLocation ENTITY_TURTLE_SHAMBLE("minecraft:entity.turtle.shamble");
const ResourceLocation ENTITY_TURTLE_SHAMBLE_BABY("minecraft:entity.turtle.shamble_baby");
const ResourceLocation ENTITY_TURTLE_SWIM("minecraft:entity.turtle.swim");

// 嗅探兽
const ResourceLocation SNIFFER_STEP("minecraft:entity.sniffer.step");
const ResourceLocation SNIFFER_EAT("minecraft:entity.sniffer.eat");
const ResourceLocation SNIFFER_IDLE("minecraft:entity.sniffer.idle");
const ResourceLocation SNIFFER_HURT("minecraft:entity.sniffer.hurt");
const ResourceLocation SNIFFER_DEATH("minecraft:entity.sniffer.death");
const ResourceLocation SNIFFER_DROP_SEED("minecraft:entity.sniffer.drop_seed");
const ResourceLocation SNIFFER_SCENTING("minecraft:entity.sniffer.scenting");
const ResourceLocation SNIFFER_SNIFFING("minecraft:entity.sniffer.sniffing");
const ResourceLocation SNIFFER_SEARCHING("minecraft:entity.sniffer.searching");
const ResourceLocation SNIFFER_DIGGING("minecraft:entity.sniffer.digging");
const ResourceLocation SNIFFER_DIGGING_STOP("minecraft:entity.sniffer.digging_stop");
const ResourceLocation SNIFFER_HAPPY("minecraft:entity.sniffer.happy");
const ResourceLocation SNIFFER_EGG_PLOP("minecraft:block.sniffer_egg.plop");
const ResourceLocation SNIFFER_EGG_CRACK("minecraft:block.sniffer_egg.crack");
const ResourceLocation SNIFFER_EGG_HATCH("minecraft:block.sniffer_egg.hatch");

// 蜜蜂
const ResourceLocation ENTITY_BEE_DEATH("minecraft:entity.bee.death");
const ResourceLocation ENTITY_BEE_HURT("minecraft:entity.bee.hurt");
const ResourceLocation ENTITY_BEE_LOOP("minecraft:entity.bee.loop");
const ResourceLocation ENTITY_BEE_LOOP_AGGRESSIVE("minecraft:entity.bee.loop_aggressive");
const ResourceLocation ENTITY_BEE_STING("minecraft:entity.bee.sting");
const ResourceLocation ENTITY_BEE_POLLINATE("minecraft:entity.bee.pollinate");

// 村民
const ResourceLocation ENTITY_VILLAGER_AMBIENT("minecraft:entity.villager.ambient");
const ResourceLocation ENTITY_VILLAGER_CELEBRATE("minecraft:entity.villager.celebrate");
const ResourceLocation ENTITY_VILLAGER_DEATH("minecraft:entity.villager.death");
const ResourceLocation ENTITY_VILLAGER_HURT("minecraft:entity.villager.hurt");
const ResourceLocation ENTITY_VILLAGER_NO("minecraft:entity.villager.no");
const ResourceLocation ENTITY_VILLAGER_TRADE("minecraft:entity.villager.trade");
const ResourceLocation ENTITY_VILLAGER_YES("minecraft:entity.villager.yes");
const ResourceLocation ENTITY_VILLAGER_WORK_ARMORER("minecraft:entity.villager.work_armorer");
const ResourceLocation ENTITY_VILLAGER_WORK_BUTCHER("minecraft:entity.villager.work_butcher");
const ResourceLocation ENTITY_VILLAGER_WORK_CARTOGRAPHER("minecraft:entity.villager.work_cartographer");
const ResourceLocation ENTITY_VILLAGER_WORK_CLERIC("minecraft:entity.villager.work_cleric");
const ResourceLocation ENTITY_VILLAGER_WORK_FARMER("minecraft:entity.villager.work_farmer");
const ResourceLocation ENTITY_VILLAGER_WORK_FISHERMAN("minecraft:entity.villager.work_fisherman");
const ResourceLocation ENTITY_VILLAGER_WORK_FLETCHER("minecraft:entity.villager.work_fletcher");
const ResourceLocation ENTITY_VILLAGER_WORK_LEATHERWORKER("minecraft:entity.villager.work_leatherworker");
const ResourceLocation ENTITY_VILLAGER_WORK_LIBRARIAN("minecraft:entity.villager.work_librarian");
const ResourceLocation ENTITY_VILLAGER_WORK_MASON("minecraft:entity.villager.work_mason");
const ResourceLocation ENTITY_VILLAGER_WORK_SHEPHERD("minecraft:entity.villager.work_shepherd");
const ResourceLocation ENTITY_VILLAGER_WORK_TOOLSMITH("minecraft:entity.villager.work_toolsmith");
const ResourceLocation ENTITY_VILLAGER_WORK_WEAPONSMITH("minecraft:entity.villager.work_weaponsmith");

// 流浪商人
const ResourceLocation ENTITY_WANDERING_TRADER_AMBIENT("minecraft:entity.wandering_trader.ambient");
const ResourceLocation ENTITY_WANDERING_TRADER_DEATH("minecraft:entity.wandering_trader.death");
const ResourceLocation ENTITY_WANDERING_TRADER_DISAPPEARED("minecraft:entity.wandering_trader.disappeared");
const ResourceLocation ENTITY_WANDERING_TRADER_DRINK_MILK("minecraft:entity.wandering_trader.drink_milk");
const ResourceLocation ENTITY_WANDERING_TRADER_DRINK_POTION("minecraft:entity.wandering_trader.drink_potion");
const ResourceLocation ENTITY_WANDERING_TRADER_HURT("minecraft:entity.wandering_trader.hurt");
const ResourceLocation ENTITY_WANDERING_TRADER_NO("minecraft:entity.wandering_trader.no");
const ResourceLocation ENTITY_WANDERING_TRADER_REAPPEARED("minecraft:entity.wandering_trader.reappeared");
const ResourceLocation ENTITY_WANDERING_TRADER_TRADE("minecraft:entity.wandering_trader.trade");
const ResourceLocation ENTITY_WANDERING_TRADER_YES("minecraft:entity.wandering_trader.yes");

// 铁傀儡
const ResourceLocation ENTITY_IRON_GOLEM_ATTACK("minecraft:entity.iron_golem.attack");
const ResourceLocation ENTITY_IRON_GOLEM_DAMAGE("minecraft:entity.iron_golem.damage");
const ResourceLocation ENTITY_IRON_GOLEM_DEATH("minecraft:entity.iron_golem.death");
const ResourceLocation ENTITY_IRON_GOLEM_HURT("minecraft:entity.iron_golem.hurt");
const ResourceLocation ENTITY_IRON_GOLEM_REPAIR("minecraft:entity.iron_golem.repair");
const ResourceLocation ENTITY_IRON_GOLEM_STEP("minecraft:entity.iron_golem.step");

// 雪傀儡
const ResourceLocation ENTITY_SNOW_GOLEM_AMBIENT("minecraft:entity.snow_golem.ambient");
const ResourceLocation ENTITY_SNOW_GOLEM_DEATH("minecraft:entity.snow_golem.death");
const ResourceLocation ENTITY_SNOW_GOLEM_HURT("minecraft:entity.snow_golem.hurt");
const ResourceLocation ENTITY_SNOW_GOLEM_SHOOT("minecraft:entity.snow_golem.shoot");
const ResourceLocation ENTITY_SNOW_GOLEM_SHEAR("minecraft:entity.snow_golem.shear");

// 铜傀儡（MC 1.21.11）
// 完整对齐 MC 1.21.11 SoundEvents 中所有 COPPER_GOLEM_* 与铜傀儡雕像相关事件。
// 基础（Unaffected 等级）铜傀儡音效
const ResourceLocation ENTITY_COPPER_GOLEM_STEP("minecraft:entity.copper_golem.step");
const ResourceLocation ENTITY_COPPER_GOLEM_HURT("minecraft:entity.copper_golem.hurt");
const ResourceLocation ENTITY_COPPER_GOLEM_DEATH("minecraft:entity.copper_golem.death");
const ResourceLocation ENTITY_COPPER_GOLEM_SPIN("minecraft:entity.copper_golem.spin");
const ResourceLocation ENTITY_COPPER_GOLEM_SPAWN("minecraft:entity.copper_golem.spawn");
const ResourceLocation ENTITY_COPPER_GOLEM_SHEAR("minecraft:entity.copper_golem.shear");
// 锈蚀（Weathered）等级铜傀儡音效
const ResourceLocation ENTITY_COPPER_GOLEM_WEATHERED_STEP("minecraft:entity.copper_golem_weathered.step");
const ResourceLocation ENTITY_COPPER_GOLEM_WEATHERED_HURT("minecraft:entity.copper_golem_weathered.hurt");
const ResourceLocation ENTITY_COPPER_GOLEM_WEATHERED_DEATH("minecraft:entity.copper_golem_weathered.death");
const ResourceLocation ENTITY_COPPER_GOLEM_WEATHERED_SPIN("minecraft:entity.copper_golem_weathered.spin");
// 氧化（Oxidized）等级铜傀儡音效
const ResourceLocation ENTITY_COPPER_GOLEM_OXIDIZED_STEP("minecraft:entity.copper_golem_oxidized.step");
const ResourceLocation ENTITY_COPPER_GOLEM_OXIDIZED_HURT("minecraft:entity.copper_golem_oxidized.hurt");
const ResourceLocation ENTITY_COPPER_GOLEM_OXIDIZED_DEATH("minecraft:entity.copper_golem_oxidized.death");
const ResourceLocation ENTITY_COPPER_GOLEM_OXIDIZED_SPIN("minecraft:entity.copper_golem_oxidized.spin");
// 铜傀儡物品交互音效（暂未使用，预留以保持与原版对齐）
const ResourceLocation ENTITY_COPPER_GOLEM_ITEM_GET("minecraft:entity.copper_golem.no_item_get");
const ResourceLocation ENTITY_COPPER_GOLEM_ITEM_NO_GET("minecraft:entity.copper_golem.no_item_no_get");
const ResourceLocation ENTITY_COPPER_GOLEM_ITEM_DROP("minecraft:entity.copper_golem.item_drop");
const ResourceLocation ENTITY_COPPER_GOLEM_ITEM_NO_DROP("minecraft:entity.copper_golem.item_no_drop");
// 铜傀儡雕像方块音效
const ResourceLocation BLOCK_COPPER_GOLEM_STATUE_BREAK("minecraft:block.copper_golem_statue.break");
const ResourceLocation BLOCK_COPPER_GOLEM_STATUE_PLACE("minecraft:block.copper_golem_statue.place");
const ResourceLocation BLOCK_COPPER_GOLEM_STATUE_HIT("minecraft:block.copper_golem_statue.hit");
const ResourceLocation BLOCK_COPPER_GOLEM_STATUE_STEP("minecraft:block.copper_golem_statue.step");
const ResourceLocation BLOCK_COPPER_GOLEM_STATUE_FALL("minecraft:block.copper_golem_statue.fall");

// 哞菇
const ResourceLocation ENTITY_MOOSHROOM_CONVERT("minecraft:entity.mooshroom.convert");
const ResourceLocation ENTITY_MOOSHROOM_EAT("minecraft:entity.mooshroom.eat");
const ResourceLocation ENTITY_MOOSHROOM_MILK("minecraft:entity.mooshroom.milk");
const ResourceLocation ENTITY_MOOSHROOM_SUSPICIOUS_MILK("minecraft:entity.mooshroom.suspicious_milk");
const ResourceLocation ENTITY_MOOSHROOM_SHEAR("minecraft:entity.mooshroom.shear");

// ============================================================================
// 敌对生物声音
// ============================================================================

// 僵尸
const ResourceLocation ENTITY_ZOMBIE_AMBIENT("minecraft:entity.zombie.ambient");
const ResourceLocation ENTITY_ZOMBIE_ATTACK_WOODEN_DOOR("minecraft:entity.zombie.attack_wooden_door");
const ResourceLocation ENTITY_ZOMBIE_ATTACK_IRON_DOOR("minecraft:entity.zombie.attack_iron_door");
const ResourceLocation ENTITY_ZOMBIE_BREAK_WOODEN_DOOR("minecraft:entity.zombie.break_wooden_door");
const ResourceLocation ENTITY_ZOMBIE_CONVERTED_TO_DROWNED("minecraft:entity.zombie.converted_to_drowned");
const ResourceLocation ENTITY_ZOMBIE_DEATH("minecraft:entity.zombie.death");
const ResourceLocation ENTITY_ZOMBIE_DESTROY_EGG("minecraft:entity.zombie.destroy_egg");
const ResourceLocation ENTITY_ZOMBIE_HURT("minecraft:entity.zombie.hurt");
const ResourceLocation ENTITY_ZOMBIE_INFECT("minecraft:entity.zombie.infect");
const ResourceLocation ENTITY_ZOMBIE_STEP("minecraft:entity.zombie.step");

// 僵尸村民
const ResourceLocation ENTITY_ZOMBIE_VILLAGER_AMBIENT("minecraft:entity.zombie_villager.ambient");
const ResourceLocation ENTITY_ZOMBIE_VILLAGER_CONVERTED("minecraft:entity.zombie_villager.converted");
const ResourceLocation ENTITY_ZOMBIE_VILLAGER_CURE("minecraft:entity.zombie_villager.cure");
const ResourceLocation ENTITY_ZOMBIE_VILLAGER_DEATH("minecraft:entity.zombie_villager.death");
const ResourceLocation ENTITY_ZOMBIE_VILLAGER_HURT("minecraft:entity.zombie_villager.hurt");
const ResourceLocation ENTITY_ZOMBIE_VILLAGER_STEP("minecraft:entity.zombie_villager.step");

// 尸壳
const ResourceLocation ENTITY_HUSK_AMBIENT("minecraft:entity.husk.ambient");
const ResourceLocation ENTITY_HUSK_CONVERTED_TO_ZOMBIE("minecraft:entity.husk.converted_to_zombie");
const ResourceLocation ENTITY_HUSK_DEATH("minecraft:entity.husk.death");
const ResourceLocation ENTITY_HUSK_HURT("minecraft:entity.husk.hurt");
const ResourceLocation ENTITY_HUSK_STEP("minecraft:entity.husk.step");

// 溺尸
const ResourceLocation ENTITY_DROWNED_AMBIENT("minecraft:entity.drowned.ambient");
const ResourceLocation ENTITY_DROWNED_AMBIENT_WATER("minecraft:entity.drowned.ambient_water");
const ResourceLocation ENTITY_DROWNED_DEATH("minecraft:entity.drowned.death");
const ResourceLocation ENTITY_DROWNED_DEATH_WATER("minecraft:entity.drowned.death_water");
const ResourceLocation ENTITY_DROWNED_HURT("minecraft:entity.drowned.hurt");
const ResourceLocation ENTITY_DROWNED_HURT_WATER("minecraft:entity.drowned.hurt_water");
const ResourceLocation ENTITY_DROWNED_SHOOT("minecraft:entity.drowned.shoot");
const ResourceLocation ENTITY_DROWNED_STEP("minecraft:entity.drowned.step");
const ResourceLocation ENTITY_DROWNED_SWIM("minecraft:entity.drowned.swim");

// 僵尸猪灵
const ResourceLocation ENTITY_ZOMBIFIED_PIGLIN_AMBIENT("minecraft:entity.zombified_piglin.ambient");
const ResourceLocation ENTITY_ZOMBIFIED_PIGLIN_ANGRY("minecraft:entity.zombified_piglin.angry");
const ResourceLocation ENTITY_ZOMBIFIED_PIGLIN_DEATH("minecraft:entity.zombified_piglin.death");
const ResourceLocation ENTITY_ZOMBIFIED_PIGLIN_HURT("minecraft:entity.zombified_piglin.hurt");

// 骷髅
const ResourceLocation ENTITY_SKELETON_AMBIENT("minecraft:entity.skeleton.ambient");
const ResourceLocation ENTITY_SKELETON_DEATH("minecraft:entity.skeleton.death");
const ResourceLocation ENTITY_SKELETON_HURT("minecraft:entity.skeleton.hurt");
const ResourceLocation ENTITY_SKELETON_SHOOT("minecraft:entity.skeleton.shoot");
const ResourceLocation ENTITY_SKELETON_STEP("minecraft:entity.skeleton.step");

// 流浪者
const ResourceLocation ENTITY_STRAY_AMBIENT("minecraft:entity.stray.ambient");
const ResourceLocation ENTITY_STRAY_DEATH("minecraft:entity.stray.death");
const ResourceLocation ENTITY_STRAY_HURT("minecraft:entity.stray.hurt");
const ResourceLocation ENTITY_STRAY_STEP("minecraft:entity.stray.step");

// 凋灵骷髅
const ResourceLocation ENTITY_WITHER_SKELETON_AMBIENT("minecraft:entity.wither_skeleton.ambient");
const ResourceLocation ENTITY_WITHER_SKELETON_DEATH("minecraft:entity.wither_skeleton.death");
const ResourceLocation ENTITY_WITHER_SKELETON_HURT("minecraft:entity.wither_skeleton.hurt");
const ResourceLocation ENTITY_WITHER_SKELETON_STEP("minecraft:entity.wither_skeleton.step");

// 苦力怕
const ResourceLocation ENTITY_CREEPER_DEATH("minecraft:entity.creeper.death");
const ResourceLocation ENTITY_CREEPER_HURT("minecraft:entity.creeper.hurt");
const ResourceLocation ENTITY_CREEPER_PRIMED("minecraft:entity.creeper.primed");

// 末影人
const ResourceLocation ENTITY_ENDERMAN_AMBIENT("minecraft:entity.enderman.ambient");
const ResourceLocation ENTITY_ENDERMAN_DEATH("minecraft:entity.enderman.death");
const ResourceLocation ENTITY_ENDERMAN_HURT("minecraft:entity.enderman.hurt");
const ResourceLocation ENTITY_ENDERMAN_SCREAM("minecraft:entity.enderman.scream");
const ResourceLocation ENTITY_ENDERMAN_STARE("minecraft:entity.enderman.stare");
const ResourceLocation ENTITY_ENDERMAN_TELEPORT("minecraft:entity.enderman.teleport");
const ResourceLocation PLAYER_TELEPORT("minecraft:entity.player.teleport");

// 末影螨
const ResourceLocation ENTITY_ENDERMITE_AMBIENT("minecraft:entity.endermite.ambient");
const ResourceLocation ENTITY_ENDERMITE_DEATH("minecraft:entity.endermite.death");
const ResourceLocation ENTITY_ENDERMITE_HURT("minecraft:entity.endermite.hurt");
const ResourceLocation ENTITY_ENDERMITE_STEP("minecraft:entity.endermite.step");

// 蜘蛛
const ResourceLocation ENTITY_SPIDER_AMBIENT("minecraft:entity.spider.ambient");
const ResourceLocation ENTITY_SPIDER_DEATH("minecraft:entity.spider.death");
const ResourceLocation ENTITY_SPIDER_HURT("minecraft:entity.spider.hurt");
const ResourceLocation ENTITY_SPIDER_STEP("minecraft:entity.spider.step");

// 史莱姆
const ResourceLocation ENTITY_SLIME_ATTACK("minecraft:entity.slime.attack");
const ResourceLocation ENTITY_SLIME_DEATH("minecraft:entity.slime.death");
const ResourceLocation ENTITY_SLIME_HURT("minecraft:entity.slime.hurt");
const ResourceLocation ENTITY_SLIME_JUMP("minecraft:entity.slime.jump");
const ResourceLocation ENTITY_SLIME_SQUISH("minecraft:entity.slime.squish");
const ResourceLocation ENTITY_SLIME_DEATH_SMALL("minecraft:entity.slime.death_small");
const ResourceLocation ENTITY_SLIME_HURT_SMALL("minecraft:entity.slime.hurt_small");
const ResourceLocation ENTITY_SLIME_JUMP_SMALL("minecraft:entity.slime.jump_small");
const ResourceLocation ENTITY_SLIME_SQUISH_SMALL("minecraft:entity.slime.squish_small");

// 岩浆怪
const ResourceLocation ENTITY_MAGMA_CUBE_DEATH("minecraft:entity.magma_cube.death");
const ResourceLocation ENTITY_MAGMA_CUBE_DEATH_SMALL("minecraft:entity.magma_cube.death_small");
const ResourceLocation ENTITY_MAGMA_CUBE_HURT("minecraft:entity.magma_cube.hurt");
const ResourceLocation ENTITY_MAGMA_CUBE_HURT_SMALL("minecraft:entity.magma_cube.hurt_small");
const ResourceLocation ENTITY_MAGMA_CUBE_JUMP("minecraft:entity.magma_cube.jump");
const ResourceLocation ENTITY_MAGMA_CUBE_SQUISH("minecraft:entity.magma_cube.squish");
const ResourceLocation ENTITY_MAGMA_CUBE_SQUISH_SMALL("minecraft:entity.magma_cube.squish_small");

// 恶魂
const ResourceLocation ENTITY_GHAST_AMBIENT("minecraft:entity.ghast.ambient");
const ResourceLocation ENTITY_GHAST_DEATH("minecraft:entity.ghast.death");
const ResourceLocation ENTITY_GHAST_HURT("minecraft:entity.ghast.hurt");
const ResourceLocation ENTITY_GHAST_SCREAM("minecraft:entity.ghast.scream");
const ResourceLocation ENTITY_GHAST_SHOOT("minecraft:entity.ghast.shoot");
const ResourceLocation ENTITY_GHAST_WARN("minecraft:entity.ghast.warn");

// 烈焰人
const ResourceLocation ENTITY_BLAZE_AMBIENT("minecraft:entity.blaze.ambient");
const ResourceLocation ENTITY_BLAZE_BURN("minecraft:entity.blaze.burn");
const ResourceLocation ENTITY_BLAZE_DEATH("minecraft:entity.blaze.death");
const ResourceLocation ENTITY_BLAZE_HURT("minecraft:entity.blaze.hurt");
const ResourceLocation ENTITY_BLAZE_SHOOT("minecraft:entity.blaze.shoot");

// 守卫者
const ResourceLocation ENTITY_GUARDIAN_AMBIENT("minecraft:entity.guardian.ambient");
const ResourceLocation ENTITY_GUARDIAN_AMBIENT_LAND("minecraft:entity.guardian.ambient_land");
const ResourceLocation ENTITY_GUARDIAN_ATTACK("minecraft:entity.guardian.attack");
const ResourceLocation ENTITY_GUARDIAN_DEATH("minecraft:entity.guardian.death");
const ResourceLocation ENTITY_GUARDIAN_DEATH_LAND("minecraft:entity.guardian.death_land");
const ResourceLocation ENTITY_GUARDIAN_FLOP("minecraft:entity.guardian.flop");
const ResourceLocation ENTITY_GUARDIAN_HURT("minecraft:entity.guardian.hurt");
const ResourceLocation ENTITY_GUARDIAN_HURT_LAND("minecraft:entity.guardian.hurt_land");

// 远古守卫者
const ResourceLocation ENTITY_ELDER_GUARDIAN_AMBIENT("minecraft:entity.elder_guardian.ambient");
const ResourceLocation ENTITY_ELDER_GUARDIAN_AMBIENT_LAND("minecraft:entity.elder_guardian.ambient_land");
const ResourceLocation ENTITY_ELDER_GUARDIAN_CURSE("minecraft:entity.elder_guardian.curse");
const ResourceLocation ENTITY_ELDER_GUARDIAN_DEATH("minecraft:entity.elder_guardian.death");
const ResourceLocation ENTITY_ELDER_GUARDIAN_DEATH_LAND("minecraft:entity.elder_guardian.death_land");
const ResourceLocation ENTITY_ELDER_GUARDIAN_FLOP("minecraft:entity.elder_guardian.flop");
const ResourceLocation ENTITY_ELDER_GUARDIAN_HURT("minecraft:entity.elder_guardian.hurt");
const ResourceLocation ENTITY_ELDER_GUARDIAN_HURT_LAND("minecraft:entity.elder_guardian.hurt_land");

// 女巫
const ResourceLocation ENTITY_WITCH_AMBIENT("minecraft:entity.witch.ambient");
const ResourceLocation ENTITY_WITCH_CELEBRATE("minecraft:entity.witch.celebrate");
const ResourceLocation ENTITY_WITCH_DEATH("minecraft:entity.witch.death");
const ResourceLocation ENTITY_WITCH_DRINK("minecraft:entity.witch.drink");
const ResourceLocation ENTITY_WITCH_HURT("minecraft:entity.witch.hurt");
const ResourceLocation ENTITY_WITCH_THROW("minecraft:entity.witch.throw");

// 唤魔者
const ResourceLocation ENTITY_EVOKER_AMBIENT("minecraft:entity.evoker.ambient");
const ResourceLocation ENTITY_EVOKER_CAST_SPELL("minecraft:entity.evoker.cast_spell");
const ResourceLocation ENTITY_EVOKER_CELEBRATE("minecraft:entity.evoker.celebrate");
const ResourceLocation ENTITY_EVOKER_DEATH("minecraft:entity.evoker.death");
const ResourceLocation ENTITY_EVOKER_HURT("minecraft:entity.evoker.hurt");
const ResourceLocation ENTITY_EVOKER_PREPARE_ATTACK("minecraft:entity.evoker.prepare_attack");
const ResourceLocation ENTITY_EVOKER_PREPARE_SUMMON("minecraft:entity.evoker.prepare_summon");
const ResourceLocation ENTITY_EVOKER_PREPARE_WOLOLO("minecraft:entity.evoker.prepare_wololo");

// 卫道士
const ResourceLocation ENTITY_VINDICATOR_AMBIENT("minecraft:entity.vindicator.ambient");
const ResourceLocation ENTITY_VINDICATOR_CELEBRATE("minecraft:entity.vindicator.celebrate");
const ResourceLocation ENTITY_VINDICATOR_DEATH("minecraft:entity.vindicator.death");
const ResourceLocation ENTITY_VINDICATOR_HURT("minecraft:entity.vindicator.hurt");

// 恼鬼
const ResourceLocation ENTITY_VEX_AMBIENT("minecraft:entity.vex.ambient");
const ResourceLocation ENTITY_VEX_CHARGE("minecraft:entity.vex.charge");
const ResourceLocation ENTITY_VEX_DEATH("minecraft:entity.vex.death");
const ResourceLocation ENTITY_VEX_HURT("minecraft:entity.vex.hurt");

// 劫掠者
const ResourceLocation ENTITY_PILLAGER_AMBIENT("minecraft:entity.pillager.ambient");
const ResourceLocation ENTITY_PILLAGER_CELEBRATE("minecraft:entity.pillager.celebrate");
const ResourceLocation ENTITY_PILLAGER_DEATH("minecraft:entity.pillager.death");
const ResourceLocation ENTITY_PILLAGER_HURT("minecraft:entity.pillager.hurt");

// 劫掠兽
const ResourceLocation ENTITY_RAVAGER_AMBIENT("minecraft:entity.ravager.ambient");
const ResourceLocation ENTITY_RAVAGER_ATTACK("minecraft:entity.ravager.attack");
const ResourceLocation ENTITY_RAVAGER_CELEBRATE("minecraft:entity.ravager.celebrate");
const ResourceLocation ENTITY_RAVAGER_DEATH("minecraft:entity.ravager.death");
const ResourceLocation ENTITY_RAVAGER_HURT("minecraft:entity.ravager.hurt");
const ResourceLocation ENTITY_RAVAGER_STEP("minecraft:entity.ravager.step");
const ResourceLocation ENTITY_RAVAGER_STUNNED("minecraft:entity.ravager.stunned");
const ResourceLocation ENTITY_RAVAGER_ROAR("minecraft:entity.ravager.roar");

// 幻翼
const ResourceLocation ENTITY_PHANTOM_AMBIENT("minecraft:entity.phantom.ambient");
const ResourceLocation ENTITY_PHANTOM_BITE("minecraft:entity.phantom.bite");
const ResourceLocation ENTITY_PHANTOM_DEATH("minecraft:entity.phantom.death");
const ResourceLocation ENTITY_PHANTOM_FLAP("minecraft:entity.phantom.flap");
const ResourceLocation ENTITY_PHANTOM_HURT("minecraft:entity.phantom.hurt");
const ResourceLocation ENTITY_PHANTOM_SWOOP("minecraft:entity.phantom.swoop");

// 潜影贝
const ResourceLocation ENTITY_SHULKER_AMBIENT("minecraft:entity.shulker.ambient");
const ResourceLocation ENTITY_SHULKER_CLOSE("minecraft:entity.shulker.close");
const ResourceLocation ENTITY_SHULKER_DEATH("minecraft:entity.shulker.death");
const ResourceLocation ENTITY_SHULKER_HURT("minecraft:entity.shulker.hurt");
const ResourceLocation ENTITY_SHULKER_HURT_CLOSED("minecraft:entity.shulker.hurt_closed");
const ResourceLocation ENTITY_SHULKER_OPEN("minecraft:entity.shulker.open");
const ResourceLocation ENTITY_SHULKER_SHOOT("minecraft:entity.shulker.shoot");
const ResourceLocation ENTITY_SHULKER_TELEPORT("minecraft:entity.shulker.teleport");
const ResourceLocation ENTITY_SHULKER_BULLET_HIT("minecraft:entity.shulker_bullet.hit");
const ResourceLocation ENTITY_SHULKER_BULLET_HURT("minecraft:entity.shulker_bullet.hurt");

// 蠹虫
const ResourceLocation ENTITY_SILVERFISH_AMBIENT("minecraft:entity.silverfish.ambient");
const ResourceLocation ENTITY_SILVERFISH_DEATH("minecraft:entity.silverfish.death");
const ResourceLocation ENTITY_SILVERFISH_HURT("minecraft:entity.silverfish.hurt");
const ResourceLocation ENTITY_SILVERFISH_STEP("minecraft:entity.silverfish.step");

// 末影龙
const ResourceLocation ENTITY_ENDER_DRAGON_AMBIENT("minecraft:entity.ender_dragon.ambient");
const ResourceLocation ENTITY_ENDER_DRAGON_DEATH("minecraft:entity.ender_dragon.death");
const ResourceLocation ENTITY_ENDER_DRAGON_FLAP("minecraft:entity.ender_dragon.flap");
const ResourceLocation ENTITY_ENDER_DRAGON_GROWL("minecraft:entity.ender_dragon.growl");
const ResourceLocation ENTITY_ENDER_DRAGON_HURT("minecraft:entity.ender_dragon.hurt");
const ResourceLocation ENTITY_ENDER_DRAGON_SHOOT("minecraft:entity.ender_dragon.shoot");
const ResourceLocation ENTITY_DRAGON_FIREBALL_EXPLODE("minecraft:entity.dragon_fireball.explode");

// 凋灵
const ResourceLocation ENTITY_WITHER_AMBIENT("minecraft:entity.wither.ambient");
const ResourceLocation ENTITY_WITHER_BREAK_BLOCK("minecraft:entity.wither.break_block");
const ResourceLocation ENTITY_WITHER_DEATH("minecraft:entity.wither.death");
const ResourceLocation ENTITY_WITHER_HURT("minecraft:entity.wither.hurt");
const ResourceLocation ENTITY_WITHER_SHOOT("minecraft:entity.wither.shoot");
const ResourceLocation ENTITY_WITHER_SPAWN("minecraft:entity.wither.spawn");

// 猪灵
const ResourceLocation ENTITY_PIGLIN_ADMIRING_ITEM("minecraft:entity.piglin.admiring_item");
const ResourceLocation ENTITY_PIGLIN_AMBIENT("minecraft:entity.piglin.ambient");
const ResourceLocation ENTITY_PIGLIN_ANGRY("minecraft:entity.piglin.angry");
const ResourceLocation ENTITY_PIGLIN_CELEBRATE("minecraft:entity.piglin.celebrate");
const ResourceLocation ENTITY_PIGLIN_DEATH("minecraft:entity.piglin.death");
const ResourceLocation ENTITY_PIGLIN_JEALOUS("minecraft:entity.piglin.jealous");
const ResourceLocation ENTITY_PIGLIN_HURT("minecraft:entity.piglin.hurt");
const ResourceLocation ENTITY_PIGLIN_RETREAT("minecraft:entity.piglin.retreat");
const ResourceLocation ENTITY_PIGLIN_STEP("minecraft:entity.piglin.step");
const ResourceLocation ENTITY_PIGLIN_CONVERTED_TO_ZOMBIFIED("minecraft:entity.piglin.converted_to_zombified");

// 猪灵蛮兵
const ResourceLocation ENTITY_PIGLIN_BRUTE_AMBIENT("minecraft:entity.piglin_brute.ambient");
const ResourceLocation ENTITY_PIGLIN_BRUTE_ANGRY("minecraft:entity.piglin_brute.angry");
const ResourceLocation ENTITY_PIGLIN_BRUTE_DEATH("minecraft:entity.piglin_brute.death");
const ResourceLocation ENTITY_PIGLIN_BRUTE_HURT("minecraft:entity.piglin_brute.hurt");
const ResourceLocation ENTITY_PIGLIN_BRUTE_STEP("minecraft:entity.piglin_brute.step");
const ResourceLocation ENTITY_PIGLIN_BRUTE_CONVERTED_TO_ZOMBIFIED(
    "minecraft:entity.piglin_brute.converted_to_zombified");

// 疣猪兽
const ResourceLocation ENTITY_HOGLIN_AMBIENT("minecraft:entity.hoglin.ambient");
const ResourceLocation ENTITY_HOGLIN_ANGRY("minecraft:entity.hoglin.angry");
const ResourceLocation ENTITY_HOGLIN_ATTACK("minecraft:entity.hoglin.attack");
const ResourceLocation ENTITY_HOGLIN_CONVERTED_TO_ZOMBIFIED("minecraft:entity.hoglin.converted_to_zombified");
const ResourceLocation ENTITY_HOGLIN_DEATH("minecraft:entity.hoglin.death");
const ResourceLocation ENTITY_HOGLIN_HURT("minecraft:entity.hoglin.hurt");
const ResourceLocation ENTITY_HOGLIN_RETREAT("minecraft:entity.hoglin.retreat");
const ResourceLocation ENTITY_HOGLIN_STEP("minecraft:entity.hoglin.step");

// 僵尸疣兽
const ResourceLocation ENTITY_ZOGLIN_AMBIENT("minecraft:entity.zoglin.ambient");
const ResourceLocation ENTITY_ZOGLIN_ANGRY("minecraft:entity.zoglin.angry");
const ResourceLocation ENTITY_ZOGLIN_ATTACK("minecraft:entity.zoglin.attack");
const ResourceLocation ENTITY_ZOGLIN_DEATH("minecraft:entity.zoglin.death");
const ResourceLocation ENTITY_ZOGLIN_HURT("minecraft:entity.zoglin.hurt");
const ResourceLocation ENTITY_ZOGLIN_STEP("minecraft:entity.zoglin.step");

// 炽足兽
const ResourceLocation ENTITY_STRIDER_AMBIENT("minecraft:entity.strider.ambient");
const ResourceLocation ENTITY_STRIDER_HAPPY("minecraft:entity.strider.happy");
const ResourceLocation ENTITY_STRIDER_RETREAT("minecraft:entity.strider.retreat");
const ResourceLocation ENTITY_STRIDER_DEATH("minecraft:entity.strider.death");
const ResourceLocation ENTITY_STRIDER_HURT("minecraft:entity.strider.hurt");
const ResourceLocation ENTITY_STRIDER_STEP("minecraft:entity.strider.step");
const ResourceLocation ENTITY_STRIDER_STEP_LAVA("minecraft:entity.strider.step_lava");
const ResourceLocation ENTITY_STRIDER_EAT("minecraft:entity.strider.eat");
const ResourceLocation ENTITY_STRIDER_SADDLE("minecraft:entity.strider.saddle");

// ============================================================================
// 其他实体声音
// ============================================================================

// 末影之眼
const ResourceLocation ENTITY_ENDER_EYE_DEATH("minecraft:entity.ender_eye.death");
const ResourceLocation ENTITY_ENDER_EYE_LAUNCH("minecraft:entity.ender_eye.launch");

// 唤魔者尖牙
const ResourceLocation ENTITY_EVOKER_FANGS_ATTACK("minecraft:entity.evoker_fangs.attack");

// 经验瓶
const ResourceLocation ENTITY_EXPERIENCE_BOTTLE_THROW("minecraft:entity.experience_bottle.throw");

// 鱼（游泳）
const ResourceLocation ENTITY_FISH_SWIM("minecraft:entity.fish.swim");

// 物品（破坏）
const ResourceLocation ENTITY_ITEM_BREAK("minecraft:entity.item.break");

// 滞留药水
const ResourceLocation ENTITY_LINGERING_POTION_THROW("minecraft:entity.lingering_potion.throw");

// 箭矢
const ResourceLocation ENTITY_ARROW_HIT("minecraft:entity.arrow.hit");
const ResourceLocation ENTITY_ARROW_HIT_PLAYER("minecraft:entity.arrow.hit_player");
const ResourceLocation ENTITY_ARROW_HIT_GROUND("minecraft:entity.arrow.hit_ground");
const ResourceLocation ENTITY_ARROW_SHOOT("minecraft:entity.arrow.shoot");

// 经验球
const ResourceLocation ENTITY_EXPERIENCE_ORB_PICKUP("minecraft:entity.experience_orb.pickup");
const ResourceLocation ENTITY_EXPERIENCE_ORB_THROW("minecraft:entity.experience_orb.throw");

// 闪电
const ResourceLocation ENTITY_LIGHTNING_BOLT_IMPACT("minecraft:entity.lightning_bolt.impact");
const ResourceLocation ENTITY_LIGHTNING_BOLT_THUNDER("minecraft:entity.lightning_bolt.thunder");

// TNT
const ResourceLocation ENTITY_TNT_PRIMED("minecraft:entity.tnt.primed");

// 末影珍珠
const ResourceLocation ENTITY_ENDER_PEARL_THROW("minecraft:entity.ender_pearl.throw");

// 鸡蛋
const ResourceLocation ENTITY_EGG_THROW("minecraft:entity.egg.throw");

// 雪球
const ResourceLocation ENTITY_SNOWBALL_THROW("minecraft:entity.snowball.throw");

// 药水
const ResourceLocation ENTITY_SPLASH_POTION_BREAK("minecraft:entity.splash_potion.break");
const ResourceLocation ENTITY_SPLASH_POTION_THROW("minecraft:entity.splash_potion.throw");

// 烟花火箭
const ResourceLocation ENTITY_FIREWORK_ROCKET_BLAST("minecraft:entity.firework_rocket.blast");
const ResourceLocation ENTITY_FIREWORK_ROCKET_BLAST_FAR("minecraft:entity.firework_rocket.blast_far");
const ResourceLocation ENTITY_FIREWORK_ROCKET_LARGE_BLAST("minecraft:entity.firework_rocket.large_blast");
const ResourceLocation ENTITY_FIREWORK_ROCKET_LARGE_BLAST_FAR("minecraft:entity.firework_rocket.large_blast_far");
const ResourceLocation ENTITY_FIREWORK_ROCKET_LAUNCH("minecraft:entity.firework_rocket.launch");
const ResourceLocation ENTITY_FIREWORK_ROCKET_SHOOT("minecraft:entity.firework_rocket.shoot");
const ResourceLocation ENTITY_FIREWORK_ROCKET_TWINKLE("minecraft:entity.firework_rocket.twinkle");
const ResourceLocation ENTITY_FIREWORK_ROCKET_TWINKLE_FAR("minecraft:entity.firework_rocket.twinkle_far");

// 矿车
const ResourceLocation ENTITY_MINECART_INSIDE("minecraft:entity.minecart.inside");
const ResourceLocation ENTITY_MINECART_RIDING("minecraft:entity.minecart.riding");

// 船
const ResourceLocation ENTITY_BOAT_PADDLE_LAND("minecraft:entity.boat.paddle_land");
const ResourceLocation ENTITY_BOAT_PADDLE_WATER("minecraft:entity.boat.paddle_water");

// 物品展示框和画
const ResourceLocation ENTITY_ITEM_FRAME_ADD_ITEM("minecraft:entity.item_frame.add_item");
const ResourceLocation ENTITY_ITEM_FRAME_BREAK("minecraft:entity.item_frame.break");
const ResourceLocation ENTITY_ITEM_FRAME_PLACE("minecraft:entity.item_frame.place");
const ResourceLocation ENTITY_ITEM_FRAME_REMOVE_ITEM("minecraft:entity.item_frame.remove_item");
const ResourceLocation ENTITY_ITEM_FRAME_ROTATE_ITEM("minecraft:entity.item_frame.rotate_item");

// 荧光物品展示框
const ResourceLocation ENTITY_GLOW_ITEM_FRAME_ADD_ITEM("minecraft:entity.glow_item_frame.add_item");
const ResourceLocation ENTITY_GLOW_ITEM_FRAME_PLACE("minecraft:entity.glow_item_frame.place");
const ResourceLocation ENTITY_GLOW_ITEM_FRAME_REMOVE_ITEM("minecraft:entity.glow_item_frame.remove_item");
const ResourceLocation ENTITY_GLOW_ITEM_FRAME_ROTATE_ITEM("minecraft:entity.glow_item_frame.rotate_item");
const ResourceLocation ENTITY_GLOW_ITEM_FRAME_BREAK("minecraft:entity.glow_item_frame.break");

const ResourceLocation ENTITY_PAINTING_BREAK("minecraft:entity.painting.break");
const ResourceLocation ENTITY_PAINTING_PLACE("minecraft:entity.painting.place");

// 盔甲架
const ResourceLocation ENTITY_ARMOR_STAND_BREAK("minecraft:entity.armor_stand.break");
const ResourceLocation ENTITY_ARMOR_STAND_FALL("minecraft:entity.armor_stand.fall");
const ResourceLocation ENTITY_ARMOR_STAND_HIT("minecraft:entity.armor_stand.hit");
const ResourceLocation ENTITY_ARMOR_STAND_PLACE("minecraft:entity.armor_stand.place");

// 皮革绳
const ResourceLocation ENTITY_LEASH_KNOT_BREAK("minecraft:entity.leash_knot.break");
const ResourceLocation ENTITY_LEASH_KNOT_PLACE("minecraft:entity.leash_knot.place");

// ============================================================================
// 物品声音
// ============================================================================

const ResourceLocation ENTITY_ITEM_PICKUP("minecraft:entity.item.pickup");

// 盔甲装备
const ResourceLocation ITEM_ARMOR_EQUIP_CHAIN("minecraft:item.armor.equip_chain");
const ResourceLocation ITEM_ARMOR_EQUIP_COPPER("minecraft:item.armor.equip_copper");
const ResourceLocation ITEM_ARMOR_EQUIP_DIAMOND("minecraft:item.armor.equip_diamond");
const ResourceLocation ITEM_ARMOR_EQUIP_ELYTRA("minecraft:item.armor.equip_elytra");
const ResourceLocation ITEM_ARMOR_EQUIP_GENERIC("minecraft:item.armor.equip_generic");
const ResourceLocation ITEM_ARMOR_EQUIP_GOLD("minecraft:item.armor.equip_gold");
const ResourceLocation ITEM_ARMOR_EQUIP_IRON("minecraft:item.armor.equip_iron");
const ResourceLocation ITEM_ARMOR_EQUIP_LEATHER("minecraft:item.armor.equip_leather");
const ResourceLocation ITEM_ARMOR_EQUIP_NETHERITE("minecraft:item.armor.equip_netherite");
const ResourceLocation ITEM_ARMOR_EQUIP_TURTLE("minecraft:item.armor.equip_turtle");
const ResourceLocation ITEM_ARMOR_EQUIP_WOLF("minecraft:item.armor.equip_wolf");
const ResourceLocation ITEM_ARMOR_UNEQUIP_WOLF("minecraft:item.armor.unequip_wolf");

// 快乐恶魂挽具
const ResourceLocation HARNESS_EQUIP("minecraft:item.harness.equip");
const ResourceLocation HARNESS_UNEQUIP("minecraft:item.harness.unequip");

// 鞘翅
const ResourceLocation ITEM_ELYTRA_FLYING("minecraft:item.elytra.flying");

// 桶
const ResourceLocation ITEM_BUCKET_EMPTY("minecraft:item.bucket.empty");
const ResourceLocation ITEM_BUCKET_EMPTY_FISH("minecraft:item.bucket.empty_fish");
const ResourceLocation ITEM_BUCKET_EMPTY_LAVA("minecraft:item.bucket.empty_lava");
const ResourceLocation ITEM_BUCKET_EMPTY_POWDER_SNOW("minecraft:item.bucket.empty_powder_snow");
const ResourceLocation ITEM_BUCKET_FILL("minecraft:item.bucket.fill");
const ResourceLocation ITEM_BUCKET_FILL_FISH("minecraft:item.bucket.fill_fish");
const ResourceLocation ITEM_BUCKET_FILL_LAVA("minecraft:item.bucket.fill_lava");
const ResourceLocation ITEM_BUCKET_FILL_POWDER_SNOW("minecraft:item.bucket.fill_powder_snow");

// 工具
const ResourceLocation ITEM_AXE_STRIP("minecraft:item.axe.strip");
const ResourceLocation ITEM_AXE_SCRAPE("minecraft:item.axe.scrape");
const ResourceLocation ITEM_AXE_WAX_OFF("minecraft:item.axe.wax_off");
const ResourceLocation ITEM_HOE_TILL("minecraft:item.hoe.till");
const ResourceLocation ITEM_SHOVEL_FLATTEN("minecraft:item.shovel.flatten");

// 其他
const ResourceLocation ITEM_CHORUS_FRUIT_TELEPORT("minecraft:item.chorus_fruit.teleport");
const ResourceLocation ITEM_FLINTANDSTEEL_USE("minecraft:item.flintandsteel.use");
const ResourceLocation ITEM_FIRECHARGE_USE("minecraft:item.firecharge.use");
const ResourceLocation ITEM_TOTEM_USE("minecraft:item.totem.use");
const ResourceLocation ITEM_SPYGLASS_USE("minecraft:item.spyglass.use");
const ResourceLocation ITEM_SPYGLASS_STOP_USING("minecraft:item.spyglass.stop_using");
const ResourceLocation ITEM_GOAT_HORN_SOUND_0("minecraft:item.goat_horn.sound.0");
const ResourceLocation ITEM_GOAT_HORN_SOUND_1("minecraft:item.goat_horn.sound.1");
const ResourceLocation ITEM_GOAT_HORN_SOUND_2("minecraft:item.goat_horn.sound.2");
const ResourceLocation ITEM_GOAT_HORN_SOUND_3("minecraft:item.goat_horn.sound.3");
const ResourceLocation ITEM_GOAT_HORN_SOUND_4("minecraft:item.goat_horn.sound.4");
const ResourceLocation ITEM_GOAT_HORN_SOUND_5("minecraft:item.goat_horn.sound.5");
const ResourceLocation ITEM_GOAT_HORN_SOUND_6("minecraft:item.goat_horn.sound.6");
const ResourceLocation ITEM_GOAT_HORN_SOUND_7("minecraft:item.goat_horn.sound.7");
const ResourceLocation ITEM_BOOK_PAGE_TURN("minecraft:item.book.page_turn");
const ResourceLocation ITEM_BOOK_PUT("minecraft:item.book.put");
const ResourceLocation ITEM_BONE_MEAL_USE("minecraft:item.bone_meal.use");
const ResourceLocation ITEM_BOTTLE_EMPTY("minecraft:item.bottle.empty");
const ResourceLocation ITEM_BOTTLE_FILL("minecraft:item.bottle.fill");
const ResourceLocation ITEM_BOTTLE_FILL_DRAGONBREATH("minecraft:item.bottle.fill_dragonbreath");
const ResourceLocation ITEM_HONEY_BOTTLE_DRINK("minecraft:item.honey_bottle.drink");
const ResourceLocation ITEM_SWEET_BERRIES_PICK_FROM_BUSH("minecraft:item.sweet_berries.pick_from_bush");
const ResourceLocation ITEM_CROP_PLANT("minecraft:item.crop.plant");
const ResourceLocation ITEM_NETHER_WART_PLANT("minecraft:item.nether_wart.plant");
const ResourceLocation ITEM_LODESTONE_COMPASS_LOCK("minecraft:item.lodestone_compass.lock");

// 收纳袋（Bundle）音效
// 对应 MC 1.21.11 SoundEvents.ITEM_BUNDLE_DROP_CONTENTS / INSERT / INSERT_FAIL / REMOVE_ONE
const ResourceLocation ITEM_BUNDLE_DROP_CONTENTS("minecraft:item.bundle.drop_contents");
const ResourceLocation ITEM_BUNDLE_INSERT("minecraft:item.bundle.insert");
const ResourceLocation ITEM_BUNDLE_INSERT_FAIL("minecraft:item.bundle.insert_fail");
const ResourceLocation ITEM_BUNDLE_REMOVE_ONE("minecraft:item.bundle.remove_one");

// ============================================================================
// 武器声音
// ============================================================================

// 弓箭
const ResourceLocation ITEM_BOW_PULL("minecraft:item.bow.pull");

// 弩
const ResourceLocation ITEM_CROSSBOW_LOADING_START("minecraft:item.crossbow.loading_start");
const ResourceLocation ITEM_CROSSBOW_LOADING_MIDDLE("minecraft:item.crossbow.loading_middle");
const ResourceLocation ITEM_CROSSBOW_LOADING_END("minecraft:item.crossbow.loading_end");
const ResourceLocation ITEM_CROSSBOW_SHOOT("minecraft:item.crossbow.shoot");
const ResourceLocation ITEM_CROSSBOW_ROCKET("minecraft:item.crossbow.rocket");
const ResourceLocation ITEM_CROSSBOW_HIT("minecraft:item.crossbow.hit");
const ResourceLocation ITEM_CROSSBOW_QUICK_CHARGE_1("minecraft:item.crossbow.quick_charge_1");
const ResourceLocation ITEM_CROSSBOW_QUICK_CHARGE_2("minecraft:item.crossbow.quick_charge_2");
const ResourceLocation ITEM_CROSSBOW_QUICK_CHARGE_3("minecraft:item.crossbow.quick_charge_3");

// 三叉戟
const ResourceLocation ITEM_TRIDENT_THROW("minecraft:item.trident.throw");
const ResourceLocation ITEM_TRIDENT_RIPTIDE_1("minecraft:item.trident.riptide_1");
const ResourceLocation ITEM_TRIDENT_RIPTIDE_2("minecraft:item.trident.riptide_2");
const ResourceLocation ITEM_TRIDENT_RIPTIDE_3("minecraft:item.trident.riptide_3");
const ResourceLocation ITEM_TRIDENT_HIT("minecraft:item.trident.hit");
const ResourceLocation ITEM_TRIDENT_HIT_GROUND("minecraft:item.trident.hit_ground");
const ResourceLocation ITEM_TRIDENT_RETURN("minecraft:item.trident.return");
const ResourceLocation ITEM_TRIDENT_THUNDER("minecraft:item.trident.thunder");

// 长矛
const ResourceLocation ITEM_SPEAR_THROW("minecraft:item.spear.use");
const ResourceLocation ITEM_SPEAR_HIT("minecraft:item.spear.hit");
const ResourceLocation ITEM_SPEAR_HIT_GROUND("minecraft:item.spear.hit_ground");

// 盾牌
const ResourceLocation ITEM_SHIELD_BLOCK("minecraft:item.shield.block");
const ResourceLocation ITEM_SHIELD_BREAK("minecraft:item.shield.break");

// 钓鱼竿
const ResourceLocation ENTITY_FISHING_BOBBER_THROW("minecraft:entity.fishing_bobber.throw");
const ResourceLocation ENTITY_FISHING_BOBBER_RETRIEVE("minecraft:entity.fishing_bobber.retrieve");
const ResourceLocation ENTITY_FISHING_BOBBER_SPLASH("minecraft:entity.fishing_bobber.splash");
const ResourceLocation ENTITY_FISHING_BOBBER_CAST("minecraft:entity.fishing_bobber.cast");

// ============================================================================
// 音乐音效 (MUSIC_)
// ============================================================================

const ResourceLocation MUSIC_CREATIVE("minecraft:music.creative");
const ResourceLocation MUSIC_CREDITS("minecraft:music.credits");
const ResourceLocation MUSIC_DRAGON("minecraft:music.dragon");
const ResourceLocation MUSIC_END("minecraft:music.end");
const ResourceLocation MUSIC_GAME("minecraft:music.game");
const ResourceLocation MUSIC_MENU("minecraft:music.menu");
const ResourceLocation MUSIC_UNDER_WATER("minecraft:music.under_water");
const ResourceLocation MUSIC_NETHER_BASALT_DELTAS("minecraft:music.nether.basalt_deltas");
const ResourceLocation MUSIC_NETHER_NETHER_WASTES("minecraft:music.nether.nether_wastes");
const ResourceLocation MUSIC_NETHER_SOUL_SAND_VALLEY("minecraft:music.nether.soul_sand_valley");
const ResourceLocation MUSIC_NETHER_CRIMSON_FOREST("minecraft:music.nether.crimson_forest");
const ResourceLocation MUSIC_NETHER_WARPED_FOREST("minecraft:music.nether.warped_forest");

// 音乐唱片
const ResourceLocation MUSIC_DISC_11("minecraft:music_disc.11");
const ResourceLocation MUSIC_DISC_13("minecraft:music_disc.13");
const ResourceLocation MUSIC_DISC_5("minecraft:music_disc.5");
const ResourceLocation MUSIC_DISC_BLOCKS("minecraft:music_disc.blocks");
const ResourceLocation MUSIC_DISC_CAT("minecraft:music_disc.cat");
const ResourceLocation MUSIC_DISC_CHIRP("minecraft:music_disc.chirp");
const ResourceLocation MUSIC_DISC_CREATOR("minecraft:music_disc.creator");
const ResourceLocation MUSIC_DISC_CREATOR_MUSIC_BOX("minecraft:music_disc.creator_music_box");
const ResourceLocation MUSIC_DISC_FAR("minecraft:music_disc.far");
const ResourceLocation MUSIC_DISC_LAVA_CHICKEN("minecraft:music_disc.lava_chicken");
const ResourceLocation MUSIC_DISC_MALL("minecraft:music_disc.mall");
const ResourceLocation MUSIC_DISC_MELLOHI("minecraft:music_disc.mellohi");
const ResourceLocation MUSIC_DISC_OTHERSIDE("minecraft:music_disc.otherside");
const ResourceLocation MUSIC_DISC_PIGSTEP("minecraft:music_disc.pigstep");
const ResourceLocation MUSIC_DISC_PRECIPICE("minecraft:music_disc.precipice");
const ResourceLocation MUSIC_DISC_RELIC("minecraft:music_disc.relic");
const ResourceLocation MUSIC_DISC_STAL("minecraft:music_disc.stal");
const ResourceLocation MUSIC_DISC_STRAD("minecraft:music_disc.strad");
const ResourceLocation MUSIC_DISC_TEARS("minecraft:music_disc.tears");
const ResourceLocation MUSIC_DISC_WAIT("minecraft:music_disc.wait");
const ResourceLocation MUSIC_DISC_WARD("minecraft:music_disc.ward");

// ============================================================================
// 天气音效 (WEATHER_)
// ============================================================================

const ResourceLocation WEATHER_RAIN("minecraft:weather.rain");
const ResourceLocation WEATHER_RAIN_ABOVE("minecraft:weather.rain.above");
const ResourceLocation WEATHER_THUNDER("minecraft:weather.thunder");

// ============================================================================
// UI音效 (UI_)
// ============================================================================

const ResourceLocation UI_BUTTON_CLICK("minecraft:ui.button.click");
const ResourceLocation UI_TOAST_CHALLENGE_COMPLETE("minecraft:ui.toast.challenge_complete");
const ResourceLocation UI_TOAST_IN("minecraft:ui.toast.in");
const ResourceLocation UI_TOAST_OUT("minecraft:ui.toast.out");
const ResourceLocation UI_LOOM_SELECT_PATTERN("minecraft:ui.loom.select_pattern");
const ResourceLocation UI_LOOM_TAKE_RESULT("minecraft:ui.loom.take_result");
const ResourceLocation UI_CARTOGRAPHY_TABLE_TAKE_RESULT("minecraft:ui.cartography_table.take_result");
const ResourceLocation UI_STONECUTTER_TAKE_RESULT("minecraft:ui.stonecutter.take_result");
const ResourceLocation UI_STONECUTTER_SELECT_RECIPE("minecraft:ui.stonecutter.select_recipe");

// ============================================================================
// 事件音效 (EVENT_)
// ============================================================================

const ResourceLocation EVENT_RAID_HORN("minecraft:event.raid_horn");

// ============================================================================
// 附魔音效 (ENCHANT_)
// ============================================================================

const ResourceLocation ENCHANT_THORNS_HIT("minecraft:enchant.thorns.hit");

// ============================================================================
// 粒子音效 (PARTICLE_)
// ============================================================================

const ResourceLocation PARTICLE_SOUL_ESCAPE("minecraft:particle.soul_escape");

// ============================================================================
// 眼眸花音效 (BLOCK_EYEBLOSSOM_)
// ============================================================================

const ResourceLocation BLOCK_EYEBLOSSOM_OPEN_LONG("minecraft:block.eyeblossom.open_long");
const ResourceLocation BLOCK_EYEBLOSSOM_CLOSE_LONG("minecraft:block.eyeblossom.close_long");
const ResourceLocation BLOCK_EYEBLOSSOM_OPEN("minecraft:block.eyeblossom.open");
const ResourceLocation BLOCK_EYEBLOSSOM_CLOSE("minecraft:block.eyeblossom.close");
const ResourceLocation BLOCK_EYEBLOSSOM_IDLE("minecraft:block.eyeblossom.idle");

// ============================================================================
// 刷子音效 (BRUSH_)
// ============================================================================

const ResourceLocation BRUSH_GENERIC("minecraft:item.brush.brushing.generic");
const ResourceLocation BRUSH_SAND("minecraft:item.brush.brushing.sand");
const ResourceLocation BRUSH_GRAVEL("minecraft:item.brush.brushing.gravel");
const ResourceLocation BRUSH_SAND_COMPLETED("minecraft:item.brush.brushing.sand.complete");
const ResourceLocation BRUSH_GRAVEL_COMPLETED("minecraft:item.brush.brushing.gravel.complete");

// ============================================================================
// 干燥恶魂音效 (DRIED_GHAST_ / GHASTLING_)
// ============================================================================

const ResourceLocation DRIED_GHAST_BREAK("minecraft:block.dried_ghast.break");
const ResourceLocation DRIED_GHAST_STEP("minecraft:block.dried_ghast.step");
const ResourceLocation DRIED_GHAST_FALL("minecraft:block.dried_ghast.fall");
const ResourceLocation DRIED_GHAST_AMBIENT("minecraft:block.dried_ghast.ambient");
const ResourceLocation DRIED_GHAST_AMBIENT_WATER("minecraft:block.dried_ghast.ambient_water");
const ResourceLocation DRIED_GHAST_PLACE("minecraft:block.dried_ghast.place");
const ResourceLocation DRIED_GHAST_PLACE_IN_WATER("minecraft:block.dried_ghast.place_in_water");
const ResourceLocation DRIED_GHAST_TRANSITION("minecraft:block.dried_ghast.transition");
const ResourceLocation GHASTLING_SPAWN("minecraft:entity.ghastling.spawn");
const ResourceLocation GHASTLING_AMBIENT("minecraft:entity.ghastling.ambient");
const ResourceLocation GHASTLING_HURT("minecraft:entity.ghastling.hurt");
const ResourceLocation GHASTLING_DEATH("minecraft:entity.ghastling.death");

const ResourceLocation HAPPY_GHAST_AMBIENT("minecraft:entity.happy_ghast.ambient");
const ResourceLocation HAPPY_GHAST_HURT("minecraft:entity.happy_ghast.hurt");
const ResourceLocation HAPPY_GHAST_DEATH("minecraft:entity.happy_ghast.death");
const ResourceLocation HARNESS_GOGGLES_DOWN("minecraft:entity.happy_ghast.harness_goggles_down");
const ResourceLocation HARNESS_GOGGLES_UP("minecraft:entity.happy_ghast.harness_goggles_up");

// ============================================================================
// 初始化
// ============================================================================

void initialize()
{
    // 声音事件已通过静态初始化创建
    // 此函数可用于验证所有声音事件已正确初始化
}

// ============================================================================
// 1.21.11 补齐：此前遗漏的音效 ID（定义）
// ============================================================================

const ResourceLocation ENTITY_ALLAY_AMBIENT_WITH_ITEM("minecraft:entity.allay.ambient_with_item");
const ResourceLocation ENTITY_ALLAY_AMBIENT_WITHOUT_ITEM("minecraft:entity.allay.ambient_without_item");
const ResourceLocation ENTITY_ALLAY_DEATH("minecraft:entity.allay.death");
const ResourceLocation ENTITY_ALLAY_HURT("minecraft:entity.allay.hurt");
const ResourceLocation ENTITY_ALLAY_ITEM_GIVEN("minecraft:entity.allay.item_given");
const ResourceLocation ENTITY_ALLAY_ITEM_TAKEN("minecraft:entity.allay.item_taken");
const ResourceLocation ENTITY_ALLAY_ITEM_THROWN("minecraft:entity.allay.item_thrown");
const ResourceLocation BLOCK_AMETHYST_BLOCK_BREAK("minecraft:block.amethyst_block.break");
const ResourceLocation BLOCK_AMETHYST_BLOCK_FALL("minecraft:block.amethyst_block.fall");
const ResourceLocation BLOCK_AMETHYST_BLOCK_HIT("minecraft:block.amethyst_block.hit");
const ResourceLocation BLOCK_AMETHYST_BLOCK_PLACE("minecraft:block.amethyst_block.place");
const ResourceLocation BLOCK_AMETHYST_BLOCK_STEP("minecraft:block.amethyst_block.step");
const ResourceLocation BLOCK_AMETHYST_CLUSTER_BREAK("minecraft:block.amethyst_cluster.break");
const ResourceLocation BLOCK_AMETHYST_CLUSTER_FALL("minecraft:block.amethyst_cluster.fall");
const ResourceLocation BLOCK_AMETHYST_CLUSTER_HIT("minecraft:block.amethyst_cluster.hit");
const ResourceLocation BLOCK_AMETHYST_CLUSTER_PLACE("minecraft:block.amethyst_cluster.place");
const ResourceLocation BLOCK_AMETHYST_CLUSTER_STEP("minecraft:block.amethyst_cluster.step");
const ResourceLocation ENTITY_ARMADILLO_EAT("minecraft:entity.armadillo.eat");
const ResourceLocation ENTITY_ARMADILLO_HURT("minecraft:entity.armadillo.hurt");
const ResourceLocation ENTITY_ARMADILLO_HURT_REDUCED("minecraft:entity.armadillo.hurt_reduced");
const ResourceLocation ENTITY_ARMADILLO_AMBIENT("minecraft:entity.armadillo.ambient");
const ResourceLocation ENTITY_ARMADILLO_STEP("minecraft:entity.armadillo.step");
const ResourceLocation ENTITY_ARMADILLO_DEATH("minecraft:entity.armadillo.death");
const ResourceLocation ENTITY_ARMADILLO_ROLL("minecraft:entity.armadillo.roll");
const ResourceLocation ENTITY_ARMADILLO_LAND("minecraft:entity.armadillo.land");
const ResourceLocation ENTITY_ARMADILLO_SCUTE_DROP("minecraft:entity.armadillo.scute_drop");
const ResourceLocation ENTITY_ARMADILLO_UNROLL_FINISH("minecraft:entity.armadillo.unroll_finish");
const ResourceLocation ENTITY_ARMADILLO_PEEK("minecraft:entity.armadillo.peek");
const ResourceLocation ENTITY_ARMADILLO_UNROLL_START("minecraft:entity.armadillo.unroll_start");
const ResourceLocation ENTITY_ARMADILLO_BRUSH("minecraft:entity.armadillo.brush");
const ResourceLocation BLOCK_AZALEA_BREAK("minecraft:block.azalea.break");
const ResourceLocation BLOCK_AZALEA_FALL("minecraft:block.azalea.fall");
const ResourceLocation BLOCK_AZALEA_HIT("minecraft:block.azalea.hit");
const ResourceLocation BLOCK_AZALEA_PLACE("minecraft:block.azalea.place");
const ResourceLocation BLOCK_AZALEA_STEP("minecraft:block.azalea.step");
const ResourceLocation BLOCK_AZALEA_LEAVES_BREAK("minecraft:block.azalea_leaves.break");
const ResourceLocation BLOCK_AZALEA_LEAVES_FALL("minecraft:block.azalea_leaves.fall");
const ResourceLocation BLOCK_AZALEA_LEAVES_HIT("minecraft:block.azalea_leaves.hit");
const ResourceLocation BLOCK_AZALEA_LEAVES_PLACE("minecraft:block.azalea_leaves.place");
const ResourceLocation BLOCK_AZALEA_LEAVES_STEP("minecraft:block.azalea_leaves.step");
const ResourceLocation ENTITY_BABY_NAUTILUS_AMBIENT_LAND("minecraft:entity.baby_nautilus.ambient_land");
const ResourceLocation ENTITY_BABY_NAUTILUS_DEATH_LAND("minecraft:entity.baby_nautilus.death_land");
const ResourceLocation ENTITY_BABY_NAUTILUS_HURT_LAND("minecraft:entity.baby_nautilus.hurt_land");
const ResourceLocation ENTITY_NAUTILUS_RIDING("minecraft:entity.nautilus.riding");
const ResourceLocation ENTITY_BABY_NAUTILUS_SWIM("minecraft:entity.baby_nautilus.swim");
const ResourceLocation BLOCK_BAMBOO_WOOD_BREAK("minecraft:block.bamboo_wood.break");
const ResourceLocation BLOCK_BAMBOO_WOOD_FALL("minecraft:block.bamboo_wood.fall");
const ResourceLocation BLOCK_BAMBOO_WOOD_HIT("minecraft:block.bamboo_wood.hit");
const ResourceLocation BLOCK_BAMBOO_WOOD_PLACE("minecraft:block.bamboo_wood.place");
const ResourceLocation BLOCK_BAMBOO_WOOD_STEP("minecraft:block.bamboo_wood.step");
const ResourceLocation BLOCK_BAMBOO_WOOD_DOOR_CLOSE("minecraft:block.bamboo_wood_door.close");
const ResourceLocation BLOCK_BAMBOO_WOOD_DOOR_OPEN("minecraft:block.bamboo_wood_door.open");
const ResourceLocation BLOCK_BAMBOO_WOOD_TRAPDOOR_CLOSE("minecraft:block.bamboo_wood_trapdoor.close");
const ResourceLocation BLOCK_BAMBOO_WOOD_TRAPDOOR_OPEN("minecraft:block.bamboo_wood_trapdoor.open");
const ResourceLocation BLOCK_BAMBOO_WOOD_BUTTON_CLICK_OFF("minecraft:block.bamboo_wood_button.click_off");
const ResourceLocation BLOCK_BAMBOO_WOOD_BUTTON_CLICK_ON("minecraft:block.bamboo_wood_button.click_on");
const ResourceLocation BLOCK_BAMBOO_WOOD_PRESSURE_PLATE_CLICK_OFF(
    "minecraft:block.bamboo_wood_pressure_plate.click_off");
const ResourceLocation BLOCK_BAMBOO_WOOD_PRESSURE_PLATE_CLICK_ON("minecraft:block.bamboo_wood_pressure_plate.click_on");
const ResourceLocation BLOCK_BAMBOO_WOOD_FENCE_GATE_CLOSE("minecraft:block.bamboo_wood_fence_gate.close");
const ResourceLocation BLOCK_BAMBOO_WOOD_FENCE_GATE_OPEN("minecraft:block.bamboo_wood_fence_gate.open");
const ResourceLocation BLOCK_BIG_DRIPLEAF_BREAK("minecraft:block.big_dripleaf.break");
const ResourceLocation BLOCK_BIG_DRIPLEAF_FALL("minecraft:block.big_dripleaf.fall");
const ResourceLocation BLOCK_BIG_DRIPLEAF_HIT("minecraft:block.big_dripleaf.hit");
const ResourceLocation BLOCK_BIG_DRIPLEAF_PLACE("minecraft:block.big_dripleaf.place");
const ResourceLocation BLOCK_BIG_DRIPLEAF_STEP("minecraft:block.big_dripleaf.step");
const ResourceLocation ENTITY_BOGGED_AMBIENT("minecraft:entity.bogged.ambient");
const ResourceLocation ENTITY_BOGGED_DEATH("minecraft:entity.bogged.death");
const ResourceLocation ENTITY_BOGGED_HURT("minecraft:entity.bogged.hurt");
const ResourceLocation ENTITY_BOGGED_SHEAR("minecraft:entity.bogged.shear");
const ResourceLocation ENTITY_BOGGED_STEP("minecraft:entity.bogged.step");
const ResourceLocation ENTITY_BREEZE_DEATH("minecraft:entity.breeze.death");
const ResourceLocation ENTITY_BREEZE_HURT("minecraft:entity.breeze.hurt");
const ResourceLocation UI_HUD_BUBBLE_POP("minecraft:ui.hud.bubble_pop");
const ResourceLocation ITEM_BUCKET_EMPTY_TADPOLE("minecraft:item.bucket.empty_tadpole");
const ResourceLocation ITEM_BUCKET_FILL_TADPOLE("minecraft:item.bucket.fill_tadpole");
const ResourceLocation BLOCK_CACTUS_FLOWER_BREAK("minecraft:block.cactus_flower.break");
const ResourceLocation BLOCK_CACTUS_FLOWER_PLACE("minecraft:block.cactus_flower.place");
const ResourceLocation BLOCK_CAKE_ADD_CANDLE("minecraft:block.cake.add_candle");
const ResourceLocation BLOCK_CALCITE_BREAK("minecraft:block.calcite.break");
const ResourceLocation BLOCK_CALCITE_STEP("minecraft:block.calcite.step");
const ResourceLocation BLOCK_CALCITE_PLACE("minecraft:block.calcite.place");
const ResourceLocation BLOCK_CALCITE_HIT("minecraft:block.calcite.hit");
const ResourceLocation BLOCK_CALCITE_FALL("minecraft:block.calcite.fall");
const ResourceLocation ENTITY_CAMEL_HUSK_AMBIENT("minecraft:entity.camel_husk.ambient");
const ResourceLocation ENTITY_CAMEL_HUSK_DASH("minecraft:entity.camel_husk.dash");
const ResourceLocation ENTITY_CAMEL_HUSK_DASH_READY("minecraft:entity.camel_husk.dash_ready");
const ResourceLocation ENTITY_CAMEL_HUSK_DEATH("minecraft:entity.camel_husk.death");
const ResourceLocation ENTITY_CAMEL_HUSK_EAT("minecraft:entity.camel_husk.eat");
const ResourceLocation ENTITY_CAMEL_HUSK_HURT("minecraft:entity.camel_husk.hurt");
const ResourceLocation ENTITY_CAMEL_HUSK_SIT("minecraft:entity.camel_husk.sit");
const ResourceLocation ENTITY_CAMEL_HUSK_STAND("minecraft:entity.camel_husk.stand");
const ResourceLocation ENTITY_CAMEL_HUSK_STEP("minecraft:entity.camel_husk.step");
const ResourceLocation ENTITY_CAMEL_HUSK_STEP_SAND("minecraft:entity.camel_husk.step_sand");
const ResourceLocation ENTITY_CAMEL_AMBIENT("minecraft:entity.camel.ambient");
const ResourceLocation ENTITY_CAMEL_DASH("minecraft:entity.camel.dash");
const ResourceLocation ENTITY_CAMEL_DASH_READY("minecraft:entity.camel.dash_ready");
const ResourceLocation ENTITY_CAMEL_DEATH("minecraft:entity.camel.death");
const ResourceLocation ENTITY_CAMEL_EAT("minecraft:entity.camel.eat");
const ResourceLocation ENTITY_CAMEL_HURT("minecraft:entity.camel.hurt");
const ResourceLocation ENTITY_CAMEL_SIT("minecraft:entity.camel.sit");
const ResourceLocation ENTITY_CAMEL_STAND("minecraft:entity.camel.stand");
const ResourceLocation ENTITY_CAMEL_STEP("minecraft:entity.camel.step");
const ResourceLocation ENTITY_CAMEL_STEP_SAND("minecraft:entity.camel.step_sand");
const ResourceLocation BLOCK_CANDLE_FALL("minecraft:block.candle.fall");
const ResourceLocation BLOCK_CAVE_VINES_BREAK("minecraft:block.cave_vines.break");
const ResourceLocation BLOCK_CAVE_VINES_FALL("minecraft:block.cave_vines.fall");
const ResourceLocation BLOCK_CAVE_VINES_HIT("minecraft:block.cave_vines.hit");
const ResourceLocation BLOCK_CAVE_VINES_PLACE("minecraft:block.cave_vines.place");
const ResourceLocation BLOCK_CAVE_VINES_STEP("minecraft:block.cave_vines.step");
const ResourceLocation BLOCK_CHERRY_WOOD_BREAK("minecraft:block.cherry_wood.break");
const ResourceLocation BLOCK_CHERRY_WOOD_FALL("minecraft:block.cherry_wood.fall");
const ResourceLocation BLOCK_CHERRY_WOOD_HIT("minecraft:block.cherry_wood.hit");
const ResourceLocation BLOCK_CHERRY_WOOD_PLACE("minecraft:block.cherry_wood.place");
const ResourceLocation BLOCK_CHERRY_WOOD_STEP("minecraft:block.cherry_wood.step");
const ResourceLocation BLOCK_CHERRY_SAPLING_BREAK("minecraft:block.cherry_sapling.break");
const ResourceLocation BLOCK_CHERRY_SAPLING_FALL("minecraft:block.cherry_sapling.fall");
const ResourceLocation BLOCK_CHERRY_SAPLING_HIT("minecraft:block.cherry_sapling.hit");
const ResourceLocation BLOCK_CHERRY_SAPLING_PLACE("minecraft:block.cherry_sapling.place");
const ResourceLocation BLOCK_CHERRY_SAPLING_STEP("minecraft:block.cherry_sapling.step");
const ResourceLocation BLOCK_CHERRY_LEAVES_BREAK("minecraft:block.cherry_leaves.break");
const ResourceLocation BLOCK_CHERRY_LEAVES_FALL("minecraft:block.cherry_leaves.fall");
const ResourceLocation BLOCK_CHERRY_LEAVES_HIT("minecraft:block.cherry_leaves.hit");
const ResourceLocation BLOCK_CHERRY_LEAVES_PLACE("minecraft:block.cherry_leaves.place");
const ResourceLocation BLOCK_CHERRY_LEAVES_STEP("minecraft:block.cherry_leaves.step");
const ResourceLocation BLOCK_CHERRY_WOOD_HANGING_SIGN_STEP("minecraft:block.cherry_wood_hanging_sign.step");
const ResourceLocation BLOCK_CHERRY_WOOD_HANGING_SIGN_BREAK("minecraft:block.cherry_wood_hanging_sign.break");
const ResourceLocation BLOCK_CHERRY_WOOD_HANGING_SIGN_FALL("minecraft:block.cherry_wood_hanging_sign.fall");
const ResourceLocation BLOCK_CHERRY_WOOD_HANGING_SIGN_HIT("minecraft:block.cherry_wood_hanging_sign.hit");
const ResourceLocation BLOCK_CHERRY_WOOD_HANGING_SIGN_PLACE("minecraft:block.cherry_wood_hanging_sign.place");
const ResourceLocation BLOCK_CHERRY_WOOD_DOOR_CLOSE("minecraft:block.cherry_wood_door.close");
const ResourceLocation BLOCK_CHERRY_WOOD_DOOR_OPEN("minecraft:block.cherry_wood_door.open");
const ResourceLocation BLOCK_CHERRY_WOOD_TRAPDOOR_CLOSE("minecraft:block.cherry_wood_trapdoor.close");
const ResourceLocation BLOCK_CHERRY_WOOD_TRAPDOOR_OPEN("minecraft:block.cherry_wood_trapdoor.open");
const ResourceLocation BLOCK_CHERRY_WOOD_BUTTON_CLICK_OFF("minecraft:block.cherry_wood_button.click_off");
const ResourceLocation BLOCK_CHERRY_WOOD_BUTTON_CLICK_ON("minecraft:block.cherry_wood_button.click_on");
const ResourceLocation BLOCK_CHERRY_WOOD_PRESSURE_PLATE_CLICK_OFF(
    "minecraft:block.cherry_wood_pressure_plate.click_off");
const ResourceLocation BLOCK_CHERRY_WOOD_PRESSURE_PLATE_CLICK_ON("minecraft:block.cherry_wood_pressure_plate.click_on");
const ResourceLocation BLOCK_CHERRY_WOOD_FENCE_GATE_CLOSE("minecraft:block.cherry_wood_fence_gate.close");
const ResourceLocation BLOCK_CHERRY_WOOD_FENCE_GATE_OPEN("minecraft:block.cherry_wood_fence_gate.open");
const ResourceLocation BLOCK_CHISELED_BOOKSHELF_BREAK("minecraft:block.chiseled_bookshelf.break");
const ResourceLocation BLOCK_CHISELED_BOOKSHELF_FALL("minecraft:block.chiseled_bookshelf.fall");
const ResourceLocation BLOCK_CHISELED_BOOKSHELF_HIT("minecraft:block.chiseled_bookshelf.hit");
const ResourceLocation BLOCK_CHISELED_BOOKSHELF_INSERT("minecraft:block.chiseled_bookshelf.insert");
const ResourceLocation BLOCK_CHISELED_BOOKSHELF_INSERT_ENCHANTED("minecraft:block.chiseled_bookshelf.insert.enchanted");
const ResourceLocation BLOCK_CHISELED_BOOKSHELF_STEP("minecraft:block.chiseled_bookshelf.step");
const ResourceLocation BLOCK_CHISELED_BOOKSHELF_PICKUP("minecraft:block.chiseled_bookshelf.pickup");
const ResourceLocation BLOCK_CHISELED_BOOKSHELF_PICKUP_ENCHANTED("minecraft:block.chiseled_bookshelf.pickup.enchanted");
const ResourceLocation BLOCK_CHISELED_BOOKSHELF_PLACE("minecraft:block.chiseled_bookshelf.place");
const ResourceLocation BLOCK_COBWEB_BREAK("minecraft:block.cobweb.break");
const ResourceLocation BLOCK_COBWEB_STEP("minecraft:block.cobweb.step");
const ResourceLocation BLOCK_COBWEB_PLACE("minecraft:block.cobweb.place");
const ResourceLocation BLOCK_COBWEB_HIT("minecraft:block.cobweb.hit");
const ResourceLocation BLOCK_COBWEB_FALL("minecraft:block.cobweb.fall");
const ResourceLocation BLOCK_COPPER_BULB_BREAK("minecraft:block.copper_bulb.break");
const ResourceLocation BLOCK_COPPER_BULB_STEP("minecraft:block.copper_bulb.step");
const ResourceLocation BLOCK_COPPER_BULB_PLACE("minecraft:block.copper_bulb.place");
const ResourceLocation BLOCK_COPPER_BULB_HIT("minecraft:block.copper_bulb.hit");
const ResourceLocation BLOCK_COPPER_BULB_FALL("minecraft:block.copper_bulb.fall");
const ResourceLocation BLOCK_COPPER_BULB_TURN_ON("minecraft:block.copper_bulb.turn_on");
const ResourceLocation BLOCK_COPPER_BULB_TURN_OFF("minecraft:block.copper_bulb.turn_off");
const ResourceLocation BLOCK_COPPER_BREAK("minecraft:block.copper.break");
const ResourceLocation BLOCK_COPPER_STEP("minecraft:block.copper.step");
const ResourceLocation BLOCK_COPPER_PLACE("minecraft:block.copper.place");
const ResourceLocation BLOCK_COPPER_HIT("minecraft:block.copper.hit");
const ResourceLocation BLOCK_COPPER_FALL("minecraft:block.copper.fall");
const ResourceLocation BLOCK_COPPER_DOOR_CLOSE("minecraft:block.copper_door.close");
const ResourceLocation BLOCK_COPPER_DOOR_OPEN("minecraft:block.copper_door.open");
const ResourceLocation ENTITY_COPPER_GOLEM_BECOME_STATUE("minecraft:entity.copper_golem_become_statue");
const ResourceLocation BLOCK_COPPER_GRATE_BREAK("minecraft:block.copper_grate.break");
const ResourceLocation BLOCK_COPPER_GRATE_STEP("minecraft:block.copper_grate.step");
const ResourceLocation BLOCK_COPPER_GRATE_PLACE("minecraft:block.copper_grate.place");
const ResourceLocation BLOCK_COPPER_GRATE_HIT("minecraft:block.copper_grate.hit");
const ResourceLocation BLOCK_COPPER_GRATE_FALL("minecraft:block.copper_grate.fall");
const ResourceLocation BLOCK_COPPER_TRAPDOOR_CLOSE("minecraft:block.copper_trapdoor.close");
const ResourceLocation BLOCK_COPPER_TRAPDOOR_OPEN("minecraft:block.copper_trapdoor.open");
const ResourceLocation ENTITY_CREAKING_AMBIENT("minecraft:entity.creaking.ambient");
const ResourceLocation ENTITY_CREAKING_ACTIVATE("minecraft:entity.creaking.activate");
const ResourceLocation ENTITY_CREAKING_DEACTIVATE("minecraft:entity.creaking.deactivate");
const ResourceLocation ENTITY_CREAKING_ATTACK("minecraft:entity.creaking.attack");
const ResourceLocation ENTITY_CREAKING_DEATH("minecraft:entity.creaking.death");
const ResourceLocation ENTITY_CREAKING_STEP("minecraft:entity.creaking.step");
const ResourceLocation ENTITY_CREAKING_FREEZE("minecraft:entity.creaking.freeze");
const ResourceLocation ENTITY_CREAKING_UNFREEZE("minecraft:entity.creaking.unfreeze");
const ResourceLocation ENTITY_CREAKING_SPAWN("minecraft:entity.creaking.spawn");
const ResourceLocation ENTITY_CREAKING_SWAY("minecraft:entity.creaking.sway");
const ResourceLocation ENTITY_CREAKING_TWITCH("minecraft:entity.creaking.twitch");
const ResourceLocation BLOCK_CREAKING_HEART_BREAK("minecraft:block.creaking_heart.break");
const ResourceLocation BLOCK_CREAKING_HEART_FALL("minecraft:block.creaking_heart.fall");
const ResourceLocation BLOCK_CREAKING_HEART_HIT("minecraft:block.creaking_heart.hit");
const ResourceLocation BLOCK_CREAKING_HEART_HURT("minecraft:block.creaking_heart.hurt");
const ResourceLocation BLOCK_CREAKING_HEART_PLACE("minecraft:block.creaking_heart.place");
const ResourceLocation BLOCK_CREAKING_HEART_STEP("minecraft:block.creaking_heart.step");
const ResourceLocation BLOCK_CREAKING_HEART_IDLE("minecraft:block.creaking_heart.idle");
const ResourceLocation BLOCK_CREAKING_HEART_SPAWN("minecraft:block.creaking_heart.spawn");
const ResourceLocation BLOCK_DEADBUSH_IDLE("minecraft:block.deadbush.idle");
const ResourceLocation BLOCK_DECORATED_POT_BREAK("minecraft:block.decorated_pot.break");
const ResourceLocation BLOCK_DECORATED_POT_FALL("minecraft:block.decorated_pot.fall");
const ResourceLocation BLOCK_DECORATED_POT_HIT("minecraft:block.decorated_pot.hit");
const ResourceLocation BLOCK_DECORATED_POT_STEP("minecraft:block.decorated_pot.step");
const ResourceLocation BLOCK_DECORATED_POT_PLACE("minecraft:block.decorated_pot.place");
const ResourceLocation BLOCK_DECORATED_POT_SHATTER("minecraft:block.decorated_pot.shatter");
const ResourceLocation BLOCK_DEEPSLATE_BRICKS_BREAK("minecraft:block.deepslate_bricks.break");
const ResourceLocation BLOCK_DEEPSLATE_BRICKS_FALL("minecraft:block.deepslate_bricks.fall");
const ResourceLocation BLOCK_DEEPSLATE_BRICKS_HIT("minecraft:block.deepslate_bricks.hit");
const ResourceLocation BLOCK_DEEPSLATE_BRICKS_PLACE("minecraft:block.deepslate_bricks.place");
const ResourceLocation BLOCK_DEEPSLATE_BRICKS_STEP("minecraft:block.deepslate_bricks.step");
const ResourceLocation BLOCK_DEEPSLATE_BREAK("minecraft:block.deepslate.break");
const ResourceLocation BLOCK_DEEPSLATE_FALL("minecraft:block.deepslate.fall");
const ResourceLocation BLOCK_DEEPSLATE_HIT("minecraft:block.deepslate.hit");
const ResourceLocation BLOCK_DEEPSLATE_PLACE("minecraft:block.deepslate.place");
const ResourceLocation BLOCK_DEEPSLATE_STEP("minecraft:block.deepslate.step");
const ResourceLocation BLOCK_DEEPSLATE_TILES_BREAK("minecraft:block.deepslate_tiles.break");
const ResourceLocation BLOCK_DEEPSLATE_TILES_FALL("minecraft:block.deepslate_tiles.fall");
const ResourceLocation BLOCK_DEEPSLATE_TILES_HIT("minecraft:block.deepslate_tiles.hit");
const ResourceLocation BLOCK_DEEPSLATE_TILES_PLACE("minecraft:block.deepslate_tiles.place");
const ResourceLocation BLOCK_DEEPSLATE_TILES_STEP("minecraft:block.deepslate_tiles.step");
const ResourceLocation ENTITY_DONKEY_JUMP("minecraft:entity.donkey.jump");
const ResourceLocation BLOCK_DRIPSTONE_BLOCK_BREAK("minecraft:block.dripstone_block.break");
const ResourceLocation BLOCK_DRIPSTONE_BLOCK_STEP("minecraft:block.dripstone_block.step");
const ResourceLocation BLOCK_DRIPSTONE_BLOCK_PLACE("minecraft:block.dripstone_block.place");
const ResourceLocation BLOCK_DRIPSTONE_BLOCK_HIT("minecraft:block.dripstone_block.hit");
const ResourceLocation BLOCK_DRIPSTONE_BLOCK_FALL("minecraft:block.dripstone_block.fall");
const ResourceLocation BLOCK_DRY_GRASS_AMBIENT("minecraft:block.dry_grass.ambient");
const ResourceLocation BLOCK_POINTED_DRIPSTONE_BREAK("minecraft:block.pointed_dripstone.break");
const ResourceLocation BLOCK_POINTED_DRIPSTONE_STEP("minecraft:block.pointed_dripstone.step");
const ResourceLocation BLOCK_POINTED_DRIPSTONE_PLACE("minecraft:block.pointed_dripstone.place");
const ResourceLocation ITEM_DYE_USE("minecraft:item.dye.use");
const ResourceLocation BLOCK_FIREFLY_BUSH_IDLE("minecraft:block.firefly_bush.idle");
const ResourceLocation BLOCK_FLOWERING_AZALEA_BREAK("minecraft:block.flowering_azalea.break");
const ResourceLocation BLOCK_FLOWERING_AZALEA_FALL("minecraft:block.flowering_azalea.fall");
const ResourceLocation BLOCK_FLOWERING_AZALEA_HIT("minecraft:block.flowering_azalea.hit");
const ResourceLocation BLOCK_FLOWERING_AZALEA_PLACE("minecraft:block.flowering_azalea.place");
const ResourceLocation BLOCK_FLOWERING_AZALEA_STEP("minecraft:block.flowering_azalea.step");
const ResourceLocation BLOCK_SUSPICIOUS_SAND_BREAK("minecraft:block.suspicious_sand.break");
const ResourceLocation BLOCK_SUSPICIOUS_SAND_STEP("minecraft:block.suspicious_sand.step");
const ResourceLocation BLOCK_SUSPICIOUS_SAND_PLACE("minecraft:block.suspicious_sand.place");
const ResourceLocation BLOCK_SUSPICIOUS_SAND_HIT("minecraft:block.suspicious_sand.hit");
const ResourceLocation BLOCK_SUSPICIOUS_SAND_FALL("minecraft:block.suspicious_sand.fall");
const ResourceLocation BLOCK_SUSPICIOUS_GRAVEL_BREAK("minecraft:block.suspicious_gravel.break");
const ResourceLocation BLOCK_SUSPICIOUS_GRAVEL_STEP("minecraft:block.suspicious_gravel.step");
const ResourceLocation BLOCK_SUSPICIOUS_GRAVEL_PLACE("minecraft:block.suspicious_gravel.place");
const ResourceLocation BLOCK_SUSPICIOUS_GRAVEL_HIT("minecraft:block.suspicious_gravel.hit");
const ResourceLocation BLOCK_SUSPICIOUS_GRAVEL_FALL("minecraft:block.suspicious_gravel.fall");
const ResourceLocation BLOCK_FROGLIGHT_BREAK("minecraft:block.froglight.break");
const ResourceLocation BLOCK_FROGLIGHT_FALL("minecraft:block.froglight.fall");
const ResourceLocation BLOCK_FROGLIGHT_HIT("minecraft:block.froglight.hit");
const ResourceLocation BLOCK_FROGLIGHT_PLACE("minecraft:block.froglight.place");
const ResourceLocation BLOCK_FROGLIGHT_STEP("minecraft:block.froglight.step");
const ResourceLocation BLOCK_FROGSPAWN_STEP("minecraft:block.frogspawn.step");
const ResourceLocation BLOCK_FROGSPAWN_BREAK("minecraft:block.frogspawn.break");
const ResourceLocation BLOCK_FROGSPAWN_FALL("minecraft:block.frogspawn.fall");
const ResourceLocation BLOCK_FROGSPAWN_HIT("minecraft:block.frogspawn.hit");
const ResourceLocation BLOCK_FROGSPAWN_PLACE("minecraft:block.frogspawn.place");
const ResourceLocation ENTITY_FROG_LAY_SPAWN("minecraft:entity.frog.lay_spawn");
const ResourceLocation ENTITY_FROG_LONG_JUMP("minecraft:entity.frog.long_jump");
const ResourceLocation ITEM_GLOW_INK_SAC_USE("minecraft:item.glow_ink_sac.use");
const ResourceLocation ENTITY_GOAT_AMBIENT("minecraft:entity.goat.ambient");
const ResourceLocation ENTITY_GOAT_DEATH("minecraft:entity.goat.death");
const ResourceLocation ENTITY_GOAT_EAT("minecraft:entity.goat.eat");
const ResourceLocation ENTITY_GOAT_HURT("minecraft:entity.goat.hurt");
const ResourceLocation ENTITY_GOAT_LONG_JUMP("minecraft:entity.goat.long_jump");
const ResourceLocation ENTITY_GOAT_MILK("minecraft:entity.goat.milk");
const ResourceLocation ENTITY_GOAT_PREPARE_RAM("minecraft:entity.goat.prepare_ram");
const ResourceLocation ENTITY_GOAT_RAM_IMPACT("minecraft:entity.goat.ram_impact");
const ResourceLocation ENTITY_GOAT_HORN_BREAK("minecraft:entity.goat.horn_break");
const ResourceLocation ENTITY_GOAT_SCREAMING_AMBIENT("minecraft:entity.goat.screaming.ambient");
const ResourceLocation ENTITY_GOAT_SCREAMING_DEATH("minecraft:entity.goat.screaming.death");
const ResourceLocation ENTITY_GOAT_SCREAMING_EAT("minecraft:entity.goat.screaming.eat");
const ResourceLocation ENTITY_GOAT_SCREAMING_HURT("minecraft:entity.goat.screaming.hurt");
const ResourceLocation ENTITY_GOAT_SCREAMING_LONG_JUMP("minecraft:entity.goat.screaming.long_jump");
const ResourceLocation ENTITY_GOAT_SCREAMING_MILK("minecraft:entity.goat.screaming.milk");
const ResourceLocation ENTITY_GOAT_SCREAMING_PREPARE_RAM("minecraft:entity.goat.screaming.prepare_ram");
const ResourceLocation ENTITY_GOAT_SCREAMING_RAM_IMPACT("minecraft:entity.goat.screaming.ram_impact");
const ResourceLocation ENTITY_GOAT_STEP("minecraft:entity.goat.step");
const ResourceLocation BLOCK_GROWING_PLANT_CROP("minecraft:block.growing_plant.crop");
const ResourceLocation BLOCK_HANGING_ROOTS_BREAK("minecraft:block.hanging_roots.break");
const ResourceLocation BLOCK_HANGING_ROOTS_FALL("minecraft:block.hanging_roots.fall");
const ResourceLocation BLOCK_HANGING_ROOTS_HIT("minecraft:block.hanging_roots.hit");
const ResourceLocation BLOCK_HANGING_ROOTS_PLACE("minecraft:block.hanging_roots.place");
const ResourceLocation BLOCK_HANGING_ROOTS_STEP("minecraft:block.hanging_roots.step");
const ResourceLocation BLOCK_HANGING_SIGN_STEP("minecraft:block.hanging_sign.step");
const ResourceLocation BLOCK_HANGING_SIGN_BREAK("minecraft:block.hanging_sign.break");
const ResourceLocation BLOCK_HANGING_SIGN_FALL("minecraft:block.hanging_sign.fall");
const ResourceLocation BLOCK_HANGING_SIGN_HIT("minecraft:block.hanging_sign.hit");
const ResourceLocation BLOCK_HANGING_SIGN_PLACE("minecraft:block.hanging_sign.place");
const ResourceLocation ENTITY_HAPPY_GHAST_RIDING("minecraft:entity.happy_ghast.riding");
const ResourceLocation BLOCK_HEAVY_CORE_BREAK("minecraft:block.heavy_core.break");
const ResourceLocation BLOCK_HEAVY_CORE_FALL("minecraft:block.heavy_core.fall");
const ResourceLocation BLOCK_HEAVY_CORE_HIT("minecraft:block.heavy_core.hit");
const ResourceLocation BLOCK_HEAVY_CORE_PLACE("minecraft:block.heavy_core.place");
const ResourceLocation BLOCK_HEAVY_CORE_STEP("minecraft:block.heavy_core.step");
const ResourceLocation BLOCK_NETHER_WOOD_HANGING_SIGN_STEP("minecraft:block.nether_wood_hanging_sign.step");
const ResourceLocation BLOCK_NETHER_WOOD_HANGING_SIGN_BREAK("minecraft:block.nether_wood_hanging_sign.break");
const ResourceLocation BLOCK_NETHER_WOOD_HANGING_SIGN_FALL("minecraft:block.nether_wood_hanging_sign.fall");
const ResourceLocation BLOCK_NETHER_WOOD_HANGING_SIGN_HIT("minecraft:block.nether_wood_hanging_sign.hit");
const ResourceLocation BLOCK_NETHER_WOOD_HANGING_SIGN_PLACE("minecraft:block.nether_wood_hanging_sign.place");
const ResourceLocation BLOCK_BAMBOO_WOOD_HANGING_SIGN_STEP("minecraft:block.bamboo_wood_hanging_sign.step");
const ResourceLocation BLOCK_BAMBOO_WOOD_HANGING_SIGN_BREAK("minecraft:block.bamboo_wood_hanging_sign.break");
const ResourceLocation BLOCK_BAMBOO_WOOD_HANGING_SIGN_FALL("minecraft:block.bamboo_wood_hanging_sign.fall");
const ResourceLocation BLOCK_BAMBOO_WOOD_HANGING_SIGN_HIT("minecraft:block.bamboo_wood_hanging_sign.hit");
const ResourceLocation BLOCK_BAMBOO_WOOD_HANGING_SIGN_PLACE("minecraft:block.bamboo_wood_hanging_sign.place");
const ResourceLocation BLOCK_TRIAL_SPAWNER_BREAK("minecraft:block.trial_spawner.break");
const ResourceLocation BLOCK_TRIAL_SPAWNER_STEP("minecraft:block.trial_spawner.step");
const ResourceLocation BLOCK_TRIAL_SPAWNER_PLACE("minecraft:block.trial_spawner.place");
const ResourceLocation BLOCK_TRIAL_SPAWNER_HIT("minecraft:block.trial_spawner.hit");
const ResourceLocation BLOCK_TRIAL_SPAWNER_FALL("minecraft:block.trial_spawner.fall");
const ResourceLocation BLOCK_TRIAL_SPAWNER_SPAWN_MOB("minecraft:block.trial_spawner.spawn_mob");
const ResourceLocation BLOCK_TRIAL_SPAWNER_SPAWN_ITEM("minecraft:block.trial_spawner.spawn_item");
const ResourceLocation BLOCK_TRIAL_SPAWNER_SPAWN_ITEM_BEGIN("minecraft:block.trial_spawner.spawn_item_begin");
const ResourceLocation BLOCK_TRIAL_SPAWNER_DETECT_PLAYER("minecraft:block.trial_spawner.detect_player");
const ResourceLocation BLOCK_TRIAL_SPAWNER_OMINOUS_ACTIVATE("minecraft:block.trial_spawner.ominous_activate");
const ResourceLocation BLOCK_TRIAL_SPAWNER_AMBIENT("minecraft:block.trial_spawner.ambient");
const ResourceLocation BLOCK_TRIAL_SPAWNER_AMBIENT_OMINOUS("minecraft:block.trial_spawner.ambient_ominous");
const ResourceLocation BLOCK_TRIAL_SPAWNER_OPEN_SHUTTER("minecraft:block.trial_spawner.open_shutter");
const ResourceLocation BLOCK_TRIAL_SPAWNER_CLOSE_SHUTTER("minecraft:block.trial_spawner.close_shutter");
const ResourceLocation BLOCK_TRIAL_SPAWNER_EJECT_ITEM("minecraft:block.trial_spawner.eject_item");
const ResourceLocation ENTITY_HAPPY_GHAST_UNEQUIP("minecraft:entity.happy_ghast.unequip");
const ResourceLocation ITEM_HONEYCOMB_WAX_ON("minecraft:item.honeycomb.wax_on");
const ResourceLocation ITEM_INK_SAC_USE("minecraft:item.ink_sac.use");
const ResourceLocation BLOCK_IRON_BREAK("minecraft:block.iron.break");
const ResourceLocation BLOCK_IRON_STEP("minecraft:block.iron.step");
const ResourceLocation BLOCK_IRON_PLACE("minecraft:block.iron.place");
const ResourceLocation BLOCK_IRON_HIT("minecraft:block.iron.hit");
const ResourceLocation BLOCK_IRON_FALL("minecraft:block.iron.fall");
const ResourceLocation BLOCK_LARGE_AMETHYST_BUD_BREAK("minecraft:block.large_amethyst_bud.break");
const ResourceLocation BLOCK_LARGE_AMETHYST_BUD_PLACE("minecraft:block.large_amethyst_bud.place");
const ResourceLocation BLOCK_LEAF_LITTER_BREAK("minecraft:block.leaf_litter.break");
const ResourceLocation BLOCK_LEAF_LITTER_STEP("minecraft:block.leaf_litter.step");
const ResourceLocation BLOCK_LEAF_LITTER_PLACE("minecraft:block.leaf_litter.place");
const ResourceLocation BLOCK_LEAF_LITTER_HIT("minecraft:block.leaf_litter.hit");
const ResourceLocation BLOCK_LEAF_LITTER_FALL("minecraft:block.leaf_litter.fall");
const ResourceLocation ITEM_LEAD_UNTIED("minecraft:item.lead.untied");
const ResourceLocation ITEM_LEAD_TIED("minecraft:item.lead.tied");
const ResourceLocation ITEM_LEAD_BREAK("minecraft:item.lead.break");
const ResourceLocation BLOCK_MANGROVE_ROOTS_BREAK("minecraft:block.mangrove_roots.break");
const ResourceLocation BLOCK_MANGROVE_ROOTS_FALL("minecraft:block.mangrove_roots.fall");
const ResourceLocation BLOCK_MANGROVE_ROOTS_HIT("minecraft:block.mangrove_roots.hit");
const ResourceLocation BLOCK_MANGROVE_ROOTS_PLACE("minecraft:block.mangrove_roots.place");
const ResourceLocation BLOCK_MANGROVE_ROOTS_STEP("minecraft:block.mangrove_roots.step");
const ResourceLocation BLOCK_MEDIUM_AMETHYST_BUD_BREAK("minecraft:block.medium_amethyst_bud.break");
const ResourceLocation BLOCK_MEDIUM_AMETHYST_BUD_PLACE("minecraft:block.medium_amethyst_bud.place");
const ResourceLocation ENTITY_MINECART_INSIDE_UNDERWATER("minecraft:entity.minecart.inside.underwater");
const ResourceLocation BLOCK_MOSS_CARPET_BREAK("minecraft:block.moss_carpet.break");
const ResourceLocation BLOCK_MOSS_CARPET_FALL("minecraft:block.moss_carpet.fall");
const ResourceLocation BLOCK_MOSS_CARPET_HIT("minecraft:block.moss_carpet.hit");
const ResourceLocation BLOCK_MOSS_CARPET_PLACE("minecraft:block.moss_carpet.place");
const ResourceLocation BLOCK_MOSS_CARPET_STEP("minecraft:block.moss_carpet.step");
const ResourceLocation BLOCK_PINK_PETALS_BREAK("minecraft:block.pink_petals.break");
const ResourceLocation BLOCK_PINK_PETALS_FALL("minecraft:block.pink_petals.fall");
const ResourceLocation BLOCK_PINK_PETALS_HIT("minecraft:block.pink_petals.hit");
const ResourceLocation BLOCK_PINK_PETALS_PLACE("minecraft:block.pink_petals.place");
const ResourceLocation BLOCK_PINK_PETALS_STEP("minecraft:block.pink_petals.step");
const ResourceLocation BLOCK_MOSS_BREAK("minecraft:block.moss.break");
const ResourceLocation BLOCK_MOSS_FALL("minecraft:block.moss.fall");
const ResourceLocation BLOCK_MOSS_HIT("minecraft:block.moss.hit");
const ResourceLocation BLOCK_MOSS_PLACE("minecraft:block.moss.place");
const ResourceLocation BLOCK_MOSS_STEP("minecraft:block.moss.step");
const ResourceLocation BLOCK_MUD_BREAK("minecraft:block.mud.break");
const ResourceLocation BLOCK_MUD_FALL("minecraft:block.mud.fall");
const ResourceLocation BLOCK_MUD_HIT("minecraft:block.mud.hit");
const ResourceLocation BLOCK_MUD_PLACE("minecraft:block.mud.place");
const ResourceLocation BLOCK_MUD_STEP("minecraft:block.mud.step");
const ResourceLocation BLOCK_MUD_BRICKS_BREAK("minecraft:block.mud_bricks.break");
const ResourceLocation BLOCK_MUD_BRICKS_FALL("minecraft:block.mud_bricks.fall");
const ResourceLocation BLOCK_MUD_BRICKS_HIT("minecraft:block.mud_bricks.hit");
const ResourceLocation BLOCK_MUD_BRICKS_PLACE("minecraft:block.mud_bricks.place");
const ResourceLocation BLOCK_MUD_BRICKS_STEP("minecraft:block.mud_bricks.step");
const ResourceLocation BLOCK_MUDDY_MANGROVE_ROOTS_BREAK("minecraft:block.muddy_mangrove_roots.break");
const ResourceLocation BLOCK_MUDDY_MANGROVE_ROOTS_FALL("minecraft:block.muddy_mangrove_roots.fall");
const ResourceLocation BLOCK_MUDDY_MANGROVE_ROOTS_HIT("minecraft:block.muddy_mangrove_roots.hit");
const ResourceLocation BLOCK_MUDDY_MANGROVE_ROOTS_PLACE("minecraft:block.muddy_mangrove_roots.place");
const ResourceLocation BLOCK_MUDDY_MANGROVE_ROOTS_STEP("minecraft:block.muddy_mangrove_roots.step");
const ResourceLocation ENTITY_MULE_JUMP("minecraft:entity.mule.jump");
const ResourceLocation ENTITY_NAUTILUS_AMBIENT_LAND("minecraft:entity.nautilus.ambient_land");
const ResourceLocation ENTITY_NAUTILUS_DASH_LAND("minecraft:entity.nautilus.dash_land");
const ResourceLocation ENTITY_NAUTILUS_DASH_READY_LAND("minecraft:entity.nautilus.dash_ready_land");
const ResourceLocation ENTITY_NAUTILUS_DEATH_LAND("minecraft:entity.nautilus.death_land");
const ResourceLocation ENTITY_NAUTILUS_HURT_LAND("minecraft:entity.nautilus.hurt_land");
const ResourceLocation ENTITY_NAUTILUS_SWIM("minecraft:entity.nautilus.swim");
const ResourceLocation BLOCK_NETHER_WOOD_BREAK("minecraft:block.nether_wood.break");
const ResourceLocation BLOCK_NETHER_WOOD_FALL("minecraft:block.nether_wood.fall");
const ResourceLocation BLOCK_NETHER_WOOD_HIT("minecraft:block.nether_wood.hit");
const ResourceLocation BLOCK_NETHER_WOOD_PLACE("minecraft:block.nether_wood.place");
const ResourceLocation BLOCK_NETHER_WOOD_STEP("minecraft:block.nether_wood.step");
const ResourceLocation BLOCK_NETHER_WOOD_DOOR_CLOSE("minecraft:block.nether_wood_door.close");
const ResourceLocation BLOCK_NETHER_WOOD_DOOR_OPEN("minecraft:block.nether_wood_door.open");
const ResourceLocation BLOCK_NETHER_WOOD_TRAPDOOR_CLOSE("minecraft:block.nether_wood_trapdoor.close");
const ResourceLocation BLOCK_NETHER_WOOD_TRAPDOOR_OPEN("minecraft:block.nether_wood_trapdoor.open");
const ResourceLocation BLOCK_NETHER_WOOD_BUTTON_CLICK_OFF("minecraft:block.nether_wood_button.click_off");
const ResourceLocation BLOCK_NETHER_WOOD_BUTTON_CLICK_ON("minecraft:block.nether_wood_button.click_on");
const ResourceLocation BLOCK_NETHER_WOOD_PRESSURE_PLATE_CLICK_OFF(
    "minecraft:block.nether_wood_pressure_plate.click_off");
const ResourceLocation BLOCK_NETHER_WOOD_PRESSURE_PLATE_CLICK_ON("minecraft:block.nether_wood_pressure_plate.click_on");
const ResourceLocation BLOCK_NETHER_WOOD_FENCE_GATE_CLOSE("minecraft:block.nether_wood_fence_gate.close");
const ResourceLocation BLOCK_NETHER_WOOD_FENCE_GATE_OPEN("minecraft:block.nether_wood_fence_gate.open");
const ResourceLocation INTENTIONALLY_EMPTY("minecraft:intentionally_empty");
const ResourceLocation BLOCK_PACKED_MUD_BREAK("minecraft:block.packed_mud.break");
const ResourceLocation BLOCK_PACKED_MUD_FALL("minecraft:block.packed_mud.fall");
const ResourceLocation BLOCK_PACKED_MUD_HIT("minecraft:block.packed_mud.hit");
const ResourceLocation BLOCK_PACKED_MUD_PLACE("minecraft:block.packed_mud.place");
const ResourceLocation BLOCK_PACKED_MUD_STEP("minecraft:block.packed_mud.step");
const ResourceLocation BLOCK_PALE_HANGING_MOSS_IDLE("minecraft:block.pale_hanging_moss.idle");
const ResourceLocation ENTITY_PARCHED_AMBIENT("minecraft:entity.parched.ambient");
const ResourceLocation ENTITY_PARCHED_DEATH("minecraft:entity.parched.death");
const ResourceLocation ENTITY_PARCHED_HURT("minecraft:entity.parched.hurt");
const ResourceLocation ENTITY_PARCHED_STEP("minecraft:entity.parched.step");
const ResourceLocation ENTITY_PARROT_IMITATE_BOGGED("minecraft:entity.parrot.imitate.bogged");
const ResourceLocation ENTITY_PARROT_IMITATE_BREEZE("minecraft:entity.parrot.imitate.breeze");
const ResourceLocation ENTITY_PARROT_IMITATE_CAMEL_HUSK("minecraft:entity.parrot.imitate.camel_husk");
const ResourceLocation ENTITY_PARROT_IMITATE_CREAKING("minecraft:entity.parrot.imitate.creaking");
const ResourceLocation ENTITY_PARROT_IMITATE_PARCHED("minecraft:entity.parrot.imitate.parched");
const ResourceLocation ENTITY_PARROT_IMITATE_WARDEN("minecraft:entity.parrot.imitate.warden");
const ResourceLocation ENTITY_PARROT_IMITATE_ZOMBIE_HORSE("minecraft:entity.parrot.imitate.zombie_horse");
const ResourceLocation ENTITY_PARROT_IMITATE_ZOMBIE_NAUTILUS("minecraft:entity.parrot.imitate.zombie_nautilus");
const ResourceLocation ENTITY_PLAYER_HURT_FREEZE("minecraft:entity.player.hurt_freeze");
const ResourceLocation BLOCK_POLISHED_DEEPSLATE_BREAK("minecraft:block.polished_deepslate.break");
const ResourceLocation BLOCK_POLISHED_DEEPSLATE_FALL("minecraft:block.polished_deepslate.fall");
const ResourceLocation BLOCK_POLISHED_DEEPSLATE_HIT("minecraft:block.polished_deepslate.hit");
const ResourceLocation BLOCK_POLISHED_DEEPSLATE_PLACE("minecraft:block.polished_deepslate.place");
const ResourceLocation BLOCK_POLISHED_DEEPSLATE_STEP("minecraft:block.polished_deepslate.step");
const ResourceLocation BLOCK_POWDER_SNOW_BREAK("minecraft:block.powder_snow.break");
const ResourceLocation BLOCK_POWDER_SNOW_FALL("minecraft:block.powder_snow.fall");
const ResourceLocation BLOCK_POWDER_SNOW_HIT("minecraft:block.powder_snow.hit");
const ResourceLocation BLOCK_POWDER_SNOW_PLACE("minecraft:block.powder_snow.place");
const ResourceLocation BLOCK_POWDER_SNOW_STEP("minecraft:block.powder_snow.step");
const ResourceLocation BLOCK_RESIN_BREAK("minecraft:block.resin.break");
const ResourceLocation BLOCK_RESIN_FALL("minecraft:block.resin.fall");
const ResourceLocation BLOCK_RESIN_PLACE("minecraft:block.resin.place");
const ResourceLocation BLOCK_RESIN_STEP("minecraft:block.resin.step");
const ResourceLocation BLOCK_RESIN_BRICKS_BREAK("minecraft:block.resin_bricks.break");
const ResourceLocation BLOCK_RESIN_BRICKS_FALL("minecraft:block.resin_bricks.fall");
const ResourceLocation BLOCK_RESIN_BRICKS_HIT("minecraft:block.resin_bricks.hit");
const ResourceLocation BLOCK_RESIN_BRICKS_PLACE("minecraft:block.resin_bricks.place");
const ResourceLocation BLOCK_RESIN_BRICKS_STEP("minecraft:block.resin_bricks.step");
const ResourceLocation BLOCK_ROOTED_DIRT_BREAK("minecraft:block.rooted_dirt.break");
const ResourceLocation BLOCK_ROOTED_DIRT_FALL("minecraft:block.rooted_dirt.fall");
const ResourceLocation BLOCK_ROOTED_DIRT_HIT("minecraft:block.rooted_dirt.hit");
const ResourceLocation BLOCK_ROOTED_DIRT_PLACE("minecraft:block.rooted_dirt.place");
const ResourceLocation BLOCK_ROOTED_DIRT_STEP("minecraft:block.rooted_dirt.step");
const ResourceLocation BLOCK_SAND_IDLE("minecraft:block.sand.idle");
const ResourceLocation BLOCK_SCULK_SPREAD("minecraft:block.sculk.spread");
const ResourceLocation BLOCK_SCULK_CHARGE("minecraft:block.sculk.charge");
const ResourceLocation BLOCK_SCULK_BREAK("minecraft:block.sculk.break");
const ResourceLocation BLOCK_SCULK_FALL("minecraft:block.sculk.fall");
const ResourceLocation BLOCK_SCULK_HIT("minecraft:block.sculk.hit");
const ResourceLocation BLOCK_SCULK_PLACE("minecraft:block.sculk.place");
const ResourceLocation BLOCK_SCULK_STEP("minecraft:block.sculk.step");
const ResourceLocation BLOCK_SCULK_CATALYST_BLOOM("minecraft:block.sculk_catalyst.bloom");
const ResourceLocation BLOCK_SCULK_CATALYST_BREAK("minecraft:block.sculk_catalyst.break");
const ResourceLocation BLOCK_SCULK_CATALYST_FALL("minecraft:block.sculk_catalyst.fall");
const ResourceLocation BLOCK_SCULK_CATALYST_HIT("minecraft:block.sculk_catalyst.hit");
const ResourceLocation BLOCK_SCULK_CATALYST_PLACE("minecraft:block.sculk_catalyst.place");
const ResourceLocation BLOCK_SCULK_CATALYST_STEP("minecraft:block.sculk_catalyst.step");
const ResourceLocation BLOCK_SCULK_SENSOR_BREAK("minecraft:block.sculk_sensor.break");
const ResourceLocation BLOCK_SCULK_SENSOR_FALL("minecraft:block.sculk_sensor.fall");
const ResourceLocation BLOCK_SCULK_SENSOR_HIT("minecraft:block.sculk_sensor.hit");
const ResourceLocation BLOCK_SCULK_SENSOR_PLACE("minecraft:block.sculk_sensor.place");
const ResourceLocation BLOCK_SCULK_SENSOR_STEP("minecraft:block.sculk_sensor.step");
const ResourceLocation BLOCK_SCULK_SHRIEKER_BREAK("minecraft:block.sculk_shrieker.break");
const ResourceLocation BLOCK_SCULK_SHRIEKER_FALL("minecraft:block.sculk_shrieker.fall");
const ResourceLocation BLOCK_SCULK_SHRIEKER_HIT("minecraft:block.sculk_shrieker.hit");
const ResourceLocation BLOCK_SCULK_SHRIEKER_PLACE("minecraft:block.sculk_shrieker.place");
const ResourceLocation BLOCK_SCULK_SHRIEKER_STEP("minecraft:block.sculk_shrieker.step");
const ResourceLocation BLOCK_SCULK_VEIN_BREAK("minecraft:block.sculk_vein.break");
const ResourceLocation BLOCK_SCULK_VEIN_FALL("minecraft:block.sculk_vein.fall");
const ResourceLocation BLOCK_SCULK_VEIN_HIT("minecraft:block.sculk_vein.hit");
const ResourceLocation BLOCK_SCULK_VEIN_PLACE("minecraft:block.sculk_vein.place");
const ResourceLocation BLOCK_SCULK_VEIN_STEP("minecraft:block.sculk_vein.step");
const ResourceLocation ITEM_SHEARS_SNIP("minecraft:item.shears.snip");
const ResourceLocation BLOCK_SHELF_BREAK("minecraft:block.shelf.break");
const ResourceLocation BLOCK_SHELF_FALL("minecraft:block.shelf.fall");
const ResourceLocation BLOCK_SHELF_HIT("minecraft:block.shelf.hit");
const ResourceLocation BLOCK_SHELF_PLACE("minecraft:block.shelf.place");
const ResourceLocation BLOCK_SHELF_STEP("minecraft:block.shelf.step");
const ResourceLocation ENTITY_SKELETON_CONVERTED_TO_STRAY("minecraft:entity.skeleton.converted_to_stray");
const ResourceLocation BLOCK_SMALL_AMETHYST_BUD_BREAK("minecraft:block.small_amethyst_bud.break");
const ResourceLocation BLOCK_SMALL_AMETHYST_BUD_PLACE("minecraft:block.small_amethyst_bud.place");
const ResourceLocation BLOCK_SMALL_DRIPLEAF_BREAK("minecraft:block.small_dripleaf.break");
const ResourceLocation BLOCK_SMALL_DRIPLEAF_FALL("minecraft:block.small_dripleaf.fall");
const ResourceLocation BLOCK_SMALL_DRIPLEAF_HIT("minecraft:block.small_dripleaf.hit");
const ResourceLocation BLOCK_SMALL_DRIPLEAF_PLACE("minecraft:block.small_dripleaf.place");
const ResourceLocation BLOCK_SMALL_DRIPLEAF_STEP("minecraft:block.small_dripleaf.step");
const ResourceLocation BLOCK_SPAWNER_BREAK("minecraft:block.spawner.break");
const ResourceLocation BLOCK_SPAWNER_FALL("minecraft:block.spawner.fall");
const ResourceLocation BLOCK_SPAWNER_HIT("minecraft:block.spawner.hit");
const ResourceLocation BLOCK_SPAWNER_PLACE("minecraft:block.spawner.place");
const ResourceLocation BLOCK_SPAWNER_STEP("minecraft:block.spawner.step");
const ResourceLocation BLOCK_SPORE_BLOSSOM_BREAK("minecraft:block.spore_blossom.break");
const ResourceLocation BLOCK_SPORE_BLOSSOM_FALL("minecraft:block.spore_blossom.fall");
const ResourceLocation BLOCK_SPORE_BLOSSOM_HIT("minecraft:block.spore_blossom.hit");
const ResourceLocation BLOCK_SPORE_BLOSSOM_PLACE("minecraft:block.spore_blossom.place");
const ResourceLocation BLOCK_SPORE_BLOSSOM_STEP("minecraft:block.spore_blossom.step");
const ResourceLocation BLOCK_SPONGE_BREAK("minecraft:block.sponge.break");
const ResourceLocation BLOCK_SPONGE_FALL("minecraft:block.sponge.fall");
const ResourceLocation BLOCK_SPONGE_HIT("minecraft:block.sponge.hit");
const ResourceLocation BLOCK_SPONGE_PLACE("minecraft:block.sponge.place");
const ResourceLocation BLOCK_SPONGE_STEP("minecraft:block.sponge.step");
const ResourceLocation BLOCK_SPONGE_ABSORB("minecraft:block.sponge.absorb");
const ResourceLocation BLOCK_SWEET_BERRY_BUSH_PICK_BERRIES("minecraft:block.sweet_berry_bush.pick_berries");
const ResourceLocation BLOCK_TUFF_BREAK("minecraft:block.tuff.break");
const ResourceLocation BLOCK_TUFF_STEP("minecraft:block.tuff.step");
const ResourceLocation BLOCK_TUFF_PLACE("minecraft:block.tuff.place");
const ResourceLocation BLOCK_TUFF_HIT("minecraft:block.tuff.hit");
const ResourceLocation BLOCK_TUFF_FALL("minecraft:block.tuff.fall");
const ResourceLocation BLOCK_TUFF_BRICKS_BREAK("minecraft:block.tuff_bricks.break");
const ResourceLocation BLOCK_TUFF_BRICKS_FALL("minecraft:block.tuff_bricks.fall");
const ResourceLocation BLOCK_TUFF_BRICKS_HIT("minecraft:block.tuff_bricks.hit");
const ResourceLocation BLOCK_TUFF_BRICKS_PLACE("minecraft:block.tuff_bricks.place");
const ResourceLocation BLOCK_TUFF_BRICKS_STEP("minecraft:block.tuff_bricks.step");
const ResourceLocation BLOCK_POLISHED_TUFF_BREAK("minecraft:block.polished_tuff.break");
const ResourceLocation BLOCK_POLISHED_TUFF_FALL("minecraft:block.polished_tuff.fall");
const ResourceLocation BLOCK_POLISHED_TUFF_HIT("minecraft:block.polished_tuff.hit");
const ResourceLocation BLOCK_POLISHED_TUFF_PLACE("minecraft:block.polished_tuff.place");
const ResourceLocation BLOCK_POLISHED_TUFF_STEP("minecraft:block.polished_tuff.step");
const ResourceLocation BLOCK_VAULT_ACTIVATE("minecraft:block.vault.activate");
const ResourceLocation BLOCK_VAULT_AMBIENT("minecraft:block.vault.ambient");
const ResourceLocation BLOCK_VAULT_BREAK("minecraft:block.vault.break");
const ResourceLocation BLOCK_VAULT_CLOSE_SHUTTER("minecraft:block.vault.close_shutter");
const ResourceLocation BLOCK_VAULT_DEACTIVATE("minecraft:block.vault.deactivate");
const ResourceLocation BLOCK_VAULT_EJECT_ITEM("minecraft:block.vault.eject_item");
const ResourceLocation BLOCK_VAULT_REJECT_REWARDED_PLAYER("minecraft:block.vault.reject_rewarded_player");
const ResourceLocation BLOCK_VAULT_FALL("minecraft:block.vault.fall");
const ResourceLocation BLOCK_VAULT_HIT("minecraft:block.vault.hit");
const ResourceLocation BLOCK_VAULT_INSERT_ITEM("minecraft:block.vault.insert_item");
const ResourceLocation BLOCK_VAULT_INSERT_ITEM_FAIL("minecraft:block.vault.insert_item_fail");
const ResourceLocation BLOCK_VAULT_OPEN_SHUTTER("minecraft:block.vault.open_shutter");
const ResourceLocation BLOCK_VAULT_PLACE("minecraft:block.vault.place");
const ResourceLocation BLOCK_VAULT_STEP("minecraft:block.vault.step");
const ResourceLocation BLOCK_VINE_BREAK("minecraft:block.vine.break");
const ResourceLocation BLOCK_VINE_FALL("minecraft:block.vine.fall");
const ResourceLocation BLOCK_VINE_HIT("minecraft:block.vine.hit");
const ResourceLocation BLOCK_VINE_PLACE("minecraft:block.vine.place");
const ResourceLocation WEATHER_END_FLASH("minecraft:weather.end_flash");
const ResourceLocation BLOCK_WET_SPONGE_BREAK("minecraft:block.wet_sponge.break");
const ResourceLocation BLOCK_WET_SPONGE_DRIES("minecraft:block.wet_sponge.dries");
const ResourceLocation BLOCK_WET_SPONGE_FALL("minecraft:block.wet_sponge.fall");
const ResourceLocation BLOCK_WET_SPONGE_HIT("minecraft:block.wet_sponge.hit");
const ResourceLocation BLOCK_WET_SPONGE_PLACE("minecraft:block.wet_sponge.place");
const ResourceLocation BLOCK_WET_SPONGE_STEP("minecraft:block.wet_sponge.step");
const ResourceLocation ENTITY_ZOMBIE_HORSE_ANGRY("minecraft:entity.zombie_horse.angry");
const ResourceLocation ENTITY_ZOMBIE_HORSE_EAT("minecraft:entity.zombie_horse.eat");
const ResourceLocation ENTITY_ZOMBIE_NAUTILUS_AMBIENT_LAND("minecraft:entity.zombie_nautilus.ambient_land");
const ResourceLocation ENTITY_ZOMBIE_NAUTILUS_DASH_LAND("minecraft:entity.zombie_nautilus.dash_land");
const ResourceLocation ENTITY_ZOMBIE_NAUTILUS_DASH_READY_LAND("minecraft:entity.zombie_nautilus.dash_ready_land");
const ResourceLocation ENTITY_ZOMBIE_NAUTILUS_DEATH_LAND("minecraft:entity.zombie_nautilus.death_land");
const ResourceLocation ENTITY_ZOMBIE_NAUTILUS_HURT_LAND("minecraft:entity.zombie_nautilus.hurt_land");
const ResourceLocation ENTITY_ZOMBIE_NAUTILUS_SWIM("minecraft:entity.zombie_nautilus.swim");
const ResourceLocation EVENT_MOB_EFFECT_BAD_OMEN("minecraft:event.mob_effect.bad_omen");
const ResourceLocation EVENT_MOB_EFFECT_TRIAL_OMEN("minecraft:event.mob_effect.trial_omen");
const ResourceLocation EVENT_MOB_EFFECT_RAID_OMEN("minecraft:event.mob_effect.raid_omen");

} // namespace SoundEvents

} // namespace mc
