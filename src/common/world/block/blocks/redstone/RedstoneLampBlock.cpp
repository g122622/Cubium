#include "RedstoneLampBlock.hpp"
#include "../../../redstone/RedstoneSystem.hpp"
#include "../../../IWorld.hpp"
#include "../../../../util/property/Properties.hpp"
#include <unordered_map>

namespace mc {
namespace blocks {

using namespace mc; // Bring BlockStateProperties into scope

RedstoneLampBlock::RedstoneLampBlock(const BlockProperties& properties)
    : Block(properties) {

    // 创建状态容器
    auto container = StateContainer<Block, BlockState>::Builder(*this)
        .add(BlockStateProperties::LIT())
        .create([](const Block& block, std::unordered_map<const IProperty*, size_t> values, u32 id) {
            return std::make_unique<BlockState>(block, std::move(values), id);
        });
    createBlockState(std::move(container));

    // 设置默认状态（熄灭）
    setDefaultState(defaultState().with(BlockStateProperties::LIT(), false));
}

bool RedstoneLampBlock::isLit(const BlockState& state) {
    return state.get(BlockStateProperties::LIT());
}

BlockState RedstoneLampBlock::withLit(BlockState state, bool lit) {
    return state.with(BlockStateProperties::LIT(), lit);
}

void RedstoneLampBlock::onBlockAdded(IWorld& world, const BlockPos& pos, const BlockState& state) {
    // 检查是否应该点亮
    bool shouldLit = world::redstone::RedstonePower::isPowered(world, pos);
    if (shouldLit != isLit(state)) {
        if (shouldLit) {
            BlockState newState = withLit(state, true);
            world.setBlockState(pos.x, pos.y, pos.z, &newState, 2);
        } else {
            world.scheduleBlockTick(pos, *this, 4, world::tick::TickPriority::High);
        }
    }
}

void RedstoneLampBlock::neighborChanged(IWorld& world, const BlockPos& pos, Block& neighborBlock,
                                         const BlockPos& neighborPos, bool isMoving) {
    MC_UNUSED(neighborBlock);
    MC_UNUSED(neighborPos);
    MC_UNUSED(isMoving);

    const BlockState* state = world.getBlockState(pos.x, pos.y, pos.z);
    if (!state) {
        return;
    }

    // 检查是否应该点亮
    bool shouldLit = world::redstone::RedstonePower::isPowered(world, pos);
    bool isCurrentlyLit = isLit(*state);

    if (shouldLit != isCurrentlyLit) {
        if (shouldLit) {
            // 被充能，立即点亮
            BlockState newState = withLit(*state, true);
            world.setBlockState(pos.x, pos.y, pos.z, &newState, 2);
        } else {
            // 失去信号，调度熄灭
            world.scheduleBlockTick(pos, *this, 4, world::tick::TickPriority::High);
        }
    }
}

void RedstoneLampBlock::tick(IWorld& world, const BlockPos& pos, BlockState& state) {
    // 检查是否应该熄灭
    bool shouldLit = world::redstone::RedstonePower::isPowered(world, pos);
    if (!shouldLit && isLit(state)) {
        BlockState newState = withLit(state, false);
        world.setBlockState(pos.x, pos.y, pos.z, &newState, 2);
    }
}

} // namespace blocks
} // namespace mc
