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

#include "BushFoliagePlacer.hpp"
#include "common/core/Types.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/gen/feature/FeatureSpread.hpp"
#include "common/world/gen/feature/tree/foliage/FoliagePlacer.hpp"
#include "common/world/gen/feature/tree/trunk/TrunkPlacer.hpp"
#include <cmath>
#include <memory>
#include <set>

namespace mc {

BushFoliagePlacer::BushFoliagePlacer(const FeatureSpread& radius, const FeatureSpread& offset)
    : FoliagePlacer(radius, offset)
{}

i32 BushFoliagePlacer::getFoliageHeight(math::Random& /*random*/, i32 /*trunkHeight*/) const
{
    return 1; // 灌木只有一层
}

void BushFoliagePlacer::placeFoliageInternal(WorldGenRegion& world,
    math::Random& random,
    i32 /*trunkHeight*/,
    const FoliagePosition& foliagePos,
    i32 foliageHeight,
    i32 radius,
    i32 /*offset*/,
    std::set<BlockPos>& foliageBlocks,
    const BlockState* foliageBlock)
{
    // 灌木：单层球形
    for (i32 y = 0; y < foliageHeight; ++y) {
        for (i32 dx = -radius; dx <= radius; ++dx) {
            for (i32 dz = -radius; dz <= radius; ++dz) {
                if (shouldSkip(random, dx, y, dz, radius, foliagePos.trunkTop)) {
                    continue;
                }

                BlockPos pos(foliagePos.pos.x + dx, foliagePos.pos.y + y, foliagePos.pos.z + dz);
                foliageBlocks.insert(pos);
            }
        }
    }
}

bool BushFoliagePlacer::shouldSkip(math::Random& random, i32 dx, i32 dy, i32 dz, i32 radius, bool /*trunkTop*/) const
{
    // 球形
    f32 dist = std::sqrt(static_cast<f32>(dx * dx + dz * dz));
    if (dist > radius + 0.5f) {
        return true;
    }

    return false;
}

std::unique_ptr<FoliagePlacer> BushFoliagePlacer::clone() const
{
    return std::make_unique<BushFoliagePlacer>(m_radius, m_offset);
}

} // namespace mc
