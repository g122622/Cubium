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
// 参考: net.minecraft.sounds.SoundEvents
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
// 参考: net.minecraft.sounds.SoundEvents
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
// 初始化
// ============================================================================

void initialize()
{
    // 声音事件已通过静态初始化创建
    // 此函数可用于验证所有声音事件已正确初始化
}

} // namespace SoundEvents

} // namespace mc
