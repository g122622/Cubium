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

#include "ForkyTrunkPlacer.hpp"
#include "common/util/Direction.hpp"
#include "common/world/block/BlockPos.hpp"

#include <optional>

namespace mc {

ForkyTrunkPlacer::ForkyTrunkPlacer(i32 baseHeight, i32 heightRandA, i32 heightRandB)
    : TrunkPlacer(baseHeight, heightRandA, heightRandB)
{}

namespace {

/// MC: Direction.Plane.HORIZONTAL.getRandomDirection(random)。
/// 项目 Directions::horizontal() 顺序为 {North,East,South,West}，与 MC 一致。
Direction randomHorizontalDirection(math::Random& random)
{
    const auto dirs = Directions::horizontal();
    return dirs[static_cast<size_t>(random.nextInt(4))];
}

} // namespace

std::vector<FoliagePosition> ForkyTrunkPlacer::placeTrunk(WorldGenRegion& world,
    math::Random& random,
    i32 height,
    const BlockPos& startPos,
    std::set<BlockPos>& trunkBlocks,
    const BlockState* trunkBlock)
{
    // MC: setDirtAt(startPos.below())
    placeDirtUnder(world, startPos.down());

    std::vector<FoliagePosition> foliagePositions;

    const Direction direction = randomHorizontalDirection(random);
    // MC: i = height - nextInt(4) - 1; j = 3 - nextInt(3)
    const i32 i = height - random.nextInt(4) - 1;
    i32 j = 3 - random.nextInt(3);

    i32 k = startPos.x;
    i32 l = startPos.z;
    std::optional<i32> topY; // MC: OptionalInt optionalint

    // MC: 第一条主干（含顶部弯曲）
    for (i32 i1 = 0; i1 < height; ++i1) {
        const i32 y = startPos.y + i1;
        if (i1 >= i && j > 0) {
            k += Directions::xOffset(direction);
            l += Directions::zOffset(direction);
            --j;
        }
        // MC: placeLog 返回 true 表示成功放置 → 记录树顶 y+1
        const BlockPos pos(k, y, l);
        if (canPlaceAt(world, pos)) {
            placeBlock(world, pos, trunkBlocks, trunkBlock);
            topY = y + 1;
        }
    }

    if (topY.has_value()) {
        foliagePositions.emplace_back(BlockPos(k, topY.value(), l), 1, false);
    }

    // MC: 第二条侧枝（仅当方向不同时）
    k = startPos.x;
    l = startPos.z;
    const Direction direction1 = randomHorizontalDirection(random);
    if (direction1 != direction) {
        // MC: j2 = i - nextInt(2) - 1; k1 = 1 + nextInt(3)
        const i32 j2 = i - random.nextInt(2) - 1;
        i32 k1 = 1 + random.nextInt(3);
        topY.reset();

        // MC: for (l1 = j2; l1 < height && k1 > 0; k1--)
        for (i32 l1 = j2; l1 < height && k1 > 0; --k1) {
            if (l1 >= 1) {
                const i32 y = startPos.y + l1;
                k += Directions::xOffset(direction1);
                l += Directions::zOffset(direction1);
                const BlockPos pos(k, y, l);
                if (canPlaceAt(world, pos)) {
                    placeBlock(world, pos, trunkBlocks, trunkBlock);
                    topY = y + 1;
                }
            }
            ++l1;
        }

        if (topY.has_value()) {
            foliagePositions.emplace_back(BlockPos(k, topY.value(), l), 0, false);
        }
    }

    return foliagePositions;
}

std::unique_ptr<TrunkPlacer> ForkyTrunkPlacer::clone() const
{
    return std::make_unique<ForkyTrunkPlacer>(m_baseHeight, m_heightRandA, m_heightRandB);
}

} // namespace mc
