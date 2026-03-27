#include "RedstoneSystem.hpp"
#include "../IWorld.hpp"
#include "../block/Block.hpp"

namespace mc {
namespace world {
namespace redstone {

RedstoneSystem& RedstoneSystem::instance() {
    static RedstoneSystem instance;
    return instance;
}

void RedstoneSystem::updateNeighbors(IWorld& world, const BlockPos& pos, Block& block) {
    for (Direction dir : Directions::all()) {
        BlockPos neighborPos = pos.offset(dir);

        const BlockState* neighborState = world.getBlockState(neighborPos.x, neighborPos.y, neighborPos.z);
        if (!neighborState || neighborState->isAir()) {
            continue;
        }

        Block& neighborBlock = const_cast<Block&>(neighborState->getBlock());
        neighborBlock.neighborChanged(world, neighborPos, block, pos, false);
    }
}

void RedstoneSystem::updateNeighborsExcept(IWorld& world, const BlockPos& pos,
                                           Block& block, Direction skipDirection) {
    for (Direction dir : Directions::all()) {
        if (dir == skipDirection) {
            continue;
        }

        BlockPos neighborPos = pos.offset(dir);

        const BlockState* neighborState = world.getBlockState(neighborPos.x, neighborPos.y, neighborPos.z);
        if (!neighborState || neighborState->isAir()) {
            continue;
        }

        Block& neighborBlock = const_cast<Block&>(neighborState->getBlock());
        neighborBlock.neighborChanged(world, neighborPos, block, pos, false);
    }
}

void RedstoneSystem::updateNeighborsHorizontalAndDown(IWorld& world, const BlockPos& pos, Block& block) {
    for (Direction dir : Directions::horizontal()) {
        BlockPos neighborPos = pos.offset(dir);

        const BlockState* neighborState = world.getBlockState(neighborPos.x, neighborPos.y, neighborPos.z);
        if (!neighborState || neighborState->isAir()) {
            continue;
        }

        Block& neighborBlock = const_cast<Block&>(neighborState->getBlock());
        neighborBlock.neighborChanged(world, neighborPos, block, pos, false);
    }

    BlockPos downPos = pos.down();
    const BlockState* downState = world.getBlockState(downPos.x, downPos.y, downPos.z);
    if (downState && !downState->isAir()) {
        Block& downBlock = const_cast<Block&>(downState->getBlock());
        downBlock.neighborChanged(world, downPos, block, pos, false);
    }
}

void RedstoneSystem::updateComparators(IWorld& world, const BlockPos& pos) {
    // 比较器可以从四个水平方向检测容器
    // 当容器内容变化时，需要更新周围可能存在的比较器
    for (Direction dir : Directions::horizontal()) {
        BlockPos neighborPos = pos.offset(dir);

        const BlockState* neighborState = world.getBlockState(neighborPos.x, neighborPos.y, neighborPos.z);
        if (!neighborState || neighborState->isAir()) {
            continue;
        }

        // 检查是否是比较器（待比较器实现后补充）
        // 如果是，触发其更新
        // 暂时触发邻居更新
        Block& neighborBlock = const_cast<Block&>(neighborState->getBlock());
        if (neighborBlock.canProvidePower(*neighborState)) {
            neighborBlock.neighborChanged(world, neighborPos, neighborBlock, pos, false);
        }
    }
}

void RedstoneSystem::scheduleUpdate(IWorld& world, const BlockPos& pos, Block& block,
                                    i32 delay, tick::TickPriority priority) {
    world.scheduleBlockTick(pos, block, delay, priority);
}

void RedstoneSystem::scheduleExtremelyHighPriorityUpdate(IWorld& world, const BlockPos& pos,
                                                          Block& block, i32 delay) {
    world.scheduleBlockTick(pos, block, delay, tick::TickPriority::ExtremelyHigh);
}

// ============================================================================
// 红石火把烧毁跟踪
// ============================================================================

bool RedstoneSystem::checkAndRecordTorchFlip(const BlockPos& pos, u64 currentTick) {
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
    while (!record.flipTimes.empty() &&
           currentTick - record.flipTimes.front() > static_cast<u64>(BURNOUT_WINDOW)) {
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

bool RedstoneSystem::isTorchBurnedOut(const BlockPos& pos, u64 currentTick) const {
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

void RedstoneSystem::clearTorchRecord(const BlockPos& pos) {
    m_torchRecords.erase(pos);
}

void RedstoneSystem::cleanupBurnoutRecords(u64 currentTick) {
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
