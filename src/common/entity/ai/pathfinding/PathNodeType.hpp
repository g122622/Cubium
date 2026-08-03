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
#include <cstddef>

namespace mc::entity::ai::pathfinding {

/**
 * @brief 路径节点类型
 *
 * 定义不同地形节点的可通行性和代价惩罚。
 * 参考 MC 1.16.5 PathNodeType（共23种类型）
 *
 * 注意：CLIMBABLE、DANGER_FALL、TRAPDOOR_DOWN、FENCE_GATE、DANGER_BERRY
 * 是MC后续版本添加的，MC 1.16.5中不存在这些类型。
 */
enum class PathNodeType : u8 {
    /// 完全阻塞，无法通行 (priority = -1.0F)
    Blocked = 0,

    /// 空气，可以跌落通过 (priority = 0.0F)
    Open = 1,

    /// 可行走的地面 (priority = 0.0F)
    Walkable = 2,

    /// 可行走的门 (priority = 0.0F)
    WalkableDoor = 3,

    /// 活板门 (priority = 0.0F)
    Trapdoor = 4,

    /// 栅栏 (priority = -1.0F，不可通行)
    Fence = 5,

    /// 岩浆 (priority = -1.0F，极度危险)
    Lava = 6,

    /// 水 (priority = 8.0F)
    Water = 7,

    /// 水边界 (priority = 8.0F)
    WaterBorder = 8,

    /// 铁轨 (priority = 0.0F)
    Rail = 9,

    /// 不可通行的铁轨 (priority = -1.0F)
    UnpassableRail = 10,

    /// 火焰危险区域 (priority = 8.0F)
    DangerFire = 11,

    /// 火焰伤害 (priority = 16.0F)
    DamageFire = 12,

    /// 仙人掌危险区域 (priority = 8.0F)
    DangerCactus = 13,

    /// 仙人掌伤害 (priority = -1.0F)
    DamageCactus = 14,

    /// 其他危险 (priority = 8.0F)
    DangerOther = 15,

    /// 其他伤害 (priority = -1.0F)
    DamageOther = 16,

    /// 打开的门 (priority = 0.0F)
    DoorOpen = 17,

    /// 关闭的木门 (priority = -1.0F)
    DoorWoodClosed = 18,

    /// 关闭的铁门 (priority = -1.0F)
    DoorIronClosed = 19,

    /// 水面突破 (priority = 4.0F)
    Breach = 20,

    /// 树叶 (priority = -1.0F)
    Leaves = 21,

    /// 粘性蜂蜜块 (priority = 8.0F)
    StickyHoney = 22,

    /// 可可果 (priority = 0.0F)
    Cocoa = 23,

    // MC 1.16.5共23种类型，以下是扩展类型（供后续版本使用）

    /// 甜浆果丛危险区域 (MC 1.15+)
    DangerBerry = 24,

    /// 攀爬（梯子、藤蔓等）(MC 1.16.5后添加)
    Climbable = 25,

    /// 跌落危险 (MC 1.16.5后添加)
    DangerFall = 26,

    /// 栅栏门 (MC 1.16.5后添加)
    FenceGate = 27,

    /// 活板门（可下落）(MC 1.16.5后添加)
    TrapdoorDown = 28,

