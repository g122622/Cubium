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
 * 取值与世界无关：一个格子属于哪种类型完全由方块状态、流体与相邻方块决定；
 * 某个类型对某个实体是否可走、代价多大，由 `MobEntity::getPathfindingMalus` 决定。
 *
 * 枚举次序与原版 `PathType` 一致。次序只影响调试输出的可读性，不参与任何逻辑；
 * 逻辑上唯一的依赖是 `getPathMalus` 的查表，那张表按同一顺序书写。
 */
enum class PathType : u8 {
    /// 完全阻塞
    Blocked = 0,
    /// 空气（可跌落穿过）
    Open = 1,
    /// 可行走的实心地面
    Walkable = 2,
    /// 可穿过的关闭木门（需要开门能力）
    WalkableDoor = 3,
    /// 活板门、睡莲、大型垂滴叶叶片
    Trapdoor = 4,
    /// 细雪（可陷入，但寻路视为不可站立）
    PowderSnow = 5,
    /// 细雪上方的危险格
    DangerPowderSnow = 6,
    /// 栅栏、墙、关闭的栅栏门
    Fence = 7,
    /// 岩浆
    Lava = 8,
    /// 水
    Water = 9,
    /// 水边缘
    WaterBorder = 10,
    /// 铁轨
    Rail = 11,
    /// 与实体当前位置不对齐的铁轨（上下车才可通行）
    UnpassableRail = 12,
    /// 火焰危险区域
    DangerFire = 13,
    /// 站在火上（燃烧伤害）
    DamageFire = 14,
    /// 其他危险区域（仙人掌、甜浆果丛周边）
    DangerOther = 15,
    /// 站在仙人掌 / 甜浆果丛上
    DamageOther = 16,
    /// 打开的门
    DoorOpen = 17,
    /// 关闭的木门
    DoorWoodClosed = 18,
    /// 关闭的铁门
    DoorIronClosed = 19,
    /// 水面突破（水生生物）
    Breach = 20,
    /// 树叶
    Leaves = 21,
    /// 蜂蜜块
    StickyHoney = 22,
    /// 可可果
    Cocoa = 23,
    /// 需要谨慎通过的伤害格（凋灵玫瑰、滴水石锥）
    DamageCautious = 24,
    /// 活板门上方的危险格
    DangerTrapdoor = 25,
};

/// 路径类型数量（不含哨兵值）。
[[nodiscard]] constexpr size_t pathTypeCount() noexcept
{
    return 26u;
}

/**
 * @brief 路径类型的默认代价（原版 `PathType.getMalus()`）。
 *
 * 负值表示该实体无法进入这种格子；只有 `MobEntity::getPathfindingMalus` 没有为
 * 该类型显式设置时才使用这里的值。
 *
 * @param type 路径类型
 * @return 代价，负值表示不可通行
 */
[[nodiscard]] constexpr f32 getPathMalus(PathType type) noexcept
{
    switch (type) {
        case PathType::Blocked:
            return -1.0f;
        case PathType::Open:
            return 0.0f;
        case PathType::Walkable:
            return 0.0f;
        case PathType::WalkableDoor:
            return 0.0f;
        case PathType::Trapdoor:
            return 0.0f;
        case PathType::PowderSnow:
            return -1.0f;
        case PathType::DangerPowderSnow:
            return 0.0f;
        case PathType::Fence:
            return -1.0f;
        case PathType::Lava:
            return -1.0f;
        case PathType::Water:
            return 8.0f;
        case PathType::WaterBorder:
            return 8.0f;
        case PathType::Rail:
            return 0.0f;
        case PathType::UnpassableRail:
            return -1.0f;
        case PathType::DangerFire:
            return 8.0f;
        case PathType::DamageFire:
            return 16.0f;
        case PathType::DangerOther:
            return 8.0f;
        case PathType::DamageOther:
            return -1.0f;
        case PathType::DoorOpen:
            return 0.0f;
        case PathType::DoorWoodClosed:
            return -1.0f;
        case PathType::DoorIronClosed:
            return -1.0f;
        case PathType::Breach:
            return 4.0f;
        case PathType::Leaves:
            return -1.0f;
        case PathType::StickyHoney:
            return 8.0f;
        case PathType::Cocoa:
            return 0.0f;
        case PathType::DamageCautious:
            return 0.0f;
        case PathType::DangerTrapdoor:
            return 0.0f;
    }
    return -1.0f;
}

/**
 * @brief 路径类型的稳定名称（协议标识，不是本地化文本）。
 *
 * 小写蛇形命名，与原版 `PathType` 的枚举名一一对应。差分探针用它输出类型，
 * 因为两侧枚举的次序不同，名字才是稳定的比较口径。
 *
 * @param type 路径类型
 * @return 名称，生命周期同字符串字面量
 */
[[nodiscard]] inline const char* getPathTypeName(PathType type) noexcept
{
    switch (type) {
        case PathType::Blocked:
            return "blocked";
        case PathType::Open:
            return "open";
        case PathType::Walkable:
            return "walkable";
        case PathType::WalkableDoor:
            return "walkable_door";
        case PathType::Trapdoor:
            return "trapdoor";
        case PathType::PowderSnow:
            return "powder_snow";
        case PathType::DangerPowderSnow:
            return "danger_powder_snow";
        case PathType::Fence:
            return "fence";
        case PathType::Lava:
            return "lava";
        case PathType::Water:
            return "water";
        case PathType::WaterBorder:
            return "water_border";
        case PathType::Rail:
            return "rail";
        case PathType::UnpassableRail:
            return "unpassable_rail";
        case PathType::DangerFire:
            return "danger_fire";
        case PathType::DamageFire:
            return "damage_fire";
        case PathType::DangerOther:
            return "danger_other";
        case PathType::DamageOther:
            return "damage_other";
        case PathType::DoorOpen:
            return "door_open";
        case PathType::DoorWoodClosed:
            return "door_wood_closed";
        case PathType::DoorIronClosed:
            return "door_iron_closed";
        case PathType::Breach:
            return "breach";
        case PathType::Leaves:
            return "leaves";
        case PathType::StickyHoney:
            return "sticky_honey";
        case PathType::Cocoa:
            return "cocoa";
        case PathType::DamageCautious:
            return "damage_cautious";
        case PathType::DangerTrapdoor:
            return "danger_trapdoor";
    }
    return "blocked";
}

} // namespace mc::entity::ai::pathfinding