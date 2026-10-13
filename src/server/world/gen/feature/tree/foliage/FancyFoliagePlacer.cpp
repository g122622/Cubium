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
#include "server/world/gen/feature/FeatureSpread.hpp"
#include "server/world/gen/feature/tree/foliage/FoliagePlacer.hpp"
#include "server/world/gen/feature/tree/trunk/TrunkPlacer.hpp"
#include <algorithm>
#include <cmath>
#include <memory>
#include <set>

namespace mc {

FancyFoliagePlacer::FancyFoliagePlacer(const FeatureSpread& radius, const FeatureSpread& offset, i32 height)
    : FoliagePlacer(radius, offset)
    , m_height(height)
{}

i32 FancyFoliagePlacer::getFoliageHeight(math::IRandom& /*random*/, i32 /*trunkHeight*/) const
{
    return m_height;
}

void FancyFoliagePlacer::placeFoliageInternal(WorldGenRegion& world,
    math::IRandom& random,
    i32 /*trunkHeight*/,
    const FoliagePosition& foliagePos,
    i32 foliageHeight,
    i32 radius,
    i32 offset,
    std::set<BlockPos>& foliageBlocks,
    const BlockState* foliageBlock)
{
    for (i32 y = offset; y >= offset - foliageHeight; --y) {
        const i32 layerRadius = radius + (y != offset && y != offset - foliageHeight ? 1 : 0);
        placeFoliageLayer(world,
            random,
            foliagePos.pos,
            layerRadius,
            foliageBlocks,
            foliagePos.pos.y + y,
            foliagePos.trunkTop,
            foliageBlock);
    }
}

bool FancyFoliagePlacer::shouldSkip(
    math::IRandom& /*random*/, i32 dx, i32 /*dy*/, i32 dz, i32 radius, bool trunkTop) const
{
    const i32 absDx = trunkTop ? std::min(std::abs(dx), std::abs(dx - 1)) : std::abs(dx);
    const i32 absDz = trunkTop ? std::min(std::abs(dz), std::abs(dz - 1)) : std::abs(dz);
    const f32 x = static_cast<f32>(absDx) + 0.5f;
    const f32 z = static_cast<f32>(absDz) + 0.5f;
    return x * x + z * z > static_cast<f32>(radius * radius);
}

std::unique_ptr<FoliagePlacer> FancyFoliagePlacer::clone() const
{
    return std::make_unique<FancyFoliagePlacer>(m_radius, m_offset, m_height);
}

} // namespace mc
