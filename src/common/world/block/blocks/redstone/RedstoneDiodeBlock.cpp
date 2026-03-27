#include "RedstoneDiodeBlock.hpp"
#include "../../../redstone/RedstoneSystem.hpp"
#include "../../../tick/base/TickPriority.hpp"
#include "../../../IWorld.hpp"
#include "../../BlockRegistry.hpp"
#include <unordered_map>

namespace mc {
namespace blocks {

RedstoneDiodeBlock::RedstoneDiodeBlock(const String& id, const BlockProperties& properties)
    : Block(properties)
    , m_id(id) {

    // 创建状态容器
    auto container = StateContainer<Block, BlockState>::Builder(*this)
        .add(BlockStateProperties::HORIZONTAL_FACING())
        .add(BlockStateProperties::POWERED())
        .create([](const Block& block, std::unordered_map<const IProperty*, size_t> values, u32 id) {
            return std::make_unique<BlockState>(block, std::move(values), id);
        });
    createBlockState(std::move(container));

    // 设置默认状态
    setDefaultState(defaultState()
        .with(BlockStateProperties::HORIZONTAL_FACING(), Direction::North)
        .with(BlockStateProperties::POWERED(), false));
}

Direction RedstoneDiodeBlock::getFacing(const BlockState& state) {
    return state.get(BlockStateProperties::HORIZONTAL_FACING());
}

bool RedstoneDiodeBlock::isPowered(const BlockState& state) {
    return state.get(BlockStateProperties::POWERED());
}

void RedstoneDiodeBlock::onBlockAdded(IWorld& world, const BlockPos& pos, const BlockState& state) {
    // 检查初始状态
    updateState(world, pos, state);
}

void RedstoneDiodeBlock::neighborChanged(IWorld& world, const BlockPos& pos, Block& neighborBlock,
                                         const BlockPos& neighborPos, bool isMoving) {
    MC_UNUSED(neighborBlock);
    MC_UNUSED(neighborPos);
    MC_UNUSED(isMoving);

    // 更新状态
    const BlockState* state = world.getBlockState(pos.x, pos.y, pos.z);
    if (state) {
        updateState(world, pos, *state);
    }
}

BlockState RedstoneDiodeBlock::updatePostPlacement(
    const BlockState& state, Direction facing,
    const BlockState& facingState, IWorld& world,
    const BlockPos& currentPos, const BlockPos& facingPos) {
    MC_UNUSED(facingState);
    MC_UNUSED(facingPos);

    // 检查是否需要更新
    if (!isLocked(world, currentPos, state)) {
        bool shouldPower = shouldBePowered(world, currentPos, state);
        bool isCurrentlyPowered = isPowered(state);

        if (shouldPower != isCurrentlyPowered) {
            // 调度更新
            world.scheduleBlockTick(currentPos, *this, getDelay(state), world::tick::TickPriority::High);
        }
    }

    return state;
}

void RedstoneDiodeBlock::tick(IWorld& world, const BlockPos& pos, BlockState& state) {
    // 如果被锁定，不更新
    if (isLocked(world, pos, state)) {
        return;
    }

    bool shouldPower = shouldBePowered(world, pos, state);
    bool isCurrentlyPowered = isPowered(state);

    if (shouldPower != isCurrentlyPowered) {
        // 改变状态
        BlockState newState = state.with(BlockStateProperties::POWERED(), shouldPower);
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

i32 RedstoneDiodeBlock::getWeakPower(
    const BlockState& state,
    IWorld& world,
    const BlockPos& pos,
    Direction side
) const {
    MC_UNUSED(world);
    MC_UNUSED(pos);

    // 只有在输出方向且已充能时才输出信号
    if (!isPowered(state)) {
        return 0;
    }

    Direction facing = getFacing(state);
    if (side == facing) {
        return calculateOutputSignal(world, pos, state);
    }

    return 0;
}

i32 RedstoneDiodeBlock::getInputSignal(IWorld& world, const BlockPos& pos, const BlockState& state) const {
    Direction facing = getFacing(state);
    Direction inputDir = Directions::opposite(facing);
    BlockPos inputPos = pos.offset(inputDir);

    const BlockState* inputState = world.getBlockState(inputPos.x, inputPos.y, inputPos.z);
    if (!inputState || inputState->isAir()) {
        return 0;
    }

    const Block& inputBlock = inputState->getBlock();

    // 获取强信号
    i32 power = inputBlock.getStrongPower(*inputState, world, inputPos, facing);

    // 如果强信号为0，检查红石线
    if (power < 15 && inputBlock.canProvidePower(*inputState)) {
        // 可能是红石线
        i32 weakPower = inputBlock.getWeakPower(*inputState, world, inputPos, facing);
        power = std::max(power, weakPower);
    }

    return power;
}

i32 RedstoneDiodeBlock::getPowerOnSides(IWorld& world, const BlockPos& pos, const BlockState& state) const {
    Direction facing = getFacing(state);
    i32 maxPower = 0;

    // 检查两个侧面（不包括前后）
    for (Direction side : Directions::horizontal()) {
        if (side == facing || side == Directions::opposite(facing)) {
            continue;
        }

        BlockPos sidePos = pos.offset(side);
        const BlockState* sideState = world.getBlockState(sidePos.x, sidePos.y, sidePos.z);

        if (sideState && !sideState->isAir()) {
            const Block& sideBlock = sideState->getBlock();
            Direction oppositeSide = Directions::opposite(side);
            i32 power = sideBlock.getStrongPower(*sideState, world, sidePos, oppositeSide);
            maxPower = std::max(maxPower, power);
        }
    }

    return maxPower;
}

bool RedstoneDiodeBlock::isLocked(IWorld& world, const BlockPos& pos, const BlockState& state) const {
    return getPowerOnSides(world, pos, state) > 0;
}

i32 RedstoneDiodeBlock::calculateOutputSignal(IWorld& world, const BlockPos& pos, const BlockState& state) const {
    MC_UNUSED(world);
    MC_UNUSED(pos);
    // 默认输出15，子类可以重写
    return isPowered(state) ? world::redstone::RedstonePower::MAX_POWER : 0;
}

void RedstoneDiodeBlock::updateState(IWorld& world, const BlockPos& pos, const BlockState& state) {
    // 如果被锁定，不更新
    if (isLocked(world, pos, state)) {
        return;
    }

    bool shouldPower = shouldBePowered(world, pos, state);
    bool isCurrentlyPowered = isPowered(state);

    if (shouldPower != isCurrentlyPowered) {
        // 确定优先级
        world::tick::TickPriority priority = world::tick::TickPriority::High;

        if (isFacingTowardsRepeater(world, pos, state)) {
            priority = world::tick::TickPriority::ExtremelyHigh;
        } else if (isCurrentlyPowered) {
            priority = world::tick::TickPriority::VeryHigh;
        }

        // 调度更新
        world.scheduleBlockTick(pos, *this, getDelay(state), priority);
    }
}

bool RedstoneDiodeBlock::isFacingTowardsRepeater(IWorld& world, const BlockPos& pos, const BlockState& state) const {
    Direction facing = getFacing(state);
    BlockPos outputPos = pos.offset(facing);

    const BlockState* outputState = world.getBlockState(outputPos.x, outputPos.y, outputPos.z);
    if (!outputState) {
        return false;
    }

    // 检查输出端是否是另一个二极管
    const Block& outputBlock = outputState->getBlock();
    return dynamic_cast<const RedstoneDiodeBlock*>(&outputBlock) != nullptr;
}

} // namespace blocks
} // namespace mc
