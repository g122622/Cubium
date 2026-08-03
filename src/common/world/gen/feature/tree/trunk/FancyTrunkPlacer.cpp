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

#include "FancyTrunkPlacer.hpp"
#include "common/core/Types.hpp"
#include "common/util/math/MathConstants.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/gen/feature/tree/trunk/TrunkPlacer.hpp"
#include <algorithm>
#include <cmath>
#include <memory>
#include <set>
#include <vector>

namespace mc {

FancyTrunkPlacer::FancyTrunkPlacer(i32 baseHeight, i32 heightRandA, i32 heightRandB)
    : TrunkPlacer(baseHeight, heightRandA, heightRandB)
{}

std::vector<FoliagePosition> FancyTrunkPlacer::placeTrunk(WorldGenRegion& world,
    math::Random& random,
    i32 height,
    const BlockPos& startPos,
    std::set<BlockPos>& trunkBlocks,
    const BlockState* trunkBlock)
{
    std::vector<FoliagePosition> foliagePositions;

    // 计算有效高度
    i32 trunkHeight = height + 2;
    i32 foliageHeight = static_cast<i32>(std::floor(static_cast<f64>(trunkHeight) * 0.618));

    // 放置底部泥土
    placeDirtUnder(world, startPos);

    // 计算分支数量
    i32 branchCount =
        std::min(1, static_cast<i32>(std::floor(1.382 + std::pow(static_cast<f64>(trunkHeight) / 13.0, 2.0))));

    // 初始化树叶位置列表
    i32 baseY = startPos.y + foliageHeight;
    i32 topY = startPos.y + trunkHeight - 5;

    // 内部结构用于跟踪分支
    struct BranchFoliage {
        BlockPos branchEnd;
        i32 branchBaseY;
    };
    std::vector<BranchFoliage> branchFoliages;

    // 从上往下生成分支
    for (i32 y = topY; y >= 0; --y) {
        f32 branchLength = _getBranchLength(trunkHeight, y);
        if (branchLength < 0.0f) {
            continue;
        }

        for (i32 b = 0; b < branchCount; ++b) {
            // 计算分支方向和长度
            f32 actualLength = branchLength * (random.nextFloat() + 0.328f);
            f32 angle = random.nextFloat() * 2.0f * math::PI;
            f32 dx = actualLength * std::sin(angle) + 0.5f;
            f32 dz = actualLength * std::cos(angle) + 0.5f;

            BlockPos branchEnd(
                startPos.x + static_cast<i32>(dx), startPos.y + y - 1, startPos.z + static_cast<i32>(dz));
            BlockPos branchTop = branchEnd.up(5);

            // 检查分支路径是否可行
            if (_checkAndPlaceBranch(world, random, startPos, branchEnd, false, trunkBlocks, trunkBlock)) {
                // 计算分支连接点
                i32 relX = startPos.x - branchEnd.x;
                i32 relZ = startPos.z - branchEnd.z;
                f64 dist =
                    static_cast<f64>(branchEnd.y) - std::sqrt(static_cast<f64>(relX * relX + relZ * relZ)) * 0.381;
                i32 connectionY = dist > static_cast<f64>(baseY) ? baseY : static_cast<i32>(dist);

                BlockPos connectionPos(startPos.x, connectionY, startPos.z);

                // 放置连接分支
                if (_checkAndPlaceBranch(world, random, connectionPos, branchEnd, true, trunkBlocks, trunkBlock)) {
                    branchFoliages.push_back({branchEnd, connectionY});
                }
            }
        }
    }

    // 放置主干
    _placeLine(world, random, startPos, startPos.up(foliageHeight), true, trunkBlocks, trunkBlock);

    // 放置分支到主干的连接
    for (const auto& bf : branchFoliages) {
        BlockPos basePos(startPos.x, bf.branchBaseY, startPos.z);
        if (!(basePos == bf.branchEnd) && _shouldKeepFoliage(trunkHeight, bf.branchBaseY - startPos.y)) {
            _placeLine(world, random, basePos, bf.branchEnd, true, trunkBlocks, trunkBlock);
        }
    }

    // 收集有效的树叶位置
    for (const auto& bf : branchFoliages) {
        if (_shouldKeepFoliage(trunkHeight, bf.branchBaseY - startPos.y)) {
            foliagePositions.emplace_back(bf.branchEnd, 0, false);
        }
    }

    // 添加顶部树叶位置
    foliagePositions.emplace_back(startPos.up(topY), 0, true);

    return foliagePositions;
}

f32 FancyTrunkPlacer::_getBranchLength(i32 trunkHeight, i32 y) const
{
    // 根据高度计算分支长度
    if (static_cast<f32>(y) < static_cast<f32>(trunkHeight) * 0.3f) {
        return -1.0f;
    }

    f32 halfHeight = static_cast<f32>(trunkHeight) / 2.0f;
    f32 diff = halfHeight - static_cast<f32>(y);
    f32 result = std::sqrt(halfHeight * halfHeight - diff * diff);

    if (diff == 0.0f) {
        result = halfHeight;
    } else if (std::abs(diff) >= halfHeight) {
        return 0.0f;
    }

    return result * 0.5f;
}

bool FancyTrunkPlacer::_checkAndPlaceBranch(WorldGenRegion& world,
    math::Random& random,
    const BlockPos& start,
    const BlockPos& end,
    bool place,
    std::set<BlockPos>& trunkBlocks,
    const BlockState* trunkBlock)
{
    if (!place && start == end) {
        return true;
    }

    BlockPos delta(end.x - start.x, end.y - start.y, end.z - start.z);
    i32 steps = _getSteps(delta);

    f32 stepX = static_cast<f32>(delta.x) / static_cast<f32>(steps);
    f32 stepY = static_cast<f32>(delta.y) / static_cast<f32>(steps);
    f32 stepZ = static_cast<f32>(delta.z) / static_cast<f32>(steps);

    for (i32 i = 0; i <= steps; ++i) {
        BlockPos pos(static_cast<i32>(0.5f + static_cast<f32>(i) * stepX + static_cast<f32>(start.x)),
            static_cast<i32>(0.5f + static_cast<f32>(i) * stepY + static_cast<f32>(start.y)),
            static_cast<i32>(0.5f + static_cast<f32>(i) * stepZ + static_cast<f32>(start.z)));

        if (place) {
            placeBlock(world, pos, trunkBlocks, trunkBlock);
        } else {
            if (!canPlaceAt(world, pos)) {
                return false;
            }
        }
    }

    return true;
}

void FancyTrunkPlacer::_placeLine(WorldGenRegion& world,
    math::Random& random,
    const BlockPos& start,
    const BlockPos& end,
    bool place,
    std::set<BlockPos>& trunkBlocks,
    const BlockState* trunkBlock)
{
    _checkAndPlaceBranch(world, random, start, end, place, trunkBlocks, trunkBlock);
}

i32 FancyTrunkPlacer::_getSteps(const BlockPos& delta) const
{
    i32 absX = std::abs(delta.x);
    i32 absY = std::abs(delta.y);
    i32 absZ = std::abs(delta.z);
    return std::max({absX, absY, absZ});
}

bool FancyTrunkPlacer::_shouldKeepFoliage(i32 trunkHeight, i32 relY) const
{
    return static_cast<f64>(relY) >= static_cast<f64>(trunkHeight) * 0.2;
}

std::unique_ptr<TrunkPlacer> FancyTrunkPlacer::clone() const
{
    return std::make_unique<FancyTrunkPlacer>(m_baseHeight, m_heightRandA, m_heightRandB);
}

} // namespace mc
