#include "DropperBlock.hpp"
#include "../../../redstone/RedstoneSystem.hpp"
#include "../../../tick/base/TickPriority.hpp"
#include "../../../IWorld.hpp"
#include <unordered_map>

namespace mc {
namespace blocks {

DropperBlock::DropperBlock(const BlockProperties& properties)
    : Block(properties) {

    // 创建状态容器
    auto container = StateContainer<Block, BlockState>::Builder(*this)
        .add(BlockStateProperties::FACING())
        .add(BlockStateProperties::TRIGGERED())
        .create([](const Block& block, std::unordered_map<const IProperty*, size_t> values, u32 id) {
            return std::make_unique<BlockState>(block, std::move(values), id);
        });
    createBlockState(std::move(container));

    // 设置默认状态
    setDefaultState(defaultState()
        .with(BlockStateProperties::FACING(), Direction::North)
        .with(BlockStateProperties::TRIGGERED(), false));
}

bool DropperBlock::isTriggered(const BlockState& state) {
    return state.get(BlockStateProperties::TRIGGERED());
}

BlockState DropperBlock::withTriggered(BlockState state, bool triggered) {
    return state.with(BlockStateProperties::TRIGGERED(), triggered);
}

Direction DropperBlock::getFacing(const BlockState& state) {
    return state.get(BlockStateProperties::FACING());
}

void DropperBlock::onBlockAdded(IWorld& world, const BlockPos& pos, const BlockState& state) {
    MC_UNUSED(world);
    MC_UNUSED(pos);
    MC_UNUSED(state);
    // 投掷器放置时不触发
}

void DropperBlock::neighborChanged(IWorld& world, const BlockPos& pos, Block& neighborBlock,
                                    const BlockPos& neighborPos, bool isMoving) {
    MC_UNUSED(neighborBlock);
    MC_UNUSED(neighborPos);
    MC_UNUSED(isMoving);

    const BlockState* state = world.getBlockState(pos.x, pos.y, pos.z);
    if (!state) {
        return;
    }

    // 检查是否应该触发
    bool shouldTrigger = world::redstone::RedstonePower::isPowered(world, pos);
    bool isCurrentlyTriggered = isTriggered(*state);

    if (shouldTrigger != isCurrentlyTriggered) {
        if (shouldTrigger) {
            // 被激活，调度投掷
            world.scheduleBlockTick(pos, *this, 4, world::tick::TickPriority::High);
        }
        // 更新触发状态
        BlockState newState = withTriggered(*state, shouldTrigger);
        world.setBlockState(pos.x, pos.y, pos.z, &newState, 2);
    }
}

BlockState DropperBlock::updatePostPlacement(
    const BlockState& state, Direction facing,
    const BlockState& facingState, IWorld& world,
    const BlockPos& currentPos, const BlockPos& facingPos) {
    MC_UNUSED(facing);
    MC_UNUSED(facingState);
    MC_UNUSED(world);
    MC_UNUSED(currentPos);
    MC_UNUSED(facingPos);

    return state;
}

void DropperBlock::tick(IWorld& world, const BlockPos& pos, BlockState& state) {
    // 尝试投掷物品
    drop(world, pos, state);
}

void DropperBlock::drop(IWorld& world, const BlockPos& pos, const BlockState& state) {
    // 尝试投掷物品
    if (tryDrop(world, pos, state)) {
        playDropSound(world, pos);
    }
}

bool DropperBlock::tryDrop(IWorld& world, const BlockPos& pos, const BlockState& state) {
    MC_UNUSED(world);
    MC_UNUSED(pos);
    MC_UNUSED(state);

    // TODO: 获取方块实体并投掷物品
    // 当前简化实现
    // 1. 获取 DropperBlockEntity
    // 2. 随机选择非空槽位
    // 3. 投掷物品（无特殊行为）
    // 4. 减少物品数量

    return false;
}

void DropperBlock::playDropSound(IWorld& world, const BlockPos& pos) {
    MC_UNUSED(world);
    MC_UNUSED(pos);
    // TODO: 播放投掷音效
    // world.playSound(pos, SoundEvents::BLOCK_DISPENSER_DROP, 1.0f, 1.0f);
}

} // namespace blocks
} // namespace mc
