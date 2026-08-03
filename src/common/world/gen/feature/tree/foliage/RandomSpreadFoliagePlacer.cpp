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

#include "RandomSpreadFoliagePlacer.hpp"
#include "common/core/Types.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/gen/feature/FeatureSpread.hpp"
#include "common/world/gen/feature/tree/foliage/FoliagePlacer.hpp"
#include "common/world/gen/feature/tree/trunk/TrunkPlacer.hpp"
#include "common/world/gen/valueprovider/IntProvider.hpp"
#include <memory>
#include <set>
#include <utility>

namespace mc {

RandomSpreadFoliagePlacer::RandomSpreadFoliagePlacer(const FeatureSpread& radius,
    const FeatureSpread& offset,
    std::unique_ptr<world::gen::valueprovider::IntProvider> foliageHeight,
    i32 leafPlacementAttempts)
    : FoliagePlacer(radius, offset)
    , m_foliageHeight(std::move(foliageHeight))
    , m_leafPlacementAttempts(leafPlacementAttempts)
{}

i32 RandomSpreadFoliagePlacer::getFoliageHeight(math::Random& random, i32 /*trunkHeight*/) const
{
    if (m_foliageHeight == nullptr) {
        return 0;
    }
    // IntProvider::sample 接受 IRandom&，math::Random 继承自 IRandom，自动向上转换
    return m_foliageHeight->sample(random);
}

void RandomSpreadFoliagePlacer::placeFoliageInternal(WorldGenRegion& /*world*/,
    math::Random& random,
    i32 /*trunkHeight*/,
    const FoliagePosition& foliagePos,
    i32 foliageHeight,
    i32 radius,
    i32 /*offset*/,
    std::set<BlockPos>& foliageBlocks,
    const BlockState* /*foliageBlock*/)
{
    // 子类只负责收集坐标，实际放置由基类 placeFoliage 末尾统一执行。
    // 偏移公式：nextInt(bound) - nextInt(bound)
    //   - bound > 0 时，范围 [-(bound-1), bound-1]，对称三角分布
    //   - bound == 0 时，nextInt(0) 非法，特判为 0
    //   - bound < 0 不应出现，防御性特判为 0
    const i32 horizontalBound = radius > 0 ? radius : 0;
    const i32 verticalBound = foliageHeight > 0 ? foliageHeight : 0;

    for (i32 i = 0; i < m_leafPlacementAttempts; ++i) {
        i32 dx = horizontalBound > 0 ? (random.nextInt(horizontalBound) - random.nextInt(horizontalBound)) : 0;
        i32 dy = verticalBound > 0 ? (random.nextInt(verticalBound) - random.nextInt(verticalBound)) : 0;
        i32 dz = horizontalBound > 0 ? (random.nextInt(horizontalBound) - random.nextInt(horizontalBound)) : 0;

        BlockPos pos(foliagePos.pos.x + dx, foliagePos.pos.y + dy, foliagePos.pos.z + dz);
        foliageBlocks.insert(pos);
    }
}

bool RandomSpreadFoliagePlacer::shouldSkip(
    math::Random& /*random*/, i32 /*dx*/, i32 /*dy*/, i32 /*dz*/, i32 /*radius*/, bool /*trunkTop*/) const
{
    // 本放置器不走 placeFoliageLayer 路径，shouldSkip 不会被实际调用。
    // 仍然覆盖为 false 以表达原版"不跳过任何位置"的语义。
    return false;
}

std::unique_ptr<FoliagePlacer> RandomSpreadFoliagePlacer::clone() const
{
    std::unique_ptr<world::gen::valueprovider::IntProvider> clonedHeight =
        m_foliageHeight ? m_foliageHeight->clone() : nullptr;
    return std::make_unique<RandomSpreadFoliagePlacer>(
        m_radius, m_offset, std::move(clonedHeight), m_leafPlacementAttempts);
}

} // namespace mc
