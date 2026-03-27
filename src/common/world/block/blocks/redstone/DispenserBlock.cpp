#include "DispenserBlock.hpp"
#include "../../../redstone/RedstoneSystem.hpp"
#include "../../../tick/base/TickPriority.hpp"
#include "../../../IWorld.hpp"
#include <unordered_map>

namespace mc {
namespace blocks {

DispenserBlock::DispenserBlock(const BlockProperties& properties)
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

bool DispenserBlock::isTriggered(const BlockState& state) {
    return state.get(BlockStateProperties::TRIGGERED());
}

BlockState DispenserBlock::withTriggered(BlockState state, bool triggered) {
    return state.with(BlockStateProperties::TRIGGERED(), triggered);
}

Direction DispenserBlock::getFacing(const BlockState& state) {
    return state.get(BlockStateProperties::FACING());
}

void DispenserBlock::onBlockAdded(IWorld& world, const BlockPos& pos, const BlockState& state) {
    MC_UNUSED(world);
    MC_UNUSED(pos);
    MC_UNUSED(state);
    // 发射器放置时不触发
}

void DispenserBlock::neighborChanged(IWorld& world, const BlockPos& pos, Block& neighborBlock,
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
            // 被激活，调度发射
            world.scheduleBlockTick(pos, *this, 4, world::tick::TickPriority::High);
        }
        // 更新触发状态
        BlockState newState = withTriggered(*state, shouldTrigger);
        world.setBlockState(pos.x, pos.y, pos.z, &newState, 2);
    }
}

BlockState DispenserBlock::updatePostPlacement(
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

void DispenserBlock::tick(IWorld& world, const BlockPos& pos, BlockState& state) {
    // 尝试发射物品
    dispense(world, pos, state);
}

void DispenserBlock::dispense(IWorld& world, const BlockPos& pos, const BlockState& state) {
    // 尝试发射物品
    if (tryDispense(world, pos, state)) {
        playDispenseSound(world, pos);
    }
}

bool DispenserBlock::tryDispense(IWorld& world, const BlockPos& pos, const BlockState& state) {
    MC_UNUSED(world);
    MC_UNUSED(pos);
    MC_UNUSED(state);

    // TODO: 获取方块实体并发射物品
    // 当前简化实现
    // 1. 获取 DispenserBlockEntity
    // 2. 随机选择非空槽位
    // 3. 根据物品类型执行发射行为
    // 4. 减少物品数量

    return false;
}

void DispenserBlock::playDispenseSound(IWorld& world, const BlockPos& pos) {
    MC_UNUSED(world);
    MC_UNUSED(pos);
    // TODO: 播放发射音效
    // world.playSound(pos, SoundEvents::BLOCK_DISPENSER_DISPENSE, 1.0f, 1.0f);
}

} // namespace blocks
} // namespace mc
