/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies of substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE ON AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
 * USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#include "AzaleaBlock.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/gen/feature/FeatureIds.hpp"

namespace mc {
namespace blocks {

// ============================================================================
// AzaleaBlock
// ============================================================================

AzaleaBlock::AzaleaBlock(const BlockProperties& properties)
    : Block(properties)
{
    // 杜鹃花丛碰撞箱比普通方块略小（MC: 16x10x16像素底座）
    m_shape = CollisionShape::fromPixelBox(0, 0, 0, 16, 16, 16);
}

const CollisionShape& AzaleaBlock::getShape(const BlockState& state) const
{
    MC_UNUSED(state);
    return m_shape;
}

bool AzaleaBlock::canGrow(
    IBlockReader& world, const BlockPos& pos, const BlockState& state, bool isClientSide) const
{
    MC_UNUSED(world);
    MC_UNUSED(pos);
    MC_UNUSED(state);
    MC_UNUSED(isClientSide);
    return true;
}

bool AzaleaBlock::canUseBonemeal(
    IWorld& world, math::IRandom& random, const BlockPos& pos, const BlockState& state) const
{
    MC_UNUSED(world);
    MC_UNUSED(pos);
    MC_UNUSED(state);
    // 45%概率（MC源码：random.nextInt(5) != 0 返回false，即20%概率返回true）
    // 实际MC是 canUseBonemeal 返回 true（总是），但grow中概率控制
    // 修正：MC中AzaleaBlock.canUseBonemeal返回true
    MC_UNUSED(random);
    return true;
}

void AzaleaBlock::grow(IWorld& world, math::IRandom& random, const BlockPos& pos, const BlockState& state)
{
    MC_UNUSED(state);
    MC_UNUSED(random);

    // TODO: 生成杜鹃树feature（需要FeatureRegistry在方块中可访问）
    // MC逻辑：在pos处放置ROOTED_AZALEA_TREE配置feature
    // 暂时使用简单的树干+叶子放置作为占位
    // 完整实现需要访问ConfiguredFeature并调用place()
}

// ============================================================================
// FloweringAzaleaBlock
// ============================================================================

FloweringAzaleaBlock::FloweringAzaleaBlock(const BlockProperties& properties)
    : Block(properties)
{
    m_shape = CollisionShape::fromPixelBox(0, 0, 0, 16, 16, 16);
}

const CollisionShape& FloweringAzaleaBlock::getShape(const BlockState& state) const
{
    MC_UNUSED(state);
    return m_shape;
}

bool FloweringAzaleaBlock::canGrow(
    IBlockReader& world, const BlockPos& pos, const BlockState& state, bool isClientSide) const
{
    MC_UNUSED(world);
    MC_UNUSED(pos);
    MC_UNUSED(state);
    MC_UNUSED(isClientSide);
    return true;
}

bool FloweringAzaleaBlock::canUseBonemeal(
    IWorld& world, math::IRandom& random, const BlockPos& pos, const BlockState& state) const
{
    MC_UNUSED(world);
    MC_UNUSED(pos);
    MC_UNUSED(state);
    MC_UNUSED(random);
    return true;
}

void FloweringAzaleaBlock::grow(IWorld& world, math::IRandom& random, const BlockPos& pos, const BlockState& state)
{
    MC_UNUSED(state);
    MC_UNUSED(random);

    // TODO: 生成杜鹃树feature（同AzaleaBlock）
}

} // namespace blocks
} // namespace mc
