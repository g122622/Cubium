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

#include "PistonStructureHelper.hpp"
#include "common/core/Types.hpp"
#include "common/util/Direction.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/Material.hpp"
#include "common/world/block/blocks/redstone/PistonBlock.hpp"
#include <cstddef>
#include <vector>

namespace mc {
namespace blocks {

PistonStructureHelper::PistonStructureHelper(
    IWorld& world, const BlockPos& pistonPos, Direction pistonFacing, bool extending)
    : m_world(world)
    , m_pistonPos(pistonPos)
    , m_facing(pistonFacing)
    , m_extending(extending)
    , m_blockToMove(extending ? pistonPos.offset(pistonFacing) : pistonPos.offset(pistonFacing, 2))
    , m_moveDirection(extending ? pistonFacing : Directions::opposite(pistonFacing))
{}

bool PistonStructureHelper::canMove()
{
    m_toMove.clear();
    m_toDestroy.clear();

    const BlockState* blockState = m_world.getBlockState(m_blockToMove);
    if (!blockState) {
        return false;
    }

    // 检查起始方块是否可以被推动
    if (!PistonBlock::canPush(*blockState, m_world, m_blockToMove, m_moveDirection, false, m_facing)) {
        // 如果是伸出且方块会被破坏，则可以推动
        if (m_extending && blockState->getMaterial().getPushReaction() == Material::PushReaction::Destroy) {
            m_toDestroy.push_back(m_blockToMove);
            return true;
        }
        return false;
    }

    // 添加方块线
    if (!_addBlockLine(m_blockToMove, m_moveDirection)) {
        return false;
    }

    // 检查粘性方块的分支
    for (size_t i = 0; i < m_toMove.size(); ++i) {
        const BlockPos& blockPos = m_toMove[i];
        const BlockState* state = m_world.getBlockState(blockPos);
        if (state && state->isStickyBlock()) {
            if (!_addBranchingBlocks(blockPos)) {
                return false;
            }
        }
    }

    return true;
}

bool PistonStructureHelper::_addBlockLine(const BlockPos& origin, Direction facingIn)
{
    const BlockState* blockState = m_world.getBlockState(origin);

    // 空气方块
    if (!blockState || blockState->isAir()) {
        return true;
    }

    // 检查是否可以被推动
    if (!PistonBlock::canPush(*blockState, m_world, origin, m_moveDirection, false, facingIn)) {
        return true;
    }

    // 不能是活塞自己
    if (origin == m_pistonPos) {
        return true;
    }

    // 已经在移动列表中
    for (const auto& pos : m_toMove) {
        if (pos == origin) {
            return true;
        }
    }

    i32 count = 1;
    // 检查是否会超过最大推动数量
    if (count + static_cast<i32>(m_toMove.size()) > MAX_PUSH_BLOCKS) {
        return false;
    }

    // 检查粘性方块链
    // 沿着推动反方向查找连续的粘性方块
    while (blockState && blockState->isStickyBlock()) {
        BlockPos prevPos = origin.offset(Directions::opposite(m_moveDirection), count);
        const BlockState* prevState = blockState;
        blockState = m_world.getBlockState(prevPos);

        if (!blockState || blockState->isAir()) {
            break;
        }

        // 检查粘连
        if (!prevState->canStickTo(*blockState)) {
            break;
        }

        // 检查是否可以被推动
        if (!PistonBlock::canPush(
                *blockState, m_world, prevPos, m_moveDirection, false, Directions::opposite(m_moveDirection))) {
            break;
        }

        // 不能是活塞自己
        if (prevPos == m_pistonPos) {
            break;
        }

        ++count;
        if (count + static_cast<i32>(m_toMove.size()) > MAX_PUSH_BLOCKS) {
            return false;
        }
    }

    // 从最远端开始添加到移动列表
    i32 movedBlocks = 0;
    for (i32 i = count - 1; i >= 0; --i) {
        m_toMove.push_back(origin.offset(Directions::opposite(m_moveDirection), i));
        ++movedBlocks;
    }

    // 沿推动方向继续检查
    i32 forwardCount = 1;
    while (true) {
        BlockPos forwardPos = origin.offset(m_moveDirection, forwardCount);

        // 检查是否已经在列表中
        i32 existingIndex = -1;
        for (size_t i = 0; i < m_toMove.size(); ++i) {
            if (m_toMove[i] == forwardPos) {
                existingIndex = static_cast<i32>(i);
                break;
            }
        }

        if (existingIndex > -1) {
            // 碰撞到已有的方块，重新排序
            _reorderListAtCollision(movedBlocks, existingIndex);

            // 检查所有涉及的粘性方块
            for (i32 i = 0; i <= existingIndex + movedBlocks; ++i) {
                const BlockPos& blockPos = m_toMove[i];
                const BlockState* state = m_world.getBlockState(blockPos);
                if (state && state->isStickyBlock()) {
                    if (!_addBranchingBlocks(blockPos)) {
                        return false;
                    }
                }
            }

            return true;
        }

        blockState = m_world.getBlockState(forwardPos);

        // 空气
        if (!blockState || blockState->isAir()) {
            return true;
        }

        // 检查是否可以被推动
        if (!PistonBlock::canPush(*blockState, m_world, forwardPos, m_moveDirection, true, m_moveDirection)) {
            return false;
        }

        // 不能是活塞自己
        if (forwardPos == m_pistonPos) {
            return false;
        }

        // 会被破坏的方块
        if (blockState->getMaterial().getPushReaction() == Material::PushReaction::Destroy) {
            m_toDestroy.push_back(forwardPos);
            return true;
        }

        // 超过最大数量
        if (m_toMove.size() >= static_cast<size_t>(MAX_PUSH_BLOCKS)) {
            return false;
        }

        m_toMove.push_back(forwardPos);
        ++movedBlocks;
        ++forwardCount;
    }
}

void PistonStructureHelper::_reorderListAtCollision(i32 p1, i32 p2)
{
    // 将移动列表分成三部分并重新排序
    std::vector<BlockPos> list1; // 前半部分
    std::vector<BlockPos> list2; // 新添加的部分
    std::vector<BlockPos> list3; // 后半部分

    // list1: 0 到 p2
    for (i32 i = 0; i < p2 && i < static_cast<i32>(m_toMove.size()); ++i) {
        list1.push_back(m_toMove[i]);
    }

    // list2: 从末尾取 p1 个
    i32 startIdx = static_cast<i32>(m_toMove.size()) - p1;
    if (startIdx >= 0) {
        for (i32 i = startIdx; i < static_cast<i32>(m_toMove.size()); ++i) {
            list2.push_back(m_toMove[i]);
        }
    }

    // list3: p2 到 size-p1
    for (i32 i = p2; i < startIdx && i < static_cast<i32>(m_toMove.size()); ++i) {
        list3.push_back(m_toMove[i]);
    }

    // 重新组合
    m_toMove.clear();
    m_toMove.insert(m_toMove.end(), list1.begin(), list1.end());
    m_toMove.insert(m_toMove.end(), list2.begin(), list2.end());
    m_toMove.insert(m_toMove.end(), list3.begin(), list3.end());
}

bool PistonStructureHelper::_addBranchingBlocks(const BlockPos& fromPos)
{
    const BlockState* blockState = m_world.getBlockState(fromPos);
    if (!blockState) {
        return true;
    }

    // 检查所有与推动方向垂直的方向
    for (Direction dir : Directions::all()) {
        if (Directions::getAxis(dir) == Directions::getAxis(m_moveDirection)) {
            continue;
        }

        BlockPos neighborPos = fromPos.offset(dir);
        const BlockState* neighborState = m_world.getBlockState(neighborPos);

        if (neighborState && neighborState->canStickTo(*blockState)) {
            if (!_addBlockLine(neighborPos, dir)) {
                return false;
            }
        }
    }

    return true;
}

} // namespace blocks
} // namespace mc
