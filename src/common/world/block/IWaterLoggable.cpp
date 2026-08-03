/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include "IWaterLoggable.hpp"
#include "../../util/property/Properties.hpp"
#include "../IWorld.hpp"
#include "../fluid/FluidTags.hpp"
#include "Block.hpp"
#include "WaterLoggableHelpers.hpp"
#include "common/util/assert/AssertMacros.hpp"

namespace mc {

bool IWaterLoggable::canContainFluid(
    IWorld& world, const BlockPos& pos, const BlockState& state, const fluid::Fluid& fluid) const
{
    MC_UNUSED(world);
    MC_UNUSED(pos);
    // 只有当方块未含水且流体为水时才能容纳
    if (isWaterlogged(state)) {
        return false;
    }
    return waterloggable::isWaterFluid(fluid);
}

bool IWaterLoggable::receiveFluid(
    IWorld& world, const BlockPos& pos, const BlockState& state, const fluid::FluidState& fluidState)
{
    // 检查是否已含水
    if (isWaterlogged(state)) {
        return false;
    }

    // 检查流体是否为水
    const fluid::Fluid& fluid = fluidState.getFluid();
    if (!waterloggable::isWaterFluid(fluid)) {
        return false;
    }

    // 只在服务端执行修改
    if (world.isClientSide()) {
        return true;
    }

    // 设置 WATERLOGGED=true
    BlockState newState = state.with(BlockStateProperties::WATERLOGGED(), true);
    world.setBlockState(pos, &newState, 3);

    // 调度流体 tick
    waterloggable::scheduleWaterTick(world, pos);

    return true;
}

fluid::Fluid* IWaterLoggable::pickupFluid(IWorld& world, const BlockPos& pos, const BlockState& state)
{
    if (!isWaterlogged(state)) {
        return nullptr;
    }

    // 只在服务端执行修改
    if (world.isClientSide()) {
        return waterloggable::getWaterFluid();
    }

    // 设置 WATERLOGGED=false
    BlockState newState = state.with(BlockStateProperties::WATERLOGGED(), false);
    world.setBlockState(pos, &newState, 3);

    // 返回水流体
    return waterloggable::getWaterFluid();
}

bool IWaterLoggable::containsFluid(IWorld& world, const BlockPos& pos, const BlockState& state) const
{
    MC_UNUSED(world);
    MC_UNUSED(pos);
    return isWaterlogged(state);
}

} // namespace mc
