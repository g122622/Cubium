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

#include "RedstoneSystem.hpp"
#include "../IWorld.hpp"
#include "../block/Block.hpp"
#include "../tick/manager/TickManager.hpp"
#include "common/core/Types.hpp"
#include "common/util/Direction.hpp"
#include "common/world/tick/base/TickPriority.hpp"
#include <cstddef>
#include <utility>

namespace mc {
namespace world {
namespace redstone {

RedstoneSystem& RedstoneSystem::instance()
{
    static RedstoneSystem instance;
    return instance;
}

void RedstoneSystem::_notifyNeighbor(IWorld& world,
    const BlockPos& neighborPos,
    const BlockState& neighborState,
    Block& sourceBlock,
    const BlockPos& sourcePos)
{
    // 使用 IWorld 的封装方法，避免直接使用 const_cast
    world.notifyNeighborChanged(neighborPos, neighborState, sourceBlock, sourcePos, false);
}

void RedstoneSystem::_updateNeighborsInDirections(
    IWorld& world, const BlockPos& pos, Block& block, const Direction* directions, size_t directionCount)
{
    for (size_t i = 0; i < directionCount; ++i) {
        Direction dir = directions[i];
        BlockPos neighborPos = pos.offset(dir);

        const BlockState* neighborState = world.getBlockState(neighborPos);
        if (!neighborState || neighborState->isAir()) {
            continue;
        }

        _notifyNeighbor(world, neighborPos, *neighborState, block, pos);
    }
}

void RedstoneSystem::updateNeighbors(IWorld& world, const BlockPos& pos, Block& block)
{
    // 防止无限递归更新
    // 如果当前位置正在更新，则跳过
    if (m_context.isUpdating(pos)) {
        return;
    }

    // 记录更新深度
    if (!m_context.canPushDepth()) {
        // 超过最大深度，停止更新
        return;
    }

    m_context.beginUpdate(pos);
    m_context.pushDepth();

    const auto& allDirs = Directions::all();
    _updateNeighborsInDirections(world, pos, block, allDirs.data(), allDirs.size());

    m_context.popDepth();
    m_context.endUpdate(pos);
}

void RedstoneSystem::updateNeighborsExcept(IWorld& world, const BlockPos& pos, Block& block, Direction skipDirection)
{
    // 防止无限递归更新
    if (m_context.isUpdating(pos)) {
        return;
    }

    if (!m_context.canPushDepth()) {
        return;
    }

    m_context.beginUpdate(pos);
    m_context.pushDepth();

    Direction directions[5];
    size_t count = 0;
    for (Direction dir : Directions::all()) {
        if (dir != skipDirection) {
            directions[count++] = dir;
        }
    }
    _updateNeighborsInDirections(world, pos, block, directions, count);

    m_context.popDepth();
    m_context.endUpdate(pos);
}

void RedstoneSystem::updateNeighborsHorizontalAndDown(IWorld& world, const BlockPos& pos, Block& block)
{
    // 防止无限递归更新
    if (m_context.isUpdating(pos)) {
        return;
    }

    if (!m_context.canPushDepth()) {
        return;
    }

    m_context.beginUpdate(pos);
    m_context.pushDepth();

    const auto& horizDirs = Directions::horizontal();
    Direction directions[5];
    size_t count = 0;
    for (Direction dir : horizDirs) {
        directions[count++] = dir;
    }
    directions[count++] = Direction::Down;
    _updateNeighborsInDirections(world, pos, block, directions, count);

    m_context.popDepth();
    m_context.endUpdate(pos);
}

void RedstoneSystem::updateComparators(IWorld& world, const BlockPos& pos)
{
    // 比较器可以从四个水平方向检测容器
    // 当容器内容变化时，需要更新周围可能存在的比较器

    // 获取容器方块（触发源）
    const BlockState* sourceState = world.getBlockState(pos);
    if (!sourceState || sourceState->isAir()) {
        return;
    }

    for (Direction dir : Directions::horizontal()) {
        BlockPos neighborPos = pos.offset(dir);

        const BlockState* neighborState = world.getBlockState(neighborPos);
        if (!neighborState || neighborState->isAir()) {
            continue;
        }

        const Block& neighborBlock = neighborState->getBlock();
        // 检查是否是红石元件（包括比较器）
        if (neighborBlock.canProvidePower(*neighborState)) {
            // 使用 IWorld 的封装方法
            world.notifyNeighborChanged(neighborPos, *neighborState, sourceState->getBlockMutable(), pos, false);
        }
    }
}

void RedstoneSystem::scheduleUpdate(
    IWorld& world, const BlockPos& pos, Block& block, i32 delay, tick::TickPriority priority)
{
    world.tickManager().scheduleBlockTick(pos, block, delay, priority);
}

void RedstoneSystem::scheduleExtremelyHighPriorityUpdate(IWorld& world, const BlockPos& pos, Block& block, i32 delay)
{
    world.tickManager().scheduleBlockTick(pos, block, delay, tick::TickPriority::ExtremelyHigh);
}

// ============================================================================
// 红石火把烧毁跟踪
// ============================================================================

bool RedstoneSystem::checkAndRecordTorchFlip(const BlockPos& pos, u64 currentTick)
{
    auto it = m_torchRecords.find(pos);
    if (it == m_torchRecords.end()) {
        // 创建新记录
        TorchBurnoutRecord record;
        record.flipTimes.push_back(currentTick);
        m_torchRecords[pos] = std::move(record);
        return false;
    }

    TorchBurnoutRecord& record = it->second;

    // 检查是否仍在烧毁冷却中
    if (record.isBurnedOut) {
        if (currentTick - record.burnoutTime < static_cast<u64>(BURNOUT_COOLDOWN)) {
            // 仍在冷却中
            return true;
        }
        // 冷却结束，重置状态
        record.isBurnedOut = false;
        record.flipTimes.clear();
    }

    // 移除过期的翻转记录
    while (!record.flipTimes.empty() && currentTick - record.flipTimes.front() > static_cast<u64>(BURNOUT_WINDOW)) {
        record.flipTimes.pop_front();
    }

    // 记录本次翻转
    record.flipTimes.push_back(currentTick);

    // 检查是否达到烧毁阈值
    if (static_cast<i32>(record.flipTimes.size()) >= BURNOUT_FLIPS) {
        // 烧毁！
        record.isBurnedOut = true;
        record.burnoutTime = currentTick;
        return true;
    }

    return false;
}

bool RedstoneSystem::isTorchBurnedOut(const BlockPos& pos, u64 currentTick) const
{
    auto it = m_torchRecords.find(pos);
    if (it == m_torchRecords.end()) {
        return false;
    }

    const TorchBurnoutRecord& record = it->second;
    if (!record.isBurnedOut) {
        return false;
    }

    // 检查是否仍在冷却中
    return currentTick - record.burnoutTime < static_cast<u64>(BURNOUT_COOLDOWN);
}

void RedstoneSystem::clearTorchRecord(const BlockPos& pos)
{
    m_torchRecords.erase(pos);
}

void RedstoneSystem::cleanupBurnoutRecords(u64 currentTick)
{
    // 清理已过冷却期的记录
    auto it = m_torchRecords.begin();
    while (it != m_torchRecords.end()) {
        TorchBurnoutRecord& record = it->second;

        if (record.isBurnedOut) {
            // 烧毁记录：检查是否已过冷却期
            if (currentTick - record.burnoutTime >= static_cast<u64>(BURNOUT_COOLDOWN)) {
                it = m_torchRecords.erase(it);
                continue;
            }
        } else {
            // 正常记录：清理过期的翻转时间
            while (!record.flipTimes.empty() &&
                currentTick - record.flipTimes.front() > static_cast<u64>(BURNOUT_WINDOW)) {
                record.flipTimes.pop_front();
            }

            // 如果没有翻转记录，删除整个记录
            if (record.flipTimes.empty()) {
                it = m_torchRecords.erase(it);
                continue;
            }
        }

        ++it;
    }
}

} // namespace redstone
} // namespace world
} // namespace mc
