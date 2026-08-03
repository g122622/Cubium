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

#include "SeagrassBlock.hpp"
#include "../../../../item/context/BlockItemUseContext.hpp"
#include "../../../../util/math/random/IRandom.hpp"
#include "../../../../util/property/Properties.hpp"
#include "../../../IWorld.hpp"
#include "../../../block/WaterLoggableHelpers.hpp"
#include "../../../fluid/Fluid.hpp"
#include "../../../fluid/FluidTags.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/PlantType.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"

namespace mc {
namespace blocks {

SeagrassBlock::SeagrassBlock(const BlockProperties& properties)
    : Block(properties)
{

    // 海草没有特殊状态
    // 形状：小型水下植物
    m_shape = CollisionShape::box(0.125f, 0.0f, 0.125f, 0.875f, 0.5f, 0.875f);
}

BlockState SeagrassBlock::getStateForPlacement(BlockItemUseContext& context)
{
    MC_UNUSED(context);
    return defaultState();
}

bool SeagrassBlock::isValidPosition(const BlockState& state, IBlockReader& world, const BlockPos& pos) const
{

    MC_UNUSED(state);

    // 检查下方支撑
    BlockPos belowPos(pos.x, pos.y - 1, pos.z);
    const BlockState* belowState = world.getBlockState(belowPos);

    if (belowState == nullptr || !belowState->isSolid()) {
        return false;
    }

    // 检查当前位置是否为水源方块
    const fluid::FluidState* fluidState = world.getFluidState(pos);
    if (fluidState == nullptr || fluidState->isEmpty()) {
        return false;
    }

    // 必须是水且为完整水源方块
    if (!fluidState->getFluid().isIn(fluid::FluidTags::WATER())) {
        return false;
    }

    // 检查流体是否为完整水源方块
    if (fluidState->getLevel() != fluid::SOURCE_LEVEL) {
        return false;
    }

    return true;
}

const CollisionShape& SeagrassBlock::getShape(const BlockState& state) const
{
    MC_UNUSED(state);
    return m_shape;
}

const CollisionShape& SeagrassBlock::getCollisionShape(const BlockState& state) const
{
    MC_UNUSED(state);
    static CollisionShape emptyShape = CollisionShape::empty();
    return emptyShape;
}

// ========== IGrowable 接口实现 ==========

bool SeagrassBlock::canGrow(IBlockReader& world, const BlockPos& pos, const BlockState& state, bool isClientSide) const
{

    MC_UNUSED(state);
    MC_UNUSED(isClientSide);

    // 海草可以生长的条件是上方有水源方块
    BlockPos abovePos(pos.x, pos.y + 1, pos.z);
    const fluid::FluidState* aboveFluid = world.getFluidState(abovePos);

    if (aboveFluid == nullptr || aboveFluid->isEmpty()) {
        return false;
    }

    // 上方必须是水且为完整水源方块
    if (!aboveFluid->getFluid().isIn(fluid::FluidTags::WATER())) {
        return false;
    }

    return aboveFluid->getLevel() == fluid::SOURCE_LEVEL;
}

bool SeagrassBlock::canUseBonemeal(
    IWorld& world, math::IRandom& random, const BlockPos& pos, const BlockState& state) const
{

    MC_UNUSED(world);
    MC_UNUSED(random);
    MC_UNUSED(pos);
    MC_UNUSED(state);

    // 海草骨粉总是有效
    return true;
}

void SeagrassBlock::grow(IWorld& world, math::IRandom& random, const BlockPos& pos, const BlockState& state)
{

    MC_UNUSED(random);
    MC_UNUSED(state);

    // 将海草变成高海草
    // 需要 VanillaBlocks::TALL_SEAGRASS 存在
    if (VanillaBlocks::TALL_SEAGRASS == nullptr) {
        return;
    }

    // 检查上方是否有空间
    BlockPos abovePos(pos.x, pos.y + 1, pos.z);
    const fluid::FluidState* aboveFluid = world.getFluidState(abovePos);

    if (aboveFluid == nullptr || aboveFluid->isEmpty()) {
        return;
    }

    if (!aboveFluid->getFluid().isIn(fluid::FluidTags::WATER())) {
        return;
    }

    if (aboveFluid->getLevel() != fluid::SOURCE_LEVEL) {
        return;
    }

    // 设置下方为高海草的下半部分
    const BlockState* lowerState = &VanillaBlocks::TALL_SEAGRASS->defaultState().with(
        BlockStateProperties::DOUBLE_BLOCK_HALF(), BlockStateProperties::DoubleBlockHalf::Lower);

    // 设置上方为高海草的上半部分
    const BlockState* upperState = &VanillaBlocks::TALL_SEAGRASS->defaultState().with(
        BlockStateProperties::DOUBLE_BLOCK_HALF(), BlockStateProperties::DoubleBlockHalf::Upper);

    world.setBlockState(pos, lowerState, 3);
    world.setBlockState(abovePos, upperState, 3);
}

// ========== 流体状态 ==========

const fluid::FluidState* SeagrassBlock::getFluidState(const BlockState& state) const
{
    MC_UNUSED(state);
    // 海草始终返回静止水的流体状态
    fluid::Fluid* waterFluid = waterloggable::getWaterFluid();
    if (waterFluid != nullptr) {
        return &waterFluid->defaultState();
    }
    return nullptr;
}

// ========== IPlantable 接口实现 ==========

PlantType SeagrassBlock::getPlantType(IBlockReader& world, const BlockPos& pos) const
{
    MC_UNUSED(world);
    MC_UNUSED(pos);
    return PlantType::Water;
}

const BlockState& SeagrassBlock::getPlant(IBlockReader& world, const BlockPos& pos) const
{
    MC_UNUSED(world);
    MC_UNUSED(pos);
    return defaultState();
}

} // namespace blocks
} // namespace mc
