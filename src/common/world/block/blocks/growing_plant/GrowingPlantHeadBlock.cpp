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
 */

#include "GrowingPlantHeadBlock.hpp"
#include "common/core/Types.hpp"
#include "common/item/context/BlockItemUseContext.hpp"
#include "common/physics/collision/CollisionShape.hpp"
#include "common/util/Direction.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/math/random/IRandom.hpp"
#include "common/util/property/Properties.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/blocks/growing_plant/GrowingPlantBlock.hpp"
#include "common/world/chunk/data/IChunk.hpp"
#include <algorithm>

namespace mc {
namespace blocks {

using namespace mc; // Bring BlockStateProperties into scope

static constexpr i32 MAX_AGE = 25;

GrowingPlantHeadBlock::GrowingPlantHeadBlock(const BlockProperties& properties,
    Direction growthDirection,
    const CollisionShape& shape,
    f32 growPerTickProbability,
    bool scheduleFluidTicks)
    : GrowingPlantBlock(properties, growthDirection, shape, scheduleFluidTicks)
    , m_growPerTickProbability(growPerTickProbability)
{}

i32 GrowingPlantHeadBlock::getAge(const BlockState& state) const
{
    return state.get(BlockStateProperties::AGE_0_25());
}

BlockState GrowingPlantHeadBlock::withAge(i32 age) const
{
    return defaultState().with(BlockStateProperties::AGE_0_25(), std::min(age, MAX_AGE));
}

BlockState GrowingPlantHeadBlock::getStateForPlacement(BlockItemUseContext& context)
{
    MC_UNUSED(context);
    return defaultState().with(BlockStateProperties::AGE_0_25(), 0);
}

void GrowingPlantHeadBlock::randomTick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random)
{
    // 检查年龄限制
    const i32 age = getAge(state);
    if (age >= MAX_AGE) {
        return;
    }

    // 检查生长概率
    if (random.nextFloat() >= m_growPerTickProbability) {
        return;
    }

    // 计算生长目标位置
    const BlockPos growPos(pos.x + Directions::xOffset(m_growthDirection),
        pos.y + Directions::yOffset(m_growthDirection),
        pos.z + Directions::zOffset(m_growthDirection));

    // 检查目标位置是否可生长
    if (!canGrowInto(world, growPos)) {
        return;
    }

    // 在当前位置放置身体方块
    const BlockState bodyState = updateBodyAfterConvertedFromHead(state);
    const Block* bodyBlock = getBodyBlock();
    if (bodyBlock) {
        world.setBlockState(pos, &bodyState, 2);
    }

    // 在目标位置放置新的头部方块
    const BlockState newHeadState = getGrowIntoState(world, growPos, state, random);
    world.setBlockState(growPos, &newHeadState, 2);
}

bool GrowingPlantHeadBlock::isValidBonemealTarget(
    IBlockReader& world, const BlockPos& pos, const BlockState& state) const
{
    MC_UNUSED(world);
    MC_UNUSED(pos);
    return getAge(state) < MAX_AGE;
}

bool GrowingPlantHeadBlock::canGrow(
    IBlockReader& world, const BlockPos& pos, const BlockState& state, bool isClientSide) const
{
    MC_UNUSED(isClientSide);
    // 参考 MC 1.21.11: GrowingPlantHeadBlock.isValidBonemealTarget
    // 检查年龄 < MAX_AGE 且生长方向下一格可生长（air）。
    if (!isValidBonemealTarget(world, pos, state)) {
        return false;
    }

    // 生长方向下一格须为 air（可生长）
    const BlockPos growPos(pos.x + Directions::xOffset(m_growthDirection),
        pos.y + Directions::yOffset(m_growthDirection),
        pos.z + Directions::zOffset(m_growthDirection));
    IWorld& iworld = const_cast<IWorld&>(static_cast<const IWorld&>(world));
    return canGrowInto(iworld, growPos);
}

bool GrowingPlantHeadBlock::canUseBonemeal(
    IWorld& world, math::IRandom& random, const BlockPos& pos, const BlockState& state) const
{
    MC_UNUSED(world);
    MC_UNUSED(random);
    MC_UNUSED(pos);
    MC_UNUSED(state);
    // 参考 MC 1.21.11: GrowingPlantHeadBlock.isBonemealSuccess
    // 恒返回 true（骨粉必定加速生长）。
    return true;
}

void GrowingPlantHeadBlock::grow(IWorld& world, math::IRandom& random, const BlockPos& pos, const BlockState& state)
{
    // 参考 MC 1.21.11: GrowingPlantHeadBlock.performBonemeal
    // 骨粉效果：在生长方向下一格放置新头部方块（age+1），原位放身体方块。
    // 注意：这里复用 randomTick 的生长逻辑，但骨粉生长应直接延伸一格。
    const i32 age = getAge(state);
    if (age >= MAX_AGE) {
        return;
    }

    // 计算生长目标位置
    const BlockPos growPos(pos.x + Directions::xOffset(m_growthDirection),
        pos.y + Directions::yOffset(m_growthDirection),
        pos.z + Directions::zOffset(m_growthDirection));

    // 检查目标位置是否可生长
    if (!canGrowInto(world, growPos)) {
        return;
    }

    // 在原头部位置放置身体方块
    const BlockState bodyState = updateBodyAfterConvertedFromHead(state);
    world.setBlockState(pos, &bodyState, 2);

    // 在生长目标位置放置新的头部方块（age+1）
    const BlockState newHeadState = getGrowIntoState(world, growPos, const_cast<BlockState&>(state), random);
    world.setBlockState(growPos, &newHeadState, 2);
}

BlockState GrowingPlantHeadBlock::getGrowIntoState(
    IWorld& world, const BlockPos& pos, BlockState& currentState, math::IRandom& random)
{
    MC_UNUSED(world);
    MC_UNUSED(pos);
    MC_UNUSED(random);
    // 默认：年龄递增
    return withAge(getAge(currentState) + 1);
}

bool GrowingPlantHeadBlock::canGrowInto(IWorld& world, const BlockPos& pos) const
{
    const BlockState* targetState = world.getBlockState(pos);
    return targetState && targetState->isAir();
}

BlockState GrowingPlantHeadBlock::updateBodyAfterConvertedFromHead(const BlockState& headState) const
{
    MC_UNUSED(headState);
    // 默认：返回身体方块的默认状态
    const Block* bodyBlock = getBodyBlock();
    return bodyBlock ? bodyBlock->defaultState() : defaultState();
}

} // namespace blocks
} // namespace mc
