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

#pragma once

#include "common/resource/ResourceLocation.hpp"

namespace mc {

/**
 * @brief 声音事件常量
 *
 * 包含所有 Minecraft 1.16.5 原版声音事件的资源位置。
 * 参考: net.minecraft.util.SoundEvents
 *
 * 使用示例:
 * @code
 * player.playSound(SoundEvents::ENTITY_GENERIC_EAT, 1.0f, 1.0f);
 * @endcode
 */
namespace SoundEvents {

// ============================================================================
// 环境音效 (AMBIENT_)
// ============================================================================

/// 洞穴环境音
extern const ResourceLocation AMBIENT_CAVE;

/// 下界玄武岩三角洲环境音
extern const ResourceLocation AMBIENT_BASALT_DELTAS_ADDITIONS;
extern const ResourceLocation AMBIENT_BASALT_DELTAS_LOOP;
extern const ResourceLocation AMBIENT_BASALT_DELTAS_MOOD;

/// 下界绯红森林环境音
extern const ResourceLocation AMBIENT_CRIMSON_FOREST_ADDITIONS;
extern const ResourceLocation AMBIENT_CRIMSON_FOREST_LOOP;
extern const ResourceLocation AMBIENT_CRIMSON_FOREST_MOOD;

/// 下界荒地环境音
extern const ResourceLocation AMBIENT_NETHER_WASTES_ADDITIONS;
extern const ResourceLocation AMBIENT_NETHER_WASTES_LOOP;
extern const ResourceLocation AMBIENT_NETHER_WASTES_MOOD;

/// 下界灵魂沙峡谷环境音
extern const ResourceLocation AMBIENT_SOUL_SAND_VALLEY_ADDITIONS;
extern const ResourceLocation AMBIENT_SOUL_SAND_VALLEY_LOOP;
extern const ResourceLocation AMBIENT_SOUL_SAND_VALLEY_MOOD;

/// 下界扭曲森林环境音
extern const ResourceLocation AMBIENT_WARPED_FOREST_ADDITIONS;
extern const ResourceLocation AMBIENT_WARPED_FOREST_LOOP;
extern const ResourceLocation AMBIENT_WARPED_FOREST_MOOD;

/// 水下环境音
extern const ResourceLocation AMBIENT_UNDERWATER_ENTER;
extern const ResourceLocation AMBIENT_UNDERWATER_EXIT;
extern const ResourceLocation AMBIENT_UNDERWATER_LOOP;
extern const ResourceLocation AMBIENT_UNDERWATER_LOOP_ADDITIONS;
extern const ResourceLocation AMBIENT_UNDERWATER_LOOP_ADDITIONS_RARE;
extern const ResourceLocation AMBIENT_UNDERWATER_LOOP_ADDITIONS_ULTRA_RARE;

// ============================================================================
// 方块音效 (BLOCK_)
// ============================================================================

/// 基础方块音效（break/fall/hit/place/step）
extern const ResourceLocation BLOCK_STONE_BREAK;
extern const ResourceLocation BLOCK_STONE_FALL;
extern const ResourceLocation BLOCK_STONE_HIT;
extern const ResourceLocation BLOCK_STONE_PLACE;
extern const ResourceLocation BLOCK_STONE_STEP;

extern const ResourceLocation BLOCK_GRASS_BREAK;
extern const ResourceLocation BLOCK_GRASS_FALL;
extern const ResourceLocation BLOCK_GRASS_HIT;
extern const ResourceLocation BLOCK_GRASS_PLACE;
extern const ResourceLocation BLOCK_GRASS_STEP;

extern const ResourceLocation BLOCK_GRAVEL_BREAK;
extern const ResourceLocation BLOCK_GRAVEL_FALL;
extern const ResourceLocation BLOCK_GRAVEL_HIT;
extern const ResourceLocation BLOCK_GRAVEL_PLACE;
extern const ResourceLocation BLOCK_GRAVEL_STEP;

extern const ResourceLocation BLOCK_SAND_BREAK;
extern const ResourceLocation BLOCK_SAND_FALL;
extern const ResourceLocation BLOCK_SAND_HIT;
extern const ResourceLocation BLOCK_SAND_PLACE;
extern const ResourceLocation BLOCK_SAND_STEP;

extern const ResourceLocation BLOCK_GLASS_BREAK;
extern const ResourceLocation BLOCK_GLASS_FALL;
extern const ResourceLocation BLOCK_GLASS_HIT;
extern const ResourceLocation BLOCK_GLASS_PLACE;
extern const ResourceLocation BLOCK_GLASS_STEP;

extern const ResourceLocation BLOCK_WOOD_BREAK;
extern const ResourceLocation BLOCK_WOOD_FALL;
extern const ResourceLocation BLOCK_WOOD_HIT;
extern const ResourceLocation BLOCK_WOOD_PLACE;
extern const ResourceLocation BLOCK_WOOD_STEP;

extern const ResourceLocation BLOCK_WOOL_BREAK;
extern const ResourceLocation BLOCK_WOOL_FALL;
extern const ResourceLocation BLOCK_WOOL_HIT;
extern const ResourceLocation BLOCK_WOOL_PLACE;
extern const ResourceLocation BLOCK_WOOL_STEP;

extern const ResourceLocation BLOCK_METAL_BREAK;
extern const ResourceLocation BLOCK_METAL_FALL;
extern const ResourceLocation BLOCK_METAL_HIT;
extern const ResourceLocation BLOCK_METAL_PLACE;
extern const ResourceLocation BLOCK_METAL_STEP;

extern const ResourceLocation BLOCK_SNOW_BREAK;
extern const ResourceLocation BLOCK_SNOW_FALL;
extern const ResourceLocation BLOCK_SNOW_HIT;
extern const ResourceLocation BLOCK_SNOW_PLACE;
extern const ResourceLocation BLOCK_SNOW_STEP;

/// 下界方块音效
extern const ResourceLocation BLOCK_ANCIENT_DEBRIS_BREAK;
extern const ResourceLocation BLOCK_ANCIENT_DEBRIS_FALL;
extern const ResourceLocation BLOCK_ANCIENT_DEBRIS_HIT;
extern const ResourceLocation BLOCK_ANCIENT_DEBRIS_PLACE;
extern const ResourceLocation BLOCK_ANCIENT_DEBRIS_STEP;

extern const ResourceLocation BLOCK_BASALT_BREAK;
extern const ResourceLocation BLOCK_BASALT_FALL;
extern const ResourceLocation BLOCK_BASALT_HIT;
extern const ResourceLocation BLOCK_BASALT_PLACE;
extern const ResourceLocation BLOCK_BASALT_STEP;

extern const ResourceLocation BLOCK_BONE_BLOCK_BREAK;
extern const ResourceLocation BLOCK_BONE_BLOCK_FALL;
extern const ResourceLocation BLOCK_BONE_BLOCK_HIT;
extern const ResourceLocation BLOCK_BONE_BLOCK_PLACE;
extern const ResourceLocation BLOCK_BONE_BLOCK_STEP;

extern const ResourceLocation BLOCK_NETHER_BRICKS_BREAK;
extern const ResourceLocation BLOCK_NETHER_BRICKS_FALL;
extern const ResourceLocation BLOCK_NETHER_BRICKS_HIT;
extern const ResourceLocation BLOCK_NETHER_BRICKS_PLACE;
extern const ResourceLocation BLOCK_NETHER_BRICKS_STEP;

extern const ResourceLocation BLOCK_NETHER_GOLD_ORE_BREAK;
extern const ResourceLocation BLOCK_NETHER_GOLD_ORE_FALL;
extern const ResourceLocation BLOCK_NETHER_GOLD_ORE_HIT;
extern const ResourceLocation BLOCK_NETHER_GOLD_ORE_PLACE;
extern const ResourceLocation BLOCK_NETHER_GOLD_ORE_STEP;

extern const ResourceLocation BLOCK_NETHER_ORE_BREAK;
extern const ResourceLocation BLOCK_NETHER_ORE_FALL;
extern const ResourceLocation BLOCK_NETHER_ORE_HIT;
extern const ResourceLocation BLOCK_NETHER_ORE_PLACE;
extern const ResourceLocation BLOCK_NETHER_ORE_STEP;

extern const ResourceLocation BLOCK_NETHERITE_BLOCK_BREAK;
extern const ResourceLocation BLOCK_NETHERITE_BLOCK_FALL;
extern const ResourceLocation BLOCK_NETHERITE_BLOCK_HIT;
extern const ResourceLocation BLOCK_NETHERITE_BLOCK_PLACE;
extern const ResourceLocation BLOCK_NETHERITE_BLOCK_STEP;

extern const ResourceLocation BLOCK_NETHERRACK_BREAK;
extern const ResourceLocation BLOCK_NETHERRACK_FALL;
extern const ResourceLocation BLOCK_NETHERRACK_HIT;
extern const ResourceLocation BLOCK_NETHERRACK_PLACE;
extern const ResourceLocation BLOCK_NETHERRACK_STEP;

extern const ResourceLocation BLOCK_NYLIUM_BREAK;
extern const ResourceLocation BLOCK_NYLIUM_FALL;
extern const ResourceLocation BLOCK_NYLIUM_HIT;
extern const ResourceLocation BLOCK_NYLIUM_PLACE;
extern const ResourceLocation BLOCK_NYLIUM_STEP;

extern const ResourceLocation BLOCK_SOUL_SAND_BREAK;
extern const ResourceLocation BLOCK_SOUL_SAND_FALL;
extern const ResourceLocation BLOCK_SOUL_SAND_HIT;
extern const ResourceLocation BLOCK_SOUL_SAND_PLACE;
extern const ResourceLocation BLOCK_SOUL_SAND_STEP;

extern const ResourceLocation BLOCK_SOUL_SOIL_BREAK;
extern const ResourceLocation BLOCK_SOUL_SOIL_FALL;
extern const ResourceLocation BLOCK_SOUL_SOIL_HIT;
extern const ResourceLocation BLOCK_SOUL_SOIL_PLACE;
extern const ResourceLocation BLOCK_SOUL_SOIL_STEP;

extern const ResourceLocation BLOCK_LODESTONE_BREAK;
extern const ResourceLocation BLOCK_LODESTONE_FALL;
extern const ResourceLocation BLOCK_LODESTONE_HIT;
extern const ResourceLocation BLOCK_LODESTONE_PLACE;
extern const ResourceLocation BLOCK_LODESTONE_STEP;

extern const ResourceLocation BLOCK_GILDED_BLACKSTONE_BREAK;
extern const ResourceLocation BLOCK_GILDED_BLACKSTONE_FALL;
extern const ResourceLocation BLOCK_GILDED_BLACKSTONE_HIT;
extern const ResourceLocation BLOCK_GILDED_BLACKSTONE_PLACE;
extern const ResourceLocation BLOCK_GILDED_BLACKSTONE_STEP;

extern const ResourceLocation BLOCK_SHROOMLIGHT_BREAK;
extern const ResourceLocation BLOCK_SHROOMLIGHT_FALL;
extern const ResourceLocation BLOCK_SHROOMLIGHT_HIT;
extern const ResourceLocation BLOCK_SHROOMLIGHT_PLACE;
extern const ResourceLocation BLOCK_SHROOMLIGHT_STEP;

extern const ResourceLocation BLOCK_WART_BLOCK_BREAK;
extern const ResourceLocation BLOCK_WART_BLOCK_FALL;
extern const ResourceLocation BLOCK_WART_BLOCK_HIT;
extern const ResourceLocation BLOCK_WART_BLOCK_PLACE;
extern const ResourceLocation BLOCK_WART_BLOCK_STEP;

extern const ResourceLocation BLOCK_STEM_BREAK;
extern const ResourceLocation BLOCK_STEM_FALL;
extern const ResourceLocation BLOCK_STEM_HIT;
extern const ResourceLocation BLOCK_STEM_PLACE;
extern const ResourceLocation BLOCK_STEM_STEP;

extern const ResourceLocation BLOCK_FUNGUS_BREAK;
extern const ResourceLocation BLOCK_FUNGUS_FALL;
extern const ResourceLocation BLOCK_FUNGUS_HIT;
extern const ResourceLocation BLOCK_FUNGUS_PLACE;
extern const ResourceLocation BLOCK_FUNGUS_STEP;

extern const ResourceLocation BLOCK_ROOTS_BREAK;
extern const ResourceLocation BLOCK_ROOTS_FALL;
extern const ResourceLocation BLOCK_ROOTS_HIT;
extern const ResourceLocation BLOCK_ROOTS_PLACE;
extern const ResourceLocation BLOCK_ROOTS_STEP;

extern const ResourceLocation BLOCK_NETHER_SPROUTS_BREAK;
extern const ResourceLocation BLOCK_NETHER_SPROUTS_FALL;
extern const ResourceLocation BLOCK_NETHER_SPROUTS_HIT;
extern const ResourceLocation BLOCK_NETHER_SPROUTS_PLACE;
extern const ResourceLocation BLOCK_NETHER_SPROUTS_STEP;

extern const ResourceLocation BLOCK_WEEPING_VINES_BREAK;
extern const ResourceLocation BLOCK_WEEPING_VINES_FALL;
extern const ResourceLocation BLOCK_WEEPING_VINES_HIT;
extern const ResourceLocation BLOCK_WEEPING_VINES_PLACE;
extern const ResourceLocation BLOCK_WEEPING_VINES_STEP;

/// 其他方块音效
extern const ResourceLocation BLOCK_BAMBOO_BREAK;
extern const ResourceLocation BLOCK_BAMBOO_FALL;
extern const ResourceLocation BLOCK_BAMBOO_HIT;
extern const ResourceLocation BLOCK_BAMBOO_PLACE;
extern const ResourceLocation BLOCK_BAMBOO_STEP;
extern const ResourceLocation BLOCK_BAMBOO_SAPLING_BREAK;
extern const ResourceLocation BLOCK_BAMBOO_SAPLING_HIT;
extern const ResourceLocation BLOCK_BAMBOO_SAPLING_PLACE;

extern const ResourceLocation BLOCK_WET_GRASS_BREAK;
extern const ResourceLocation BLOCK_WET_GRASS_FALL;
extern const ResourceLocation BLOCK_WET_GRASS_HIT;
extern const ResourceLocation BLOCK_WET_GRASS_PLACE;
extern const ResourceLocation BLOCK_WET_GRASS_STEP;

extern const ResourceLocation BLOCK_VINE_STEP;

extern const ResourceLocation BLOCK_CORAL_BLOCK_BREAK;
extern const ResourceLocation BLOCK_CORAL_BLOCK_FALL;
extern const ResourceLocation BLOCK_CORAL_BLOCK_HIT;
extern const ResourceLocation BLOCK_CORAL_BLOCK_PLACE;
extern const ResourceLocation BLOCK_CORAL_BLOCK_STEP;

extern const ResourceLocation BLOCK_CROP_BREAK;
extern const ResourceLocation BLOCK_NETHER_WART_BREAK;
extern const ResourceLocation BLOCK_SWEET_BERRY_BUSH_BREAK;
extern const ResourceLocation BLOCK_SWEET_BERRY_BUSH_PLACE;
extern const ResourceLocation BLOCK_CAVE_VINES_PICK_BERRIES;
extern const ResourceLocation BLOCK_LILY_PAD_PLACE;

/// 大滴叶倾斜音效
extern const ResourceLocation BLOCK_BIG_DRIPLEAF_TILT_DOWN;
extern const ResourceLocation BLOCK_BIG_DRIPLEAF_TILT_UP;

/// 滴水石音效
extern const ResourceLocation BLOCK_POINTED_DRIPSTONE_HIT;
extern const ResourceLocation BLOCK_POINTED_DRIPSTONE_FALL;
extern const ResourceLocation BLOCK_POINTED_DRIPSTONE_LAND;
extern const ResourceLocation BLOCK_POINTED_DRIPSTONE_DRIP_WATER;
extern const ResourceLocation BLOCK_POINTED_DRIPSTONE_DRIP_LAVA;
extern const ResourceLocation BLOCK_POINTED_DRIPSTONE_DRIP_WATER_INTO_CAULDRON;
extern const ResourceLocation BLOCK_POINTED_DRIPSTONE_DRIP_LAVA_INTO_CAULDRON;

extern const ResourceLocation BLOCK_WATER_AMBIENT;

extern const ResourceLocation BLOCK_BARREL_CLOSE;
extern const ResourceLocation BLOCK_BARREL_OPEN;
extern const ResourceLocation BLOCK_CHEST_LOCKED;
extern const ResourceLocation BLOCK_CHORUS_FLOWER_DEATH;
extern const ResourceLocation BLOCK_CHORUS_FLOWER_GROW;
extern const ResourceLocation BLOCK_COMPOSTER_EMPTY;
extern const ResourceLocation BLOCK_COMPOSTER_FILL;
extern const ResourceLocation BLOCK_COMPOSTER_FILL_SUCCESS;
extern const ResourceLocation BLOCK_COMPOSTER_READY;

/// 合成器
extern const ResourceLocation BLOCK_CRAFTER_CRAFT;
extern const ResourceLocation BLOCK_CRAFTER_FAIL;
extern const ResourceLocation BLOCK_FURNACE_FIRE_CRACKLE;
extern const ResourceLocation BLOCK_LEVER_CLICK;
extern const ResourceLocation BLOCK_PUMPKIN_CARVE;
extern const ResourceLocation BLOCK_TRIPWIRE_ATTACH;
extern const ResourceLocation BLOCK_TRIPWIRE_CLICK_OFF;
extern const ResourceLocation BLOCK_TRIPWIRE_CLICK_ON;
extern const ResourceLocation BLOCK_TRIPWIRE_DETACH;

/// 木门
extern const ResourceLocation BLOCK_WOODEN_DOOR_OPEN;
extern const ResourceLocation BLOCK_WOODEN_DOOR_CLOSE;

/// 铁门
extern const ResourceLocation BLOCK_IRON_DOOR_OPEN;
extern const ResourceLocation BLOCK_IRON_DOOR_CLOSE;

/// 栅栏门
extern const ResourceLocation BLOCK_FENCE_GATE_OPEN;
extern const ResourceLocation BLOCK_FENCE_GATE_CLOSE;

/// 活板门
extern const ResourceLocation BLOCK_WOODEN_TRAPDOOR_OPEN;
extern const ResourceLocation BLOCK_WOODEN_TRAPDOOR_CLOSE;
extern const ResourceLocation BLOCK_IRON_TRAPDOOR_OPEN;
extern const ResourceLocation BLOCK_IRON_TRAPDOOR_CLOSE;

/// 箱子
extern const ResourceLocation BLOCK_CHEST_OPEN;
extern const ResourceLocation BLOCK_CHEST_CLOSE;
extern const ResourceLocation BLOCK_ENDER_CHEST_OPEN;
extern const ResourceLocation BLOCK_ENDER_CHEST_CLOSE;
extern const ResourceLocation BLOCK_SHULKER_BOX_OPEN;
extern const ResourceLocation BLOCK_SHULKER_BOX_CLOSE;

/// 铜箱子开合音效（MC 1.21.11）
// Unaffected 与 Exposed 等级共用 block.copper_chest.open/close
// Weathered 等级使用 block.copper_chest_weathered.open/close
// Oxidized 等级使用 block.copper_chest_oxidized.open/close
// 涂蜡变体复用对应氧化等级的声音事件
extern const ResourceLocation BLOCK_COPPER_CHEST_OPEN;
extern const ResourceLocation BLOCK_COPPER_CHEST_CLOSE;
extern const ResourceLocation BLOCK_COPPER_CHEST_WEATHERED_OPEN;
extern const ResourceLocation BLOCK_COPPER_CHEST_WEATHERED_CLOSE;
extern const ResourceLocation BLOCK_COPPER_CHEST_OXIDIZED_OPEN;
extern const ResourceLocation BLOCK_COPPER_CHEST_OXIDIZED_CLOSE;

/// 活塞
extern const ResourceLocation BLOCK_PISTON_EXTEND;
extern const ResourceLocation BLOCK_PISTON_CONTRACT;

/// 按钮
extern const ResourceLocation BLOCK_STONE_BUTTON_CLICK_ON;
extern const ResourceLocation BLOCK_STONE_BUTTON_CLICK_OFF;
extern const ResourceLocation BLOCK_WOODEN_BUTTON_CLICK_ON;
extern const ResourceLocation BLOCK_WOODEN_BUTTON_CLICK_OFF;

/// 压力板
extern const ResourceLocation BLOCK_STONE_PRESSURE_PLATE_CLICK_ON;
extern const ResourceLocation BLOCK_STONE_PRESSURE_PLATE_CLICK_OFF;
extern const ResourceLocation BLOCK_WOODEN_PRESSURE_PLATE_CLICK_ON;
extern const ResourceLocation BLOCK_WOODEN_PRESSURE_PLATE_CLICK_OFF;
extern const ResourceLocation BLOCK_METAL_PRESSURE_PLATE_CLICK_ON;
extern const ResourceLocation BLOCK_METAL_PRESSURE_PLATE_CLICK_OFF;

/// 音符盒
extern const ResourceLocation BLOCK_NOTE_BLOCK_BASS;
extern const ResourceLocation BLOCK_NOTE_BLOCK_SNARE;
extern const ResourceLocation BLOCK_NOTE_BLOCK_HAT;
extern const ResourceLocation BLOCK_NOTE_BLOCK_BASEDRUM;
extern const ResourceLocation BLOCK_NOTE_BLOCK_BELL;
extern const ResourceLocation BLOCK_NOTE_BLOCK_FLUTE;
extern const ResourceLocation BLOCK_NOTE_BLOCK_CHIME;
extern const ResourceLocation BLOCK_NOTE_BLOCK_GUITAR;
extern const ResourceLocation BLOCK_NOTE_BLOCK_XYLOPHONE;
extern const ResourceLocation BLOCK_NOTE_BLOCK_IRON_XYLOPHONE;
extern const ResourceLocation BLOCK_NOTE_BLOCK_COW_BELL;
extern const ResourceLocation BLOCK_NOTE_BLOCK_DIDGERIDOO;
extern const ResourceLocation BLOCK_NOTE_BLOCK_BIT;
extern const ResourceLocation BLOCK_NOTE_BLOCK_BANJO;
extern const ResourceLocation BLOCK_NOTE_BLOCK_PLING;
extern const ResourceLocation BLOCK_NOTE_BLOCK_HARP;

/// 火和岩浆
extern const ResourceLocation BLOCK_FIRE_AMBIENT;
extern const ResourceLocation BLOCK_FIRE_EXTINGUISH;
extern const ResourceLocation BLOCK_LAVA_AMBIENT;
extern const ResourceLocation BLOCK_LAVA_EXTINGUISH;
extern const ResourceLocation BLOCK_LAVA_POP;

/// 传送门
extern const ResourceLocation BLOCK_PORTAL_AMBIENT;
extern const ResourceLocation BLOCK_PORTAL_TRAVEL;
extern const ResourceLocation BLOCK_PORTAL_TRIGGER;

/// 末地传送门
extern const ResourceLocation BLOCK_END_PORTAL_FRAME_FILL;
extern const ResourceLocation BLOCK_END_PORTAL_SPAWN;
extern const ResourceLocation BLOCK_END_GATEWAY_SPAWN;

/// 信标
extern const ResourceLocation BLOCK_BEACON_ACTIVATE;
extern const ResourceLocation BLOCK_BEACON_AMBIENT;
extern const ResourceLocation BLOCK_BEACON_DEACTIVATE;
extern const ResourceLocation BLOCK_BEACON_POWER_SELECT;

/// 酿造台
extern const ResourceLocation BLOCK_BREWING_STAND_BREW;

/// 铁砧
extern const ResourceLocation BLOCK_ANVIL_BREAK;
extern const ResourceLocation BLOCK_ANVIL_DESTROY;
extern const ResourceLocation BLOCK_ANVIL_FALL;
extern const ResourceLocation BLOCK_ANVIL_HIT;
extern const ResourceLocation BLOCK_ANVIL_LAND;
extern const ResourceLocation BLOCK_ANVIL_PLACE;
extern const ResourceLocation BLOCK_ANVIL_STEP;
extern const ResourceLocation BLOCK_ANVIL_USE;

/// 营火
extern const ResourceLocation BLOCK_CAMPFIRE_CRACKLE;
extern const ResourceLocation BLOCK_CAMPFIRE_EXTINGUISH;

/// 蜡烛
extern const ResourceLocation BLOCK_CANDLE_AMBIENT;
extern const ResourceLocation BLOCK_CANDLE_BREAK;
extern const ResourceLocation BLOCK_CANDLE_EXTINGUISH;
extern const ResourceLocation BLOCK_CANDLE_HIT;
extern const ResourceLocation BLOCK_CANDLE_PLACE;
extern const ResourceLocation BLOCK_CANDLE_STEP;

/// 蜂箱
extern const ResourceLocation BLOCK_BEEHIVE_DRIP;
extern const ResourceLocation BLOCK_BEEHIVE_ENTER;
extern const ResourceLocation BLOCK_BEEHIVE_EXIT;
extern const ResourceLocation BLOCK_BEEHIVE_SHEAR;
extern const ResourceLocation BLOCK_BEEHIVE_WORK;

/// 告示牌
extern const ResourceLocation BLOCK_SIGN_WAXED_INTERACT_FAIL;
extern const ResourceLocation BLOCK_HANGING_SIGN_WAXED_INTERACT_FAIL;

/// 钟
extern const ResourceLocation BLOCK_BELL_USE;
extern const ResourceLocation BLOCK_BELL_RESONATE;

/// 紫水晶
extern const ResourceLocation BLOCK_AMETHYST_BLOCK_CHIME;
extern const ResourceLocation BLOCK_AMETHYST_BLOCK_RESONATE;

/// 研磨台
extern const ResourceLocation BLOCK_GRINDSTONE_USE;

/// 高炉和烟熏炉
extern const ResourceLocation BLOCK_BLASTFURNACE_FIRE_CRACKLE;
extern const ResourceLocation BLOCK_SMOKER_SMOKE;

/// 工作台
extern const ResourceLocation BLOCK_SMITHING_TABLE_USE;
extern const ResourceLocation BLOCK_ENCHANTMENT_TABLE_USE;

/// 发射器和投掷器
extern const ResourceLocation BLOCK_DISPENSER_DISPENSE;
extern const ResourceLocation BLOCK_DISPENSER_FAIL;
extern const ResourceLocation BLOCK_DISPENSER_LAUNCH;

/// 比较器和红石
extern const ResourceLocation BLOCK_COMPARATOR_CLICK;
extern const ResourceLocation BLOCK_REDSTONE_TORCH_BURNOUT;
extern const ResourceLocation BLOCK_REDSTONE_BREAK;
extern const ResourceLocation BLOCK_REDSTONE_HIT;
extern const ResourceLocation BLOCK_REDSTONE_PLACE;
extern const ResourceLocation BLOCK_REDSTONE_STEP;
extern const ResourceLocation BLOCK_REDSTONE_FALL;

/// 气泡柱
extern const ResourceLocation BLOCK_BUBBLE_COLUMN_BUBBLE_POP;
extern const ResourceLocation BLOCK_BUBBLE_COLUMN_UPWARDS_AMBIENT;
extern const ResourceLocation BLOCK_BUBBLE_COLUMN_UPWARDS_INSIDE;
extern const ResourceLocation BLOCK_BUBBLE_COLUMN_WHIRLPOOL_AMBIENT;
extern const ResourceLocation BLOCK_BUBBLE_COLUMN_WHIRLPOOL_INSIDE;

/// 潮涌核心
extern const ResourceLocation BLOCK_CONDUIT_ACTIVATE;
extern const ResourceLocation BLOCK_CONDUIT_AMBIENT;
extern const ResourceLocation BLOCK_CONDUIT_AMBIENT_SHORT;
extern const ResourceLocation BLOCK_CONDUIT_ATTACK_TARGET;
extern const ResourceLocation BLOCK_CONDUIT_DEACTIVATE;

/// 重生锚
extern const ResourceLocation BLOCK_RESPAWN_ANCHOR_AMBIENT;
extern const ResourceLocation BLOCK_RESPAWN_ANCHOR_CHARGE;
extern const ResourceLocation BLOCK_RESPAWN_ANCHOR_DEPLETE;
extern const ResourceLocation BLOCK_RESPAWN_ANCHOR_SET_SPAWN;

/// 梯子
extern const ResourceLocation BLOCK_LADDER_BREAK;
extern const ResourceLocation BLOCK_LADDER_FALL;
extern const ResourceLocation BLOCK_LADDER_HIT;
extern const ResourceLocation BLOCK_LADDER_PLACE;
extern const ResourceLocation BLOCK_LADDER_STEP;

/// 史莱姆块
extern const ResourceLocation BLOCK_SLIME_BLOCK_BREAK;
extern const ResourceLocation BLOCK_SLIME_BLOCK_FALL;
extern const ResourceLocation BLOCK_SLIME_BLOCK_HIT;
extern const ResourceLocation BLOCK_SLIME_BLOCK_PLACE;
extern const ResourceLocation BLOCK_SLIME_BLOCK_STEP;

/// 蜂蜜块
extern const ResourceLocation BLOCK_HONEY_BLOCK_BREAK;
extern const ResourceLocation BLOCK_HONEY_BLOCK_FALL;
extern const ResourceLocation BLOCK_HONEY_BLOCK_HIT;
extern const ResourceLocation BLOCK_HONEY_BLOCK_PLACE;
extern const ResourceLocation BLOCK_HONEY_BLOCK_SLIDE;
extern const ResourceLocation BLOCK_HONEY_BLOCK_STEP;

/// 脚手架
extern const ResourceLocation BLOCK_SCAFFOLDING_BREAK;
extern const ResourceLocation BLOCK_SCAFFOLDING_FALL;
extern const ResourceLocation BLOCK_SCAFFOLDING_HIT;
extern const ResourceLocation BLOCK_SCAFFOLDING_PLACE;
extern const ResourceLocation BLOCK_SCAFFOLDING_STEP;

/// 书架（Shelf，1.21.4+ 木质变体）
extern const ResourceLocation BLOCK_SHELF_ACTIVATE;
extern const ResourceLocation BLOCK_SHELF_DEACTIVATE;
extern const ResourceLocation BLOCK_SHELF_PLACE_ITEM;
extern const ResourceLocation BLOCK_SHELF_TAKE_ITEM;
extern const ResourceLocation BLOCK_SHELF_SINGLE_SWAP;
extern const ResourceLocation BLOCK_SHELF_MULTI_SWAP;

/// 饰纹陶罐
extern const ResourceLocation BLOCK_DECORATED_POT_INSERT;
extern const ResourceLocation BLOCK_DECORATED_POT_INSERT_FAIL;

/// 灯笼
extern const ResourceLocation BLOCK_LANTERN_BREAK;
extern const ResourceLocation BLOCK_LANTERN_FALL;
extern const ResourceLocation BLOCK_LANTERN_HIT;
extern const ResourceLocation BLOCK_LANTERN_PLACE;
extern const ResourceLocation BLOCK_LANTERN_STEP;

/// 锁链
extern const ResourceLocation BLOCK_CHAIN_BREAK;
extern const ResourceLocation BLOCK_CHAIN_FALL;
extern const ResourceLocation BLOCK_CHAIN_HIT;
extern const ResourceLocation BLOCK_CHAIN_PLACE;
extern const ResourceLocation BLOCK_CHAIN_STEP;

/// 幽匿感测体
extern const ResourceLocation BLOCK_SCULK_SENSOR_CLICKING;
extern const ResourceLocation BLOCK_SCULK_SENSOR_CLICKING_STOP;

/// 幽匿尖啸体
extern const ResourceLocation BLOCK_SCULK_SHRIEKER_SHRIEK;

/// 铜傀儡雕像（铜傀儡变雕像音效，玩家右键切换姿态时播放）
extern const ResourceLocation BLOCK_COPPER_GOLEM_BECOME_STATUE;

/// 监守者
/// 完整对齐 MC 1.21.11 SoundEvents 中所有 WARDEN_* 事件（共 21 个）。
/// 参考: net.minecraft.sounds.SoundEvents
extern const ResourceLocation ENTITY_WARDEN_AGITATED;
extern const ResourceLocation ENTITY_WARDEN_AMBIENT;
extern const ResourceLocation ENTITY_WARDEN_ANGRY;
extern const ResourceLocation ENTITY_WARDEN_ATTACK_IMPACT;
extern const ResourceLocation ENTITY_WARDEN_DEATH;
extern const ResourceLocation ENTITY_WARDEN_DIG;
extern const ResourceLocation ENTITY_WARDEN_EMERGE;
extern const ResourceLocation ENTITY_WARDEN_HEARTBEAT;
extern const ResourceLocation ENTITY_WARDEN_HURT;
extern const ResourceLocation ENTITY_WARDEN_LISTENING;
extern const ResourceLocation ENTITY_WARDEN_LISTENING_ANGRY;
extern const ResourceLocation ENTITY_WARDEN_NEARBY_CLOSE;
extern const ResourceLocation ENTITY_WARDEN_NEARBY_CLOSER;
extern const ResourceLocation ENTITY_WARDEN_NEARBY_CLOSEST;
extern const ResourceLocation ENTITY_WARDEN_ROAR;
extern const ResourceLocation ENTITY_WARDEN_SNIFF;
extern const ResourceLocation ENTITY_WARDEN_SONIC_BOOM;
extern const ResourceLocation ENTITY_WARDEN_SONIC_CHARGE;
extern const ResourceLocation ENTITY_WARDEN_STEP;
extern const ResourceLocation ENTITY_WARDEN_TENDRIL_CLICKS;

// ============================================================================
// 实体通用声音
// ============================================================================

/// 通用进食声音
extern const ResourceLocation ENTITY_GENERIC_EAT;

/// 通用饮水声音
extern const ResourceLocation ENTITY_GENERIC_DRINK;

/// 通用窒息声音
extern const ResourceLocation ENTITY_GENERIC_HURT;

/// 通用死亡声音
extern const ResourceLocation ENTITY_GENERIC_DEATH;

/// 通用燃烧声音
extern const ResourceLocation ENTITY_GENERIC_BURN;

/// 通用灭火声音
extern const ResourceLocation ENTITY_GENERIC_EXTINGUISH_FIRE;

/// 通用大摔落声
extern const ResourceLocation ENTITY_GENERIC_BIG_FALL;

/// 通用小摔落声
extern const ResourceLocation ENTITY_GENERIC_SMALL_FALL;

/// 通用溅水声
extern const ResourceLocation ENTITY_GENERIC_SPLASH;

/// 通用游泳声
extern const ResourceLocation ENTITY_GENERIC_SWIM;

/// 通用爆炸声
extern const ResourceLocation ENTITY_GENERIC_EXPLODE;

// ============================================================================
// 试炼密室相关音效 (Trial Chambers)
// ============================================================================

/// 试炼刷怪笼即将生成物品的警告音效
/// 对应 MC Java: SoundEvents.TRIAL_SPAWNER_ABOUT_TO_SPAWN_ITEM
extern const ResourceLocation TRIAL_SPAWNER_ABOUT_TO_SPAWN_ITEM;

/// 风弹投掷声
extern const ResourceLocation ENTITY_WIND_CHARGE_THROW;

/// 风弹风爆声
extern const ResourceLocation ENTITY_WIND_CHARGE_WIND_BURST;

/// 旋风人风弹风爆声
extern const ResourceLocation ENTITY_BREEZE_WIND_CHARGE_BURST;

/// 旋风人吸气（射击前蓄力）
extern const ResourceLocation ENTITY_BREEZE_INHALE;

/// 旋风人发射风弹
extern const ResourceLocation ENTITY_BREEZE_SHOOT;

/// 旋风人蓄力（长跳前吸气）
extern const ResourceLocation ENTITY_BREEZE_CHARGE;

/// 旋风人跳跃
extern const ResourceLocation ENTITY_BREEZE_JUMP;

/// 旋风人着陆
extern const ResourceLocation ENTITY_BREEZE_LAND;

/// 旋风人滑行
extern const ResourceLocation ENTITY_BREEZE_SLIDE;

/// 旋风人偏转投射物
extern const ResourceLocation ENTITY_BREEZE_DEFLECT;

/// 旋风人呼啸环境音（随机间隔播放）
extern const ResourceLocation ENTITY_BREEZE_WHIRL;

/// 旋风人地面环境音（onGround 时播放）
extern const ResourceLocation ENTITY_BREEZE_IDLE_GROUND;

/// 旋风人空中环境音（不在地面时播放）
extern const ResourceLocation ENTITY_BREEZE_IDLE_AIR;

// ============================================================================
// 重锤声音
// ============================================================================

/// 重锤砸地攻击（轻）
extern const ResourceLocation ITEM_MACE_SMASH_GROUND;

/// 重锤砸地攻击（重，下落距离>5格）
extern const ResourceLocation ITEM_MACE_SMASH_GROUND_HEAVY;

/// 重锤空中砸击
extern const ResourceLocation ITEM_MACE_SMASH_AIR;

// ============================================================================
// 玩家声音
// ============================================================================

/// 玩家打嗝声音（进食完成后）
extern const ResourceLocation ENTITY_PLAYER_BURP;

/// 玩家受伤声音
extern const ResourceLocation ENTITY_PLAYER_HURT;

/// 玩家受伤声音（溺水）
extern const ResourceLocation ENTITY_PLAYER_HURT_DROWN;

/// 玩家受伤声音（燃烧）
extern const ResourceLocation ENTITY_PLAYER_HURT_ON_FIRE;

/// 玩家受伤声音（甜浆果丛）
extern const ResourceLocation ENTITY_PLAYER_HURT_SWEET_BERRY_BUSH;

/// 玩家死亡声音
extern const ResourceLocation ENTITY_PLAYER_DEATH;

/// 玩家溅水声
extern const ResourceLocation ENTITY_PLAYER_SPLASH;

/// 玩家高速溅水声
extern const ResourceLocation ENTITY_PLAYER_SPLASH_HIGH_SPEED;

/// 玩家游泳声
extern const ResourceLocation ENTITY_PLAYER_SWIM;

/// 玩家脚步声
extern const ResourceLocation ENTITY_PLAYER_STEP;

/// 玩家攻击（横扫）声音
extern const ResourceLocation ENTITY_PLAYER_ATTACK_SWEEP;

/// 玩家攻击（暴击）声音
extern const ResourceLocation ENTITY_PLAYER_ATTACK_CRIT;

/// 玩家攻击（击退）声音
extern const ResourceLocation ENTITY_PLAYER_ATTACK_KNOCKBACK;

/// 玩家攻击（击中实体）声音
extern const ResourceLocation ENTITY_PLAYER_ATTACK_STRONG;

/// 玩家攻击（未击中）声音
extern const ResourceLocation ENTITY_PLAYER_ATTACK_NODAMAGE;

/// 玩家攻击（弱攻击）声音
extern const ResourceLocation ENTITY_PLAYER_ATTACK_WEAK;

/// 玩家呼吸声（水下）
extern const ResourceLocation ENTITY_PLAYER_BREATH;

/// 玩家等级提升声音
extern const ResourceLocation ENTITY_PLAYER_LEVELUP;

/// 玩家大摔落声
extern const ResourceLocation ENTITY_PLAYER_BIG_FALL;

/// 玩家小摔落声
extern const ResourceLocation ENTITY_PLAYER_SMALL_FALL;

// ============================================================================
// 友好生物声音
// ============================================================================

/// 鸡
extern const ResourceLocation ENTITY_CHICKEN_AMBIENT;
extern const ResourceLocation ENTITY_CHICKEN_DEATH;
extern const ResourceLocation ENTITY_CHICKEN_EGG;
extern const ResourceLocation ENTITY_CHICKEN_HURT;
extern const ResourceLocation ENTITY_CHICKEN_STEP;

/// 牛
extern const ResourceLocation ENTITY_COW_AMBIENT;
extern const ResourceLocation ENTITY_COW_DEATH;
extern const ResourceLocation ENTITY_COW_HURT;
extern const ResourceLocation ENTITY_COW_MILK;
extern const ResourceLocation ENTITY_COW_STEP;

/// 猪
extern const ResourceLocation ENTITY_PIG_AMBIENT;
extern const ResourceLocation ENTITY_PIG_DEATH;
extern const ResourceLocation ENTITY_PIG_HURT;
extern const ResourceLocation ENTITY_PIG_SADDLE;
extern const ResourceLocation ENTITY_PIG_STEP;

/// 羊
extern const ResourceLocation ENTITY_SHEEP_AMBIENT;
extern const ResourceLocation ENTITY_SHEEP_DEATH;
extern const ResourceLocation ENTITY_SHEEP_HURT;
extern const ResourceLocation ENTITY_SHEEP_SHEAR;
extern const ResourceLocation ENTITY_SHEEP_STEP;

/// 马
extern const ResourceLocation ENTITY_HORSE_AMBIENT;
extern const ResourceLocation ENTITY_HORSE_ANGRY;
extern const ResourceLocation ENTITY_HORSE_ARMOR;
extern const ResourceLocation ENTITY_HORSE_BREATHE;
extern const ResourceLocation ENTITY_HORSE_DEATH;
extern const ResourceLocation ENTITY_HORSE_EAT;
extern const ResourceLocation ENTITY_HORSE_GALLOP;
extern const ResourceLocation ENTITY_HORSE_HURT;
extern const ResourceLocation ENTITY_HORSE_JUMP;
extern const ResourceLocation ENTITY_HORSE_LAND;
extern const ResourceLocation ENTITY_HORSE_SADDLE;
extern const ResourceLocation ENTITY_HORSE_STEP;
extern const ResourceLocation ENTITY_HORSE_STEP_WOOD;

/// 驴
extern const ResourceLocation ENTITY_DONKEY_AMBIENT;
extern const ResourceLocation ENTITY_DONKEY_ANGRY;
extern const ResourceLocation ENTITY_DONKEY_CHEST;
extern const ResourceLocation ENTITY_DONKEY_DEATH;
extern const ResourceLocation ENTITY_DONKEY_EAT;
extern const ResourceLocation ENTITY_DONKEY_HURT;

/// 骡
extern const ResourceLocation ENTITY_MULE_AMBIENT;
extern const ResourceLocation ENTITY_MULE_ANGRY;
extern const ResourceLocation ENTITY_MULE_CHEST;
extern const ResourceLocation ENTITY_MULE_DEATH;
extern const ResourceLocation ENTITY_MULE_EAT;
extern const ResourceLocation ENTITY_MULE_HURT;

/// 羊驼
extern const ResourceLocation ENTITY_LLAMA_AMBIENT;
extern const ResourceLocation ENTITY_LLAMA_ANGRY;
extern const ResourceLocation ENTITY_LLAMA_CHEST;
extern const ResourceLocation ENTITY_LLAMA_DEATH;
extern const ResourceLocation ENTITY_LLAMA_EAT;
extern const ResourceLocation ENTITY_LLAMA_HURT;
extern const ResourceLocation ENTITY_LLAMA_SPIT;
extern const ResourceLocation ENTITY_LLAMA_STEP;
extern const ResourceLocation ENTITY_LLAMA_SWAG;

/// 猫
extern const ResourceLocation ENTITY_CAT_AMBIENT;
extern const ResourceLocation ENTITY_CAT_STRAY_AMBIENT;
extern const ResourceLocation ENTITY_CAT_DEATH;
extern const ResourceLocation ENTITY_CAT_EAT;
extern const ResourceLocation ENTITY_CAT_HISS;
extern const ResourceLocation ENTITY_CAT_BEG_FOR_FOOD;
extern const ResourceLocation ENTITY_CAT_HURT;
extern const ResourceLocation ENTITY_CAT_PURR;
extern const ResourceLocation ENTITY_CAT_PURREOW;

/// 豹猫
extern const ResourceLocation ENTITY_OCELOT_AMBIENT;
extern const ResourceLocation ENTITY_OCELOT_DEATH;
extern const ResourceLocation ENTITY_OCELOT_HURT;

/// 狼
extern const ResourceLocation ENTITY_WOLF_AMBIENT;
extern const ResourceLocation ENTITY_WOLF_DEATH;
extern const ResourceLocation ENTITY_WOLF_GROWL;
extern const ResourceLocation ENTITY_WOLF_HOWL;
extern const ResourceLocation ENTITY_WOLF_HURT;
extern const ResourceLocation ENTITY_WOLF_PANT;
extern const ResourceLocation ENTITY_WOLF_SHAKE;
extern const ResourceLocation ENTITY_WOLF_STEP;
extern const ResourceLocation ENTITY_WOLF_WHINE;

/// 狼铠音效
extern const ResourceLocation ENTITY_WOLF_ARMOR_BREAK;
extern const ResourceLocation ENTITY_WOLF_ARMOR_CRACK;
extern const ResourceLocation ENTITY_WOLF_ARMOR_DAMAGE;
extern const ResourceLocation ENTITY_WOLF_ARMOR_REPAIR;

/// 兔子
extern const ResourceLocation ENTITY_RABBIT_AMBIENT;
extern const ResourceLocation ENTITY_RABBIT_ATTACK;
extern const ResourceLocation ENTITY_RABBIT_DEATH;
extern const ResourceLocation ENTITY_RABBIT_HURT;
extern const ResourceLocation ENTITY_RABBIT_JUMP;

/// 北极熊
extern const ResourceLocation ENTITY_POLAR_BEAR_AMBIENT;
extern const ResourceLocation ENTITY_POLAR_BEAR_AMBIENT_BABY;
extern const ResourceLocation ENTITY_POLAR_BEAR_DEATH;
extern const ResourceLocation ENTITY_POLAR_BEAR_HURT;
extern const ResourceLocation ENTITY_POLAR_BEAR_STEP;
extern const ResourceLocation ENTITY_POLAR_BEAR_WARNING;

/// 蝙蝠
extern const ResourceLocation ENTITY_BAT_AMBIENT;
extern const ResourceLocation ENTITY_BAT_DEATH;
extern const ResourceLocation ENTITY_BAT_HURT;
extern const ResourceLocation ENTITY_BAT_LOOP;
extern const ResourceLocation ENTITY_BAT_TAKEOFF;

/// 狐狸
extern const ResourceLocation ENTITY_FOX_AGGRO;
extern const ResourceLocation ENTITY_FOX_AMBIENT;
extern const ResourceLocation ENTITY_FOX_BITE;
extern const ResourceLocation ENTITY_FOX_DEATH;
extern const ResourceLocation ENTITY_FOX_EAT;
extern const ResourceLocation ENTITY_FOX_HURT;
extern const ResourceLocation ENTITY_FOX_SCREECH;
extern const ResourceLocation ENTITY_FOX_SLEEP;
extern const ResourceLocation ENTITY_FOX_SNIFF;
extern const ResourceLocation ENTITY_FOX_SPIT;
extern const ResourceLocation ENTITY_FOX_TELEPORT;

/// 熊猫
extern const ResourceLocation ENTITY_PANDA_AGGRESSIVE_AMBIENT;
extern const ResourceLocation ENTITY_PANDA_AMBIENT;
extern const ResourceLocation ENTITY_PANDA_BITE;
extern const ResourceLocation ENTITY_PANDA_CANT_BREED;
extern const ResourceLocation ENTITY_PANDA_DEATH;
extern const ResourceLocation ENTITY_PANDA_EAT;
extern const ResourceLocation ENTITY_PANDA_HURT;
extern const ResourceLocation ENTITY_PANDA_PRE_SNEEZE;
extern const ResourceLocation ENTITY_PANDA_SNEEZE;
extern const ResourceLocation ENTITY_PANDA_STEP;
extern const ResourceLocation ENTITY_PANDA_WORRIED_AMBIENT;

/// 鹦鹉
extern const ResourceLocation ENTITY_PARROT_AMBIENT;
extern const ResourceLocation ENTITY_PARROT_DEATH;
extern const ResourceLocation ENTITY_PARROT_EAT;
extern const ResourceLocation ENTITY_PARROT_FLY;
extern const ResourceLocation ENTITY_PARROT_HURT;
extern const ResourceLocation ENTITY_PARROT_IMITATE_BLAZE;
extern const ResourceLocation ENTITY_PARROT_IMITATE_CREEPER;
extern const ResourceLocation ENTITY_PARROT_IMITATE_DROWNED;
extern const ResourceLocation ENTITY_PARROT_IMITATE_ELDER_GUARDIAN;
extern const ResourceLocation ENTITY_PARROT_IMITATE_ENDER_DRAGON;
extern const ResourceLocation ENTITY_PARROT_IMITATE_ENDERMITE;
extern const ResourceLocation ENTITY_PARROT_IMITATE_EVOKER;
extern const ResourceLocation ENTITY_PARROT_IMITATE_GHAST;
extern const ResourceLocation ENTITY_PARROT_IMITATE_GUARDIAN;
extern const ResourceLocation ENTITY_PARROT_IMITATE_HOGLIN;
extern const ResourceLocation ENTITY_PARROT_IMITATE_HUSK;
extern const ResourceLocation ENTITY_PARROT_IMITATE_ILLUSIONER;
extern const ResourceLocation ENTITY_PARROT_IMITATE_MAGMA_CUBE;
extern const ResourceLocation ENTITY_PARROT_IMITATE_PHANTOM;
extern const ResourceLocation ENTITY_PARROT_IMITATE_PIGLIN;
extern const ResourceLocation ENTITY_PARROT_IMITATE_PIGLIN_BRUTE;
extern const ResourceLocation ENTITY_PARROT_IMITATE_PILLAGER;
extern const ResourceLocation ENTITY_PARROT_IMITATE_RAVAGER;
extern const ResourceLocation ENTITY_PARROT_IMITATE_SHULKER;
extern const ResourceLocation ENTITY_PARROT_IMITATE_SILVERFISH;
extern const ResourceLocation ENTITY_PARROT_IMITATE_SKELETON;
extern const ResourceLocation ENTITY_PARROT_IMITATE_SLIME;
extern const ResourceLocation ENTITY_PARROT_IMITATE_SPIDER;
extern const ResourceLocation ENTITY_PARROT_IMITATE_STRAY;
extern const ResourceLocation ENTITY_PARROT_IMITATE_VEX;
extern const ResourceLocation ENTITY_PARROT_IMITATE_VINDICATOR;
extern const ResourceLocation ENTITY_PARROT_IMITATE_WITCH;
extern const ResourceLocation ENTITY_PARROT_IMITATE_WITHER;
extern const ResourceLocation ENTITY_PARROT_IMITATE_WITHER_SKELETON;
extern const ResourceLocation ENTITY_PARROT_IMITATE_ZOGLIN;
extern const ResourceLocation ENTITY_PARROT_IMITATE_ZOMBIE;
extern const ResourceLocation ENTITY_PARROT_IMITATE_ZOMBIE_VILLAGER;
extern const ResourceLocation ENTITY_PARROT_STEP;

/// 骷髅马
extern const ResourceLocation ENTITY_SKELETON_HORSE_AMBIENT;
extern const ResourceLocation ENTITY_SKELETON_HORSE_AMBIENT_WATER;
extern const ResourceLocation ENTITY_SKELETON_HORSE_DEATH;
extern const ResourceLocation ENTITY_SKELETON_HORSE_GALLOP_WATER;
extern const ResourceLocation ENTITY_SKELETON_HORSE_HURT;
extern const ResourceLocation ENTITY_SKELETON_HORSE_JUMP_WATER;
extern const ResourceLocation ENTITY_SKELETON_HORSE_STEP_WATER;
extern const ResourceLocation ENTITY_SKELETON_HORSE_SWIM;

/// 僵尸马
extern const ResourceLocation ENTITY_ZOMBIE_HORSE_AMBIENT;
extern const ResourceLocation ENTITY_ZOMBIE_HORSE_DEATH;
extern const ResourceLocation ENTITY_ZOMBIE_HORSE_HURT;

/// 幻术师
extern const ResourceLocation ENTITY_ILLUSIONER_AMBIENT;
extern const ResourceLocation ENTITY_ILLUSIONER_CAST_SPELL;
extern const ResourceLocation ENTITY_ILLUSIONER_DEATH;
extern const ResourceLocation ENTITY_ILLUSIONER_HURT;
extern const ResourceLocation ENTITY_ILLUSIONER_MIRROR_MOVE;
extern const ResourceLocation ENTITY_ILLUSIONER_PREPARE_BLINDNESS;
extern const ResourceLocation ENTITY_ILLUSIONER_PREPARE_MIRROR;

/// 敌对生物通用
extern const ResourceLocation ENTITY_HOSTILE_BIG_FALL;
extern const ResourceLocation ENTITY_HOSTILE_DEATH;
extern const ResourceLocation ENTITY_HOSTILE_HURT;
extern const ResourceLocation ENTITY_HOSTILE_SMALL_FALL;
extern const ResourceLocation ENTITY_HOSTILE_SPLASH;
extern const ResourceLocation ENTITY_HOSTILE_SWIM;

/// 海豚
extern const ResourceLocation ENTITY_DOLPHIN_AMBIENT;
extern const ResourceLocation ENTITY_DOLPHIN_AMBIENT_WATER;
extern const ResourceLocation ENTITY_DOLPHIN_ATTACK;
extern const ResourceLocation ENTITY_DOLPHIN_DEATH;
extern const ResourceLocation ENTITY_DOLPHIN_EAT;
extern const ResourceLocation ENTITY_DOLPHIN_HURT;
extern const ResourceLocation ENTITY_DOLPHIN_JUMP;
extern const ResourceLocation ENTITY_DOLPHIN_PLAY;
extern const ResourceLocation ENTITY_DOLPHIN_SPLASH;
extern const ResourceLocation ENTITY_DOLPHIN_SWIM;

/// 美西螈
extern const ResourceLocation ENTITY_AXOLOTL_ATTACK;
extern const ResourceLocation ENTITY_AXOLOTL_DEATH;
extern const ResourceLocation ENTITY_AXOLOTL_HURT;
extern const ResourceLocation ENTITY_AXOLOTL_IDLE_AIR;
extern const ResourceLocation ENTITY_AXOLOTL_IDLE_WATER;
extern const ResourceLocation ENTITY_AXOLOTL_SPLASH;
extern const ResourceLocation ENTITY_AXOLOTL_SWIM;
extern const ResourceLocation ITEM_BUCKET_FILL_AXOLOTL;
extern const ResourceLocation ITEM_BUCKET_EMPTY_AXOLOTL;

/// 鹦鹉螺（成体）
extern const ResourceLocation ENTITY_NAUTILUS_AMBIENT;
extern const ResourceLocation ENTITY_NAUTILUS_AMBIENT_ON_LAND;
extern const ResourceLocation ENTITY_NAUTILUS_HURT;
extern const ResourceLocation ENTITY_NAUTILUS_HURT_ON_LAND;
extern const ResourceLocation ENTITY_NAUTILUS_DEATH;
extern const ResourceLocation ENTITY_NAUTILUS_DEATH_ON_LAND;
extern const ResourceLocation ENTITY_NAUTILUS_DASH;
extern const ResourceLocation ENTITY_NAUTILUS_DASH_ON_LAND;
extern const ResourceLocation ENTITY_NAUTILUS_DASH_READY;
extern const ResourceLocation ENTITY_NAUTILUS_DASH_READY_ON_LAND;
extern const ResourceLocation ENTITY_NAUTILUS_EAT;
extern const ResourceLocation ENTITY_NAUTILUS_SADDLE_EQUIP;
extern const ResourceLocation ENTITY_NAUTILUS_SADDLE_UNDERWATER_EQUIP;

/// 鹦鹉螺（幼体）
extern const ResourceLocation ENTITY_BABY_NAUTILUS_AMBIENT;
extern const ResourceLocation ENTITY_BABY_NAUTILUS_AMBIENT_ON_LAND;
extern const ResourceLocation ENTITY_BABY_NAUTILUS_HURT;
extern const ResourceLocation ENTITY_BABY_NAUTILUS_HURT_ON_LAND;
extern const ResourceLocation ENTITY_BABY_NAUTILUS_DEATH;
extern const ResourceLocation ENTITY_BABY_NAUTILUS_DEATH_ON_LAND;
extern const ResourceLocation ENTITY_BABY_NAUTILUS_EAT;

/// 僵尸鹦鹉螺
extern const ResourceLocation ENTITY_ZOMBIE_NAUTILUS_AMBIENT;
extern const ResourceLocation ENTITY_ZOMBIE_NAUTILUS_AMBIENT_ON_LAND;
extern const ResourceLocation ENTITY_ZOMBIE_NAUTILUS_HURT;
extern const ResourceLocation ENTITY_ZOMBIE_NAUTILUS_HURT_ON_LAND;
extern const ResourceLocation ENTITY_ZOMBIE_NAUTILUS_DEATH;
extern const ResourceLocation ENTITY_ZOMBIE_NAUTILUS_DEATH_ON_LAND;
extern const ResourceLocation ENTITY_ZOMBIE_NAUTILUS_DASH;
extern const ResourceLocation ENTITY_ZOMBIE_NAUTILUS_DASH_ON_LAND;
extern const ResourceLocation ENTITY_ZOMBIE_NAUTILUS_DASH_READY;
extern const ResourceLocation ENTITY_ZOMBIE_NAUTILUS_DASH_READY_ON_LAND;
extern const ResourceLocation ENTITY_ZOMBIE_NAUTILUS_EAT;

/// 鱿鱼
extern const ResourceLocation ENTITY_SQUID_AMBIENT;
extern const ResourceLocation ENTITY_SQUID_DEATH;
extern const ResourceLocation ENTITY_SQUID_HURT;
extern const ResourceLocation ENTITY_SQUID_SQUIRT;

/// 发光鱿鱼
extern const ResourceLocation ENTITY_GLOW_SQUID_AMBIENT;
extern const ResourceLocation ENTITY_GLOW_SQUID_DEATH;
extern const ResourceLocation ENTITY_GLOW_SQUID_HURT;
extern const ResourceLocation ENTITY_GLOW_SQUID_SQUIRT;

/// 鱼
extern const ResourceLocation ENTITY_COD_AMBIENT;
extern const ResourceLocation ENTITY_COD_DEATH;
extern const ResourceLocation ENTITY_COD_FLOP;
extern const ResourceLocation ENTITY_COD_HURT;

extern const ResourceLocation ENTITY_SALMON_AMBIENT;
extern const ResourceLocation ENTITY_SALMON_DEATH;
extern const ResourceLocation ENTITY_SALMON_FLOP;
extern const ResourceLocation ENTITY_SALMON_HURT;

extern const ResourceLocation ENTITY_TROPICAL_FISH_AMBIENT;
extern const ResourceLocation ENTITY_TROPICAL_FISH_DEATH;
extern const ResourceLocation ENTITY_TROPICAL_FISH_FLOP;
extern const ResourceLocation ENTITY_TROPICAL_FISH_HURT;

extern const ResourceLocation ENTITY_PUFFER_FISH_AMBIENT;
extern const ResourceLocation ENTITY_PUFFER_FISH_BLOW_OUT;
extern const ResourceLocation ENTITY_PUFFER_FISH_BLOW_UP;
extern const ResourceLocation ENTITY_PUFFER_FISH_DEATH;
extern const ResourceLocation ENTITY_PUFFER_FISH_FLOP;
extern const ResourceLocation ENTITY_PUFFER_FISH_HURT;
extern const ResourceLocation ENTITY_PUFFER_FISH_STING;

/// 海龟
extern const ResourceLocation ENTITY_TURTLE_AMBIENT_LAND;
extern const ResourceLocation ENTITY_TURTLE_DEATH;
extern const ResourceLocation ENTITY_TURTLE_DEATH_BABY;
extern const ResourceLocation ENTITY_TURTLE_EGG_BREAK;
extern const ResourceLocation ENTITY_TURTLE_EGG_CRACK;
extern const ResourceLocation ENTITY_TURTLE_EGG_HATCH;
extern const ResourceLocation ENTITY_TURTLE_HURT;
extern const ResourceLocation ENTITY_TURTLE_HURT_BABY;
extern const ResourceLocation ENTITY_TURTLE_LAY_EGG;
extern const ResourceLocation ENTITY_TURTLE_SHAMBLE;
extern const ResourceLocation ENTITY_TURTLE_SHAMBLE_BABY;
extern const ResourceLocation ENTITY_TURTLE_SWIM;

/// 嗅探兽
extern const ResourceLocation SNIFFER_STEP;
extern const ResourceLocation SNIFFER_EAT;
extern const ResourceLocation SNIFFER_IDLE;
extern const ResourceLocation SNIFFER_HURT;
extern const ResourceLocation SNIFFER_DEATH;
extern const ResourceLocation SNIFFER_DROP_SEED;
extern const ResourceLocation SNIFFER_SCENTING;
extern const ResourceLocation SNIFFER_SNIFFING;
extern const ResourceLocation SNIFFER_SEARCHING;
extern const ResourceLocation SNIFFER_DIGGING;
extern const ResourceLocation SNIFFER_DIGGING_STOP;
extern const ResourceLocation SNIFFER_HAPPY;
extern const ResourceLocation SNIFFER_EGG_PLOP;
extern const ResourceLocation SNIFFER_EGG_CRACK;
extern const ResourceLocation SNIFFER_EGG_HATCH;

/// 蜜蜂
extern const ResourceLocation ENTITY_BEE_DEATH;
extern const ResourceLocation ENTITY_BEE_HURT;
extern const ResourceLocation ENTITY_BEE_LOOP;
extern const ResourceLocation ENTITY_BEE_LOOP_AGGRESSIVE;
extern const ResourceLocation ENTITY_BEE_STING;
extern const ResourceLocation ENTITY_BEE_POLLINATE;

/// 村民
extern const ResourceLocation ENTITY_VILLAGER_AMBIENT;
extern const ResourceLocation ENTITY_VILLAGER_CELEBRATE;
extern const ResourceLocation ENTITY_VILLAGER_DEATH;
extern const ResourceLocation ENTITY_VILLAGER_HURT;
extern const ResourceLocation ENTITY_VILLAGER_NO;
extern const ResourceLocation ENTITY_VILLAGER_TRADE;
extern const ResourceLocation ENTITY_VILLAGER_YES;
extern const ResourceLocation ENTITY_VILLAGER_WORK_ARMORER;
extern const ResourceLocation ENTITY_VILLAGER_WORK_BUTCHER;
extern const ResourceLocation ENTITY_VILLAGER_WORK_CARTOGRAPHER;
extern const ResourceLocation ENTITY_VILLAGER_WORK_CLERIC;
extern const ResourceLocation ENTITY_VILLAGER_WORK_FARMER;
extern const ResourceLocation ENTITY_VILLAGER_WORK_FISHERMAN;
extern const ResourceLocation ENTITY_VILLAGER_WORK_FLETCHER;
extern const ResourceLocation ENTITY_VILLAGER_WORK_LEATHERWORKER;
extern const ResourceLocation ENTITY_VILLAGER_WORK_LIBRARIAN;
extern const ResourceLocation ENTITY_VILLAGER_WORK_MASON;
extern const ResourceLocation ENTITY_VILLAGER_WORK_SHEPHERD;
extern const ResourceLocation ENTITY_VILLAGER_WORK_TOOLSMITH;
extern const ResourceLocation ENTITY_VILLAGER_WORK_WEAPONSMITH;

/// 流浪商人
extern const ResourceLocation ENTITY_WANDERING_TRADER_AMBIENT;
extern const ResourceLocation ENTITY_WANDERING_TRADER_DEATH;
extern const ResourceLocation ENTITY_WANDERING_TRADER_DISAPPEARED;
extern const ResourceLocation ENTITY_WANDERING_TRADER_DRINK_MILK;
extern const ResourceLocation ENTITY_WANDERING_TRADER_DRINK_POTION;
extern const ResourceLocation ENTITY_WANDERING_TRADER_HURT;
extern const ResourceLocation ENTITY_WANDERING_TRADER_NO;
extern const ResourceLocation ENTITY_WANDERING_TRADER_REAPPEARED;
extern const ResourceLocation ENTITY_WANDERING_TRADER_TRADE;
extern const ResourceLocation ENTITY_WANDERING_TRADER_YES;

/// 铁傀儡
extern const ResourceLocation ENTITY_IRON_GOLEM_ATTACK;
extern const ResourceLocation ENTITY_IRON_GOLEM_DAMAGE;
extern const ResourceLocation ENTITY_IRON_GOLEM_DEATH;
extern const ResourceLocation ENTITY_IRON_GOLEM_HURT;
extern const ResourceLocation ENTITY_IRON_GOLEM_REPAIR;
extern const ResourceLocation ENTITY_IRON_GOLEM_STEP;

/// 雪傀儡
extern const ResourceLocation ENTITY_SNOW_GOLEM_AMBIENT;
extern const ResourceLocation ENTITY_SNOW_GOLEM_DEATH;
extern const ResourceLocation ENTITY_SNOW_GOLEM_HURT;
extern const ResourceLocation ENTITY_SNOW_GOLEM_SHOOT;
extern const ResourceLocation ENTITY_SNOW_GOLEM_SHEAR;

/// 铜傀儡（MC 1.21.11）
/// 完整对齐 MC 1.21.11 SoundEvents 中所有 COPPER_GOLEM_* 与铜傀儡雕像相关事件。
/// 参考: net.minecraft.sounds.SoundEvents
// 基础（Unaffected 等级）铜傀儡音效
extern const ResourceLocation ENTITY_COPPER_GOLEM_STEP;
extern const ResourceLocation ENTITY_COPPER_GOLEM_HURT;
extern const ResourceLocation ENTITY_COPPER_GOLEM_DEATH;
extern const ResourceLocation ENTITY_COPPER_GOLEM_SPIN;
extern const ResourceLocation ENTITY_COPPER_GOLEM_SPAWN;
extern const ResourceLocation ENTITY_COPPER_GOLEM_SHEAR;
// 锈蚀（Weathered）等级铜傀儡音效
extern const ResourceLocation ENTITY_COPPER_GOLEM_WEATHERED_STEP;
extern const ResourceLocation ENTITY_COPPER_GOLEM_WEATHERED_HURT;
extern const ResourceLocation ENTITY_COPPER_GOLEM_WEATHERED_DEATH;
extern const ResourceLocation ENTITY_COPPER_GOLEM_WEATHERED_SPIN;
// 氧化（Oxidized）等级铜傀儡音效
extern const ResourceLocation ENTITY_COPPER_GOLEM_OXIDIZED_STEP;
extern const ResourceLocation ENTITY_COPPER_GOLEM_OXIDIZED_HURT;
extern const ResourceLocation ENTITY_COPPER_GOLEM_OXIDIZED_DEATH;
extern const ResourceLocation ENTITY_COPPER_GOLEM_OXIDIZED_SPIN;
// 铜傀儡物品交互音效（暂未使用，预留以保持与原版对齐）
extern const ResourceLocation ENTITY_COPPER_GOLEM_ITEM_GET;
extern const ResourceLocation ENTITY_COPPER_GOLEM_ITEM_NO_GET;
extern const ResourceLocation ENTITY_COPPER_GOLEM_ITEM_DROP;
extern const ResourceLocation ENTITY_COPPER_GOLEM_ITEM_NO_DROP;
// 铜傀儡雕像方块音效
extern const ResourceLocation BLOCK_COPPER_GOLEM_STATUE_BREAK;
extern const ResourceLocation BLOCK_COPPER_GOLEM_STATUE_PLACE;
extern const ResourceLocation BLOCK_COPPER_GOLEM_STATUE_HIT;
extern const ResourceLocation BLOCK_COPPER_GOLEM_STATUE_STEP;
extern const ResourceLocation BLOCK_COPPER_GOLEM_STATUE_FALL;

/// 哞菇
extern const ResourceLocation ENTITY_MOOSHROOM_CONVERT;
extern const ResourceLocation ENTITY_MOOSHROOM_EAT;
extern const ResourceLocation ENTITY_MOOSHROOM_MILK;
extern const ResourceLocation ENTITY_MOOSHROOM_SUSPICIOUS_MILK;
extern const ResourceLocation ENTITY_MOOSHROOM_SHEAR;

// ============================================================================
// 敌对生物声音
// ============================================================================

/// 僵尸
extern const ResourceLocation ENTITY_ZOMBIE_AMBIENT;
extern const ResourceLocation ENTITY_ZOMBIE_ATTACK_WOODEN_DOOR;
extern const ResourceLocation ENTITY_ZOMBIE_ATTACK_IRON_DOOR;
extern const ResourceLocation ENTITY_ZOMBIE_BREAK_WOODEN_DOOR;
extern const ResourceLocation ENTITY_ZOMBIE_CONVERTED_TO_DROWNED;
extern const ResourceLocation ENTITY_ZOMBIE_DEATH;
extern const ResourceLocation ENTITY_ZOMBIE_DESTROY_EGG;
extern const ResourceLocation ENTITY_ZOMBIE_HURT;
extern const ResourceLocation ENTITY_ZOMBIE_INFECT;
extern const ResourceLocation ENTITY_ZOMBIE_STEP;

/// 僵尸村民
extern const ResourceLocation ENTITY_ZOMBIE_VILLAGER_AMBIENT;
extern const ResourceLocation ENTITY_ZOMBIE_VILLAGER_CONVERTED;
extern const ResourceLocation ENTITY_ZOMBIE_VILLAGER_CURE;
extern const ResourceLocation ENTITY_ZOMBIE_VILLAGER_DEATH;
extern const ResourceLocation ENTITY_ZOMBIE_VILLAGER_HURT;
extern const ResourceLocation ENTITY_ZOMBIE_VILLAGER_STEP;

/// 尸壳
extern const ResourceLocation ENTITY_HUSK_AMBIENT;
extern const ResourceLocation ENTITY_HUSK_CONVERTED_TO_ZOMBIE;
extern const ResourceLocation ENTITY_HUSK_DEATH;
extern const ResourceLocation ENTITY_HUSK_HURT;
extern const ResourceLocation ENTITY_HUSK_STEP;

/// 溺尸
extern const ResourceLocation ENTITY_DROWNED_AMBIENT;
extern const ResourceLocation ENTITY_DROWNED_AMBIENT_WATER;
extern const ResourceLocation ENTITY_DROWNED_DEATH;
extern const ResourceLocation ENTITY_DROWNED_DEATH_WATER;
extern const ResourceLocation ENTITY_DROWNED_HURT;
extern const ResourceLocation ENTITY_DROWNED_HURT_WATER;
extern const ResourceLocation ENTITY_DROWNED_SHOOT;
extern const ResourceLocation ENTITY_DROWNED_STEP;
extern const ResourceLocation ENTITY_DROWNED_SWIM;

/// 僵尸猪灵
extern const ResourceLocation ENTITY_ZOMBIFIED_PIGLIN_AMBIENT;
extern const ResourceLocation ENTITY_ZOMBIFIED_PIGLIN_ANGRY;
extern const ResourceLocation ENTITY_ZOMBIFIED_PIGLIN_DEATH;
extern const ResourceLocation ENTITY_ZOMBIFIED_PIGLIN_HURT;

/// 骷髅
extern const ResourceLocation ENTITY_SKELETON_AMBIENT;
extern const ResourceLocation ENTITY_SKELETON_DEATH;
extern const ResourceLocation ENTITY_SKELETON_HURT;
extern const ResourceLocation ENTITY_SKELETON_SHOOT;
extern const ResourceLocation ENTITY_SKELETON_STEP;

/// 流浪者
extern const ResourceLocation ENTITY_STRAY_AMBIENT;
extern const ResourceLocation ENTITY_STRAY_DEATH;
extern const ResourceLocation ENTITY_STRAY_HURT;
extern const ResourceLocation ENTITY_STRAY_STEP;

/// 凋灵骷髅
extern const ResourceLocation ENTITY_WITHER_SKELETON_AMBIENT;
extern const ResourceLocation ENTITY_WITHER_SKELETON_DEATH;
extern const ResourceLocation ENTITY_WITHER_SKELETON_HURT;
extern const ResourceLocation ENTITY_WITHER_SKELETON_STEP;

/// 苦力怕
extern const ResourceLocation ENTITY_CREEPER_DEATH;
extern const ResourceLocation ENTITY_CREEPER_HURT;
extern const ResourceLocation ENTITY_CREEPER_PRIMED;

/// 末影人
extern const ResourceLocation ENTITY_ENDERMAN_AMBIENT;
extern const ResourceLocation ENTITY_ENDERMAN_DEATH;
extern const ResourceLocation ENTITY_ENDERMAN_HURT;
extern const ResourceLocation ENTITY_ENDERMAN_SCREAM;
extern const ResourceLocation ENTITY_ENDERMAN_STARE;
extern const ResourceLocation ENTITY_ENDERMAN_TELEPORT;

/// 末影螨
extern const ResourceLocation ENTITY_ENDERMITE_AMBIENT;
extern const ResourceLocation ENTITY_ENDERMITE_DEATH;
extern const ResourceLocation ENTITY_ENDERMITE_HURT;
extern const ResourceLocation ENTITY_ENDERMITE_STEP;

/// 蜘蛛
extern const ResourceLocation ENTITY_SPIDER_AMBIENT;
extern const ResourceLocation ENTITY_SPIDER_DEATH;
extern const ResourceLocation ENTITY_SPIDER_HURT;
extern const ResourceLocation ENTITY_SPIDER_STEP;

/// 史莱姆
extern const ResourceLocation ENTITY_SLIME_ATTACK;
extern const ResourceLocation ENTITY_SLIME_DEATH;
extern const ResourceLocation ENTITY_SLIME_HURT;
extern const ResourceLocation ENTITY_SLIME_JUMP;
extern const ResourceLocation ENTITY_SLIME_SQUISH;
extern const ResourceLocation ENTITY_SLIME_DEATH_SMALL;
extern const ResourceLocation ENTITY_SLIME_HURT_SMALL;
extern const ResourceLocation ENTITY_SLIME_JUMP_SMALL;
extern const ResourceLocation ENTITY_SLIME_SQUISH_SMALL;

/// 岩浆怪
extern const ResourceLocation ENTITY_MAGMA_CUBE_DEATH;
extern const ResourceLocation ENTITY_MAGMA_CUBE_DEATH_SMALL;
extern const ResourceLocation ENTITY_MAGMA_CUBE_HURT;
extern const ResourceLocation ENTITY_MAGMA_CUBE_HURT_SMALL;
extern const ResourceLocation ENTITY_MAGMA_CUBE_JUMP;
extern const ResourceLocation ENTITY_MAGMA_CUBE_SQUISH;
extern const ResourceLocation ENTITY_MAGMA_CUBE_SQUISH_SMALL;

/// 恶魂
extern const ResourceLocation ENTITY_GHAST_AMBIENT;
extern const ResourceLocation ENTITY_GHAST_DEATH;
extern const ResourceLocation ENTITY_GHAST_HURT;
extern const ResourceLocation ENTITY_GHAST_SCREAM;
extern const ResourceLocation ENTITY_GHAST_SHOOT;
extern const ResourceLocation ENTITY_GHAST_WARN;

/// 烈焰人
extern const ResourceLocation ENTITY_BLAZE_AMBIENT;
extern const ResourceLocation ENTITY_BLAZE_BURN;
extern const ResourceLocation ENTITY_BLAZE_DEATH;
extern const ResourceLocation ENTITY_BLAZE_HURT;
extern const ResourceLocation ENTITY_BLAZE_SHOOT;

/// 守卫者
extern const ResourceLocation ENTITY_GUARDIAN_AMBIENT;
extern const ResourceLocation ENTITY_GUARDIAN_AMBIENT_LAND;
extern const ResourceLocation ENTITY_GUARDIAN_ATTACK;
extern const ResourceLocation ENTITY_GUARDIAN_DEATH;
extern const ResourceLocation ENTITY_GUARDIAN_DEATH_LAND;
extern const ResourceLocation ENTITY_GUARDIAN_FLOP;
extern const ResourceLocation ENTITY_GUARDIAN_HURT;
extern const ResourceLocation ENTITY_GUARDIAN_HURT_LAND;

/// 远古守卫者
extern const ResourceLocation ENTITY_ELDER_GUARDIAN_AMBIENT;
extern const ResourceLocation ENTITY_ELDER_GUARDIAN_AMBIENT_LAND;
extern const ResourceLocation ENTITY_ELDER_GUARDIAN_CURSE;
extern const ResourceLocation ENTITY_ELDER_GUARDIAN_DEATH;
extern const ResourceLocation ENTITY_ELDER_GUARDIAN_DEATH_LAND;
extern const ResourceLocation ENTITY_ELDER_GUARDIAN_FLOP;
extern const ResourceLocation ENTITY_ELDER_GUARDIAN_HURT;
extern const ResourceLocation ENTITY_ELDER_GUARDIAN_HURT_LAND;

/// 女巫
extern const ResourceLocation ENTITY_WITCH_AMBIENT;
extern const ResourceLocation ENTITY_WITCH_CELEBRATE;
extern const ResourceLocation ENTITY_WITCH_DEATH;
extern const ResourceLocation ENTITY_WITCH_DRINK;
extern const ResourceLocation ENTITY_WITCH_HURT;
extern const ResourceLocation ENTITY_WITCH_THROW;

/// 唤魔者
extern const ResourceLocation ENTITY_EVOKER_AMBIENT;
extern const ResourceLocation ENTITY_EVOKER_CAST_SPELL;
extern const ResourceLocation ENTITY_EVOKER_CELEBRATE;
extern const ResourceLocation ENTITY_EVOKER_DEATH;
extern const ResourceLocation ENTITY_EVOKER_HURT;
extern const ResourceLocation ENTITY_EVOKER_PREPARE_ATTACK;
extern const ResourceLocation ENTITY_EVOKER_PREPARE_SUMMON;
extern const ResourceLocation ENTITY_EVOKER_PREPARE_WOLOLO;

/// 卫道士
extern const ResourceLocation ENTITY_VINDICATOR_AMBIENT;
extern const ResourceLocation ENTITY_VINDICATOR_CELEBRATE;
extern const ResourceLocation ENTITY_VINDICATOR_DEATH;
extern const ResourceLocation ENTITY_VINDICATOR_HURT;

/// 恼鬼
extern const ResourceLocation ENTITY_VEX_AMBIENT;
extern const ResourceLocation ENTITY_VEX_CHARGE;
extern const ResourceLocation ENTITY_VEX_DEATH;
extern const ResourceLocation ENTITY_VEX_HURT;

/// 劫掠者
extern const ResourceLocation ENTITY_PILLAGER_AMBIENT;
extern const ResourceLocation ENTITY_PILLAGER_CELEBRATE;
extern const ResourceLocation ENTITY_PILLAGER_DEATH;
extern const ResourceLocation ENTITY_PILLAGER_HURT;

/// 劫掠兽
extern const ResourceLocation ENTITY_RAVAGER_AMBIENT;
extern const ResourceLocation ENTITY_RAVAGER_ATTACK;
extern const ResourceLocation ENTITY_RAVAGER_CELEBRATE;
extern const ResourceLocation ENTITY_RAVAGER_DEATH;
extern const ResourceLocation ENTITY_RAVAGER_HURT;
extern const ResourceLocation ENTITY_RAVAGER_STEP;
extern const ResourceLocation ENTITY_RAVAGER_STUNNED;
extern const ResourceLocation ENTITY_RAVAGER_ROAR;

/// 幻翼
extern const ResourceLocation ENTITY_PHANTOM_AMBIENT;
extern const ResourceLocation ENTITY_PHANTOM_BITE;
extern const ResourceLocation ENTITY_PHANTOM_DEATH;
extern const ResourceLocation ENTITY_PHANTOM_FLAP;
extern const ResourceLocation ENTITY_PHANTOM_HURT;
extern const ResourceLocation ENTITY_PHANTOM_SWOOP;

/// 潜影贝
extern const ResourceLocation ENTITY_SHULKER_AMBIENT;
extern const ResourceLocation ENTITY_SHULKER_CLOSE;
extern const ResourceLocation ENTITY_SHULKER_DEATH;
extern const ResourceLocation ENTITY_SHULKER_HURT;
extern const ResourceLocation ENTITY_SHULKER_HURT_CLOSED;
extern const ResourceLocation ENTITY_SHULKER_OPEN;
extern const ResourceLocation ENTITY_SHULKER_SHOOT;
extern const ResourceLocation ENTITY_SHULKER_TELEPORT;
extern const ResourceLocation ENTITY_SHULKER_BULLET_HIT;
extern const ResourceLocation ENTITY_SHULKER_BULLET_HURT;

/// 蠹虫
extern const ResourceLocation ENTITY_SILVERFISH_AMBIENT;
extern const ResourceLocation ENTITY_SILVERFISH_DEATH;
extern const ResourceLocation ENTITY_SILVERFISH_HURT;
extern const ResourceLocation ENTITY_SILVERFISH_STEP;

/// 末影龙
extern const ResourceLocation ENTITY_ENDER_DRAGON_AMBIENT;
extern const ResourceLocation ENTITY_ENDER_DRAGON_DEATH;
extern const ResourceLocation ENTITY_ENDER_DRAGON_FLAP;
extern const ResourceLocation ENTITY_ENDER_DRAGON_GROWL;
extern const ResourceLocation ENTITY_ENDER_DRAGON_HURT;
extern const ResourceLocation ENTITY_ENDER_DRAGON_SHOOT;
extern const ResourceLocation ENTITY_DRAGON_FIREBALL_EXPLODE;

/// 凋灵
extern const ResourceLocation ENTITY_WITHER_AMBIENT;
extern const ResourceLocation ENTITY_WITHER_BREAK_BLOCK;
extern const ResourceLocation ENTITY_WITHER_DEATH;
extern const ResourceLocation ENTITY_WITHER_HURT;
extern const ResourceLocation ENTITY_WITHER_SHOOT;
extern const ResourceLocation ENTITY_WITHER_SPAWN;

/// 猪灵
extern const ResourceLocation ENTITY_PIGLIN_ADMIRING_ITEM;
extern const ResourceLocation ENTITY_PIGLIN_AMBIENT;
extern const ResourceLocation ENTITY_PIGLIN_ANGRY;
extern const ResourceLocation ENTITY_PIGLIN_CELEBRATE;
extern const ResourceLocation ENTITY_PIGLIN_DEATH;
extern const ResourceLocation ENTITY_PIGLIN_JEALOUS;
extern const ResourceLocation ENTITY_PIGLIN_HURT;
extern const ResourceLocation ENTITY_PIGLIN_RETREAT;
extern const ResourceLocation ENTITY_PIGLIN_STEP;
extern const ResourceLocation ENTITY_PIGLIN_CONVERTED_TO_ZOMBIFIED;

/// 猪灵蛮兵
extern const ResourceLocation ENTITY_PIGLIN_BRUTE_AMBIENT;
extern const ResourceLocation ENTITY_PIGLIN_BRUTE_ANGRY;
extern const ResourceLocation ENTITY_PIGLIN_BRUTE_DEATH;
extern const ResourceLocation ENTITY_PIGLIN_BRUTE_HURT;
extern const ResourceLocation ENTITY_PIGLIN_BRUTE_STEP;
extern const ResourceLocation ENTITY_PIGLIN_BRUTE_CONVERTED_TO_ZOMBIFIED;

/// 疣猪兽
extern const ResourceLocation ENTITY_HOGLIN_AMBIENT;
extern const ResourceLocation ENTITY_HOGLIN_ANGRY;
extern const ResourceLocation ENTITY_HOGLIN_ATTACK;
extern const ResourceLocation ENTITY_HOGLIN_CONVERTED_TO_ZOMBIFIED;
extern const ResourceLocation ENTITY_HOGLIN_DEATH;
extern const ResourceLocation ENTITY_HOGLIN_HURT;
extern const ResourceLocation ENTITY_HOGLIN_RETREAT;
extern const ResourceLocation ENTITY_HOGLIN_STEP;

/// 僵尸疣兽
extern const ResourceLocation ENTITY_ZOGLIN_AMBIENT;
extern const ResourceLocation ENTITY_ZOGLIN_ANGRY;
extern const ResourceLocation ENTITY_ZOGLIN_ATTACK;
extern const ResourceLocation ENTITY_ZOGLIN_DEATH;
extern const ResourceLocation ENTITY_ZOGLIN_HURT;
extern const ResourceLocation ENTITY_ZOGLIN_STEP;

/// 炽足兽
extern const ResourceLocation ENTITY_STRIDER_AMBIENT;
extern const ResourceLocation ENTITY_STRIDER_HAPPY;
extern const ResourceLocation ENTITY_STRIDER_RETREAT;
extern const ResourceLocation ENTITY_STRIDER_DEATH;
extern const ResourceLocation ENTITY_STRIDER_HURT;
extern const ResourceLocation ENTITY_STRIDER_STEP;
extern const ResourceLocation ENTITY_STRIDER_STEP_LAVA;
extern const ResourceLocation ENTITY_STRIDER_EAT;
extern const ResourceLocation ENTITY_STRIDER_SADDLE;

// ============================================================================
// 其他实体声音
// ============================================================================

/// 末影之眼
extern const ResourceLocation ENTITY_ENDER_EYE_DEATH;
extern const ResourceLocation ENTITY_ENDER_EYE_LAUNCH;

/// 唤魔者尖牙
extern const ResourceLocation ENTITY_EVOKER_FANGS_ATTACK;

/// 经验瓶
extern const ResourceLocation ENTITY_EXPERIENCE_BOTTLE_THROW;

/// 鱼（游泳）
extern const ResourceLocation ENTITY_FISH_SWIM;

/// 物品（破坏）
extern const ResourceLocation ENTITY_ITEM_BREAK;

/// 滞留药水
extern const ResourceLocation ENTITY_LINGERING_POTION_THROW;

/// 箭矢
extern const ResourceLocation ENTITY_ARROW_HIT;
extern const ResourceLocation ENTITY_ARROW_HIT_PLAYER;
extern const ResourceLocation ENTITY_ARROW_HIT_GROUND;
extern const ResourceLocation ENTITY_ARROW_SHOOT;

/// 经验球
extern const ResourceLocation ENTITY_EXPERIENCE_ORB_PICKUP;
extern const ResourceLocation ENTITY_EXPERIENCE_ORB_THROW;

/// 闪电
extern const ResourceLocation ENTITY_LIGHTNING_BOLT_IMPACT;
extern const ResourceLocation ENTITY_LIGHTNING_BOLT_THUNDER;

/// TNT
extern const ResourceLocation ENTITY_TNT_PRIMED;

/// 末影珍珠
extern const ResourceLocation ENTITY_ENDER_PEARL_THROW;

/// 鸡蛋
extern const ResourceLocation ENTITY_EGG_THROW;

/// 雪球
extern const ResourceLocation ENTITY_SNOWBALL_THROW;

/// 药水
extern const ResourceLocation ENTITY_SPLASH_POTION_BREAK;
extern const ResourceLocation ENTITY_SPLASH_POTION_THROW;

/// 烟花火箭
extern const ResourceLocation ENTITY_FIREWORK_ROCKET_BLAST;
extern const ResourceLocation ENTITY_FIREWORK_ROCKET_BLAST_FAR;
extern const ResourceLocation ENTITY_FIREWORK_ROCKET_LARGE_BLAST;
extern const ResourceLocation ENTITY_FIREWORK_ROCKET_LARGE_BLAST_FAR;
extern const ResourceLocation ENTITY_FIREWORK_ROCKET_LAUNCH;
extern const ResourceLocation ENTITY_FIREWORK_ROCKET_SHOOT;
extern const ResourceLocation ENTITY_FIREWORK_ROCKET_TWINKLE;
extern const ResourceLocation ENTITY_FIREWORK_ROCKET_TWINKLE_FAR;

/// 矿车
extern const ResourceLocation ENTITY_MINECART_INSIDE;
extern const ResourceLocation ENTITY_MINECART_RIDING;

/// 船
extern const ResourceLocation ENTITY_BOAT_PADDLE_LAND;
extern const ResourceLocation ENTITY_BOAT_PADDLE_WATER;

/// 物品展示框和画
extern const ResourceLocation ENTITY_ITEM_FRAME_ADD_ITEM;
extern const ResourceLocation ENTITY_ITEM_FRAME_BREAK;
extern const ResourceLocation ENTITY_ITEM_FRAME_PLACE;
extern const ResourceLocation ENTITY_ITEM_FRAME_REMOVE_ITEM;
extern const ResourceLocation ENTITY_ITEM_FRAME_ROTATE_ITEM;

extern const ResourceLocation ENTITY_PAINTING_BREAK;
extern const ResourceLocation ENTITY_PAINTING_PLACE;

/// 盔甲架
extern const ResourceLocation ENTITY_ARMOR_STAND_BREAK;
extern const ResourceLocation ENTITY_ARMOR_STAND_FALL;
extern const ResourceLocation ENTITY_ARMOR_STAND_HIT;
extern const ResourceLocation ENTITY_ARMOR_STAND_PLACE;

/// 皮革绳
extern const ResourceLocation ENTITY_LEASH_KNOT_BREAK;
extern const ResourceLocation ENTITY_LEASH_KNOT_PLACE;

// ============================================================================
// 物品声音
// ============================================================================

/// 物品拾取声音
extern const ResourceLocation ENTITY_ITEM_PICKUP;

/// 盔甲装备
extern const ResourceLocation ITEM_ARMOR_EQUIP_CHAIN;
extern const ResourceLocation ITEM_ARMOR_EQUIP_COPPER;
extern const ResourceLocation ITEM_ARMOR_EQUIP_DIAMOND;
extern const ResourceLocation ITEM_ARMOR_EQUIP_ELYTRA;
extern const ResourceLocation ITEM_ARMOR_EQUIP_GENERIC;
extern const ResourceLocation ITEM_ARMOR_EQUIP_GOLD;
extern const ResourceLocation ITEM_ARMOR_EQUIP_IRON;
extern const ResourceLocation ITEM_ARMOR_EQUIP_LEATHER;
extern const ResourceLocation ITEM_ARMOR_EQUIP_NETHERITE;
extern const ResourceLocation ITEM_ARMOR_EQUIP_TURTLE;
extern const ResourceLocation ITEM_ARMOR_EQUIP_WOLF;
extern const ResourceLocation ITEM_ARMOR_UNEQUIP_WOLF;

/// 鞘翅
extern const ResourceLocation ITEM_ELYTRA_FLYING;

/// 桶
extern const ResourceLocation ITEM_BUCKET_EMPTY;
extern const ResourceLocation ITEM_BUCKET_EMPTY_FISH;
extern const ResourceLocation ITEM_BUCKET_EMPTY_LAVA;
extern const ResourceLocation ITEM_BUCKET_EMPTY_POWDER_SNOW;
extern const ResourceLocation ITEM_BUCKET_FILL;
extern const ResourceLocation ITEM_BUCKET_FILL_FISH;
extern const ResourceLocation ITEM_BUCKET_FILL_LAVA;
extern const ResourceLocation ITEM_BUCKET_FILL_POWDER_SNOW;

/// 工具
extern const ResourceLocation ITEM_AXE_STRIP;
extern const ResourceLocation ITEM_AXE_SCRAPE;
extern const ResourceLocation ITEM_AXE_WAX_OFF;
extern const ResourceLocation ITEM_HOE_TILL;
extern const ResourceLocation ITEM_SHOVEL_FLATTEN;

/// 其他
extern const ResourceLocation ITEM_CHORUS_FRUIT_TELEPORT;
extern const ResourceLocation ITEM_FLINTANDSTEEL_USE;
extern const ResourceLocation ITEM_FIRECHARGE_USE;
extern const ResourceLocation ITEM_TOTEM_USE;
extern const ResourceLocation ITEM_BOOK_PAGE_TURN;
extern const ResourceLocation ITEM_BOOK_PUT;
extern const ResourceLocation ITEM_BONE_MEAL_USE;
extern const ResourceLocation ITEM_BOTTLE_EMPTY;
extern const ResourceLocation ITEM_BOTTLE_FILL;
extern const ResourceLocation ITEM_BOTTLE_FILL_DRAGONBREATH;
extern const ResourceLocation ITEM_HONEY_BOTTLE_DRINK;
extern const ResourceLocation ITEM_SWEET_BERRIES_PICK_FROM_BUSH;
extern const ResourceLocation ITEM_CROP_PLANT;
extern const ResourceLocation ITEM_NETHER_WART_PLANT;
extern const ResourceLocation ITEM_LODESTONE_COMPASS_LOCK;

/// 收纳袋（Bundle）音效
/// 对应 MC 1.21.11 SoundEvents.ITEM_BUNDLE_DROP_CONTENTS / INSERT / INSERT_FAIL / REMOVE_ONE
/// 用于 BundleItem 的插入、取出、丢出和失败反馈
extern const ResourceLocation ITEM_BUNDLE_DROP_CONTENTS;
extern const ResourceLocation ITEM_BUNDLE_INSERT;
extern const ResourceLocation ITEM_BUNDLE_INSERT_FAIL;
extern const ResourceLocation ITEM_BUNDLE_REMOVE_ONE;

// ============================================================================
// 武器声音
// ============================================================================

/// 弓箭
extern const ResourceLocation ITEM_BOW_PULL;

/// 弩
extern const ResourceLocation ITEM_CROSSBOW_LOADING_START;
extern const ResourceLocation ITEM_CROSSBOW_LOADING_MIDDLE;
extern const ResourceLocation ITEM_CROSSBOW_LOADING_END;
extern const ResourceLocation ITEM_CROSSBOW_SHOOT;
extern const ResourceLocation ITEM_CROSSBOW_ROCKET;
extern const ResourceLocation ITEM_CROSSBOW_HIT;
extern const ResourceLocation ITEM_CROSSBOW_QUICK_CHARGE_1;
extern const ResourceLocation ITEM_CROSSBOW_QUICK_CHARGE_2;
extern const ResourceLocation ITEM_CROSSBOW_QUICK_CHARGE_3;

/// 三叉戟
extern const ResourceLocation ITEM_TRIDENT_THROW;
extern const ResourceLocation ITEM_TRIDENT_RIPTIDE_1;
extern const ResourceLocation ITEM_TRIDENT_RIPTIDE_2;
extern const ResourceLocation ITEM_TRIDENT_RIPTIDE_3;
extern const ResourceLocation ITEM_TRIDENT_HIT;
extern const ResourceLocation ITEM_TRIDENT_HIT_GROUND;
extern const ResourceLocation ITEM_TRIDENT_RETURN;
extern const ResourceLocation ITEM_TRIDENT_THUNDER;

/// 长矛
extern const ResourceLocation ITEM_SPEAR_THROW;
extern const ResourceLocation ITEM_SPEAR_HIT;
extern const ResourceLocation ITEM_SPEAR_HIT_GROUND;

/// 盾牌
extern const ResourceLocation ITEM_SHIELD_BLOCK;
extern const ResourceLocation ITEM_SHIELD_BREAK;

/// 钓鱼竿
extern const ResourceLocation ENTITY_FISHING_BOBBER_THROW;
extern const ResourceLocation ENTITY_FISHING_BOBBER_RETRIEVE;
extern const ResourceLocation ENTITY_FISHING_BOBBER_SPLASH;
extern const ResourceLocation ENTITY_FISHING_BOBBER_CAST;

// ============================================================================
// 音乐音效 (MUSIC_)
// ============================================================================

extern const ResourceLocation MUSIC_CREATIVE;
extern const ResourceLocation MUSIC_CREDITS;
extern const ResourceLocation MUSIC_DRAGON;
extern const ResourceLocation MUSIC_END;
extern const ResourceLocation MUSIC_GAME;
extern const ResourceLocation MUSIC_MENU;
extern const ResourceLocation MUSIC_UNDER_WATER;
extern const ResourceLocation MUSIC_NETHER_BASALT_DELTAS;
extern const ResourceLocation MUSIC_NETHER_NETHER_WASTES;
extern const ResourceLocation MUSIC_NETHER_SOUL_SAND_VALLEY;
extern const ResourceLocation MUSIC_NETHER_CRIMSON_FOREST;
extern const ResourceLocation MUSIC_NETHER_WARPED_FOREST;

/// 音乐唱片
extern const ResourceLocation MUSIC_DISC_11;
extern const ResourceLocation MUSIC_DISC_13;
extern const ResourceLocation MUSIC_DISC_5;
extern const ResourceLocation MUSIC_DISC_BLOCKS;
extern const ResourceLocation MUSIC_DISC_CAT;
extern const ResourceLocation MUSIC_DISC_CHIRP;
extern const ResourceLocation MUSIC_DISC_CREATOR;
extern const ResourceLocation MUSIC_DISC_CREATOR_MUSIC_BOX;
extern const ResourceLocation MUSIC_DISC_FAR;
extern const ResourceLocation MUSIC_DISC_LAVA_CHICKEN;
extern const ResourceLocation MUSIC_DISC_MALL;
extern const ResourceLocation MUSIC_DISC_MELLOHI;
extern const ResourceLocation MUSIC_DISC_OTHERSIDE;
extern const ResourceLocation MUSIC_DISC_PIGSTEP;
extern const ResourceLocation MUSIC_DISC_PRECIPICE;
extern const ResourceLocation MUSIC_DISC_RELIC;
extern const ResourceLocation MUSIC_DISC_STAL;
extern const ResourceLocation MUSIC_DISC_STRAD;
extern const ResourceLocation MUSIC_DISC_TEARS;
extern const ResourceLocation MUSIC_DISC_WAIT;
extern const ResourceLocation MUSIC_DISC_WARD;

// ============================================================================
// 天气音效 (WEATHER_)
// ============================================================================

/// 环境天气雨声
extern const ResourceLocation WEATHER_RAIN;
extern const ResourceLocation WEATHER_RAIN_ABOVE;

/// 环境天气雷声
extern const ResourceLocation WEATHER_THUNDER;

// ============================================================================
// UI音效 (UI_)
// ============================================================================

extern const ResourceLocation UI_BUTTON_CLICK;
extern const ResourceLocation UI_TOAST_CHALLENGE_COMPLETE;
extern const ResourceLocation UI_TOAST_IN;
extern const ResourceLocation UI_TOAST_OUT;
extern const ResourceLocation UI_LOOM_SELECT_PATTERN;
extern const ResourceLocation UI_LOOM_TAKE_RESULT;
extern const ResourceLocation UI_CARTOGRAPHY_TABLE_TAKE_RESULT;
extern const ResourceLocation UI_STONECUTTER_TAKE_RESULT;
extern const ResourceLocation UI_STONECUTTER_SELECT_RECIPE;

// ============================================================================
// 事件音效 (EVENT_)
// ============================================================================

extern const ResourceLocation EVENT_RAID_HORN;

// ============================================================================
// 附魔音效 (ENCHANT_)
// ============================================================================

extern const ResourceLocation ENCHANT_THORNS_HIT;

// ============================================================================
// 粒子音效 (PARTICLE_)
// ============================================================================

extern const ResourceLocation PARTICLE_SOUL_ESCAPE;

// ============================================================================
// 眼眸花音效 (BLOCK_EYEBLOSSOM_)
// ============================================================================

/// 眼眸花开放长音（昼夜节律触发）
extern const ResourceLocation BLOCK_EYEBLOSSOM_OPEN_LONG;
/// 眼眸花闭合长音（昼夜节律触发）
extern const ResourceLocation BLOCK_EYEBLOSSOM_CLOSE_LONG;
/// 眼眸花开放短音（连锁触发）
extern const ResourceLocation BLOCK_EYEBLOSSOM_OPEN;
/// 眼眸花闭合短音（连锁触发）
extern const ResourceLocation BLOCK_EYEBLOSSOM_CLOSE;
/// 眼眸花环境空闲音
extern const ResourceLocation BLOCK_EYEBLOSSOM_IDLE;

// ============================================================================
// 刷子音效 (BRUSH_)
// ============================================================================

/// 刷子通用刷扫音效（非 BrushableBlock 方块使用）
/// 对应 MC Java: SoundEvents.BRUSH_GENERIC = "item.brush.brushing.generic"
extern const ResourceLocation BRUSH_GENERIC;

/// 刷扫可疑沙音效
/// 对应 MC Java: SoundEvents.BRUSH_SAND = "item.brush.brushing.sand"
extern const ResourceLocation BRUSH_SAND;

/// 刷扫可疑沙砾音效
/// 对应 MC Java: SoundEvents.BRUSH_GRAVEL = "item.brush.brushing.gravel"
extern const ResourceLocation BRUSH_GRAVEL;

/// 刷扫可疑沙完成音效（刷出物品时播放）
/// 对应 MC Java: SoundEvents.BRUSH_SAND_COMPLETED = "item.brush.brushing.sand.complete"
extern const ResourceLocation BRUSH_SAND_COMPLETED;

/// 刷扫可疑沙砾完成音效（刷出物品时播放）
/// 对应 MC Java: SoundEvents.BRUSH_GRAVEL_COMPLETED = "item.brush.brushing.gravel.complete"
extern const ResourceLocation BRUSH_GRAVEL_COMPLETED;

// ============================================================================
// 初始化
// ============================================================================

/**
 * @brief 初始化所有声音事件常量
 *
 * 必须在使用任何声音事件前调用。
 * 通常在游戏启动时调用。
 */
void initialize();

} // namespace SoundEvents

} // namespace mc
