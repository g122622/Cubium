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
 * IMPLIED, INCLUDING BUT NOT LIMITED TO ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#pragma once

#include "common/core/Types.hpp"
#include "common/world/block/BlockPos.hpp"
#include <unordered_map>

namespace mc {

// 前向声明
class Entity;
class LivingEntity;
class Item;
class IInventory;
namespace entity {
class VillagerEntity;

namespace ai {
namespace goal {
namespace villager {

/**
 * @brief 计算实体到方块位置中心的距离平方
 *
 * 使用方块中心点 (x+0.5, y, z+0.5) 计算距离，
 * 适用于检测实体是否到达某个方块位置。
 *
 * @param entity 实体
 * @param pos 方块位置
 * @return 距离平方
 */
[[nodiscard]] f32 distanceToBlockCenter(const Entity* entity, const BlockPos& pos);

/**
 * @brief 检查实体是否在指定距离内
 *
 * @param entity 实体
 * @param pos 方块位置
 * @param maxDistance 最大距离
 * @return 是否在范围内
 */
[[nodiscard]] bool isWithinDistance(const Entity* entity, const BlockPos& pos, f32 maxDistance);

/**
 * @brief 从库存中抛出一半匹配的物品给目标实体
 *
 * 遍历库存，找到第一个匹配 itemFilter 的物品：
 * - 如果数量 > maxStackSize/2，抛出 count/2 个
 * - 如果数量 > 24 但不超过半组，保留24个，抛出剩余
 *
 * @param villager 分享物品的村民
 * @param inventory 源村民库存
 * @param itemFilter 允许抛出的物品及其点数映射
 * @param target 目标实体
 * @return 是否成功抛出了物品
 */
bool throwHalfStackToTarget(VillagerEntity* villager,
    IInventory& inventory,
    const std::unordered_map<const Item*, i32>& itemFilter,
    LivingEntity* target);

} // namespace villager
} // namespace goal
} // namespace ai
} // namespace entity
} // namespace mc
