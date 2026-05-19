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
#include "../../../../block/BlockRegistry.hpp"
#include "../../../../block/VanillaBlocks.hpp"
#include "../../../chunk/IChunkGenerator.hpp"
#include <cmath>

namespace mc {

// ============================================================================
// DarkOakTrunkPlacer 实现 - 参考 MC DarkOakTrunkPlacer.java
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

    // MC 第32-36行: 在树干底部放置泥土
    BlockPos below = startPos.down();
    placeDirtUnder(world, below);
    placeDirtUnder(world, BlockPos(below.x + 1, below.y, below.z));
    placeDirtUnder(world, BlockPos(below.x, below.y, below.z + 1));
    placeDirtUnder(world, BlockPos(below.x + 1, below.y, below.z + 1));

    // MC 第37-39行: 随机方向和弯曲参数
    i32 bendStart = height - random.nextInt(4); // i: 开始弯曲的高度
    i32 bendLength = 2 - random.nextInt(3);     // j: 弯曲长度

    i32 x = startPos.x;
    i32 z = startPos.z;
    i32 y = startPos.y;
    i32 topY = y + height - 1;

    // MC 第47-62行: 生成2x2树干，可能带有弯曲
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

    // MC 第64行: 顶部树叶位置
    foliagePositions.emplace_back(BlockPos(x, topY, z), 0, true);

