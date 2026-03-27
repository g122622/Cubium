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
    // 遍历六个方向，通知相邻方块更新
    for (Direction dir : Directions::all()) {
        BlockPos neighborPos = pos.offset(dir);

        const BlockState* neighborState = world.getBlockState(neighborPos.x, neighborPos.y, neighborPos.z);
        if (!neighborState || neighborState->isAir()) {
            continue;
        }

        // 触发邻居更新回调
        Block& neighborBlock = const_cast<Block&>(neighborState->getBlock());
        neighborBlock.neighborChanged(world, neighborPos, block, pos, false);
    }
}

void RedstoneSystem::updateNeighborsExcept(IWorld& world, const BlockPos& pos,
                                           Block& block, Direction skipDirection) {
    // 遍历六个方向，跳过指定方向
    for (Direction dir : Directions::all()) {
        if (dir == skipDirection) {
            continue;
        }

        BlockPos neighborPos = pos.offset(dir);

        const BlockState* neighborState = world.getBlockState(neighborPos.x, neighborPos.y, neighborPos.z);
        if (!neighborState || neighborState->isAir()) {
            continue;
        }

        // 触发邻居更新回调
        Block& neighborBlock = const_cast<Block&>(neighborState->getBlock());
        neighborBlock.neighborChanged(world, neighborPos, block, pos, false);
    }
}

void RedstoneSystem::updateNeighborsHorizontalAndDown(IWorld& world, const BlockPos& pos, Block& block) {
    // 更新水平方向（北、东、南、西）和下方
    for (Direction dir : Directions::horizontal()) {
        BlockPos neighborPos = pos.offset(dir);

        const BlockState* neighborState = world.getBlockState(neighborPos.x, neighborPos.y, neighborPos.z);
        if (!neighborState || neighborState->isAir()) {
            continue;
        }

        Block& neighborBlock = const_cast<Block&>(neighborState->getBlock());
        neighborBlock.neighborChanged(world, neighborPos, block, pos, false);
    }

    // 更新下方
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

} // namespace redstone
} // namespace world
} // namespace mc
