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
#include "common/util/Direction.hpp"
#include "common/world/block/BlockPos.hpp"
#include <vector>

namespace mc {

// Forward declarations
class IWorld;

namespace blocks {

/**
 * @brief 活塞推动结构计算器
 *
 * 计算活塞推动时涉及的方块链，包括：
 * - 要推动的方块列表
 * - 要破坏的方块列表
 * - 黏液块/蜂蜜块的粘连处理
 *
 * 参考: net.minecraft.block.PistonBlockStructureHelper
 */
class PistonStructureHelper {
public:
    /**
     * @brief 构造活塞推动结构计算器
     *
     * @param world 世界引用
     * @param pistonPos 活塞位置
     * @param pistonFacing 活塞朝向
     * @param extending 是否伸出（true=伸出，false=收回）
     */
    PistonStructureHelper(IWorld& world, const BlockPos& pistonPos, Direction pistonFacing, bool extending);

    /**
     * @brief 检查是否可以移动
     *
     * 计算推动链并返回是否可行。
     *
     * @return 如果可以推动返回 true
     */
    [[nodiscard]] bool canMove();

    /**
     * @brief 获取要移动的方块列表
     *
     * @return 要移动的方块位置列表（按推动顺序）
     */
    [[nodiscard]] const std::vector<BlockPos>& getBlocksToMove() const { return m_toMove; }

    /**
     * @brief 获取要破坏的方块列表
     *
     * @return 要破坏的方块位置列表
     */
    [[nodiscard]] const std::vector<BlockPos>& getBlocksToDestroy() const { return m_toDestroy; }

private:
    /**
     * @brief 添加方块线
     *
     * 从原点开始沿指定方向添加方块到推动链。
     *
     * @param origin 起始位置
     * @param facing 检查方向
     * @return 如果可以继续推动返回 true
     */
    [[nodiscard]] bool _addBlockLine(const BlockPos& origin, Direction facing);

    /**
     * @brief 添加分支方块
     *
     * 当遇到粘性方块时，检查并添加与其粘连的方块。
     *
     * @param fromPos 粘性方块位置
     * @return 如果所有分支都可以推动返回 true
     */
    [[nodiscard]] bool _addBranchingBlocks(const BlockPos& fromPos);

    /**
     * @brief 在碰撞时重新排序列表
     *
     * 当推动链中的方块发生碰撞时，重新排序以正确处理推动顺序。
     *
     * @param p1 新添加的方块数量
     * @param p2 碰撞点在列表中的索引
     */
    void _reorderListAtCollision(i32 p1, i32 p2);

    // 世界引用
    IWorld& m_world;

    // 活塞位置
    BlockPos m_pistonPos;

    // 活塞朝向
    Direction m_facing;

    // 是否伸出
    bool m_extending;

    // 要移动的方块（起始位置）
    BlockPos m_blockToMove;

    // 移动方向
    Direction m_moveDirection;

    // 要移动的方块列表
    std::vector<BlockPos> m_toMove;

    // 要破坏的方块列表
    std::vector<BlockPos> m_toDestroy;

    /// 最大推动方块数
    static constexpr i32 MAX_PUSH_BLOCKS = 12;
};

} // namespace blocks
} // namespace mc
