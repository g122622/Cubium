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

#include "common/core/Types.hpp"

namespace mc::world {

/**
 * @brief 世界事件ID常量
 *
 * 用于 IWorld::playEvent() 方法，触发音效和粒子效果。
 * 参考: net.minecraftforge.common.util.Constants.WorldEvents
 * 参考: net.minecraft.client.renderer.WorldRenderer.playEvent
 *
 * 事件分为以下几类：
 * - 1000-1039: 音效事件（同步播放声音）
 * - 1500-1503: 特殊效果事件（堆肥、岩浆熄灭等）
 * - 2000-2008: 粒子/效果事件（方块破坏、药水效果等）
 * - 3000-3001: 末地传送门事件
 */
namespace WorldEvents {

// ============================================================================
// 音效事件 (1000-1039)
// ============================================================================

/// 发射器发射物品音效
constexpr i32 DISPENSER_DISPENSE_SOUND = 1000;

/// 发射器失败音效
constexpr i32 DISPENSER_FAIL_SOUND = 1001;

/// 发射器发射投掷物音效（箭、蛋、雪球等）
constexpr i32 DISPENSER_LAUNCH_SOUND = 1002;

/// 末影之眼发射音效
constexpr i32 ENDER_EYE_LAUNCH_SOUND = 1003;

/// 烟花火箭发射音效
constexpr i32 FIREWORK_SHOOT_SOUND = 1004;

/// 铁门打开音效
constexpr i32 IRON_DOOR_OPEN_SOUND = 1005;

/// 木门打开音效
constexpr i32 WOODEN_DOOR_OPEN_SOUND = 1006;

/// 木活板门打开音效
constexpr i32 WOODEN_TRAPDOOR_OPEN_SOUND = 1007;

/// 栅栏门打开音效
constexpr i32 FENCE_GATE_OPEN_SOUND = 1008;

/// 火焰熄灭音效
constexpr i32 FIRE_EXTINGUISH_SOUND = 1009;

/// 播放唱片音效（data 为唱片比较器输出信号强度，1-15 为播放，0 为停止）
constexpr i32 PLAY_RECORD_SOUND = 1010;

/// 停止唱片音效
/// 参考 MC 1.21.11: JukeboxSongPlayer.stop() 发送 levelEvent(1011, pos, 0)
constexpr i32 STOP_RECORD_SOUND = 1011;

/// 铁门关闭音效
/// 注意: MC 协议中 1011 实际上是停止唱片音效 (STOP_RECORD_SOUND)，
/// 铁门关闭音效应使用不同的事件ID。此处保留仅为兼容，后续应修正。
constexpr i32 IRON_DOOR_CLOSE_SOUND = 1011;

/// 木门关闭音效
constexpr i32 WOODEN_DOOR_CLOSE_SOUND = 1012;

/// 木活板门关闭音效
constexpr i32 WOODEN_TRAPDOOR_CLOSE_SOUND = 1013;

/// 栅栏门关闭音效
constexpr i32 FENCE_GATE_CLOSE_SOUND = 1014;

/// 恶魂警告音效
constexpr i32 GHAST_WARN_SOUND = 1015;

/// 恶魂射击音效
constexpr i32 GHAST_SHOOT_SOUND = 1016;

/// 末影龙射击音效
constexpr i32 ENDER_DRAGON_SHOOT_SOUND = 1017;

/// 烈焰人射击音效
constexpr i32 BLAZE_SHOOT_SOUND = 1018;

/// 僵尸攻击木门音效
constexpr i32 ZOMBIE_ATTACK_DOOR_WOOD_SOUND = 1019;

/// 僵尸攻击铁门音效
constexpr i32 ZOMBIE_ATTACK_DOOR_IRON_SOUND = 1020;

/// 僵尸破坏木门音效
constexpr i32 ZOMBIE_BREAK_DOOR_WOOD_SOUND = 1021;

/// 凋灵破坏方块音效
constexpr i32 WITHER_BREAK_BLOCK_SOUND = 1022;

/// 凋灵破坏方块
constexpr i32 WITHER_BREAK_BLOCK = 1023;

/// 凋灵射击音效
constexpr i32 WITHER_SHOOT_SOUND = 1024;

/// 蝙蝠起飞音效
constexpr i32 BAT_TAKEOFF_SOUND = 1025;

/// 僵尸感染音效
constexpr i32 ZOMBIE_INFECT_SOUND = 1026;

/// 僵尸村民转化音效
constexpr i32 ZOMBIE_VILLAGER_CONVERTED_SOUND = 1027;

/// 末影龙死亡音效
/// 参考: net.minecraft.world.level.block.LevelEvent.SOUND_DRAGON_DEATH
constexpr i32 DRAGON_DEATH_SOUND = 1028;

/// 铁砧损坏音效
constexpr i32 ANVIL_DESTROYED_SOUND = 1029;

/// 铁砧使用音效
constexpr i32 ANVIL_USE_SOUND = 1030;

/// 铁砧落地音效
constexpr i32 ANVIL_LAND_SOUND = 1031;

/// 传送门传送音效
constexpr i32 PORTAL_TRAVEL_SOUND = 1032;

/// 紫颂花生长音效
constexpr i32 CHORUS_FLOWER_GROW_SOUND = 1033;

/// 紫颂花死亡音效
constexpr i32 CHORUS_FLOWER_DEATH_SOUND = 1034;

/// 酿造台酿造音效
constexpr i32 BREWING_STAND_BREW_SOUND = 1035;

/// 铁活板门关闭音效
constexpr i32 IRON_TRAPDOOR_CLOSE_SOUND = 1036;

/// 铁活板门打开音效
constexpr i32 IRON_TRAPDOOR_OPEN_SOUND = 1037;

/// 末地传送门生成音效（末影龙战斗结束时传送门出现）
/// 参考: net.minecraft.world.level.block.LevelEvent.SOUND_END_PORTAL_SPAWN
constexpr i32 END_PORTAL_SPAWN_SOUND = 1038;

/// 幻翼咬击音效
constexpr i32 PHANTOM_BITE_SOUND = 1039;

/// 僵尸转化为溺尸音效
constexpr i32 ZOMBIE_CONVERT_TO_DROWNED_SOUND = 1040;

/// 尸壳转化为僵尸音效
constexpr i32 HUSK_CONVERT_TO_ZOMBIE_SOUND = 1041;

/// 研磨台使用音效
constexpr i32 GRINDSTONE_USE_SOUND = 1042;

/// 书翻页音效
constexpr i32 ITEM_BOOK_TURN_PAGE_SOUND = 1043;

/// 锻造台使用音效
/// 参考: net.minecraft.world.level.block.LevelEvent.SOUND_SMITHING_TABLE_USED
constexpr i32 SMITHING_TABLE_USE_SOUND = 1044;

/// 滴石着陆音效
/// 参考: net.minecraft.world.level.block.LevelEvent.SOUND_POINTED_DRIPSTONE_LAND
constexpr i32 POINTED_DRIPSTONE_LAND_SOUND = 1045;

/// 岩浆滴入炼药锅音效
/// 参考: net.minecraft.world.level.block.LevelEvent.SOUND_DRIP_LAVA_INTO_CAULDRON
constexpr i32 DRIP_LAVA_INTO_CAULDRON_SOUND = 1046;

/// 水滴入炼药锅音效
/// 参考: net.minecraft.world.level.block.LevelEvent.SOUND_DRIP_WATER_INTO_CAULDRON
constexpr i32 DRIP_WATER_INTO_CAULDRON_SOUND = 1047;

/// 骷髅转化为流浪者音效
/// 参考: net.minecraft.world.level.block.LevelEvent.SOUND_SKELETON_TO_STRAY
constexpr i32 SKELETON_CONVERT_TO_STRAY_SOUND = 1048;

/// 合成器合成成功音效
/// 参考: net.minecraft.world.level.block.LevelEvent.SOUND_CRAFTER_CRAFT
constexpr i32 CRAFTER_CRAFT_SOUND = 1049;

/// 合成器合成失败音效
/// 参考: net.minecraft.world.level.block.LevelEvent.SOUND_CRAFTER_FAIL
constexpr i32 CRAFTER_FAIL_SOUND = 1050;

/// 风弹射击音效
/// 参考: net.minecraft.world.level.block.LevelEvent.SOUND_WIND_CHARGE_SHOOT
constexpr i32 WIND_CHARGE_SHOOT_SOUND = 1051;

// ============================================================================
// 特殊效果事件 (1500-1505)
// ============================================================================

/// 堆肥桶填充事件
/// data > 0: 堆肥桶还有空间可填充
/// data <= 0: 堆肥桶已满
constexpr i32 COMPOSTER_FILLED_UP = 1500;

/// 岩浆熄灭事件（水接触岩浆）
constexpr i32 LAVA_EXTINGUISH = 1501;

/// 红石火把熄灭事件
constexpr i32 REDSTONE_TORCH_BURNOUT = 1502;

/// 末地传送门框架填充事件（放置末影之眼）
constexpr i32 END_PORTAL_FRAME_FILL = 1503;

/// 滴石滴水粒子/音效事件（钟乳石尖端滴水到炼药锅或泥巴变粘土时触发）
constexpr i32 DRIPSTONE_DRIP = 1504;

/// 植物生长粒子与音效事件（蜜蜂授粉促进作物生长时触发）
/// 与 BONEMEAL_PARTICLES(2005) 的区别：2005 是骨粉专用，同时播放骨粉使用音效和粒子；
/// 1505 是植物生长通用效果，播放音效和生成粒子，但不包含骨粉特有音效。
/// MC 1.21 将此事件从 2011 迁移至 1505，同时保留了 2011 的兼容。
/// 参考: net.minecraft.world.level.block.LevelEvent.PARTICLES_AND_SOUND_PLANT_GROWTH
constexpr i32 PLANT_GROWTH_EFFECT = 1505;

// ============================================================================
// 粒子/效果事件 (2000-2013)
// ============================================================================

/// 发射器烟雾粒子
/// data 为烟雾方向（Direction.getIndex()）
constexpr i32 DISPENSER_SMOKE = 2000;

/// 方块破坏效果
/// data 为方块状态ID（Block.getStateId）
constexpr i32 BREAK_BLOCK_EFFECTS = 2001;

/// 即时药水效果粒子
/// data 为药水颜色RGB整数
constexpr i32 POTION_IMPACT_INSTANT = 2002;

/// 末影之眼破碎效果
constexpr i32 ENDER_EYE_SHATTER = 2003;

/// 生物刷怪笼粒子
constexpr i32 MOB_SPAWNER_PARTICLES = 2004;

/// 骨粉粒子效果
/// data 为粒子数量（0则生成15个）
constexpr i32 BONEMEAL_PARTICLES = 2005;

/// 末影龙火球命中效果
constexpr i32 DRAGON_FIREBALL_HIT = 2006;

/// 药水效果粒子
/// data 为药水颜色RGB整数
constexpr i32 POTION_IMPACT = 2007;

/// 爆炸粒子效果
constexpr i32 SPAWN_EXPLOSION_PARTICLE = 2008;

/// 湿海绵在下界变干蒸汽效果
/// 生成 8 个云粒子（蒸汽），并播放火焰熄灭音效
/// 参考: net.minecraft.block.WetSpongeBlock.onBlockAdded
constexpr i32 WET_SPONGE_DRY = 2009;

/// 白烟粒子效果（方向性）
/// 与 DISPENSER_SMOKE(2000) 类似但为白色烟雾
/// data 为烟雾方向（Direction.getIndex()）
/// 参考: net.minecraft.world.level.block.LevelEvent.PARTICLES_SHOOT_WHITE_SMOKE
constexpr i32 SHOOT_WHITE_SMOKE = 2010;

/// 植物生长粒子效果（蜜蜂授粉促进作物生长）
/// data 为粒子数量（通常为15）
/// 与 BONEMEAL_PARTICLES(2005) 的区别：2005 同时播放骨粉使用音效，
/// 2011 仅播放生长粒子，无骨粉音效。
/// 注意: MC 1.21 将植物生长效果迁移至 1505，2011 保留兼容。
/// 参考: net.minecraft.world.level.block.LevelEvent.PARTICLES_BEE_GROWTH
constexpr i32 PLANT_GROWTH_PARTICLES = 2011;

/// 海龟蛋放置粒子效果
/// 参考: net.minecraft.world.level.block.LevelEvent.PARTICLES_TURTLE_EGG_PLACEMENT
constexpr i32 TURTLE_EGG_PLACEMENT = 2012;

/// 猛击攻击粒子效果（监守者猛击地面的冲击波）
/// 参考: net.minecraft.world.level.block.LevelEvent.PARTICLES_SMASH_ATTACK
constexpr i32 SMASH_ATTACK = 2013;

// ============================================================================
// 末地传送门事件 (3000-3001)
// ============================================================================

/// 末地传送门生成效果
constexpr i32 GATEWAY_SPAWN_EFFECTS = 3000;

/// 末影人咆哮音效
constexpr i32 ENDERMAN_GROWL_SOUND = 3001;

// ============================================================================
// 新增事件 (3002-3021)
// ============================================================================

/// 电火花粒子效果（红石相关）
/// 参考: net.minecraft.world.level.block.LevelEvent.PARTICLES_ELECTRIC_SPARK
constexpr i32 ELECTRIC_SPARK = 3002;

/// 涂蜡音效与粒子效果（铜块涂蜡时触发）
/// 参考: net.minecraft.world.level.block.LevelEvent.PARTICLES_AND_SOUND_WAX_ON
constexpr i32 WAX_ON = 3003;

/// 除蜡粒子效果（铜块除蜡时触发）
/// 参考: net.minecraft.world.level.block.LevelEvent.PARTICLES_WAX_OFF
constexpr i32 WAX_OFF = 3004;

/// 刮削粒子效果（铜块刮削时触发）
/// 参考: net.minecraft.world.level.block.LevelEvent.PARTICLES_SCRAPE
constexpr i32 SCRAPE = 3005;

/// 幽匿充能粒子效果
/// 参考: net.minecraft.world.level.block.LevelEvent.PARTICLES_SCULK_CHARGE
constexpr i32 SCULK_CHARGE = 3006;

/// 幽匿尖啸体粒子效果
/// 参考: net.minecraft.world.level.block.LevelEvent.PARTICLES_SCULK_SHRIEK
constexpr i32 SCULK_SHRIEK = 3007;

/// 考古刷完成粒子与音效效果
/// 参考: net.minecraft.world.level.block.LevelEvent.PARTICLES_AND_SOUND_BRUSH_BLOCK_COMPLETE
constexpr i32 BRUSH_BLOCK_COMPLETE = 3008;

/// 蛋壳破裂粒子效果
/// 参考: net.minecraft.world.level.block.LevelEvent.PARTICLES_EGG_CRACK
constexpr i32 EGG_CRACK = 3009;

/// 试炼刷怪笼生成粒子效果
/// 参考: net.minecraft.world.level.block.LevelEvent.PARTICLES_TRIAL_SPAWNER_SPAWN
constexpr i32 TRIAL_SPAWNER_SPAWN = 3011;

/// 试炼刷怪笼在指定位置生成生物粒子效果
/// 参考: net.minecraft.world.level.block.LevelEvent.PARTICLES_TRIAL_SPAWNER_SPAWN_MOB_AT
constexpr i32 TRIAL_SPAWNER_SPAWN_MOB_AT = 3012;

/// 试炼刷怪笼检测玩家粒子效果
/// 参考: net.minecraft.world.level.block.LevelEvent.PARTICLES_TRIAL_SPAWNER_DETECT_PLAYER
constexpr i32 TRIAL_SPAWNER_DETECT_PLAYER = 3013;

/// 宝库激活动画效果
/// 参考: net.minecraft.world.level.block.LevelEvent.ANIMATION_VAULT_ACTIVATE
constexpr i32 VAULT_ACTIVATE = 3015;

/// 宝库停用动画效果
/// 参考: net.minecraft.world.level.block.LevelEvent.ANIMATION_VAULT_DEACTIVATE
constexpr i32 VAULT_DEACTIVATE = 3016;

/// 宝库弹出物品动画效果
/// 参考: net.minecraft.world.level.block.LevelEvent.ANIMATION_VAULT_EJECT_ITEM
constexpr i32 VAULT_EJECT_ITEM = 3017;

/// 蛛网生成动画效果
/// 参考: net.minecraft.world.level.block.LevelEvent.ANIMATION_SPAWN_COBWEB
constexpr i32 SPAWN_COBWEB = 3018;

/// 试炼刷怪笼检测玩家（不祥）粒子效果
/// 参考: net.minecraft.world.level.block.LevelEvent.PARTICLES_TRIAL_SPAWNER_DETECT_PLAYER_OMINOUS
constexpr i32 TRIAL_SPAWNER_DETECT_PLAYER_OMINOUS = 3019;

/// 试炼刷怪笼变为不祥状态粒子效果
/// 参考: net.minecraft.world.level.block.LevelEvent.PARTICLES_TRIAL_SPAWNER_BECOME_OMINOUS
constexpr i32 TRIAL_SPAWNER_BECOME_OMINOUS = 3020;

/// 试炼刷怪笼弹出物品粒子效果
/// 参考: net.minecraft.world.level.block.LevelEvent.PARTICLES_TRIAL_SPAWNER_SPAWN_ITEM
constexpr i32 TRIAL_SPAWNER_SPAWN_ITEM = 3021;

} // namespace WorldEvents

} // namespace mc::world
