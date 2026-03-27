#include "RedstoneRepeaterBlock.hpp"
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

bool RedstoneRepeaterBlock::shouldBePowered(IWorld& world, const BlockPos& pos,
                                            const BlockState& state) const {
    // 获取输入信号
    return getInputSignal(world, pos, state) > 0;
}

bool RedstoneRepeaterBlock::isLocked(IWorld& world, const BlockPos& pos,
                                     const BlockState& state) const {
    // 检查侧面信号
    return getPowerOnSides(world, pos, state) > 0;
}

} // namespace blocks
} // namespace mc
