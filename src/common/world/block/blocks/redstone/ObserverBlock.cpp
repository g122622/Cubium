#include "ObserverBlock.hpp"
#include "../../../redstone/RedstoneSystem.hpp"
#include "../../../tick/base/TickPriority.hpp"
#include "../../../IWorld.hpp"
#include <unordered_map>

namespace mc {
namespace blocks {

ObserverBlock::ObserverBlock(const BlockProperties& properties)
    : Block(properties) {

    // 创建状态容器
    auto container = StateContainer<Block, BlockState>::Builder(*this)
        .add(BlockStateProperties::FACING())
        .add(BlockStateProperties::POWERED())
        .create([](const Block& block, std::unordered_map<const IProperty*, size_t> values, u32 id) {
            return std::make_unique<BlockState>(block, std::move(values), id);
        });
    createBlockState(std::move(container));

    // 设置默认状态
    setDefaultState(defaultState()
        .with(BlockStateProperties::FACING(), Direction::South)
        .with(BlockStateProperties::POWERED(), false));
}

Direction ObserverBlock::getFacing(const BlockState& state) {
    return state.get(BlockStateProperties::FACING());
}

bool ObserverBlock::isPowered(const BlockState& state) {
    return state.get(BlockStateProperties::POWERED());
}

BlockState ObserverBlock::withPowered(BlockState state, bool powered) {
    return state.with(BlockStateProperties::POWERED(), powered);
}

void ObserverBlock::onBlockAdded(IWorld& world, const BlockPos& pos, const BlockState& state) {
    MC_UNUSED(world);
    MC_UNUSED(pos);
    MC_UNUSED(state);
    // 侦测器放置时不立即触发
}

void ObserverBlock::neighborChanged(IWorld& world, const BlockPos& pos, Block& neighborBlock,
                                    const BlockPos& neighborPos, bool isMoving) {
    MC_UNUSED(neighborBlock);
    MC_UNUSED(isMoving);

    const BlockState* state = world.getBlockState(pos.x, pos.y, pos.z);
    if (!state) {
        return;
    }

    // 检查变化是否来自侦测面（背面）
    Direction facing = getFacing(*state);
    Direction observeDir = Directions::opposite(facing);
    BlockPos observePos = pos.offset(observeDir);

    // 只有侦测面的变化才触发
    if (neighborPos == observePos) {
        detect(world, pos, *state);
    }
}

BlockState ObserverBlock::updatePostPlacement(
    const BlockState& state, Direction facing,
    const BlockState& facingState, IWorld& world,
    const BlockPos& currentPos, const BlockPos& facingPos) {
    MC_UNUSED(facingState);
    MC_UNUSED(world);
    MC_UNUSED(currentPos);
    MC_UNUSED(facingPos);

    // 当侦测面放置/移除方块时触发
    Direction observeDir = Directions::opposite(getFacing(state));
    if (facing == observeDir) {
        // 调度检测
        detect(world, currentPos, state);
    }

    return state;
}

void ObserverBlock::tick(IWorld& world, const BlockPos& pos, BlockState& state) {
    if (isPowered(state)) {
        // 脉冲结束，停止输出
        BlockState newState = withPowered(state, false);
        world.setBlockState(pos.x, pos.y, pos.z, &newState, 2);

        // 通知输出端相邻方块更新
        Direction facing = getFacing(state);
        BlockPos outputPos = pos.offset(facing);
        const BlockState* outputState = world.getBlockState(outputPos.x, outputPos.y, outputPos.z);
        if (outputState && !outputState->isAir()) {
            Block& outputBlock = const_cast<Block&>(outputState->getBlock());
            outputBlock.neighborChanged(world, outputPos, *this, pos, false);
        }
    }
}

i32 ObserverBlock::getWeakPower(
    const BlockState& state,
    IWorld& world,
    const BlockPos& pos,
    Direction side
) const {
    MC_UNUSED(world);
    MC_UNUSED(pos);

    // 只在输出方向输出信号
    if (side != getFacing(state)) {
        return 0;
    }

    return isPowered(state) ? world::redstone::RedstonePower::MAX_POWER : 0;
}

i32 ObserverBlock::getStrongPower(
    const BlockState& state,
    IWorld& world,
    const BlockPos& pos,
    Direction side
) const {
    // 侦测器只输出弱信号
    return getWeakPower(state, world, pos, side);
}

void ObserverBlock::detect(IWorld& world, const BlockPos& pos, const BlockState& state) {
    // 如果已经在输出，不重复触发
    if (isPowered(state)) {
        return;
    }

    // 立即激活并调度脉冲
    BlockState newState = withPowered(state, true);
    world.setBlockState(pos.x, pos.y, pos.z, &newState, 2);

    // 调度脉冲结束
    world.scheduleBlockTick(pos, *this, PULSE_DURATION, world::tick::TickPriority::High);

    // 通知输出端相邻方块更新
    Direction facing = getFacing(state);
    BlockPos outputPos = pos.offset(facing);
    const BlockState* outputState = world.getBlockState(outputPos.x, outputPos.y, outputPos.z);
    if (outputState && !outputState->isAir()) {
        Block& outputBlock = const_cast<Block&>(outputState->getBlock());
        outputBlock.neighborChanged(world, outputPos, *this, pos, false);
    }
}

} // namespace blocks
} // namespace mc