    /// 其他
    Other = 255,
};

/// PathNodeType 数量（仅用于数组大小推导，不是合法 PathNodeType）
/// 注意：由于枚举底层类型为 u8（最大 255），Count 必须独立于枚举之外定义。
[[nodiscard]] constexpr size_t pathNodeTypeCount() noexcept
{
    return 256u;
}

/**
 * @brief 获取节点类型的代价惩罚
 *
 * MC 1.16.5 PathNodeType.getPriority():
 * - BLOCKED: -1.0F (不可通行)
 * - OPEN, WALKABLE, WALKABLE_DOOR, TRAPDOOR, RAIL, DOOR_OPEN, COCOA, CLIMBABLE, FENCE_GATE, TRAPDOOR_DOWN: 0.0F
 * - WATER, WATER_BORDER, DANGER_FIRE, DANGER_CACTUS, DANGER_OTHER, STICKY_HONEY, DANGER_BERRY: 8.0F
 * - DAMAGE_FIRE: 16.0F
 * - FENCE, LAVA, DAMAGE_CACTUS, DAMAGE_OTHER, LEAVES, DOOR_WOOD_CLOSED, DOOR_IRON_CLOSED, DANGER_FALL: -1.0F
 * - BREACH: 4.0F
 * - UNPASSABLE_RAIL: -1.0F
 *
 * @param type 节点类型
 * @return 代价惩罚值（负值表示危险/不可通行）
 */
[[nodiscard]] inline f32 getPathCostPenalty(PathNodeType type)
{
    switch (type) {
        case PathNodeType::Blocked:
            return -1.0f; // 完全阻塞
        case PathNodeType::Open:
            return 0.0f;
        case PathNodeType::Walkable:
            return 0.0f;
        case PathNodeType::WalkableDoor:
            return 0.0f;
        case PathNodeType::Trapdoor:
            return 0.0f;
        case PathNodeType::Fence:
            return -1.0f; // 不可通行
        case PathNodeType::Lava:
            return -1.0f; // 极度危险
        case PathNodeType::Water:
            return 8.0f; // 高代价但可通行
        case PathNodeType::WaterBorder:
            return 8.0f;
        case PathNodeType::Rail:
            return 0.0f;
        case PathNodeType::UnpassableRail:
            return -1.0f;
        case PathNodeType::DangerFire:
            return 8.0f;
        case PathNodeType::DamageFire:
            return 16.0f; // 非常高代价
        case PathNodeType::DangerCactus:
            return 8.0f;
        case PathNodeType::DamageCactus:
            return -1.0f;
        case PathNodeType::DangerOther:
            return 8.0f;
        case PathNodeType::DamageOther:
            return -1.0f;
        case PathNodeType::DoorOpen:
            return 0.0f;
        case PathNodeType::DoorWoodClosed:
            return -1.0f;
        case PathNodeType::DoorIronClosed:
            return -1.0f;
        case PathNodeType::Breach:
            return 4.0f;
        case PathNodeType::Leaves:
            return -1.0f;
        case PathNodeType::StickyHoney:
            return 8.0f;
        case PathNodeType::Cocoa:
            return 0.0f;
        case PathNodeType::DangerBerry:
            return 8.0f;
        case PathNodeType::Climbable:
            return 0.0f;
        case PathNodeType::DangerFall:
            return -1.0f;
        case PathNodeType::FenceGate:
            return 0.0f;
        case PathNodeType::TrapdoorDown:
            return 0.0f;
        case PathNodeType::Other:
        default:
            return 0.0f;
    }
}

/**
 * @brief 获取危险类型
 *
 * MC 1.16.5 PathNodeType.getDanger():
 * - DAMAGE_FIRE, DANGER_FIRE -> DANGER_FIRE
 * - DAMAGE_CACTUS, DANGER_CACTUS -> DANGER_CACTUS
 * - DAMAGE_OTHER, DANGER_OTHER -> DANGER_OTHER
 * - DANGER_BERRY -> DANGER_BERRY (MC 1.15+)
 * - LAVA -> DAMAGE_FIRE
 * - 其他 -> null
 *
 * @param type 节点类型
 * @return 危险类型，如果不是危险则返回 Blocked（作为 null）
 */
[[nodiscard]] inline PathNodeType getDanger(PathNodeType type)
{
    switch (type) {
        case PathNodeType::DamageFire:
        case PathNodeType::DangerFire:
            return PathNodeType::DangerFire;
        case PathNodeType::DamageCactus:
        case PathNodeType::DangerCactus:
            return PathNodeType::DangerCactus;
        case PathNodeType::DamageOther:
        case PathNodeType::DangerOther:
            return PathNodeType::DangerOther;
        case PathNodeType::DangerBerry:
            return PathNodeType::DangerBerry;
        case PathNodeType::Lava:
            return PathNodeType::DamageFire;
        default:
            return PathNodeType::Blocked; // 表示 null
    }
}

/**
 * @brief 检查节点是否可通行
 *
 * MC 1.16.5: 只有特定类型被认为是可行走的。
 * Open 表示空气/空空间，实体会跌落，不是可行走地面。
 * Blocked、Lava、Fence 等类型有负代价但这里只检查可行走类型。
 *
 * @param type 节点类型
 * @return 是否可通行
 */
[[nodiscard]] inline bool isWalkable(PathNodeType type)
{
    switch (type) {
        case PathNodeType::Walkable:
        case PathNodeType::WalkableDoor:
        case PathNodeType::Water:
        case PathNodeType::Climbable:
        case PathNodeType::DoorOpen:
        case PathNodeType::FenceGate:
        case PathNodeType::Trapdoor:
        case PathNodeType::TrapdoorDown:
        case PathNodeType::Rail:
        case PathNodeType::Cocoa:
        case PathNodeType::Breach:
            return true;
        default:
            return false;
    }
}

/**
 * @brief 检查节点是否危险
 * @param type 节点类型
 * @return 是否为危险节点
 */
[[nodiscard]] inline bool isDangerous(PathNodeType type)
{
    return getDanger(type) != PathNodeType::Blocked;
}

} // namespace mc::entity::ai::pathfinding
