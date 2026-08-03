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

#include "ItemUseContext.hpp"
#include "common/core/Types.hpp"
#include "common/util/Direction.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockState.hpp"
#include <vector>

namespace mc {

/**
 * @brief 方块放置上下文
 *
 * 继承自 ItemUseContext，提供方块放置专用的上下文信息。
 * 包含放置位置计算、可替换方块检测等功能。
 */
class BlockItemUseContext : public ItemUseContext {
public:
    /**
     * @brief 构造方块放置上下文
     * @param world 世界引用（IWorld接口）
     * @param player 玩家指针（可为nullptr）
     * @param stack 物品堆
     * @param hitPos 击中点（世界坐标）
     * @param blockPos 击中的方块位置
     * @param face 击中的面
     * @param playerYaw 玩家yaw角度（用于计算水平朝向）
     * @param playerPitch 玩家pitch角度（度数，正值俯视，负值仰视；用于 getNearestLookingDirections 排序）
     */
    BlockItemUseContext(IWorld& world,
        Player* player,
        const ItemStack& stack,
        const Vector3& hitPos,
        const BlockPos& blockPos,
        Direction face,
        f32 playerYaw,
        f32 playerPitch);

    ~BlockItemUseContext() override = default;

    // ========== 放置位置 ==========

    /**
     * @brief 获取实际放置位置
     *
     * 如果点击的方块可替换，返回点击的方块位置；
     * 否则返回相邻位置（击中面的另一侧）。
     *
     * @return 放置位置
     */
    [[nodiscard]] BlockPos placementPos() const { return m_placementPos; }

    /**
     * @brief 获取相邻位置（击中面的另一侧）
     * @return 相邻方块位置
     */
    [[nodiscard]] BlockPos adjacentPos() const { return m_adjacentPos; }

    // ========== 状态检查 ==========

    /**
     * @brief 检查是否可以放置方块
     *
     * 检查放置位置是否可以放置方块：
     * 1. 放置位置在世界边界内
     * 2. 放置位置为空气或可替换方块
     *
     * @return 是否可以放置
     */
    [[nodiscard]] bool canPlace() const;

    /**
     * @brief 是否替换点击的方块
     *
     * 如果点击的方块可替换（如水、草等），则为 true，
     * 此时 placementPos() 等于 blockPos()。
     *
     * @return 是否替换点击的方块
     */
    [[nodiscard]] bool replacingClickedBlock() const { return m_replacingClickedBlock; }

    // ========== 方向相关 ==========

    /**
     * @brief 获取玩家水平朝向
     *
     * 根据玩家的 yaw 角度计算面向的方向。
     * 返回 NORTH, SOUTH, EAST 或 WEST。
     *
     * @return 水平方向
     */
    [[nodiscard]] Direction horizontalDirection() const { return m_horizontalDirection; }

    /**
     * @brief 获取放置时的面向方向
     *
     * 对于需要朝向的方块（如楼梯、门），返回方块应该面向的方向。
     * 通常是玩家的朝向的反方向。
     *
     * @return 方块应该面向的方向
     */
    [[nodiscard]] Direction placementDirection() const;

    /**
     * @brief 获取放置位置当前方块状态
     * @return 方块状态，如果位置无效返回 nullptr
     */
    [[nodiscard]] const BlockState* getBlockStateAtPlacementPos() const;

    /**
     * @brief 获取世界引用
     * @return 世界引用
     */
    [[nodiscard]] IWorld& getWorld() { return m_world; }
    [[nodiscard]] const IWorld& getWorld() const { return m_world; }

    /**
     * @brief 获取物品堆（可修改）
     * @return 物品堆引用
     */
    [[nodiscard]] ItemStack& getItemStack() { return ItemUseContext::getItemStackMut(); }

    /**
     * @brief 获取物品堆（只读）
     * @return 物品堆引用
     */
    [[nodiscard]] const ItemStack& getItemStack() const { return ItemUseContext::getItemStack(); }

    /**
     * @brief 获取玩家视线方向的优先级列表
     *
     * 返回按玩家视线方向排序的方向列表。
     * 第一个方向是玩家面向的方向，后面是其他方向。
     * 用于需要找到可附着面的方块（如可可豆）。
     *
     * @return 方向列表（从最优先到最低优）
     */
    [[nodiscard]] std::vector<Direction> getNearestLookingDirections() const;

private:
    /**
     * @brief 初始化放置上下文
     */
    void _initialize();

    /**
     * @brief 检查方块是否可替换
     * @param pos 方块位置
     * @return 是否可替换
     */
    [[nodiscard]] bool _canReplace(const BlockPos& pos) const;

    BlockPos m_adjacentPos;          // 相邻位置（击中面的另一侧）
    BlockPos m_placementPos;         // 实际放置位置
    bool m_replacingClickedBlock;    // 是否替换点击的方块
    Direction m_horizontalDirection; // 玩家水平朝向
};

} // namespace mc
