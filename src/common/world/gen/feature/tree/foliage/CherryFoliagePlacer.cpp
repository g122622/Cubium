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
 * copies of substantial portions of the Software.
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

#include "CherryFoliagePlacer.hpp"
#include "common/core/Types.hpp"
#include "common/util/Direction.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/WorldConstants.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/gen/chunk/IChunkGenerator.hpp"
#include "common/world/gen/feature/FeatureSpread.hpp"
#include "common/world/gen/feature/tree/foliage/FoliagePlacer.hpp"
#include "common/world/gen/feature/tree/trunk/TrunkPlacer.hpp"
#include <algorithm>
#include <cmath>
#include <memory>
#include <set>

namespace mc {

CherryFoliagePlacer::CherryFoliagePlacer(const FeatureSpread& radius,
    const FeatureSpread& offset,
    i32 height,
    f32 wideBottomLayerHoleChance,
    f32 cornerHoleChance,
    f32 hangingLeavesChance,
    f32 hangingLeavesExtensionChance)
    : FoliagePlacer(radius, offset)
    , m_height(height)
    , m_wideBottomLayerHoleChance(wideBottomLayerHoleChance)
    , m_cornerHoleChance(cornerHoleChance)
    , m_hangingLeavesChance(hangingLeavesChance)
    , m_hangingLeavesExtensionChance(hangingLeavesExtensionChance)
{}

i32 CherryFoliagePlacer::getFoliageHeight(math::Random& /*random*/, i32 /*trunkHeight*/) const
{
    return m_height;
}

void CherryFoliagePlacer::placeFoliageInternal(WorldGenRegion& world,
    math::Random& random,
    i32 /*trunkHeight*/,
    const FoliagePosition& foliagePos,
    i32 foliageHeight,
    i32 radius,
    i32 offset,
    std::set<BlockPos>& foliageBlocks,
    const BlockState* foliageBlock)
{
    // 有效半径
    i32 effectiveRadius = radius + foliagePos.radiusBonus - 1;
    BlockPos basePos = foliagePos.pos.up(offset);

    // 第 foliageHeight-3 层：半径 effectiveRadius-2
    if (foliageHeight - 3 >= 0) {
        placeFoliageLayer(world,
            random,
            basePos,
            std::max(effectiveRadius - 2, 0),
            foliageBlocks,
            basePos.y + foliageHeight - 3,
            foliagePos.trunkTop,
            foliageBlock);
    }

    // 第 foliageHeight-4 层：半径 effectiveRadius-1
    if (foliageHeight - 4 >= 0) {
        placeFoliageLayer(world,
            random,
            basePos,
            std::max(effectiveRadius - 1, 0),
            foliageBlocks,
            basePos.y + foliageHeight - 4,
            foliagePos.trunkTop,
            foliageBlock);
    }

    // 第 foliageHeight-5 到 0 层：满半径
    for (i32 y = foliageHeight - 5; y >= 0; --y) {
        placeFoliageLayer(
            world, random, basePos, effectiveRadius, foliageBlocks, basePos.y + y, foliagePos.trunkTop, foliageBlock);
    }

    // 第 -1 层：满半径 + 垂叶
    placeLeavesRowWithHangingLeavesBelow(world,
        random,
        basePos,
        effectiveRadius,
        basePos.y - 1,
        foliageBlocks,
        foliageBlock,
        m_hangingLeavesChance,
        m_hangingLeavesExtensionChance);

    // 第 -2 层：半径 effectiveRadius-1 + 垂叶
    placeLeavesRowWithHangingLeavesBelow(world,
        random,
        basePos,
        std::max(effectiveRadius - 1, 0),
        basePos.y - 2,
        foliageBlocks,
        foliageBlock,
        m_hangingLeavesChance,
        m_hangingLeavesExtensionChance);
}

bool CherryFoliagePlacer::shouldSkip(math::Random& random, i32 dx, i32 dy, i32 dz, i32 radius, bool /*trunkTop*/) const
{
    // 底层宽孔洞：y == -1 且在边缘
    if (dy == -1 && (std::abs(dx) == radius || std::abs(dz) == radius) &&
        random.nextFloat() < m_wideBottomLayerHoleChance) {
        return true;
    }

    // 角落位置
    bool isCorner = std::abs(dx) == radius && std::abs(dz) == radius;

    if (radius > 2) {
        // 大半径：角落 或 x+z > radius*2-2 时以概率跳过
        if (isCorner && random.nextFloat() < m_cornerHoleChance) {
            return true;
        }
        if (std::abs(dx) + std::abs(dz) > radius * 2 - 2 && random.nextFloat() < m_cornerHoleChance) {
            return true;
        }
    } else {
        // 小半径：仅角落以概率跳过
        if (isCorner && random.nextFloat() < m_cornerHoleChance) {
            return true;
        }
    }

    return false;
}

void CherryFoliagePlacer::placeLeavesRowWithHangingLeavesBelow(WorldGenRegion& world,
    math::Random& random,
    const BlockPos& centerPos,
    i32 radius,
    i32 y,
    std::set<BlockPos>& foliageBlocks,
    const BlockState* foliageBlock,
    f32 hangingChance,
    f32 extensionChance)
{
    // 先放置主树叶层
    placeFoliageLayer(world, random, centerPos, radius, foliageBlocks, y, false, foliageBlock);

    // 然后在边缘位置尝试放置垂叶
    i32 radiusOffset = 0; // doubleTrunk = false，所以 offset = 0
    BlockPos referencePos = centerPos.down();

    // 遍历4个水平方向
    const Direction horizontalDirs[] = {Direction::North, Direction::East, Direction::South, Direction::West};

    for (Direction dir : horizontalDirs) {
        // 获取顺时针方向的垂直方向（用于确定起始偏移）
        Direction clockwiseDir = Directions::rotateY(dir);
        Axis axis = Directions::getAxis(clockwiseDir);

        // 计算起始偏移（沿顺时针方向）
        i32 startJ;
        if (axis == Axis::X) {
            startJ = (Directions::getAxisDirection(clockwiseDir) == AxisDirection::Positive) ? radius + radiusOffset
                                                                                             : radius;
        } else {
            startJ = radius;
        }

        // 计算起始位置：沿顺时针方向偏移 startJ，沿主方向偏移 -radius
        BlockPos mutablePos = centerPos;
        // 先移动到 y-1 位置
        mutablePos.y = y - 1;

        // 计算沿顺时针方向的偏移
        if (clockwiseDir == Direction::North) {
            mutablePos.z -= startJ;
        } else if (clockwiseDir == Direction::South) {
            mutablePos.z += startJ;
        } else if (clockwiseDir == Direction::West) {
            mutablePos.x -= startJ;
        } else {
            mutablePos.x += startJ;
        }

        // 沿主方向偏移 -radius
        if (dir == Direction::North) {
            mutablePos.z += radius;
        } else if (dir == Direction::South) {
            mutablePos.z -= radius;
        } else if (dir == Direction::West) {
            mutablePos.x += radius;
        } else {
            mutablePos.x -= radius;
        }

        // 沿主方向遍历边缘
        for (i32 k = -radius; k <= radius + radiusOffset; ++k) {
            // 计算当前位置
            BlockPos edgePos = mutablePos;
            if (dir == Direction::North) {
                edgePos.z -= k;
            } else if (dir == Direction::South) {
                edgePos.z += k;
            } else if (dir == Direction::West) {
                edgePos.x -= k;
            } else {
                edgePos.x += k;
            }

            // 检查上方是否有树叶
            BlockPos abovePos = edgePos.up();
            bool hasLeafAbove = foliageBlocks.count(abovePos) > 0;

            if (hasLeafAbove) {
                // 检查曼哈顿距离限制
                i32 manhattanDist = std::abs(edgePos.x - referencePos.x) + std::abs(edgePos.y - referencePos.y) +
                    std::abs(edgePos.z - referencePos.z);
                if (manhattanDist >= 7) {
                    continue;
                }

                // 以 hangingChance 概率放置垂叶
                if (random.nextFloat() < hangingChance) {
                    // 检查位置是否可放置
                    if (edgePos.y >= world::MIN_BUILD_HEIGHT && edgePos.y < world::MAX_BUILD_HEIGHT) {
                        const BlockState* state = world.getBlockState(edgePos.x, edgePos.y, edgePos.z);
                        if (state == nullptr || state->isAir()) {
                            if (foliageBlock != nullptr) {
                                world.setBlockState(edgePos, foliageBlock);
                                foliageBlocks.insert(edgePos);

                                // 以 extensionChance 概率向下延伸一格
                                BlockPos belowPos = edgePos.down();
                                i32 belowManhattan = std::abs(belowPos.x - referencePos.x) +
                                    std::abs(belowPos.y - referencePos.y) + std::abs(belowPos.z - referencePos.z);
                                if (belowManhattan < 7 && random.nextFloat() < extensionChance) {
                                    if (belowPos.y >= world::MIN_BUILD_HEIGHT && belowPos.y < world::MAX_BUILD_HEIGHT) {
                                        const BlockState* belowState =
                                            world.getBlockState(belowPos.x, belowPos.y, belowPos.z);
                                        if (belowState == nullptr || belowState->isAir()) {
                                            world.setBlockState(belowPos, foliageBlock);
                                            foliageBlocks.insert(belowPos);
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

std::unique_ptr<FoliagePlacer> CherryFoliagePlacer::clone() const
{
    return std::make_unique<CherryFoliagePlacer>(m_radius,
        m_offset,
        m_height,
        m_wideBottomLayerHoleChance,
        m_cornerHoleChance,
        m_hangingLeavesChance,
        m_hangingLeavesExtensionChance);
}

} // namespace mc
