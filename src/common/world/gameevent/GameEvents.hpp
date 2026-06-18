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

#include "GameEvent.hpp"

namespace mc::gameevent {

/**
 * @brief 游戏事件常量
 *
 * 定义所有 Minecraft 原版游戏事件，每个事件包含标识符和通知半径。
 * 大多数事件的默认通知半径为 16 格，少数事件有特殊半径。
 *
 * 与 WorldEvents（世界事件/levelEvent）不同：
 * - WorldEvents 是服务端向客户端广播的音效/粒子事件（如门开关、唱片播放）
 * - GameEvents 是服务端内部事件分发，通知附近的 GameEventListener（如幽匿感测体）
 *
 * 参考: net.minecraft.world.level.gameevent.GameEvent (MC 1.21.11)
 */
namespace GameEvents {

// ============================================================================
// 方块事件
// ============================================================================

/// 方块激活（如拉杆拉动、按钮按下）
inline const GameEvent BLOCK_ACTIVATE("block_activate");

/// 方块附着（如绊线钩连接）
inline const GameEvent BLOCK_ATTACH("block_attach");

/// 方块变化（如炼药锅水位变化、洞穴藤蔓生长）
inline const GameEvent BLOCK_CHANGE("block_change");

/// 方块关闭（如门关闭、陷阱箱关闭）
inline const GameEvent BLOCK_CLOSE("block_close");

/// 方块失活（如拉杆复位、按钮弹起）
inline const GameEvent BLOCK_DEACTIVATE("block_deactivate");

/// 方块销毁
inline const GameEvent BLOCK_DESTROY("block_destroy");

/// 方块脱离（如绊线断开）
inline const GameEvent BLOCK_DETACH("block_detach");

/// 方块打开（如门打开、箱子打开）
inline const GameEvent BLOCK_OPEN("block_open");

/// 方块放置
inline const GameEvent BLOCK_PLACE("block_place");

// ============================================================================
// 容器事件
// ============================================================================

/// 容器关闭
inline const GameEvent CONTAINER_CLOSE("container_close");

/// 容器打开
inline const GameEvent CONTAINER_OPEN("container_open");

// ============================================================================
// 实体事件
// ============================================================================

/// 实体饮用
inline const GameEvent DRINK("drink");

/// 实体进食
inline const GameEvent EAT("eat");

/// 鞘翅滑翔
inline const GameEvent ELYTRA_GLIDE("elytra_glide");

/// 实体受伤
inline const GameEvent ENTITY_DAMAGE("entity_damage");

/// 实体死亡
inline const GameEvent ENTITY_DIE("entity_die");

/// 实体下坐骑
inline const GameEvent ENTITY_DISMOUNT("entity_dismount");

/// 实体交互
inline const GameEvent ENTITY_INTERACT("entity_interact");

/// 实体上坐骑
inline const GameEvent ENTITY_MOUNT("entity_mount");

/// 实体放置（如盔甲架放置）
inline const GameEvent ENTITY_PLACE("entity_place");

/// 实体动作
inline const GameEvent ENTITY_ACTION("entity_action");

/// 装备更换
inline const GameEvent EQUIP("equip");

/// 卸下装备
inline const GameEvent UNEQUIP("unequip");

// ============================================================================
// 环境事件
// ============================================================================

/// 爆炸
inline const GameEvent EXPLODE("explode");

/// 振翅（如鹦鹉扇翅）
inline const GameEvent FLAP("flap");

/// 流体拾取（如从炼药锅取水）
inline const GameEvent FLUID_PICKUP("fluid_pickup");

/// 流体放置（如向炼药锅注水）
inline const GameEvent FLUID_PLACE("fluid_place");

/// 落地
inline const GameEvent HIT_GROUND("hit_ground");

/// 闪电击中（通知半径 32）
inline const GameEvent LIGHTNING_STRIKE("lightning_strike");

/// 溅水
inline const GameEvent SPLASH("splash");

/// 行走
inline const GameEvent STEP("step");

/// 游泳
inline const GameEvent SWIM("swim");

/// 传送
inline const GameEvent TELEPORT("teleport");

// ============================================================================
// 物品与交互事件
// ============================================================================

/// 乐器演奏
inline const GameEvent INSTRUMENT_PLAY("instrument_play");

/// 物品交互完成
inline const GameEvent ITEM_INTERACT_FINISH("item_interact_finish");

/// 物品交互开始
inline const GameEvent ITEM_INTERACT_START("item_interact_start");

/// 音符盒演奏
inline const GameEvent NOTE_BLOCK_PLAY("note_block_play");

/// 引信点燃（如 TNT 点燃）
inline const GameEvent PRIME_FUSE("prime_fuse");

/// 弹射物落地
inline const GameEvent PROJECTILE_LAND("projectile_land");

/// 弹射物发射
inline const GameEvent PROJECTILE_SHOOT("projectile_shoot");

/// 剪切
inline const GameEvent SHEAR("shear");

// ============================================================================
// 唱片机事件（通知半径 10）
// ============================================================================

/// 唱片机播放（通知半径 10 格）
inline const GameEvent JUKEBOX_PLAY("jukebox_play", 10);

/// 唱片机停止播放（通知半径 10 格）
inline const GameEvent JUKEBOX_STOP_PLAY("jukebox_stop_play", 10);

// ============================================================================
// 幽匿事件
// ============================================================================

/// 幽匿感测体触须点击
inline const GameEvent SCULK_SENSOR_TENDRILS_CLICKING("sculk_sensor_tendrils_clicking");

/// 尖啸（通知半径 32 格，如幽匿尖啸体发出）
inline const GameEvent SHRIEK("shriek", 32);

/// 共鸣频率 1-15（由幽匿感测体共振产生）
inline const GameEvent RESONATE_1("resonate_1");
inline const GameEvent RESONATE_2("resonate_2");
inline const GameEvent RESONATE_3("resonate_3");
inline const GameEvent RESONATE_4("resonate_4");
inline const GameEvent RESONATE_5("resonate_5");
inline const GameEvent RESONATE_6("resonate_6");
inline const GameEvent RESONATE_7("resonate_7");
inline const GameEvent RESONATE_8("resonate_8");
inline const GameEvent RESONATE_9("resonate_9");
inline const GameEvent RESONATE_10("resonate_10");
inline const GameEvent RESONATE_11("resonate_11");
inline const GameEvent RESONATE_12("resonate_12");
inline const GameEvent RESONATE_13("resonate_13");
inline const GameEvent RESONATE_14("resonate_14");
inline const GameEvent RESONATE_15("resonate_15");

} // namespace GameEvents

} // namespace mc::gameevent
