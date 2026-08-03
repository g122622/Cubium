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

#include "AcaciaFoliagePlacer.hpp"
#include "common/core/Types.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/gen/feature/FeatureSpread.hpp"
#include "common/world/gen/feature/tree/foliage/FoliagePlacer.hpp"
#include "common/world/gen/feature/tree/trunk/TrunkPlacer.hpp"
#include <memory>
#include <set>

namespace mc {

AcaciaFoliagePlacer::AcaciaFoliagePlacer(const FeatureSpread& radius, const FeatureSpread& offset)
    : FoliagePlacer(radius, offset)
{}

i32 AcaciaFoliagePlacer::getFoliageHeight(math::Random& /*random*/, i32 /*trunkHeight*/) const
{
    return 0;
}

void AcaciaFoliagePlacer::placeFoliageInternal(WorldGenRegion& world,
    math::Random& random,
    i32 /*trunkHeight*/,
    const FoliagePosition& foliagePos,
    i32 foliageHeight,
    i32 radius,
    i32 offset,
    std::set<BlockPos>& foliageBlocks,
    const BlockState* foliageBlock)
{
    // 金合欢有3层树叶，不同的半径和Y偏移
    bool trunkTop = foliagePos.trunkTop;

    // 第一层，半径=radius+radiusBonus，Y偏移=-1-foliageHeight
    _placeFoliageLayer(world,
        random,
        foliagePos,
        radius + foliagePos.radiusBonus,
        -1 - foliageHeight,
        foliageBlocks,
        foliageBlock,
        trunkTop);

    // 第二层，半径=radius-1，Y偏移=-foliageHeight
    _placeFoliageLayer(world, random, foliagePos, radius - 1, -foliageHeight, foliageBlocks, foliageBlock, trunkTop);

    // 第三层，半径=radius+radiusBonus-1，Y偏移=0
    _placeFoliageLayer(
        world, random, foliagePos, radius + foliagePos.radiusBonus - 1, 0, foliageBlocks, foliageBlock, trunkTop);
}

void AcaciaFoliagePlacer::_placeFoliageLayer(WorldGenRegion& world,
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

bool AcaciaFoliagePlacer::shouldSkip(math::Random& /*random*/, i32 dx, i32 dy, i32 dz, i32 radius, bool trunkTop) const
{
    if (dy == 0) {
        // 第一层（dy=0时，实际是y=-1-foliageHeight层）
        // 跳过角落，但保留边缘
        return (dx > 1 || dz > 1) && dx != 0 && dz != 0;
    } else {
        // 其他层：跳过角落
        return dx == radius && dz == radius && radius > 0;
    }
}

std::unique_ptr<FoliagePlacer> AcaciaFoliagePlacer::clone() const
{
    return std::make_unique<AcaciaFoliagePlacer>(m_radius, m_offset);
}

} // namespace mc
