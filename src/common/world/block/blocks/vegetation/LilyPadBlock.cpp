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

#include "LilyPadBlock.hpp"

#include "common/item/context/BlockItemUseContext.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/Material.hpp"
#include "common/world/block/PlantType.hpp"
#include "common/world/block/blocks/agricultural/BushBlock.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/fluid/Fluid.hpp"

namespace mc {
namespace blocks {

LilyPadBlock::LilyPadBlock(const BlockProperties& properties)
    : BushBlock(properties)
{

    // 睡莲形状：扁平的圆形，略微高于水面
    m_shape = CollisionShape::box(0.0625f, 0.0f, 0.0625f, 0.9375f, 0.015625f, 0.9375f);
}

BlockState LilyPadBlock::getStateForPlacement(BlockItemUseContext& context)
{
    return defaultState();
}

bool LilyPadBlock::isValidPosition(const BlockState& state, IBlockReader& world, const BlockPos& pos) const
{

    MC_UNUSED(state);

    // 检查下方是否为水
    BlockPos belowPos(pos.x, pos.y - 1, pos.z);
    const BlockState* belowState = world.getBlockState(belowPos);

    if (belowState == nullptr) {
        return false;
    }

    // 检查是否为水方块（包括静止水和流动水）
    if (!belowState->is(VanillaBlocks::WATER)) {
        // 也可以检查材料是否为水
        const Material& material = belowState->getMaterial();
        if (!material.isLiquid()) {
            return false;
        }
    }

    // 检查当前方块是否为空气或水
    const BlockState* currentState = world.getBlockState(pos);
    if (currentState == nullptr || currentState->isAir()) {
        return true;
    }

    // 如果当前是水，检查流体级别是否为满（级别 0 表示满）
    // 满水源的级别为 0-7，只有级别 0-7 的静止水可以放置睡莲
    const fluid::FluidState* fluidState = currentState->getFluidState();
    if (fluidState != nullptr && !fluidState->isEmpty()) {
        // 满水源的级别为 0-7，只有级别 0-7 的静止水可以放置睡莲
        return fluidState->getLevel() <= 7;
    }

    return false;
}

const CollisionShape& LilyPadBlock::getShape(const BlockState& state) const
{
    MC_UNUSED(state);
    return m_shape;
}

const CollisionShape& LilyPadBlock::getCollisionShape(const BlockState& state) const
{
    MC_UNUSED(state);
    // 睡莲有很小的碰撞箱，可以踩上去
    return m_shape;
}

void LilyPadBlock::onEntityCollision(const BlockState& state, IWorld& world, const BlockPos& pos, Entity& entity) const
{
    MC_UNUSED(state);
    MC_UNUSED(world);
    MC_UNUSED(pos);
    MC_UNUSED(entity);

    // 踩在睡莲上不会造成伤害
    // 但如果实体太大可能会破坏睡莲
}

bool LilyPadBlock::canSustain(const BlockState& groundState, IWorld& world, const BlockPos& groundPos) const
{
    MC_UNUSED(world);
    MC_UNUSED(groundPos);

    // 睡莲需要水作为支撑
    const Material& material = groundState.getMaterial();
    return material.isLiquid();
}

PlantType LilyPadBlock::getPlantType(IBlockReader& world, const BlockPos& pos) const
{
    MC_UNUSED(world);
    MC_UNUSED(pos);
    return PlantType::Water;
}

} // namespace blocks
} // namespace mc
