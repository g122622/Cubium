#include "RedstoneRepeaterBlock.hpp"
#include "../../../IWorld.hpp"
#include <unordered_map>

namespace mc {
namespace blocks {

RedstoneRepeaterBlock::RedstoneRepeaterBlock(const BlockProperties& properties)
    : RedstoneDiodeBlock("redstone_repeater", properties) {

    // 创建状态容器 - 包含基类的 FACING 和 POWERED，以及中继器特有的 DELAY 和 LOCKED
    auto container = StateContainer<Block, BlockState>::Builder(*this)
        .add(BlockStateProperties::HORIZONTAL_FACING())
        .add(BlockStateProperties::POWERED())
        .add(BlockStateProperties::DELAY_1_4())
        .add(BlockStateProperties::LOCKED())
        .create([](const Block& block, std::unordered_map<const IProperty*, size_t> values, u32 id) {
            return std::make_unique<BlockState>(block, std::move(values), id);
        });
    createBlockState(std::move(container));

    // 设置默认状态
    setDefaultState(defaultState()
        .with(BlockStateProperties::HORIZONTAL_FACING(), Direction::North)
        .with(BlockStateProperties::POWERED(), false)
        .with(BlockStateProperties::DELAY_1_4(), 1)
        .with(BlockStateProperties::LOCKED(), false));
}

BlockState RedstoneRepeaterBlock::updatePostPlacement(
    const BlockState& state, Direction facing,
    const BlockState& facingState, IWorld& world,
    const BlockPos& currentPos, const BlockPos& facingPos) {

    MC_UNUSED(facingState);
    MC_UNUSED(facingPos);

    // MC Java: 如果更新方向不是中继器的朝向方向，更新 LOCKED 状态
    Direction blockFacing = getFacing(state);
    if (Directions::getAxis(facing) != Directions::getAxis(blockFacing)) {
        // 检查是否被锁定
        bool locked = isLocked(world, currentPos, state);
        if (isLockedState(state) != locked) {
            return withLocked(state, locked);
        }
    }

    // 调用基类实现
    return RedstoneDiodeBlock::updatePostPlacement(state, facing, facingState, world, currentPos, facingPos);
}

i32 RedstoneRepeaterBlock::getDelay(const BlockState& state) const {
    return getDelaySetting(state) * DELAY_MULTIPLIER;
}

i32 RedstoneRepeaterBlock::getDelaySetting(const BlockState& state) {
    return state.get(BlockStateProperties::DELAY_1_4());
}

BlockState RedstoneRepeaterBlock::withDelay(BlockState state, i32 delay) {
    return state.with(BlockStateProperties::DELAY_1_4(), std::clamp(delay, MIN_DELAY, MAX_DELAY));
}

bool RedstoneRepeaterBlock::isLockedState(const BlockState& state) {
    return state.get(BlockStateProperties::LOCKED());
}

BlockState RedstoneRepeaterBlock::withLocked(BlockState state, bool locked) {
    return state.with(BlockStateProperties::LOCKED(), locked);
}

bool RedstoneRepeaterBlock::shouldBePowered(IWorld& world, const BlockPos& pos,
                                            const BlockState& state) const {
    // 如果被锁定，保持当前状态
    if (isLockedState(state)) {
        return isPowered(state);
    }
    // 获取输入信号
    return getInputSignal(world, pos, state) > 0;
}

bool RedstoneRepeaterBlock::isLocked(IWorld& world, const BlockPos& pos,
                                     const BlockState& state) const {
    // 检查侧面是否有来自其他二极管的信号
    return getPowerOnSides(world, pos, state) > 0;
}

} // namespace blocks
} // namespace mc
