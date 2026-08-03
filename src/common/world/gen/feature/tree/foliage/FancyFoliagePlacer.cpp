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

#include "FancyFoliagePlacer.hpp"
#include "common/core/Types.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/gen/feature/FeatureSpread.hpp"
#include "common/world/gen/feature/tree/foliage/FoliagePlacer.hpp"
#include "common/world/gen/feature/tree/trunk/TrunkPlacer.hpp"
#include <algorithm>
#include <cmath>
#include <memory>
#include <set>

namespace mc {

FancyFoliagePlacer::FancyFoliagePlacer(const FeatureSpread& radius, const FeatureSpread& offset, i32 height)
    : FoliagePlacer(radius, offset)
    , m_height(height)
{}

i32 FancyFoliagePlacer::getFoliageHeight(math::Random& random, i32 /*trunkHeight*/) const
{
    return m_height + random.nextInt(3);
}

void FancyFoliagePlacer::placeFoliageInternal(WorldGenRegion& world,
    math::Random& random,
    i32 /*trunkHeight*/,
    const FoliagePosition& foliagePos,
    i32 foliageHeight,
    i32 radius,
    i32 /*offset*/,
    std::set<BlockPos>& foliageBlocks,
    const BlockState* foliageBlock)
{
    // 精美树叶：更大更密集的球形
    for (i32 y = -1; y <= foliageHeight; ++y) {
        // 中间最大，上下略小
        i32 layerRadius = radius;
        if (y < 0 || y >= foliageHeight - 1) {
            layerRadius = std::max(1, radius - 1);
        }

        for (i32 dx = -layerRadius; dx <= layerRadius; ++dx) {
            for (i32 dz = -layerRadius; dz <= layerRadius; ++dz) {
                if (shouldSkip(random, dx, y, dz, layerRadius, foliagePos.trunkTop)) {
                    continue;
                }

                BlockPos pos(foliagePos.pos.x + dx, foliagePos.pos.y + y, foliagePos.pos.z + dz);
                foliageBlocks.insert(pos);
            }
        }
    }
}

bool FancyFoliagePlacer::shouldSkip(
    math::Random& /*random*/, i32 dx, i32 /*dy*/, i32 dz, i32 radius, bool /*trunkTop*/) const
{
    // 使用半径的平方作为阈值，创建更圆形的树叶
    f32 absDx = std::abs(static_cast<f32>(dx) + 0.5f);
    f32 absDz = std::abs(static_cast<f32>(dz) + 0.5f);

    return absDx + absDz > static_cast<f32>(radius * radius);
}

std::unique_ptr<FoliagePlacer> FancyFoliagePlacer::clone() const
{
    return std::make_unique<FancyFoliagePlacer>(m_radius, m_offset, m_height);
}

} // namespace mc
