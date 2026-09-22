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
#include "server/world/gen/feature/tree/trunk/TrunkPlacer.hpp"
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

    // 分支连接点的高度上限（绝对 Y）
    i32 baseY = startPos.y + foliageHeight;

    // 自顶向下扫描分支时的起始高度偏移。此处必须保持**相对**语义（相对 startPos.y），
    // 一旦混入绝对 Y，_getBranchLength 收到的参数会超出 [0, trunkHeight] 区间，
    // 使 |halfHeight - y| 恒 >= halfHeight 而恒返回 0，分支长度退化为 0。
    i32 topOffset = trunkHeight - 5;

    // 待放置的分支列表。树干顶端本身视作一条特殊分支参与统一处理：
    // 连接点固定为树冠基点，终点为树干顶端，它同时承担"把主干从树冠基点延伸至顶端"的职责。
    struct BranchFoliage {
        BlockPos branchEnd;
        i32 branchBaseY;
        bool trunkTop;
    };
    std::vector<BranchFoliage> branchFoliages;
    branchFoliages.push_back({startPos.up(topOffset), baseY, true});

    // 自顶向下扫描分支
    for (i32 relY = topOffset; relY >= 0; --relY) {
        f32 branchLength = _getBranchLength(trunkHeight, relY);
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
                startPos.x + static_cast<i32>(dx), startPos.y + relY - 1, startPos.z + static_cast<i32>(dz));
            BlockPos branchTop = branchEnd.up(5);

            // 先检查分支末端向上 5 格的空间是否通畅（只探测，不放置）
            if (!_makeLimb(world, random, branchEnd, branchTop, false, trunkBlocks, trunkBlock)) {
                continue;
            }

            // 计算分支与树干的连接点，连接点不高于树冠基点
            i32 relX = startPos.x - branchEnd.x;
            i32 relZ = startPos.z - branchEnd.z;
            f64 dist = static_cast<f64>(branchEnd.y) - std::sqrt(static_cast<f64>(relX * relX + relZ * relZ)) * 0.381;
            i32 connectionY = dist > static_cast<f64>(baseY) ? baseY : static_cast<i32>(dist);

            BlockPos connectionPos(startPos.x, connectionY, startPos.z);

            // 再探测连接路径是否通畅（只探测，不放置）；放置统一推迟到扫描结束之后
            if (_makeLimb(world, random, connectionPos, branchEnd, false, trunkBlocks, trunkBlock)) {
                branchFoliages.push_back({branchEnd, connectionY, false});
            }
        }
    }

    // 放置主干（自起始位置到树冠基点）
    _makeLimb(world, random, startPos, startPos.up(foliageHeight), true, trunkBlocks, trunkBlock);

    // 放置各分支到主干的连接，并收集有效的树叶位置
    for (const auto& bf : branchFoliages) {
        BlockPos basePos(startPos.x, bf.branchBaseY, startPos.z);

        // 连接点与分支末端重合时无需放置，否则会得到零长度的 limb
        if (!(basePos == bf.branchEnd) && _shouldKeepFoliage(trunkHeight, bf.branchBaseY - startPos.y)) {
            _makeLimb(world, random, basePos, bf.branchEnd, true, trunkBlocks, trunkBlock);
        }

        if (_shouldKeepFoliage(trunkHeight, bf.branchBaseY - startPos.y)) {
            foliagePositions.emplace_back(bf.branchEnd, 0, bf.trunkTop);
        }
    }

    return foliagePositions;
}

f32 FancyTrunkPlacer::_getBranchLength(i32 trunkHeight, i32 relY) const
{
    // 根据相对起始位置的高度计算分支长度，relY 必须落在 [0, trunkHeight] 区间内
    if (static_cast<f32>(relY) < static_cast<f32>(trunkHeight) * 0.3f) {
        return -1.0f;
    }

    f32 halfHeight = static_cast<f32>(trunkHeight) / 2.0f;
    f32 diff = halfHeight - static_cast<f32>(relY);
    f32 result = std::sqrt(halfHeight * halfHeight - diff * diff);

    if (diff == 0.0f) {
        result = halfHeight;
    } else if (std::abs(diff) >= halfHeight) {
        return 0.0f;
    }

    return result * 0.5f;
}

bool FancyTrunkPlacer::_makeLimb(WorldGenRegion& world,
    math::Random& random,
    const BlockPos& start,
    const BlockPos& end,
    bool place,
    std::set<BlockPos>& trunkBlocks,
    const BlockState* trunkBlock)
{
    // 零长度 limb 在仅探测时视为通畅。该判断是承重的，不可删除：一旦让 start == end
    // 走到下方，steps 为 0，步长退化成 0.0f / 0.0f = NaN，再经浮点转整型会得到越界坐标。
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
