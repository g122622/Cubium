#include "IWaterLoggable.hpp"
#include "WaterLoggableHelpers.hpp"
#include "../fluid/FluidRegistry.hpp"
#include "../fluid/FluidTags.hpp"
#include "../IWorld.hpp"
#include "Block.hpp"
#include "../../util/property/Properties.hpp"

namespace mc {

bool IWaterLoggable::canContainFluid(
    IWorld& world,
    const BlockPos& pos,
    const BlockState& state,
    const fluid::Fluid& fluid) const {
    MC_UNUSED(world);
    MC_UNUSED(pos);
    // 只有当方块未含水且流体为水时才能容纳
    if (isWaterlogged(state)) {
        return false;
    }
    return fluid.isIn(fluid::FluidTags::WATER());
}

bool IWaterLoggable::receiveFluid(
    IWorld& world,
    const BlockPos& pos,
    const BlockState* state,
    const fluid::FluidState& fluidState) {
    if (state == nullptr) {
        return false;
    }

    // 检查是否已含水
    if (isWaterlogged(*state)) {
        return false;
    }

    // 检查流体是否为水
    const fluid::Fluid& fluid = fluidState.getFluid();
    if (!fluid.isIn(fluid::FluidTags::WATER())) {
        return false;
    }

    // 只在服务端执行修改
    if (world.isClientSide()) {
        return true;
    }

    // 设置 WATERLOGGED=true
    BlockState newState = state->with(BlockStateProperties::WATERLOGGED(), true);
    world.setBlockState(pos, &newState, 3);

    // 调度流体 tick
    waterloggable::scheduleWaterTick(world, pos);

    return true;
}

fluid::Fluid* IWaterLoggable::pickupFluid(
    IWorld& world,
    const BlockPos& pos,
    const BlockState& state) {
    if (!isWaterlogged(state)) {
        return nullptr;
    }

    // 只在服务端执行修改
    if (world.isClientSide()) {
        return fluid::FluidRegistry::instance().getFluid(fluid::FluidRegistry::WATER_ID);
    }

    // 设置 WATERLOGGED=false
    BlockState newState = state.with(BlockStateProperties::WATERLOGGED(), false);
    world.setBlockState(pos, &newState, 3);

    // 返回水流体
    return fluid::FluidRegistry::instance().getFluid(fluid::FluidRegistry::WATER_ID);
}

bool IWaterLoggable::containsFluid(
    IWorld& world,
    const BlockPos& pos,
    const BlockState& state) const {
    MC_UNUSED(world);
    MC_UNUSED(pos);
    return isWaterlogged(state);
}

} // namespace mc
