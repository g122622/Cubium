#include "world/blockentity/storage/TrappedChestEntity.hpp"
#include "world/IWorld.hpp"
#include "world/World.hpp"
#include "world/block/Block.hpp"
#include "world/blockentity/BlockEntityType.hpp"

namespace mc {
namespace blockentity {

TrappedChestEntity::TrappedChestEntity(const BlockPos& pos)
    : ChestEntity(BlockEntityType::TrappedChest, pos) {
}

i32 TrappedChestEntity::getRedstoneSignal(World& world) const {
    // 信号强度等于打开玩家数，但不超过15
    i32 playerCount = getOpenCount();

    // 如果是双箱，需要计算两个箱子的打开数
    ChestEntity* connected = getConnectedChest(world);
    if (connected) {
        playerCount += connected->getOpenCount();
    }

    return std::min(playerCount, 15);
}

void TrappedChestEntity::openContainer() {
    // 先增加计数
    ChestEntity::openContainer();

    // 通知邻居更新红石信号
    notifyNeighbors(*static_cast<World*>(nullptr)); // TODO: 需要传入World引用
}

void TrappedChestEntity::closeContainer() {
    // 先减少计数
    ChestEntity::closeContainer();

    // 通知邻居更新红石信号
    notifyNeighbors(*static_cast<World*>(nullptr)); // TODO: 需要传入World引用
}

void TrappedChestEntity::notifyNeighbors(World& world) {
    if (!world.isRemote()) {
        // 通知四周和下方的方块更新红石信号
        const Block* block = getBlockState() ? getBlockState()->getBlock() : nullptr;
        if (block) {
            // TODO: 实现邻居通知
            // world.notifyNeighborsOfStateChange(m_pos, block);
            // world.notifyNeighborsOfStateChange(m_pos.down(), block);
        }
    }
}

} // namespace blockentity
} // namespace mc
