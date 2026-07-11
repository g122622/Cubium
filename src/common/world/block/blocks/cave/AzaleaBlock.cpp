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
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/BlockRegistry.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/gen/chunk/IChunkGenerator.hpp"

namespace mc {
namespace blocks {

// ============================================================================
// AzaleaBlock
// ============================================================================

AzaleaBlock::AzaleaBlock(SaplingBlock::TreeGenerator treeGenerator, const BlockProperties& properties)
    : Block(properties)
    , IGrowable()
    , m_treeGenerator(std::move(treeGenerator))
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
    MC_UNUSED(random);
    MC_UNUSED(state);

    // MC 1.21.11: performBonemeal 调用 TreeGrower.AZALEA.growTree(...)
    // AZALEA 无 mega/flowers 变体，仅直接放置 AZALEA_TREE 配置特征。
    if (!m_treeGenerator) {
        return;
    }

    // 通过 IWorld::createFeatureRegion() 从已加载区块构建 WorldGenRegion
    // ServerWorld 会重写此方法返回有效区域；客户端返回 nullptr
    auto region = world.createFeatureRegion(pos);
    if (region == nullptr) {
        // 非服务器环境或周围区块未加载，无法生成树木
        return;
    }

    // 使用世界种子和位置派生随机数种子，保证同位置生成的树形稳定
    u64 seed = world.seed();
    seed ^= static_cast<u64>(static_cast<i64>(pos.x)) * 3129871ULL;
    seed ^= static_cast<u64>(static_cast<i64>(pos.y)) * 116129781ULL;
    seed ^= static_cast<u64>(static_cast<i64>(pos.z)) * 42317861ULL;

    math::Random rng(0);
    rng.setSeedWithHash(static_cast<i64>(seed));

    // MC 1.21.11: growTree 在放置前先把方块设为流体遗留状态（空气或水）
    // 杜鹃方块无流体，直接置为空气
    const BlockState* airState = BlockRegistry::instance().airState();
    world.setBlockState(pos, airState, 2);

    // 通过 WorldGenRegion 调用杜鹃树生成器
    m_treeGenerator(*region, pos, rng);
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

FloweringAzaleaBlock::FloweringAzaleaBlock(SaplingBlock::TreeGenerator treeGenerator, const BlockProperties& properties)
    : AzaleaBlock(std::move(treeGenerator), properties)
{
    // 与 AzaleaBlock 相同的碰撞箱
}

} // namespace blocks
} // namespace mc
