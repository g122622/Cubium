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

namespace mc {

ForkyTrunkPlacer::ForkyTrunkPlacer(i32 baseHeight, i32 heightRandA, i32 heightRandB)
    : TrunkPlacer(baseHeight, heightRandA, heightRandB)
{}

std::vector<FoliagePosition> ForkyTrunkPlacer::placeTrunk(WorldGenRegion& world,
    math::Random& random,
    i32 height,
    const BlockPos& startPos,
    std::set<BlockPos>& trunkBlocks,
    const BlockState* trunkBlock)
{
    std::vector<FoliagePosition> foliagePositions;

    // 在树干底部放置泥土
    placeDirtUnder(world, startPos.down());

    // 随机方向和弯曲参数
    i32 bendStart = height - random.nextInt(4) - 1; // 开始弯曲的高度
    i32 bendLength = 1 + random.nextInt(2);         // 弯曲长度

    // 随机选择第一个弯曲方向（只能是水平方向）
    i32 dirIndex = random.nextInt(4);
    constexpr i32 DX[] = {0, 1, 0, -1}; // South, West, North, East 对应的 X 偏移
    constexpr i32 DZ[] = {1, 0, -1, 0}; // South, West, North, East 对应的 Z 偏移
    i32 dx = DX[dirIndex];
    i32 dz = DZ[dirIndex];

    // 随机选择第二个弯曲方向（可选侧分支）
    i32 dirIndex1 = random.nextInt(4);
    bool hasSideBranch = dirIndex != dirIndex1;
    i32 dx1 = DX[dirIndex1];
    i32 dz1 = DZ[dirIndex1];

    i32 x = startPos.x;
    i32 z = startPos.z;
    i32 topY = startPos.y + height - 1;

    // 生成树干
    for (i32 dy = 0; dy < height; ++dy) {
        i32 currentY = startPos.y + dy;

        // 主树干
        placeBlock(world, BlockPos(startPos.x, currentY, startPos.z), trunkBlocks, trunkBlock);

        // 弯曲逻辑
        if (dy >= bendStart && bendLength > 0) {
            // 向第一个方向弯曲
            x += dx;
            z += dz;
            --bendLength;

            // 放置弯曲位置的方块
            placeBlock(world, BlockPos(x, currentY, z), trunkBlocks, trunkBlock);

            // 如果有侧分支，也放置
            if (hasSideBranch && dy < height - 1) {
                i32 sideX = startPos.x + dx1;
                i32 sideZ = startPos.z + dz1;
                placeBlock(world, BlockPos(sideX, currentY, sideZ), trunkBlocks, trunkBlock);
            }
        }
    }

    // 主分支树叶位置，radiusBonus = 1
    foliagePositions.emplace_back(BlockPos(x, topY, z), 1, true);

    // 侧分支树叶位置
    if (hasSideBranch) {
        i32 sideX = startPos.x + dx1;
        i32 sideZ = startPos.z + dz1;
        // 侧分支的 radiusBonus = 0
        foliagePositions.emplace_back(BlockPos(sideX, topY, sideZ), 0, false);
    }

    return foliagePositions;
}

std::unique_ptr<TrunkPlacer> ForkyTrunkPlacer::clone() const
{
    return std::make_unique<ForkyTrunkPlacer>(m_baseHeight, m_heightRandA, m_heightRandB);
}

} // namespace mc
