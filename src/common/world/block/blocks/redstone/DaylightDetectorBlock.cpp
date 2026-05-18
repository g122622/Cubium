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
#include "../../../IWorld.hpp"
#include "../../../lighting/InternalLightUtils.hpp"
#include "../../../redstone/RedstoneSystem.hpp"
#include "../../../tick/base/TickPriority.hpp"
#include "../../../tick/manager/TickManager.hpp"
#include <cmath>
#include <unordered_map>

namespace mc {
namespace blocks {

DaylightDetectorBlock::DaylightDetectorBlock(const BlockProperties& properties)
    : Block(properties)
{

    // 创建状态容器
    auto container = StateContainer<Block, BlockState>::Builder(*this)
                         .add(BlockStateProperties::POWER_0_15())
                         .add(BlockStateProperties::INVERTED())
                         .create([](const Block& block, std::vector<size_t> values, const std::vector<StateHolder<Block, BlockState>::PropertyLayout>* propertyLayouts, const std::vector<BlockState*>* allStates, u32 id) {
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
    i32 power = calculateSignalStrength(world, pos, newInverted);
    newState = withPower(newState, power);

    world.setBlockState(pos, &newState, 2);

    // 通知相邻方块
    notifyNeighbors(world, pos);
}

void DaylightDetectorBlock::onBlockAdded(IWorld& world, const BlockPos& pos, const BlockState& state)
{
    // 立即更新信号强度
    updatePower(world, pos, state);
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
    updatePower(world, pos, state);

    // 继续调度下一次更新
    world.tickManager().scheduleBlockTick(pos, *this, UPDATE_DELAY, world::tick::TickPriority::Normal);
}

i32 DaylightDetectorBlock::getWeakPower(
    const BlockState& state, IWorld& world, const BlockPos& pos, Direction side) const
{
    MC_UNUSED(world);
    MC_UNUSED(pos);
    MC_UNUSED(side);

    // 日光探测器向所有方向输出信号
    return getPower(state);
}

i32 DaylightDetectorBlock::getStrongPower(
    const BlockState& state, IWorld& world, const BlockPos& pos, Direction side) const
{
    // 日光探测器只输出弱信号
    return getWeakPower(state, world, pos, side);
}

i32 DaylightDetectorBlock::calculateSignalStrength(IWorld& world, const BlockPos& pos, bool inverted)
{
    // 检查维度是否有天空光照（主世界有，下界和末地没有）
    if (!world.hasSkyLight()) {
        return inverted ? 15 : 0;
    }

    // MC Java: int i = world.getLightFor(LightType.SKY, pos) - world.getSkylightSubtracted();
    // 注意：获取的是探测器位置的天空光照，不是上方位置
    u8 skyLight = world.getSkyLight(pos);

    // 使用 InternalLightUtils 计算天空减暗因子
    i64 dayTime = world.dayTime();
    i32 skyDarkening = InternalLightUtils::calculateSkyDarkening(dayTime, world.isRaining(), world.isThundering());

    i32 i = static_cast<i32>(skyLight) - skyDarkening;

    // MC Java: 反相模式在余弦调整之前反转
    // 参考 DaylightDetectorBlock.updatePower() 第48-67行
    if (inverted) {
        i = 15 - std::max(0, i);
    }

    // 获取天体角度进行余弦调整
    // MC Java: float f = world.getCelestialAngleRadians(1.0F);
    if (i > 0) {
        f32 celestialAngle = InternalLightUtils::getCelestialAngle(dayTime);
        // 转换为弧度（getCelestialAngle 返回 0.0-1.0）
        constexpr f32 TWO_PI = 6.28318530718f;
        f32 f = celestialAngle * TWO_PI;

        // MC Java: float f1 = f < (float)Math.PI ? 0.0F : ((float)Math.PI * 2F);
        constexpr f32 PI = 3.14159265359f;
        f32 f1 = f < PI ? 0.0f : TWO_PI;

        // MC Java: f = f + (f1 - f) * 0.2F;
        f = f + (f1 - f) * 0.2f;

        // MC Java: i = Math.round((float)i * MathHelper.cos(f));
        i = static_cast<i32>(std::round(static_cast<f32>(i) * std::cos(f)));
    }

    return std::clamp(i, 0, 15);
}

void DaylightDetectorBlock::updatePower(IWorld& world, const BlockPos& pos, const BlockState& state)
{
    bool inverted = isInverted(state);
    i32 oldPower = getPower(state);
    i32 newPower = calculateSignalStrength(world, pos, inverted);

    if (oldPower != newPower) {
        BlockState newState = withPower(state, newPower);
        world.setBlockState(pos, &newState, 2);

        // 通知相邻方块更新
        notifyNeighbors(world, pos);
    }
}

void DaylightDetectorBlock::notifyNeighbors(IWorld& world, const BlockPos& pos)
{
    // 获取当前方块用于通知
    const BlockState* currentState = world.getBlockState(pos);
    if (!currentState) {
        return;
    }
    const Block& block = currentState->getBlock();

    // 通知六个方向的相邻方块
    for (Direction dir : Directions::all()) {
        BlockPos neighborPos = pos.offset(dir);
        const BlockState* neighborState = world.getBlockState(neighborPos);

        if (neighborState && !neighborState->isAir()) {
            Block& neighborBlock = const_cast<Block&>(neighborState->getBlock());
            neighborBlock.neighborChanged(world, neighborPos, const_cast<Block&>(block), pos, false);
        }
    }
}

} // namespace blocks
} // namespace mc
