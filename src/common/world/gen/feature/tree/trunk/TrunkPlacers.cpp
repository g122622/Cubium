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

#include "TrunkPlacers.hpp"
#include "common/core/Constants.hpp"
#include "common/core/Types.hpp"
#include "common/util/Direction.hpp"
#include "common/world/block/BlockRegistry.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/gen/chunk/IChunkGenerator.hpp"
#include <cmath>

namespace mc {

// ============================================================================
// DarkOakTrunkPlacer 实现
// ============================================================================

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

// ============================================================================
// FancyTrunkPlacer 实现
// ============================================================================

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

// ============================================================================
// ForkyTrunkPlacer 实现
// ============================================================================

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
    // Direction 水平顺序: South=0, West=1, North=2, East=3 (在 Planes.HORIZONTAL 中)
    // 但这里我们直接使用偏移量计算
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

// ============================================================================
// GiantTrunkPlacer 实现
// ============================================================================

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

// ============================================================================
// MegaJungleTrunkPlacer 实现
// ============================================================================

MegaJungleTrunkPlacer::MegaJungleTrunkPlacer(i32 baseHeight, i32 heightRandA, i32 heightRandB)
    : TrunkPlacer(baseHeight, heightRandA, heightRandB)
{}

std::vector<FoliagePosition> MegaJungleTrunkPlacer::placeTrunk(WorldGenRegion& world,
    math::Random& random,
    i32 height,
    const BlockPos& startPos,
    std::set<BlockPos>& trunkBlocks,
    const BlockState* trunkBlock)
{
    std::vector<FoliagePosition> foliagePositions;

    // 先调用父类 GiantTrunkPlacer 放置基础树干和树叶
    foliagePositions = GiantTrunkPlacer(m_baseHeight, m_heightRandA, m_heightRandB)
                           .placeTrunk(world, random, height, startPos, trunkBlocks, trunkBlock);

    // 在树干上生成分支
    // 从 height-2-随机(4) 开始，每隔 2+随机(4) 格，向下到 height/2
    for (i32 branchHeight = height - 2 - random.nextInt(4); branchHeight > height / 2;
        branchHeight -= 2 + random.nextInt(4)) {

        // 随机角度
        f32 angle = random.nextFloat() * 2.0f * math::PI;
        i32 dx = 0;
        i32 dz = 0;

        // 沿着角度方向放置5个方块
        for (i32 step = 0; step < 5; ++step) {
            dx = static_cast<i32>(1.5f + std::cos(angle) * static_cast<f32>(step));
            dz = static_cast<i32>(1.5f + std::sin(angle) * static_cast<f32>(step));

            BlockPos branchPos(startPos.x + dx, startPos.y + branchHeight - 3 + step / 2, startPos.z + dz);

            // 放置分支方块（使用2x2方式）
            placeTrunkLayer2x2(world, branchPos, trunkBlocks, trunkBlock);
        }

        // 添加分支末端的树叶位置
        foliagePositions.emplace_back(BlockPos(startPos.x + dx, startPos.y + branchHeight, startPos.z + dz), -2, false);
    }

    return foliagePositions;
}

std::unique_ptr<TrunkPlacer> MegaJungleTrunkPlacer::clone() const
{
    return std::make_unique<MegaJungleTrunkPlacer>(m_baseHeight, m_heightRandA, m_heightRandB);
}

} // namespace mc
