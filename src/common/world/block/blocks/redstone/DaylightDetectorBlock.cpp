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

#include "DaylightDetectorBlock.hpp"
#include "../../../../util/math/MathConstants.hpp"
#include "../../../IWorld.hpp"
#include "../../../lighting/InternalLightUtils.hpp"
#include "../../../redstone/RedstoneSystem.hpp"
#include "../../../tick/base/TickPriority.hpp"
#include "../../../tick/manager/TickManager.hpp"
#include "common/core/BlockRaycastResult.hpp"
#include "common/core/Types.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/item/core/ActionResult.hpp"
#include "common/item/core/BlockActionResult.hpp"
#include "common/util/Direction.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/math/random/IRandom.hpp"
#include "common/util/property/Properties.hpp"
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

DaylightDetectorBlock::DaylightDetectorBlock(const BlockProperties& properties)
    : Block(properties)
{

    // 创建状态容器
    auto container =
        StateContainer<Block, BlockState>::Builder(*this)
            .add(BlockStateProperties::POWER_0_15())
            .add(BlockStateProperties::INVERTED())
            .create([](const Block& block,
                        std::vector<size_t> values,
                        const std::vector<StateHolder<Block, BlockState>::PropertyLayout>* propertyLayouts,
                        const std::vector<BlockState*>* allStates,
                        u32 id) {
                return std::make_unique<BlockState>(block, std::move(values), propertyLayouts, allStates, id);
            });
    createBlockState(std::move(container));

    // 设置默认状态
    setDefaultState(
        defaultState().with(BlockStateProperties::POWER_0_15(), 0).with(BlockStateProperties::INVERTED(), false));
}

i32 DaylightDetectorBlock::getPower(const BlockState& state)
{
    return state.get(BlockStateProperties::POWER_0_15());
}

BlockActionResult DaylightDetectorBlock::onBlockActivated(const BlockState& state,
    IWorld& world,
    const BlockPos& pos,
    Player& player,
    Hand hand,
    const BlockRaycastResult& hit)
{
    MC_UNUSED(player);
    MC_UNUSED(hand);
    MC_UNUSED(hit);

    // 对齐 vanilla DaylightDetectorBlock.use：右键切换昼夜模式（翻转 INVERTED）并重算 power。
    // toggleMode 内部 setBlockState 写回新 state + _notifyNeighbors。不检查手持物（空手右键即可）。
    toggleMode(world, pos, state);
    return ActionResultType::Success;
}

BlockState DaylightDetectorBlock::withPower(BlockState state, i32 power)
{
    return state.with(BlockStateProperties::POWER_0_15(), std::clamp(power, 0, 15));
}

bool DaylightDetectorBlock::isInverted(const BlockState& state)
{
    return state.get(BlockStateProperties::INVERTED());
}

BlockState DaylightDetectorBlock::withInverted(BlockState state, bool inverted)
{
    return state.with(BlockStateProperties::INVERTED(), inverted);
}

void DaylightDetectorBlock::toggleMode(IWorld& world, const BlockPos& pos, const BlockState& state)
{
    bool newInverted = !isInverted(state);
    BlockState newState = withInverted(state, newInverted);

    // 立即更新信号强度
    i32 power = _calculateSignalStrength(world, pos, newInverted);
    newState = withPower(newState, power);

    world.setBlockState(pos, &newState, 2);

    // 通知相邻方块
    _notifyNeighbors(world, pos);
}

void DaylightDetectorBlock::onBlockAdded(IWorld& world, const BlockPos& pos, const BlockState& state)
{
    // 立即更新信号强度
    _updatePower(world, pos, state);
}

void DaylightDetectorBlock::neighborChanged(
    IWorld& world, const BlockPos& pos, Block& neighborBlock, const BlockPos& neighborPos, bool isMoving)
{
    MC_UNUSED(neighborBlock);
    MC_UNUSED(neighborPos);
    MC_UNUSED(isMoving);

    // 调度更新
    world.tickManager().scheduleBlockTick(pos, *this, UPDATE_DELAY, world::tick::TickPriority::Normal);
}

void DaylightDetectorBlock::tick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random)
{
    MC_UNUSED(random);
    // 更新信号强度
    _updatePower(world, pos, state);

    // 继续调度下一次更新
    world.tickManager().scheduleBlockTick(pos, *this, UPDATE_DELAY, world::tick::TickPriority::Normal);
}

i32 DaylightDetectorBlock::getWeakPower(
    const BlockState& state, IWorld& world, const BlockPos& pos, Direction side) const noexcept
{
    MC_UNUSED(world);
    MC_UNUSED(pos);
    MC_UNUSED(side);

    // 日光探测器向所有方向输出信号
    return getPower(state);
}

i32 DaylightDetectorBlock::getStrongPower(
    const BlockState& state, IWorld& world, const BlockPos& pos, Direction side) const noexcept
{
    // 日光探测器只输出弱信号
    return getWeakPower(state, world, pos, side);
}

i32 DaylightDetectorBlock::_calculateSignalStrength(IWorld& world, const BlockPos& pos, bool inverted)
{
    // 检查维度是否有天空光照（主世界有，下界和末地没有）
    if (!world.hasSkyLight()) {
        return inverted ? 15 : 0;
    }

    // 获取探测器位置的天空光照
    u8 skyLight = world.getSkyLight(pos);

    // 使用 InternalLightUtils 计算天空减暗因子
    // dayTimeOfDay() 返回 0-23999 范围的时间
    i64 tod = world.dayTimeOfDay();
    i32 skyDarkening = InternalLightUtils::calculateSkyDarkening(tod, world.isRaining(), world.isThundering());

    i32 i = static_cast<i32>(skyLight) - skyDarkening;

    // 反相模式在余弦调整之前反转
    if (inverted) {
        i = 15 - std::max(0, i);
    }

    // 进行余弦调整
    if (i > 0) {
        f32 celestialAngle = InternalLightUtils::getCelestialAngle(tod);
        // 转换为弧度（getCelestialAngle 返回 0.0-1.0）
        f32 f = celestialAngle * math::TWO_PI;

        f32 f1 = f < math::PI ? 0.0f : math::TWO_PI;

        f = f + (f1 - f) * 0.2f;

        i = static_cast<i32>(std::round(static_cast<f32>(i) * std::cos(f)));
    }

    return std::clamp(i, 0, 15);
}

void DaylightDetectorBlock::_updatePower(IWorld& world, const BlockPos& pos, const BlockState& state)
{
    bool inverted = isInverted(state);
    i32 oldPower = getPower(state);
    i32 newPower = _calculateSignalStrength(world, pos, inverted);

    if (oldPower != newPower) {
        BlockState newState = withPower(state, newPower);
        world.setBlockState(pos, &newState, 2);

        // 通知相邻方块更新
        _notifyNeighbors(world, pos);
    }
}

void DaylightDetectorBlock::_notifyNeighbors(IWorld& world, const BlockPos& pos)
{
    // 获取当前方块用于通知
    const BlockState* currentState = world.getBlockState(pos);
    if (!currentState) {
        return;
    }
    Block& block = currentState->getBlockMutable();

    // 通知六个方向的相邻方块
    for (Direction dir : Directions::all()) {
        BlockPos neighborPos = pos.offset(dir);
        const BlockState* neighborState = world.getBlockState(neighborPos);

        if (neighborState && !neighborState->isAir()) {
            Block& neighborBlock = neighborState->getBlockMutable();
            neighborBlock.neighborChanged(world, neighborPos, block, pos, false);
        }
    }
}

} // namespace blocks
} // namespace mc
