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

#include "DarkOakFoliagePlacer.hpp"
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

DarkOakFoliagePlacer::DarkOakFoliagePlacer(const FeatureSpread& radius, const FeatureSpread& offset, i32 height)
    : FoliagePlacer(radius, offset)
    , m_height(height)
{}

i32 DarkOakFoliagePlacer::getFoliageHeight(math::Random& /*random*/, i32 /*trunkHeight*/) const
{
    return 4;
}

void DarkOakFoliagePlacer::placeFoliageInternal(WorldGenRegion& world,
    math::Random& random,
    i32 /*trunkHeight*/,
    const FoliagePosition& foliagePos,
    i32 foliageHeight,
    i32 radius,
    i32 offset,
    std::set<BlockPos>& foliageBlocks,
    const BlockState* foliageBlock)
{
    // 深色橡树有多层不同半径的树叶
    bool trunkTop = foliagePos.trunkTop;

    if (trunkTop) {
        // trunkTop为true时的层叠
        // 第一层，半径=radius+2，Y偏移=-1
        _placeFoliageLayer(world, random, foliagePos, radius + 2, -1, foliageBlocks, foliageBlock, trunkTop);
        // 第二层，半径=radius+3，Y偏移=0
        _placeFoliageLayer(world, random, foliagePos, radius + 3, 0, foliageBlocks, foliageBlock, trunkTop);
        // 第三层，半径=radius+2，Y偏移=1
        _placeFoliageLayer(world, random, foliagePos, radius + 2, 1, foliageBlocks, foliageBlock, trunkTop);
        // 可选第四层，半径=radius，Y偏移=2
        if (random.nextBoolean()) {
            _placeFoliageLayer(world, random, foliagePos, radius, 2, foliageBlocks, foliageBlock, trunkTop);
        }
    } else {
        // trunkTop为false时的层叠
        // 第一层，半径=radius+2，Y偏移=-1
        _placeFoliageLayer(world, random, foliagePos, radius + 2, -1, foliageBlocks, foliageBlock, trunkTop);
        // 第二层，半径=radius+1，Y偏移=0
        _placeFoliageLayer(world, random, foliagePos, radius + 1, 0, foliageBlocks, foliageBlock, trunkTop);
    }
}

void DarkOakFoliagePlacer::_placeFoliageLayer(WorldGenRegion& world,
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

bool DarkOakFoliagePlacer::shouldSkip(math::Random& random, i32 dx, i32 dy, i32 dz, i32 radius, bool trunkTop) const
{
    if (dy == -1 && !trunkTop) {
        // 第一层且非trunkTop：跳过角落
        return dx == radius && dz == radius;
    } else if (dy == 1) {
        // 第三层：跳过角落附近
        return dx + dz > radius * 2 - 2;
    } else {
        // 其他情况调用基类
        return FoliagePlacer::shouldSkip(random, dx, dy, dz, radius, trunkTop);
    }
}

bool DarkOakFoliagePlacer::_shouldSkipBase(
    math::Random& random, i32 dx, i32 dy, i32 dz, i32 radius, bool trunkTop) const
{
    return FoliagePlacer::shouldSkip(random, dx, dy, dz, radius, trunkTop);
}

std::unique_ptr<FoliagePlacer> DarkOakFoliagePlacer::clone() const
{
    return std::make_unique<DarkOakFoliagePlacer>(m_radius, m_offset, m_height);
}

} // namespace mc
