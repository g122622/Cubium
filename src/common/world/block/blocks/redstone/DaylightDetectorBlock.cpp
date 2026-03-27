#include "DaylightDetectorBlock.hpp"
#include "../../../redstone/RedstoneSystem.hpp"
#include "../../../tick/base/TickPriority.hpp"
#include "../../../IWorld.hpp"
#include <unordered_map>

namespace mc {
namespace blocks {

DaylightDetectorBlock::DaylightDetectorBlock(const BlockProperties& properties)
    : Block(properties) {

    // 创建状态容器
    auto container = StateContainer<Block, BlockState>::Builder(*this)
        .add(BlockStateProperties::POWER_0_15())
        .add(BlockStateProperties::INVERTED())
        .create([](const Block& block, std::unordered_map<const IProperty*, size_t> values, u32 id) {
            return std::make_unique<BlockState>(block, std::move(values), id);
        });
    createBlockState(std::move(container));

    // 设置默认状态
    setDefaultState(defaultState()
        .with(BlockStateProperties::POWER_0_15(), 0)
        .with(BlockStateProperties::INVERTED(), false));
}

i32 DaylightDetectorBlock::getPower(const BlockState& state) {
    return state.get(BlockStateProperties::POWER_0_15());
}

BlockState DaylightDetectorBlock::withPower(BlockState state, i32 power) {
    return state.with(BlockStateProperties::POWER_0_15(), std::clamp(power, 0, 15));
}

bool DaylightDetectorBlock::isInverted(const BlockState& state) {
    return state.get(BlockStateProperties::INVERTED());
}

BlockState DaylightDetectorBlock::withInverted(BlockState state, bool inverted) {
    return state.with(BlockStateProperties::INVERTED(), inverted);
}

void DaylightDetectorBlock::toggleMode(IWorld& world, const BlockPos& pos, const BlockState& state) {
    bool newInverted = !isInverted(state);
    BlockState newState = withInverted(state, newInverted);

    // 立即更新信号强度
    i32 power = calculateSignalStrength(world, pos, newInverted);
    newState = withPower(newState, power);

    world.setBlockState(pos.x, pos.y, pos.z, &newState, 2);

    // 通知相邻方块
    notifyNeighbors(world, pos);
}

void DaylightDetectorBlock::onBlockAdded(IWorld& world, const BlockPos& pos, const BlockState& state) {
    // 立即更新信号强度
    updatePower(world, pos, state);
}

void DaylightDetectorBlock::neighborChanged(IWorld& world, const BlockPos& pos, Block& neighborBlock,
                                            const BlockPos& neighborPos, bool isMoving) {
    MC_UNUSED(neighborBlock);
    MC_UNUSED(neighborPos);
    MC_UNUSED(isMoving);

    // 调度更新
    world.scheduleBlockTick(pos, *this, UPDATE_DELAY, world::tick::TickPriority::Normal);
}

void DaylightDetectorBlock::tick(IWorld& world, const BlockPos& pos, BlockState& state) {
    // 更新信号强度
    updatePower(world, pos, state);

    // 继续调度下一次更新
    world.scheduleBlockTick(pos, *this, UPDATE_DELAY, world::tick::TickPriority::Normal);
}

i32 DaylightDetectorBlock::getWeakPower(
    const BlockState& state,
    IWorld& world,
    const BlockPos& pos,
    Direction side
) const {
    MC_UNUSED(world);
    MC_UNUSED(pos);
    MC_UNUSED(side);

    // 日光探测器向所有方向输出信号
    return getPower(state);
}

i32 DaylightDetectorBlock::getStrongPower(
    const BlockState& state,
    IWorld& world,
    const BlockPos& pos,
    Direction side
) const {
    // 日光探测器只输出弱信号
    return getWeakPower(state, world, pos, side);
}

i32 DaylightDetectorBlock::calculateSignalStrength(IWorld& world, const BlockPos& pos, bool inverted) {
    // TODO: 实现天空光照检测
    // 当前简化实现，返回0
    MC_UNUSED(world);
    MC_UNUSED(pos);
    MC_UNUSED(inverted);
    return 0;
}

void DaylightDetectorBlock::updatePower(IWorld& world, const BlockPos& pos, const BlockState& state) {
    bool inverted = isInverted(state);
    i32 oldPower = getPower(state);
    i32 newPower = calculateSignalStrength(world, pos, inverted);

    if (oldPower != newPower) {
        BlockState newState = withPower(state, newPower);
        world.setBlockState(pos.x, pos.y, pos.z, &newState, 2);

        // 通知相邻方块更新
        notifyNeighbors(world, pos);
    }
}

void DaylightDetectorBlock::notifyNeighbors(IWorld& world, const BlockPos& pos) {
    // 获取当前方块用于通知
    const BlockState* currentState = world.getBlockState(pos.x, pos.y, pos.z);
    if (!currentState) {
        return;
    }
    const Block& block = currentState->getBlock();

    // 通知六个方向的相邻方块
    for (Direction dir : Directions::all()) {
        BlockPos neighborPos = pos.offset(dir);
        const BlockState* neighborState = world.getBlockState(neighborPos.x, neighborPos.y, neighborPos.z);

        if (neighborState && !neighborState->isAir()) {
            Block& neighborBlock = const_cast<Block&>(neighborState->getBlock());
            neighborBlock.neighborChanged(world, neighborPos, const_cast<Block&>(block), pos, false);
        }
    }
}

} // namespace blocks
} // namespace mc
