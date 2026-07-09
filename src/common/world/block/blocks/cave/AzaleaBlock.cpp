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
 * IMPLIED, CONDITIONS OF ANY KIND, either express or implied. See the
 * COPYRIGHT NOTICES AND LICENSE file for more details.
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "AzaleaBlock.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"

namespace mc {
namespace blocks {

// ============================================================================
// AzaleaBlock
// ============================================================================

AzaleaBlock::AzaleaBlock(const BlockProperties& properties)
    : Block(properties)
    , IGrowable()
{
    // MC 1.21.11: Shapes.or(Block.column(16.0, 8.0, 16.0), Block.column(4.0, 0.0, 8.0))
    // 上半部分：16x8像素（从Y=8到Y=16），底部茎干：4x8像素（从Y=0到Y=8）
    m_shape = CollisionShape::fromPixelBox(0, 8, 0, 16, 16, 16);
    m_shape.addBox(6, 0, 6, 10, 8, 10);
}

const CollisionShape& AzaleaBlock::getShape(const BlockState& state) const
{
    MC_UNUSED(state);
    return m_shape;
}

bool AzaleaBlock::isValidPosition(const BlockState& state, IBlockReader& world, const BlockPos& pos) const
{
    MC_UNUSED(state);

    // MC 1.21.11: mayPlaceOn 检查下方方块是否为黏土或可种植面
    const BlockPos belowPos(pos.x, pos.y - 1, pos.z);
    const BlockState* belowState = world.getBlockState(belowPos);
    if (!belowState) {
        return false;
    }

    return mayPlaceOn(*belowState, world, belowPos);
}

bool AzaleaBlock::mayPlaceOn(const BlockState& state, IBlockReader& world, const BlockPos& pos) const
{
    MC_UNUSED(world);
    MC_UNUSED(pos);

    // MC 1.21.11: AzaleaBlock.mayPlaceOn 检查 CLAY 或 super.mayPlaceOn
    // super.mayPlaceOn 是 VegetationBlock 的默认实现，检查下方方块是否可种植
    // 此处简化为检查黏土或下方方块上表面坚固
    if (state.is(VanillaBlocks::CLAY)) {
        return true;
    }

    // 检查下方方块是否有向上的坚固面（等价于 VegetationBlock 的默认 mayPlaceOn）
    return state.isSolidSide(world, pos, Direction::Up);
}

bool AzaleaBlock::canGrow(IBlockReader& world, const BlockPos& pos, const BlockState& state, bool isClientSide) const
{
    MC_UNUSED(state);
    MC_UNUSED(isClientSide);

    // MC 1.21.11: isValidBonemealTarget 检查上方无流体
    // IBlockReader 继承自 IWorld
    return world.getFluidState(pos.x, pos.y + 1, pos.z) == nullptr;
}

bool AzaleaBlock::canUseBonemeal(
    IWorld& world, math::IRandom& random, const BlockPos& pos, const BlockState& state) const
{
    MC_UNUSED(world);
    MC_UNUSED(pos);
    MC_UNUSED(state);

    // MC 1.21.11: isBonemealSuccess 返回 random.nextFloat() < 0.45
    return random.nextFloat() < 0.45f;
}

void AzaleaBlock::grow(IWorld& world, math::IRandom& random, const BlockPos& pos, const BlockState& state)
{
    MC_UNUSED(state);

    // MC 1.21.11: TreeGrower.AZALEA.growTree(...)
    // 需要 ConfiguredFeatureRegistry 在方块中可访问才能完成实现
    // 当前占位：骨粉效果尚未接入杜鹃树 feature
}

// ========== IPlantable 接口实现 ==========

PlantType AzaleaBlock::getPlantType(IBlockReader& world, const BlockPos& pos) const
{
    MC_UNUSED(world);
    MC_UNUSED(pos);
    // MC 1.21.11: AzaleaBlock 继承 VegetationBlock -> BushBlock，返回 PlantType.Plains
    return PlantType::Plains;
}

const BlockState& AzaleaBlock::getPlant(IBlockReader& world, const BlockPos& pos) const
{
    MC_UNUSED(world);
    return defaultState();
}

// ============================================================================
// FloweringAzaleaBlock
// ============================================================================

FloweringAzaleaBlock::FloweringAzaleaBlock(const BlockProperties& properties)
    : AzaleaBlock(properties)
{
    // 与 AzaleaBlock 相同的碰撞箱
}

} // namespace blocks
} // namespace mc
