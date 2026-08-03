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
 * THE SOFTWARE IS PROVIDED "AS IS", ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include "BendingTrunkPlacer.hpp"
#include "common/core/Types.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/gen/feature/tree/trunk/TrunkPlacer.hpp"
#include "common/world/gen/valueprovider/IntProvider.hpp"
#include <memory>
#include <set>
#include <utility>
#include <vector>

namespace mc {

BendingTrunkPlacer::BendingTrunkPlacer(i32 baseHeight,
    i32 heightRandA,
    i32 heightRandB,
    i32 minHeightForLeaves,
    std::unique_ptr<world::gen::valueprovider::IntProvider> bendLength)
    : TrunkPlacer(baseHeight, heightRandA, heightRandB)
    , m_minHeightForLeaves(minHeightForLeaves)
    , m_bendLength(std::move(bendLength))
{}

std::vector<FoliagePosition> BendingTrunkPlacer::placeTrunk(WorldGenRegion& world,
    math::Random& random,
    i32 height,
    const BlockPos& startPos,
    std::set<BlockPos>& trunkBlocks,
    const BlockState* trunkBlock)
{
    std::vector<FoliagePosition> foliagePositions;

    // 随机选择弯曲的水平方向
    i32 dirIndex = random.nextInt(4);
    constexpr i32 DX[] = {0, 1, 0, -1}; // 南、西、北、东对应的X偏移
    constexpr i32 DZ[] = {1, 0, -1, 0}; // 南、西、北、东对应的Z偏移
    i32 dx = DX[dirIndex];
    i32 dz = DZ[dirIndex];

    const i32 i = height - 1; // 垂直部分的有效高度上限

    // 当前放置位置，从起点开始
    i32 curX = startPos.x;
    i32 curZ = startPos.z;
    i32 curY = startPos.y;

    // 在树干底部放置泥土
    placeDirtUnder(world, startPos.down());

    // 垂直阶段：逐层向上放置树干
    for (i32 j = 0; j <= i; ++j) {
        // 接近顶部时开始向弯曲方向偏移
        // 条件：j + 1 >= i + random(0,2)，即在顶部1-2层开始水平偏移
        if (j + 1 >= i + random.nextInt(2)) {
            curX += dx;
            curZ += dz;
        }

        // 放置树干方块
        if (canPlaceAt(world, BlockPos(curX, curY, curZ))) {
            placeBlock(world, BlockPos(curX, curY, curZ), trunkBlocks, trunkBlock);
        }

        // 达到最低树叶高度后，添加树叶附着点
        if (j >= m_minHeightForLeaves) {
            foliagePositions.emplace_back(BlockPos(curX, curY, curZ), 0, false);
        }

        ++curY;
    }

    // 水平弯曲阶段：采样弯曲长度，沿弯曲方向水平延伸
    // 垂直循环结束后 curY = startPos.y + height，即比最后放置的垂直方块高一格
    // 水平方块在这个高度放置，形成轻微的台阶效果
    const i32 bendLen = m_bendLength->sample(random);

    for (i32 k = 0; k <= bendLen; ++k) {
        if (canPlaceAt(world, BlockPos(curX, curY, curZ))) {
            placeBlock(world, BlockPos(curX, curY, curZ), trunkBlocks, trunkBlock);
        }

        // 水平弯曲部分的每一格都产生树叶附着点
        foliagePositions.emplace_back(BlockPos(curX, curY, curZ), 0, false);

        // 沿弯曲方向水平移动
        curX += dx;
        curZ += dz;
    }

    return foliagePositions;
}

std::unique_ptr<TrunkPlacer> BendingTrunkPlacer::clone() const
{
    return std::make_unique<BendingTrunkPlacer>(
        m_baseHeight, m_heightRandA, m_heightRandB, m_minHeightForLeaves, m_bendLength->clone());
}

} // namespace mc
