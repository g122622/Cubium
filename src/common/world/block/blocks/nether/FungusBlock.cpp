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
 * LIABILITY, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
 * USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#include "FungusBlock.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/math/random/IRandom.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/BlockRegistry.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/gen/chunk/IChunkGenerator.hpp"
#include "common/world/gen/feature/nether/HugeFungusFeature.hpp"

namespace mc {
namespace blocks {

FungusBlock::FungusBlock(FungusType fungusType, const BlockProperties& properties)
    : SimpleBlock(properties)
    , m_fungusType(fungusType)
{}

// ========== IGrowable 接口实现 ==========

bool FungusBlock::canGrow(IBlockReader& world, const BlockPos& pos, const BlockState& state, bool isClientSide) const
{
    MC_UNUSED(state);
    MC_UNUSED(isClientSide);

    // wiki tech_下界菌.txt#骨粉：对种植在对应菌岩上的下界菌使用骨粉，可使其长成巨型真菌。
    // 绯红菌 → 绯红菌岩；诡异菌 → 诡异菌岩。
    const BlockPos belowPos(pos.x, pos.y - 1, pos.z);
    const BlockState* belowState = world.getBlockState(belowPos);
    if (belowState == nullptr) {
        return false;
    }

    if (m_fungusType == FungusType::Crimson) {
        return VanillaBlocks::CRIMSON_NYLIUM != nullptr && belowState->is(VanillaBlocks::CRIMSON_NYLIUM);
    }
    return VanillaBlocks::WARPED_NYLIUM != nullptr && belowState->is(VanillaBlocks::WARPED_NYLIUM);
}

bool FungusBlock::canUseBonemeal(
    IWorld& world, math::IRandom& random, const BlockPos& pos, const BlockState& state) const
{
    MC_UNUSED(world);
    MC_UNUSED(pos);
    MC_UNUSED(state);
    // wiki :骨粉有 40% 概率使下界菌长成巨型真菌（对齐 Java isBonemealSuccess）。
    return random.nextFloat() < 0.4f;
}

void FungusBlock::grow(IWorld& world, math::IRandom& random, const BlockPos& pos, const BlockState& state)
{
    MC_UNUSED(random);
    MC_UNUSED(state);

    // 通过 IWorld::createFeatureRegion() 从已加载区块构建临时 WorldGenRegion
    // ServerWorld 会重写此方法返回有效 WorldGenRegion；
    // 客户端和其他实现返回 nullptr
    auto region = world.createFeatureRegion(pos);
    if (region == nullptr) {
        // 非服务器环境或周围区块未加载，无法生成巨型真菌
        return;
    }

    // 使用世界种子和位置派生随机数种子（与 SaplingBlock::grow 一致）
    u64 seed = world.seed();
    seed ^= static_cast<u64>(static_cast<i64>(pos.x)) * 3129871ULL;
    seed ^= static_cast<u64>(static_cast<i64>(pos.y)) * 116129781ULL;
    seed ^= static_cast<u64>(static_cast<i64>(pos.z)) * 42317861ULL;

    math::Random rng(0);
    rng.setSeedWithHash(static_cast<i64>(seed));

    // 清除下界菌方块（巨型真菌会从该位置向上生成）
    const BlockState* airState = BlockRegistry::instance().airState();
    world.setBlockState(pos, airState, 2);

    // 通过 WorldGenRegion 调用巨型真菌生成器
    HugeFungusFeatureConfig config(m_fungusType, true);
    HugeFungusFeature feature;
    feature.place(*region, rng, pos, config);
}

} // namespace blocks
} // namespace mc
