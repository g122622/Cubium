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

#include "TargetBlock.hpp"
#include "../../../../util/property/Properties.hpp"
#include "../../../IWorld.hpp"
#include "../../../redstone/RedstoneSystem.hpp"
#include "../../../tick/base/TickPriority.hpp"
#include "../../../tick/manager/TickManager.hpp"
#include "common/core/Types.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/math/random/IRandom.hpp"
#include "common/util/property/StateContainer.hpp"
#include "common/util/property/StateHolder.hpp"
#include "common/world/block/Block.hpp"
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

namespace mc {
namespace blocks {

using namespace mc; // Bring BlockStateProperties into scope

TargetBlock::TargetBlock(const BlockProperties& properties)
    : Block(properties)
{

    // 创建状态容器
    auto container =
        StateContainer<Block, BlockState>::Builder(*this)
            .add(BlockStateProperties::POWER_0_15())
            .create([](const Block& block,
                        std::vector<size_t> values,
                        const std::vector<StateHolder<Block, BlockState>::PropertyLayout>* propertyLayouts,
                        const std::vector<BlockState*>* allStates,
                        u32 id) {
                return std::make_unique<BlockState>(block, std::move(values), propertyLayouts, allStates, id);
            });
    createBlockState(std::move(container));

    // 设置默认状态
    setDefaultState(defaultState().with(BlockStateProperties::POWER_0_15(), 0));
}

i32 TargetBlock::getPower(const BlockState& state)
{
    return state.get(BlockStateProperties::POWER_0_15());
}

BlockState TargetBlock::withPower(BlockState state, i32 power)
{
    power = std::max(0, std::min(power, 15));
    return state.with(BlockStateProperties::POWER_0_15(), power);
}

i32 TargetBlock::calculatePower(f32 hitX, f32 hitY, f32 hitZ)
{
    // 计算命中点到方块中心（0.5, 0.5, 0.5）的距离
    f32 dx = hitX - 0.5f;
    f32 dy = hitY - 0.5f;
    f32 dz = hitZ - 0.5f;

    // 计算距离
    f32 distance = std::sqrt(dx * dx + dy * dy + dz * dz);

    // 最大距离约为 0.866（从中心到角落）
    // 映射到 0-15 的信号强度
    // 距离越近，信号越强
    constexpr f32 MAX_DISTANCE = 0.866f;

    // 线性插值：距离为0时输出15，距离为MAX时输出0
    i32 power = static_cast<i32>((1.0f - distance / MAX_DISTANCE) * 15.0f);

    return std::max(0, std::min(power, 15));
}

void TargetBlock::neighborChanged(
    IWorld& world, const BlockPos& pos, Block& neighborBlock, const BlockPos& neighborPos, bool isMoving)
{
    MC_UNUSED(neighborBlock);
    MC_UNUSED(neighborPos);
    MC_UNUSED(isMoving);

    // 标靶不响应红石信号，只响应箭矢命中
    MC_UNUSED(world);
    MC_UNUSED(pos);
}

void TargetBlock::tick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random)
{
    MC_UNUSED(random);
    // 信号持续时间结束，重置为0
    i32 currentPower = getPower(state);
    if (currentPower > 0) {
        BlockState newState = withPower(state, 0);
        world.setBlockState(pos, &newState, 3);

        // 通知相邻方块
        world::redstone::RedstoneSystem::instance().updateNeighbors(world, pos, *this);
    }
}

i32 TargetBlock::getWeakPower(
    const BlockState& state, IWorld& world, const BlockPos& pos, Direction side) const noexcept
{
    MC_UNUSED(world);
    MC_UNUSED(pos);
    MC_UNUSED(side);

    return getPower(state);
}

i32 TargetBlock::getStrongPower(
    const BlockState& state, IWorld& world, const BlockPos& pos, Direction side) const noexcept
{
    MC_UNUSED(world);
    MC_UNUSED(pos);
    MC_UNUSED(side);

    return getPower(state);
}

void TargetBlock::onHitByArrow(
    IWorld& world, const BlockPos& pos, const BlockState& state, f32 hitX, f32 hitY, f32 hitZ)
{
    // 计算输出信号强度
    i32 power = calculatePower(hitX, hitY, hitZ);

    // 更新方块状态
    BlockState newState = withPower(state, power);
    world.setBlockState(pos, &newState, 3);

    // 通知相邻方块
    world::redstone::RedstoneSystem::instance().updateNeighbors(world, pos, *this);

    // 调度信号结束
    world.tickManager().scheduleBlockTick(pos, *this, SIGNAL_DURATION, world::tick::TickPriority::High);
}

} // namespace blocks
} // namespace mc
