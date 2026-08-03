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

#include "DarkOakTrunkPlacer.hpp"
#include "common/core/Types.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/gen/feature/tree/trunk/TrunkPlacer.hpp"
#include <memory>
#include <set>
#include <vector>

namespace mc {

DarkOakTrunkPlacer::DarkOakTrunkPlacer(i32 baseHeight, i32 heightRandA, i32 heightRandB)
    : TrunkPlacer(baseHeight, heightRandA, heightRandB)
{}

std::vector<FoliagePosition> DarkOakTrunkPlacer::placeTrunk(WorldGenRegion& world,
    math::Random& random,
    i32 height,
    const BlockPos& startPos,
    std::set<BlockPos>& trunkBlocks,
    const BlockState* trunkBlock)
{
    std::vector<FoliagePosition> foliagePositions;

    // 在树干底部放置泥土
    BlockPos below = startPos.down();
    placeDirtUnder(world, below);
    placeDirtUnder(world, BlockPos(below.x + 1, below.y, below.z));
    placeDirtUnder(world, BlockPos(below.x, below.y, below.z + 1));
    placeDirtUnder(world, BlockPos(below.x + 1, below.y, below.z + 1));

    // 随机方向和弯曲参数
    i32 bendStart = height - random.nextInt(4); // 开始弯曲的高度
    i32 bendLength = 2 - random.nextInt(3);     // 弯曲长度

    i32 x = startPos.x;
    i32 z = startPos.z;
    i32 y = startPos.y;
    i32 topY = y + height - 1;

    // 生成2x2树干，可能带有弯曲
    for (i32 dy = 0; dy < height; ++dy) {
        // 弯曲逻辑
        if (dy >= bendStart && bendLength > 0) {
            // 随机选择一个水平方向进行弯曲
            i32 dirIdx = random.nextInt(4);
            constexpr i32 DX[] = {0, 1, 0, -1};
            constexpr i32 DZ[] = {1, 0, -1, 0};
            x += DX[dirIdx];
            z += DZ[dirIdx];
            --bendLength;
        }

        i32 currentY = y + dy;
        BlockPos basePos(x, currentY, z);

        // 放置2x2树干
        placeBlock(world, basePos, trunkBlocks, trunkBlock);
        placeBlock(world, BlockPos(basePos.x + 1, currentY, basePos.z), trunkBlocks, trunkBlock);
        placeBlock(world, BlockPos(basePos.x, currentY, basePos.z + 1), trunkBlocks, trunkBlock);
        placeBlock(world, BlockPos(basePos.x + 1, currentY, basePos.z + 1), trunkBlocks, trunkBlock);
    }

    // 顶部树叶位置
    foliagePositions.emplace_back(BlockPos(x, topY, z), 0, true);

    // 四个角落可能生成额外枝干
    for (i32 cornerX = -1; cornerX <= 2; ++cornerX) {
        for (i32 cornerZ = -1; cornerZ <= 2; ++cornerZ) {
            // 只处理角落位置 (不是中心2x2区域)
            if ((cornerX < 0 || cornerX > 1 || cornerZ < 0 || cornerZ > 1) && random.nextInt(3) <= 0) {
                // 生成枝干
                i32 branchLength = random.nextInt(3) + 2; // 2-4格高

                for (i32 b = 0; b < branchLength; ++b) {
                    placeBlock(world,
                        BlockPos(startPos.x + cornerX, topY - b - 1, startPos.z + cornerZ),
                        trunkBlocks,
                        trunkBlock);
                }

                // 枝干末端树叶位置
                foliagePositions.emplace_back(BlockPos(x + cornerX, topY, z + cornerZ), 0, false);
            }
        }
    }

    return foliagePositions;
}

std::unique_ptr<TrunkPlacer> DarkOakTrunkPlacer::clone() const
{
    return std::make_unique<DarkOakTrunkPlacer>(m_baseHeight, m_heightRandA, m_heightRandB);
}

} // namespace mc
