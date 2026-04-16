#include "TripWireHookBlock.hpp"
#include "TripWireBlock.hpp"
#include "../../../redstone/RedstoneSystem.hpp"
#include "../../../tick/base/TickPriority.hpp"
#include "../../../IWorld.hpp"
#include "../../../../util/property/Properties.hpp"
#include <unordered_map>

namespace mc {
namespace blocks {

using namespace mc; // Bring BlockStateProperties into scope

TripWireHookBlock::TripWireHookBlock(const BlockProperties& properties)
    : Block(properties) {

    // 创建状态容器
    auto container = StateContainer<Block, BlockState>::Builder(*this)
        .add(BlockStateProperties::HORIZONTAL_FACING())
        .add(BlockStateProperties::POWERED())
        .add(BlockStateProperties::ATTACHED())
        .create([](const Block& block, std::unordered_map<const IProperty*, size_t> values, u32 id) {
            return std::make_unique<BlockState>(block, std::move(values), id);
        });
    createBlockState(std::move(container));

    // 设置默认状态
    setDefaultState(defaultState()
        .with(BlockStateProperties::HORIZONTAL_FACING(), Direction::North)
        .with(BlockStateProperties::POWERED(), false)
        .with(BlockStateProperties::ATTACHED(), false));
}

bool TripWireHookBlock::isPowered(const BlockState& state) {
    return state.get(BlockStateProperties::POWERED());
}

bool TripWireHookBlock::isConnected(const BlockState& state) {
    return state.get(BlockStateProperties::ATTACHED());
}

Direction TripWireHookBlock::getFacing(const BlockState& state) {
    return state.get(BlockStateProperties::HORIZONTAL_FACING());
}

BlockState TripWireHookBlock::withPowered(BlockState state, bool powered) {
    return state.with(BlockStateProperties::POWERED(), powered);
}

BlockState TripWireHookBlock::withConnected(BlockState state, bool connected) {
    return state.with(BlockStateProperties::ATTACHED(), connected);
}

void TripWireHookBlock::onBlockAdded(IWorld& world, const BlockPos& pos, const BlockState& state) {
    // 检查支撑方块
    Direction facing = getFacing(state);
    BlockPos attachPos = pos.offset(Directions::opposite(facing));
    const BlockState* attachState = world.getBlockState(attachPos);

    if (!attachState || !attachState->isSolid()) {
        // 没有支撑，掉落
        // TODO: 掉落物品
        world.setBlockState(pos, nullptr, 3);
    }
}

void TripWireHookBlock::neighborChanged(IWorld& world, const BlockPos& pos, Block& neighborBlock,
                                         const BlockPos& neighborPos, bool isMoving) {
    MC_UNUSED(neighborBlock);
    MC_UNUSED(neighborPos);
    MC_UNUSED(isMoving);

    const BlockState* state = world.getBlockState(pos);
    if (!state) {
        return;
    }

    Direction facing = getFacing(*state);

    // 检查支撑方块
    BlockPos attachPos = pos.offset(Directions::opposite(facing));
    const BlockState* attachState = world.getBlockState(attachPos);

    if (!attachState || !attachState->isSolid()) {
        // 没有支撑，掉落
        // TODO: 掉落物品
        world.setBlockState(pos, nullptr, 3);
    } else {
        // 重新计算状态
        calculateState(world, pos, facing, *state, true);
    }
}

void TripWireHookBlock::tick(IWorld& world, const BlockPos& pos, BlockState& state) {
    Direction facing = getFacing(state);
    calculateState(world, pos, facing, state, false);
}

void TripWireHookBlock::onBlockRemoved(IWorld& world, const BlockPos& pos, const BlockState& state) {
    // 如果是触发状态，通知相邻方块
    if (isPowered(state)) {
        world::redstone::RedstoneSystem::instance().updateNeighbors(world, pos, *this);
    }
}

BlockState TripWireHookBlock::updatePostPlacement(
    const BlockState& state, Direction facing,
    const BlockState& facingState, IWorld& world,
    const BlockPos& currentPos, const BlockPos& facingPos) {
    MC_UNUSED(facingState);
    MC_UNUSED(world);
    MC_UNUSED(currentPos);
    MC_UNUSED(facingPos);

    // 如果支撑方块被移除，返回空气
    Direction hookFacing = getFacing(state);
    if (facing == Directions::opposite(hookFacing)) {
        if (!facingState.isSolid()) {
            // 将被移除
        }
    }

    return state;
}

i32 TripWireHookBlock::getWeakPower(const BlockState& state, IWorld& world,
                                     const BlockPos& pos, Direction side) const {
    MC_UNUSED(world);
    MC_UNUSED(pos);

    // 只在朝向方向输出强信号
    if (side == getFacing(state)) {
        return isPowered(state) ? 15 : 0;
    }
    return 0;
}

i32 TripWireHookBlock::getStrongPower(const BlockState& state, IWorld& world,
                                       const BlockPos& pos, Direction side) const {
    MC_UNUSED(world);
    MC_UNUSED(pos);

    // 只在朝向方向输出强信号
    if (side == getFacing(state)) {
        return isPowered(state) ? 15 : 0;
    }
    return 0;
}

bool TripWireHookBlock::calculateState(IWorld& world, const BlockPos& pos, Direction facing,
                                        const BlockState& currentState, bool shouldTriggerOnChange) {
    MC_UNUSED(shouldTriggerOnChange);

    // 检测绊线链
    BlockPos otherHookPos;
    bool foundChain = checkForTripwire(world, pos, facing, otherHookPos);

    // 检查绊线是否被触发
    bool isTripwirePowered = false;

    if (foundChain) {
        // 检查所有绊线的状态
        Direction checkDir = facing;
        BlockPos checkPos = pos.offset(checkDir);

        // TODO: 检查所有绊线

        MC_UNUSED(otherHookPos);
    }

    // 更新状态
    bool wasPowered = isPowered(currentState);
    bool shouldPower = foundChain && isTripwirePowered;

    if (shouldPower != wasPowered) {
        BlockState newState = currentState;
        newState = withPowered(newState, shouldPower);
        newState = withConnected(newState, foundChain);
        world.setBlockState(pos, &newState, 3);

        // 通知相邻方块
        world::redstone::RedstoneSystem::instance().updateNeighbors(world, pos, *this);
        return true;
    }

    return false;
}

bool TripWireHookBlock::checkForTripwire(IWorld& world, const BlockPos& pos,
                                          Direction facing, BlockPos& outOtherHookPos) const {
    // 沿朝向检查最多42格
    constexpr i32 MAX_DISTANCE = 42;

    for (i32 i = 1; i <= MAX_DISTANCE; ++i) {
        BlockPos checkPos = pos.offset(facing, i);
        const BlockState* state = world.getBlockState(checkPos);

        if (!state) {
            return false;
        }

        // 检查是否是绊线钩
        if (state->is(this)) {
            Direction hookFacing = getFacing(*state);
            if (hookFacing == Directions::opposite(facing)) {
                // 找到另一端的绊线钩
                outOtherHookPos = checkPos;
                return true;
            }
            return false;
        }

        // 检查是否是绊线（需要获取 TripWireBlock 实例）
        // TODO: 当 VanillaBlocks 中有 TripWireBlock 注册时使用
        // if (!state->is(VanillaBlocks::TRIPWIRE)) {
        //     return false;
        // }
        // 暂时跳过绊线检查，等待 VanillaBlocks 注册
    }

    return false;
}

} // namespace blocks
} // namespace mc