    // MC 第66-78行: 四个角落可能生成额外枝干
    for (i32 cornerX = -1; cornerX <= 2; ++cornerX) {
        for (i32 cornerZ = -1; cornerZ <= 2; ++cornerZ) {
            // 只处理角落位置 (不是中心2x2区域)
            if ((cornerX < 0 || cornerX > 1 || cornerZ < 0 || cornerZ > 1) && random.nextInt(3) <= 0) {
                // MC 第69-73行: 生成枝干
                i32 branchLength = random.nextInt(3) + 2; // j3: 2-4格高

                for (i32 b = 0; b < branchLength; ++b) {
                    placeBlock(world,
                        BlockPos(startPos.x + cornerX, topY - b - 1, startPos.z + cornerZ),
                        trunkBlocks,
                        trunkBlock);
                }

                // MC 第75行: 枝干末端树叶位置
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
// FancyTrunkPlacer 实现 - 参考 MC FancyTrunkPlacer.java
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

    // MC 第34-36行: 计算有效高度
    i32 trunkHeight = height + 2;
    i32 foliageHeight = static_cast<i32>(std::floor(static_cast<f64>(trunkHeight) * 0.618));

    // MC 第37-39行: 放置底部泥土
    placeDirtUnder(world, startPos);

    // MC 第42行: 计算分支数量
    i32 branchCount =
        std::min(1, static_cast<i32>(std::floor(1.382 + std::pow(static_cast<f64>(trunkHeight) / 13.0, 2.0))));

    // MC 第43-46行: 初始化树叶位置列表
    i32 baseY = startPos.y + foliageHeight;
    i32 topY = startPos.y + trunkHeight - 5; // j1 = trunkHeight - 5

    // 内部结构用于跟踪分支
    struct BranchFoliage {
        BlockPos branchEnd;
        i32 branchBaseY;
    };
    std::vector<BranchFoliage> branchFoliages;

    // MC 第48-71行: 从上往下生成分支
    for (i32 y = topY; y >= 0; --y) {
        f32 branchLength = getBranchLength(trunkHeight, y);
        if (branchLength < 0.0f) {
            continue;
        }

        for (i32 b = 0; b < branchCount; ++b) {
            // MC 第53-56行: 计算分支方向和长度
            f32 actualLength = branchLength * (random.nextFloat() + 0.328f);
            f32 angle = random.nextFloat() * 2.0f * math::PI;
            f32 dx = actualLength * std::sin(angle) + 0.5f;
            f32 dz = actualLength * std::cos(angle) + 0.5f;

            BlockPos branchEnd(
                startPos.x + static_cast<i32>(dx), startPos.y + y - 1, startPos.z + static_cast<i32>(dz));
            BlockPos branchTop = branchEnd.up(5);

            // MC 第59行: 检查分支路径是否可行
            if (checkAndPlaceBranch(world, random, startPos, branchEnd, false, trunkBlocks, trunkBlock)) {
                // MC 第60-63行: 计算分支连接点
                i32 relX = startPos.x - branchEnd.x;
                i32 relZ = startPos.z - branchEnd.z;
                f64 dist =
                    static_cast<f64>(branchEnd.y) - std::sqrt(static_cast<f64>(relX * relX + relZ * relZ)) * 0.381;
                i32 connectionY = dist > static_cast<f64>(baseY) ? baseY : static_cast<i32>(dist);

                BlockPos connectionPos(startPos.x, connectionY, startPos.z);

                // MC 第65行: 放置连接分支
                if (checkAndPlaceBranch(world, random, connectionPos, branchEnd, true, trunkBlocks, trunkBlock)) {
                    branchFoliages.push_back({branchEnd, connectionY});
                }
            }
        }
    }

    // MC 第73行: 放置主干
    placeLine(world, random, startPos, startPos.up(foliageHeight), true, trunkBlocks, trunkBlock);

    // MC 第74行: 放置分支到主干的连接
    for (const auto& bf : branchFoliages) {
        BlockPos basePos(startPos.x, bf.branchBaseY, startPos.z);
        if (!(basePos == bf.branchEnd) && shouldKeepFoliage(trunkHeight, bf.branchBaseY - startPos.y)) {
            placeLine(world, random, basePos, bf.branchEnd, true, trunkBlocks, trunkBlock);
        }
    }

    // MC 第76-81行: 收集有效的树叶位置
    for (const auto& bf : branchFoliages) {
        if (shouldKeepFoliage(trunkHeight, bf.branchBaseY - startPos.y)) {
            foliagePositions.emplace_back(bf.branchEnd, 0, false);
        }
    }

    // 添加顶部树叶位置
    foliagePositions.emplace_back(startPos.up(topY), 0, true);

    return foliagePositions;
}

f32 FancyTrunkPlacer::getBranchLength(i32 trunkHeight, i32 y) const
{
    // MC 第148-163行
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

bool FancyTrunkPlacer::checkAndPlaceBranch(WorldGenRegion& world,
    math::Random& random,
    const BlockPos& start,
    const BlockPos& end,
    bool place,
    std::set<BlockPos>& trunkBlocks,
    const BlockState* trunkBlock)
{
    // MC 第86-108行
    if (!place && start == end) {
        return true;
    }

    BlockPos delta(end.x - start.x, end.y - start.y, end.z - start.z);
    i32 steps = getSteps(delta);

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

void FancyTrunkPlacer::placeLine(WorldGenRegion& world,
    math::Random& random,
    const BlockPos& start,
    const BlockPos& end,
    bool place,
    std::set<BlockPos>& trunkBlocks,
    const BlockState* trunkBlock)
{
    checkAndPlaceBranch(world, random, start, end, place, trunkBlocks, trunkBlock);
}

i32 FancyTrunkPlacer::getSteps(const BlockPos& delta) const
{
    // MC 第110-115行
    i32 absX = std::abs(delta.x);
    i32 absY = std::abs(delta.y);
    i32 absZ = std::abs(delta.z);
    return std::max({absX, absY, absZ});
}

bool FancyTrunkPlacer::shouldKeepFoliage(i32 trunkHeight, i32 relY) const
{
    // MC 第133-135行
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

    // 主树干
    for (i32 y = 0; y < height; ++y) {
        placeBlock(world, BlockPos(startPos.x, startPos.y + y, startPos.z), trunkBlocks, trunkBlock);
    }

    // 在上半部分生成分叉
    i32 branchStartY = startPos.y + height / 2;
    i32 numBranches = 2 + random.nextInt(3);

    for (i32 i = 0; i < numBranches; ++i) {
        // 分叉起点
        i32 branchY = branchStartY + random.nextInt(height / 2);
        BlockPos branchStart(startPos.x, branchY, startPos.z);

        // 分叉长度和方向
        i32 branchLength = 2 + random.nextInt(4);
        BlockPos branchEnd = generateBranch(world, random, branchStart, branchLength, trunkBlocks, trunkBlock);

        // 在分叉末端添加树叶
        foliagePositions.emplace_back(BlockPos(branchEnd.x, branchEnd.y + 1, branchEnd.z), 2 + random.nextInt(2), true);
    }

    // 顶部树叶
    foliagePositions.emplace_back(BlockPos(startPos.x, startPos.y + height, startPos.z), 2, true);

    return foliagePositions;
}

BlockPos ForkyTrunkPlacer::generateBranch(WorldGenRegion& world,
    math::Random& random,
    const BlockPos& startPos,
    i32 length,
    std::set<BlockPos>& trunkBlocks,
    const BlockState* trunkBlock)
{
    // 随机方向
    i32 dx = random.nextInt(3) - 1; // -1, 0, 1
    i32 dz = random.nextInt(3) - 1;

    // 确保方向不为零
    if (dx == 0 && dz == 0) {
        dx = random.nextBoolean() ? 1 : -1;
    }

    i32 x = startPos.x;
    i32 y = startPos.y;
    i32 z = startPos.z;

    for (i32 i = 0; i < length; ++i) {
        x += dx;
        z += dz;
        // 分叉略微向上生长
        if (random.nextInt(3) == 0) {
            y += 1;
        }

        placeBlock(world, BlockPos(x, y, z), trunkBlocks, trunkBlock);
    }

    return BlockPos(x, y, z);
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
// MegaJungleTrunkPlacer 实现 - 参考 MC MegaJungleTrunkPlacer.java
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

    // MC 第29-31行: 先调用父类 GiantTrunkPlacer 放置基础树干和树叶
    foliagePositions = GiantTrunkPlacer(m_baseHeight, m_heightRandA, m_heightRandB)
                           .placeTrunk(world, random, height, startPos, trunkBlocks, trunkBlock);

    // MC 第33-46行: 在树干上生成分支
    // 从 height-2-随机(4) 开始，每隔 2+随机(4) 格，向下到 height/2
    for (i32 branchHeight = height - 2 - random.nextInt(4); branchHeight > height / 2;
        branchHeight -= 2 + random.nextInt(4)) {

        // MC 第34行: 随机角度
        f32 angle = random.nextFloat() * 2.0f * math::PI;
        i32 dx = 0;
        i32 dz = 0;

        // MC 第38-43行: 沿着角度方向放置5个方块
        for (i32 step = 0; step < 5; ++step) {
            dx = static_cast<i32>(1.5f + std::cos(angle) * static_cast<f32>(step));
            dz = static_cast<i32>(1.5f + std::sin(angle) * static_cast<f32>(step));

            BlockPos branchPos(startPos.x + dx, startPos.y + branchHeight - 3 + step / 2, startPos.z + dz);

            // 放置分支方块（使用2x2方式）
            placeTrunkLayer2x2(world, branchPos, trunkBlocks, trunkBlock);
        }

        // MC 第45行: 添加分支末端的树叶位置
        foliagePositions.emplace_back(BlockPos(startPos.x + dx, startPos.y + branchHeight, startPos.z + dz), -2, false);
    }

    return foliagePositions;
}

std::unique_ptr<TrunkPlacer> MegaJungleTrunkPlacer::clone() const
{
    return std::make_unique<MegaJungleTrunkPlacer>(m_baseHeight, m_heightRandA, m_heightRandB);
}

} // namespace mc
