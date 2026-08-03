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

#include "SpruceFoliagePlacer.hpp"
#include "common/core/Types.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/gen/feature/FeatureSpread.hpp"
#include "common/world/gen/feature/tree/foliage/FoliagePlacer.hpp"
#include "common/world/gen/feature/tree/trunk/TrunkPlacer.hpp"
#include <algorithm>
#include <memory>
#include <set>

namespace mc {

SpruceFoliagePlacer::SpruceFoliagePlacer(const FeatureSpread& radius, const FeatureSpread& offset, i32 height)
    : FoliagePlacer(radius, offset)
    , m_height(height)
{}

i32 SpruceFoliagePlacer::getFoliageHeight(math::Random& random, i32 trunkHeight) const
{
    return std::max(4, trunkHeight - m_height);
}

void SpruceFoliagePlacer::placeFoliageInternal(WorldGenRegion& world,
    math::Random& random,
    i32 /*trunkHeight*/,
    const FoliagePosition& foliagePos,
    i32 foliageHeight,
    i32 radius,
    i32 offset,
    std::set<BlockPos>& foliageBlocks,
    const BlockState* foliageBlock)
{
    // 云杉树叶从顶部向下层叠，半径从顶部开始逐渐增大
    i32 currentRadius = random.nextInt(2);
    i32 minRadius = 1;
    i32 nextRadius = 0;

    for (i32 y = offset; y >= -foliageHeight; --y) {
        // 放置当前层的树叶
        _placeFoliageLayer(
            world, random, foliagePos, currentRadius, y, foliageBlocks, foliageBlock, foliagePos.trunkTop);

        // 半径递增逻辑
        if (currentRadius >= minRadius) {
            currentRadius = nextRadius;
            nextRadius = 1;
            minRadius = std::min(minRadius + 1, radius + foliagePos.radiusBonus);
        } else {
            ++currentRadius;
        }
    }
}

void SpruceFoliagePlacer::_placeFoliageLayer(WorldGenRegion& world,
    math::Random& random,
    const FoliagePosition& foliagePos,
    i32 radius,
    i32 yOffset,
    std::set<BlockPos>& foliageBlocks,
    const BlockState* foliageBlock,
    bool trunkTop)
{
    for (i32 dx = -radius; dx <= radius; ++dx) {
        for (i32 dz = -radius; dz <= radius; ++dz) {
            if (shouldSkip(random, dx, yOffset, dz, radius, trunkTop)) {
                continue;
            }

            BlockPos pos(foliagePos.pos.x + dx, foliagePos.pos.y + yOffset, foliagePos.pos.z + dz);
            foliageBlocks.insert(pos);
        }
    }
}

bool SpruceFoliagePlacer::shouldSkip(
    math::Random& /*random*/, i32 dx, i32 /*dy*/, i32 dz, i32 radius, bool /*trunkTop*/) const
{
    // 只跳过角落且半径>0的情况
    return dx == radius && dz == radius && radius > 0;
}

std::unique_ptr<FoliagePlacer> SpruceFoliagePlacer::clone() const
{
    return std::make_unique<SpruceFoliagePlacer>(m_radius, m_offset, m_height);
}

} // namespace mc
