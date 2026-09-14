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

#include "GiantTrunkPlacer.hpp"
#include "common/util/assert/AssertAll.hpp"

namespace mc {

GiantTrunkPlacer::GiantTrunkPlacer(i32 baseHeight, i32 heightRandA, i32 heightRandB)
    : TrunkPlacer(baseHeight, heightRandA, heightRandB)
{}

std::vector<FoliagePosition> GiantTrunkPlacer::placeTrunk(WorldGenRegion& world,
    math::Random& random,
    i32 height,
    const BlockPos& startPos,
    std::set<BlockPos>& trunkBlocks,
    const BlockState* trunkBlock)
{
    MC_UNUSED(random);
    std::vector<FoliagePosition> foliagePositions;

    // 2x2 树干
    for (i32 y = 0; y < height; ++y) {
        placeTrunkLayer2x2(world, BlockPos(startPos.x, startPos.y + y, startPos.z), trunkBlocks, trunkBlock);
    }

    // 顶部多个树叶位置
    i32 topY = startPos.y + height;
    foliagePositions.emplace_back(BlockPos(startPos.x, topY - 3, startPos.z), 3, false);
    foliagePositions.emplace_back(BlockPos(startPos.x, topY - 1, startPos.z), 2, true);

    return foliagePositions;
}

std::unique_ptr<TrunkPlacer> GiantTrunkPlacer::clone() const
{
    return std::make_unique<GiantTrunkPlacer>(m_baseHeight, m_heightRandA, m_heightRandB);
}

} // namespace mc
