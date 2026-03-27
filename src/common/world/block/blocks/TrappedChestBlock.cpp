#include "world/block/blocks/TrappedChestBlock.hpp"
#include "world/blockentity/storage/TrappedChestEntity.hpp"

namespace mc {
namespace blocks {

TrappedChestBlock::TrappedChestBlock(const BlockProperties& properties)
    : ChestBlock(properties) {
}

i32 TrappedChestBlock::getWeakPower(
    const BlockState& state,
    IWorld& world,
    const BlockPos& pos,
    Direction side
) const {
    BlockEntity* blockEntity = world.getBlockEntity(pos);
    if (!blockEntity || blockEntity->getType() != BlockEntityType::TrappedChest) {
        return 0;
    }

    auto* chest = static_cast<blockentity::TrappedChestEntity*>(blockEntity);

    // 需要转换为World来获取完整的红石信号计算
    // 这里暂时返回打开计数
    return chest->getOpenCount();
}

i32 TrappedChestBlock::getStrongPower(
    const BlockState& state,
    IWorld& world,
    const BlockPos& pos,
    Direction side
) const {
    // 仅从顶面输出强充能
    if (side != Direction::Up) {
        return 0;
    }
    return getWeakPower(state, world, pos, side);
}

} // namespace blocks
} // namespace mc
