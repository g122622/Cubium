#include "world/blockentity/storage/TrappedChestEntity.hpp"
#include "world/IWorld.hpp"
#include "world/block/Block.hpp"
#include "world/blockentity/BlockEntityType.hpp"
#include "world/redstone/RedstoneSystem.hpp"
#include "util/assert/AssertAll.hpp"

namespace mc {
namespace blockentity {

TrappedChestEntity::TrappedChestEntity(const BlockPos& pos)
    : ChestEntity(BlockEntityType::TrappedChest, pos) {
}

std::unique_ptr<BlockEntity> TrappedChestEntity::clone() const {
    auto cloned = std::make_unique<TrappedChestEntity>(getPos());

    nlohmann::json state;
    save(state);
    const bool loaded = cloned->load(state);
    MC_ASSERT(loaded && "TrappedChestEntity clone load failed");

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

    if (m_world != nullptr) {
        notifyNeighbors(*m_world);
    }
}

void TrappedChestEntity::closeContainer() {
    // 先减少计数
    ChestEntity::closeContainer();

    if (m_world != nullptr) {
        notifyNeighbors(*m_world);
    }
}

void TrappedChestEntity::notifyNeighbors(IWorld& world) {
    const BlockState* state = world.getBlockState(getPos());
    if (state == nullptr) {
        return;
    }

    const Block& block = state->getBlock();
    world::redstone::RedstoneSystem::instance().updateNeighbors(
        world, getPos(), const_cast<Block&>(block));
    world::redstone::RedstoneSystem::instance().updateComparators(world, getPos());
}

} // namespace blockentity
} // namespace mc
