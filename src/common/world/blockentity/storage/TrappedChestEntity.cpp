#include "world/blockentity/storage/TrappedChestEntity.hpp"
#include "world/IWorld.hpp"
#include "world/block/Block.hpp"
#include "world/blockentity/BlockEntityType.hpp"

namespace mc {
namespace blockentity {

TrappedChestEntity::TrappedChestEntity(const BlockPos& pos)
    : ChestEntity(BlockEntityType::TrappedChest, pos) {
}

std::unique_ptr<BlockEntity> TrappedChestEntity::clone() const {
    auto cloned = std::make_unique<TrappedChestEntity>(getPos());
    // TODO: 复制物品数据
    return cloned;
}

i32 TrappedChestEntity::getRedstoneSignal(IWorld& world) const {
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

    // TODO: 需要访问 World 来通知邻居更新红石信号
    // 目前暂时跳过，待 ContainerBlockEntity 添加 World 引用后实现
}

void TrappedChestEntity::closeContainer() {
    // 先减少计数
    ChestEntity::closeContainer();

    // TODO: 需要访问 World 来通知邻居更新红石信号
    // 目前暂时跳过，待 ContainerBlockEntity 添加 World 引用后实现
}

void TrappedChestEntity::notifyNeighbors(IWorld& world) {
    // TODO: 实现邻居通知
    // IWorld 接口目前没有 isRemote() 方法，需要扩展接口或通过其他方式判断
    (void)world;
}

} // namespace blockentity
} // namespace mc
