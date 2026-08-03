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

#include "StraightTrunkPlacer.hpp"
#include "common/core/Types.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/gen/feature/tree/trunk/TrunkPlacer.hpp"
#include <set>
#include <vector>

namespace mc {

StraightTrunkPlacer::StraightTrunkPlacer(i32 baseHeight, i32 heightRandA, i32 heightRandB)
    : TrunkPlacer(baseHeight, heightRandA, heightRandB)
{}

std::vector<FoliagePosition> StraightTrunkPlacer::placeTrunk(WorldGenRegion& world,
    math::Random& /*random*/,
    i32 height,
    const BlockPos& startPos,
    std::set<BlockPos>& trunkBlocks,
    const BlockState* trunkBlock)
{
    // 在起始位置下方放置泥土
    placeDirtUnder(world, startPos);

    // 生成垂直树干
    for (i32 i = 0; i < height; ++i) {
        BlockPos pos = startPos.up(i);
        if (canPlaceAt(world, pos)) {
            placeBlock(world, pos, trunkBlocks, trunkBlock);
        }
    }

    // 返回树叶位置（在树干顶部）
    std::vector<FoliagePosition> foliagePositions;
    BlockPos topPos = startPos.up(height);
    foliagePositions.emplace_back(topPos, 0, false);

    return foliagePositions;
}

} // namespace mc
