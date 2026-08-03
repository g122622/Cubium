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

#include "CherryTrunkPlacer.hpp"
#include "common/core/Types.hpp"
#include "common/util/Direction.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/gen/feature/Feature.hpp"
#include "common/world/gen/feature/tree/trunk/TrunkPlacer.hpp"
#include <algorithm>
#include <cstdlib>
#include <memory>
#include <set>
#include <vector>

namespace mc {

CherryTrunkPlacer::CherryTrunkPlacer(i32 baseHeight,
    i32 heightRandA,
    i32 heightRandB,
    i32 branchCountMin,
    i32 branchCountMax,
    i32 branchHorizontalLengthMin,
    i32 branchHorizontalLengthMax,
    i32 branchStartOffsetFromTopMin,
    i32 branchStartOffsetFromTopMax,
    i32 branchEndOffsetFromTopMin,
    i32 branchEndOffsetFromTopMax)
    : TrunkPlacer(baseHeight, heightRandA, heightRandB)
    , m_branchCountMin(branchCountMin)
    , m_branchCountMax(branchCountMax)
    , m_branchHorizontalLengthMin(branchHorizontalLengthMin)
    , m_branchHorizontalLengthMax(branchHorizontalLengthMax)
    , m_branchStartOffsetFromTopMin(branchStartOffsetFromTopMin)
    , m_branchStartOffsetFromTopMax(branchStartOffsetFromTopMax)
    , m_branchEndOffsetFromTopMin(branchEndOffsetFromTopMin)
    , m_branchEndOffsetFromTopMax(branchEndOffsetFromTopMax)
{}

std::vector<FoliagePosition> CherryTrunkPlacer::placeTrunk(WorldGenRegion& world,
    math::Random& random,
    i32 height,
    const BlockPos& startPos,
    std::set<BlockPos>& trunkBlocks,
    const BlockState* trunkBlock)
{
    // 在起始位置下方放置泥土
    placeDirtUnder(world, startPos);

    // 计算两个分支的起始高度（从树顶偏移）
    i32 branchStartI =
        std::max(0, height - 1 + random.nextInt(m_branchStartOffsetFromTopMin, m_branchStartOffsetFromTopMax + 1));
    i32 branchStartJ =
        std::max(0, height - 1 + random.nextInt(m_branchStartOffsetFromTopMin, m_branchStartOffsetFromTopMax + 1));

    // 确保 j != i，避免两个分支在同一高度
    if (branchStartJ >= branchStartI) {
        branchStartJ++;
    }

    // 确定分支数量
    i32 branchCount = random.nextInt(m_branchCountMin, m_branchCountMax + 1);
    bool hasThirdBranch = branchCount >= 3;
    bool hasSecondBranch = branchCount >= 2;

    // 根据分支数量确定树干高度
    i32 trunkHeight;
    if (hasThirdBranch) {
        trunkHeight = height;
    } else if (hasSecondBranch) {
        trunkHeight = std::max(branchStartI, branchStartJ) + 1;
    } else {
        trunkHeight = branchStartI + 1;
    }

    // 放置主树干
    for (i32 y = 0; y < trunkHeight; ++y) {
        placeBlock(world, startPos.up(y), trunkBlocks, trunkBlock);
    }

    std::vector<FoliagePosition> foliagePositions;

    // 如果有3个分支，在树顶添加树叶位置
    if (hasThirdBranch) {
        foliagePositions.emplace_back(startPos.up(trunkHeight), 0, false);
    }

    // 选择随机水平方向
    Direction direction = Directions::horizontal()[random.nextInt(0, 4)];

    // 生成第一个分支
    foliagePositions.push_back(generateBranch(world,
        random,
        height,
        startPos,
        trunkBlock,
        direction,
        branchStartI,
        branchStartI < trunkHeight - 1,
        trunkBlocks));

    // 生成第二个分支（如果有2+个分支）
    if (hasSecondBranch) {
        foliagePositions.push_back(generateBranch(world,
            random,
            height,
            startPos,
            trunkBlock,
            Directions::opposite(direction),
            branchStartJ,
            branchStartJ < trunkHeight - 1,
            trunkBlocks));
    }

    return foliagePositions;
}

FoliagePosition CherryTrunkPlacer::generateBranch(WorldGenRegion& world,
    math::Random& random,
    i32 treeHeight,
    const BlockPos& startPos,
    const BlockState* trunkBlock,
    Direction direction,
    i32 branchStartHeight,
    bool canExtend,
    std::set<BlockPos>& trunkBlocks)
{
    // 起始位置
    BlockPos branchPos = startPos.up(branchStartHeight);

    // 计算分支末端高度（相对于树根的Y坐标）
    i32 endY = treeHeight - 1 + random.nextInt(m_branchEndOffsetFromTopMin, m_branchEndOffsetFromTopMax + 1);

    // 判断分支是否需要向上弯曲
    bool needsVerticalSection = canExtend || endY < branchStartHeight;

    // 计算水平长度
    i32 horizontalLength =
        random.nextInt(m_branchHorizontalLengthMin, m_branchHorizontalLengthMax + 1) + (needsVerticalSection ? 1 : 0);

    // 计算分支目标位置
    BlockPos targetPos;
    if (direction == Direction::North) {
        targetPos = BlockPos(startPos.x, startPos.y + endY, startPos.z - horizontalLength);
    } else if (direction == Direction::South) {
        targetPos = BlockPos(startPos.x, startPos.y + endY, startPos.z + horizontalLength);
    } else if (direction == Direction::West) {
        targetPos = BlockPos(startPos.x - horizontalLength, startPos.y + endY, startPos.z);
    } else {
        targetPos = BlockPos(startPos.x + horizontalLength, startPos.y + endY, startPos.z);
    }

    // 放置水平原木
    i32 horizontalLogs = needsVerticalSection ? 2 : 1;
    for (i32 i = 0; i < horizontalLogs; ++i) {
        if (direction == Direction::North) {
            branchPos = BlockPos(branchPos.x, branchPos.y, branchPos.z - 1);
        } else if (direction == Direction::South) {
            branchPos = BlockPos(branchPos.x, branchPos.y, branchPos.z + 1);
        } else if (direction == Direction::West) {
            branchPos = BlockPos(branchPos.x - 1, branchPos.y, branchPos.z);
        } else {
            branchPos = BlockPos(branchPos.x + 1, branchPos.y, branchPos.z);
        }
        placeBlock(world, branchPos, trunkBlocks, trunkBlock);
    }

    // 向目标位置弯曲行走
    Direction verticalDir = (targetPos.y > branchPos.y) ? Direction::Up : Direction::Down;

    while (true) {
        i32 manhattanDist = std::abs(targetPos.x - branchPos.x) + std::abs(targetPos.y - branchPos.y) +
            std::abs(targetPos.z - branchPos.z);
        if (manhattanDist == 0) {
            break;
        }

        // 根据Y距离与曼哈顿距离的比例决定垂直或水平移动
        f32 verticalProb = static_cast<f32>(std::abs(targetPos.y - branchPos.y)) / static_cast<f32>(manhattanDist);
        bool moveVertical = random.nextFloat() < verticalProb;

        if (moveVertical && branchPos.y != targetPos.y) {
            branchPos = BlockPos(branchPos.x, branchPos.y + (verticalDir == Direction::Up ? 1 : -1), branchPos.z);
            placeBlock(world, branchPos, trunkBlocks, trunkBlock);
        } else if (branchPos.x != targetPos.x || branchPos.z != targetPos.z) {
            if (branchPos.x != targetPos.x) {
                i32 dx = (targetPos.x > branchPos.x) ? 1 : -1;
                branchPos = BlockPos(branchPos.x + dx, branchPos.y, branchPos.z);
            } else {
                i32 dz = (targetPos.z > branchPos.z) ? 1 : -1;
                branchPos = BlockPos(branchPos.x, branchPos.y, branchPos.z + dz);
            }
            placeBlock(world, branchPos, trunkBlocks, trunkBlock);
        } else if (branchPos.y != targetPos.y) {
            branchPos = BlockPos(branchPos.x, branchPos.y + (verticalDir == Direction::Up ? 1 : -1), branchPos.z);
            placeBlock(world, branchPos, trunkBlocks, trunkBlock);
        } else {
            break;
        }
    }

    // 返回分支末端上方一格的树叶位置
    return FoliagePosition(targetPos.up(), 0, false);
}

std::unique_ptr<TrunkPlacer> CherryTrunkPlacer::clone() const
{
    return std::make_unique<CherryTrunkPlacer>(m_baseHeight,
        m_heightRandA,
        m_heightRandB,
        m_branchCountMin,
        m_branchCountMax,
        m_branchHorizontalLengthMin,
        m_branchHorizontalLengthMax,
        m_branchStartOffsetFromTopMin,
        m_branchStartOffsetFromTopMax,
        m_branchEndOffsetFromTopMin,
        m_branchEndOffsetFromTopMax);
}

} // namespace mc
