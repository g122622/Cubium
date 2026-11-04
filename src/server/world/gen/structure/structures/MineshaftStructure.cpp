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

#include "MineshaftStructure.hpp"

#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/IWorldWriter.hpp"
#include "common/world/WorldConstants.hpp"
#include "common/world/biome/BiomeIds.hpp"
#include "common/world/biome/BiomeTag.hpp"
#include "common/world/biome/BiomeTags.hpp"
#include "common/world/block/BlockUpdateFlags.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/chunk/data/Heightmap.hpp"
#include "common/world/gen/structure/StructureBoundingBox.hpp"
#include "server/world/gen/chunk/IChunkGenerator.hpp"
#include "server/world/gen/structure/Structure.hpp"
#include <array>
#include <cmath>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace mc::world::gen::structure {

// ============================================================================
// 辅助函数实现
// ============================================================================

std::unique_ptr<MineshaftPiece> createMineshaftPiece(std::vector<std::unique_ptr<MineshaftPiece>>& pieces,
    math::IRandom& rng,
    i32 x,
    i32 y,
    i32 z,
    i32 direction,
    i32 depth,
    MineshaftType type)
{
    i32 chance = rng.nextInt(100);

    std::unique_ptr<MineshaftPiece> created;

    if (chance >= 80) {
        // 交叉点：高度由一次 nextInt(4) 决定（0 表示双层，高 6；否则高 2），
        // 水平范围按朝向取固定的偏移表。
        //
        // 原版 MineShaftCrossing.findCrossing：
        //   int i = nextInt(4) == 0 ? 6 : 2;
        //   北 -> (-1, 0, -4) .. (3, i, 0)
        //   南 -> (-1, 0,  0) .. (3, i, 4)
        //   西 -> (-4, 0, -1) .. (0, i, 3)
        //   东 -> ( 0, 0, -1) .. (4, i, 3)
        // 此前既没有那次 nextInt(4)，偏移也与原版不同。
        const i32 crossHeight = (rng.nextInt(4) == 0) ? 6 : 2;
        i32 minX = x - 1, maxX = x + 3;
        i32 minZ = z - 4, maxZ = z;
        switch (direction) {
            case 1: // 南
                minZ = z;
                maxZ = z + 4;
                break;
            case 2: // 西
                minX = x - 4;
                maxX = x;
                minZ = z - 1;
                maxZ = z + 3;
                break;
            case 3: // 东
                minX = x;
                maxX = x + 4;
                minZ = z - 1;
                maxZ = z + 3;
                break;
            case 0: // 北
            default:
                break;
        }
        const i32 minY = y;
        const i32 maxY = y + crossHeight;

        created = std::make_unique<MineshaftCross>(
            MineshaftPieceTypes::CROSS, minX, minY, minZ, maxX, maxY, maxZ, direction, type);
    } else if (chance >= 70) {
        // 楼梯：固定下降 5 格、水平延伸 8 格，按朝向取偏移表。
        //
        // 原版 MineshaftStairs.findStairs：
        //   北 -> (0, -5, -8) .. (2, 2, 0)
        //   南 -> (0, -5,  0) .. (2, 2, 8)
        //   西 -> (-8, -5, 0) .. (0, 2, 2)
        //   东 -> ( 0, -5, 0) .. (8, 2, 2)
        // 此前 Y 方向只降了 4 格（length/2），与原版差 1。
        constexpr i32 kStairsDrop = 5;
        constexpr i32 kStairsReach = 8;
        i32 minX = x, maxX = x + 2;
        i32 minZ = z, maxZ = z + 2;
        switch (direction) {
            case 0: // 北
                minZ = z - kStairsReach;
                maxZ = z;
                break;
            case 1: // 南
                minZ = z;
                maxZ = z + kStairsReach;
                break;
            case 2: // 西
                minX = x - kStairsReach;
                maxX = x;
                break;
            case 3: // 东
                minX = x;
                maxX = x + kStairsReach;
                break;
        }
        const i32 minY = y - kStairsDrop;
        const i32 maxY = y + 2;

        created = std::make_unique<MineshaftStairs>(
            MineshaftPieceTypes::STAIRS, minX, minY, minZ, maxX, maxY, maxZ, direction, type);
    } else {
        // 走廊长度从 nextInt(3)+2 起逐个尝试缩短，第一个不与已有构件相交的长度胜出。
        //
        // 原版 findCorridorSize 的写法是
        //   for (int i = nextInt(3) + 2; i > 0; i--) { int j = i * 5; ... 相交就继续缩短 }
        // 因此走廊跨度恒为 5 的倍数（10 / 15 / 20）。此前用固定长度减去后直接构造，
        // 边界为此偏移 1 格。
        constexpr i32 kCorridorLengthStep = 5;
        i32 minX = x, maxX = x + 2;
        i32 minZ = z, maxZ = z + 2;
        i32 minY = y, maxY = y + 2;
        bool found = false;
        for (i32 attempt = rng.nextInt(3) + 2; attempt > 0; --attempt) {
            const i32 span = attempt * kCorridorLengthStep - 1;
            i32 trialMinX = x, trialMaxX = x + 2;
            i32 trialMinZ = z, trialMaxZ = z + 2;
            switch (direction) {
                case 0: // 北
                    trialMinZ = z - span;
                    trialMaxZ = z;
                    break;
                case 1: // 南
                    trialMinZ = z;
                    trialMaxZ = z + span;
                    break;
                case 2: // 西
                    trialMinX = x - span;
                    trialMaxX = x;
                    break;
                case 3: // 东
                default:
                    trialMinX = x;
                    trialMaxX = x + span;
                    break;
            }
            const StructureBoundingBox trial(trialMinX, minY, trialMinZ, trialMaxX, maxY, trialMaxZ);
            bool collides = false;
            for (const auto& existing : pieces) {
                if (existing->boundingBox().intersects(trial)) {
                    collides = true;
                    break;
                }
            }
            if (!collides) {
                minX = trialMinX;
                maxX = trialMaxX;
                minZ = trialMinZ;
                maxZ = trialMaxZ;
                found = true;
                break;
            }
        }
        if (!found) {
            return nullptr;
        }

        created = std::make_unique<MineshaftCorridor>(
            MineshaftPieceTypes::CORRIDOR, rng, minX, minY, minZ, maxX, maxY, maxZ, direction, type);
    }

    // 【必须设置生成深度】构件类型 ID（ROOM=60/CORRIDOR=61/CROSS=62/STAIRS=63）与生成深度
    // 是两个不同的量。addMineshaftPiece 用 `depth > 8` 限制展开层数；此前实现把类型 ID
    // 当深度传入，首次展开即 61 > 8 直接失败，矿井永远只剩起点房间一个构件。
    if (created) {
        created->setGenDepth(depth);

        // 每个构件都必须记录朝向，否则 getRotation() 恒为 NONE。
        //
        // 原版 StructurePiece.setOrientation 的映射：
        //   NORTH -> mirror NONE,        rotation NONE
        //   SOUTH -> mirror LEFT_RIGHT,  rotation NONE
        //   WEST  -> mirror LEFT_RIGHT,  rotation CLOCKWISE_90
        //   EAST  -> mirror NONE,        rotation CLOCKWISE_90
        // 朝向同时决定构件内部的方块布局，缺了它整个矿井的构件全部按朝北绘制。
        switch (direction) {
            case 1: // 南
                created->setMirror(Mirror::LeftRight);
                created->setRotation(Rotation::None);
                break;
            case 2: // 西
                created->setMirror(Mirror::LeftRight);
                created->setRotation(Rotation::Clockwise90);
                break;
            case 3: // 东
                created->setMirror(Mirror::None);
                created->setRotation(Rotation::Clockwise90);
                break;
            case 0: // 北
            default:
                created->setMirror(Mirror::None);
                created->setRotation(Rotation::None);
                break;
        }

        // 新构件不得与已有构件重叠。
        //
        // 原版 findCorridorSize / findCrossing / findStairs 在返回包围盒前都会调用
        // findCollisionPiece，一旦相交就返回 null、该分支不再生成。这一步是把
        // 「每段三分支、深度 8 层」的指数展开收敛到实际几十个构件的关键——
        // 缺了它矿井会膨胀到数千个构件（实测 5681）。
        const StructureBoundingBox box = created->boundingBox();
        for (const auto& existing : pieces) {
            if (existing->boundingBox().intersects(box)) {
                return nullptr;
            }
        }
    }

    return created;
}

void addMineshaftPiece(MineshaftPiece* parent,
    std::vector<std::unique_ptr<MineshaftPiece>>& pieces,
    math::IRandom& rng,
    i32 x,
    i32 y,
    i32 z,
    i32 direction,
    i32 depth,
    MineshaftType type)
{
    // 深度限制。原版 generateAndAddPiece 的判据是 `p_227714_ > 8`，而 p_227714_ 是
    // **调用方传入的深度**（即新构件的父深度），子构件随后取 p_227714_ + 1。
    // 因此最大构件深度是 9，不是 8——判据比的是父深度。
    // 这里 depth 已经是新构件自身的深度，所以要拿 depth - 1 去比。
    if (depth - 1 > 8) {
        return;
    }

    // 检查距离限制（防止无限扩展）
    if (parent) {
        i32 dx = std::abs(x - parent->minX());
        i32 dz = std::abs(z - parent->minZ());
        if (dx > 80 || dz > 80) {
            return;
        }
    }

    // 创建新片段
    auto piece = createMineshaftPiece(pieces, rng, x, y, z, direction, depth, type);
    if (!piece) {
        return;
    }

    // 必须**先登记再展开子构件**。原版 generateAndAddPiece 的顺序是
    //   accessor.addPiece(piece); piece.addChildren(...);
    // 此前这里先展开、由调用方随后才 push，结果是子构件排在父构件之前，
    // 整座矿井的 pieces 顺序完全倒置。
    pieces.push_back(std::move(piece));
    pieces.back()->buildComponent(pieces, rng, 8);
}

// ============================================================================
// MineshaftPiece 实现
// ============================================================================

MineshaftPiece::MineshaftPiece(
    i32 type, i32 minX, i32 minY, i32 minZ, i32 maxX, i32 maxY, i32 maxZ, MineshaftType mineshaftType)
    : StructurePiece(type, minX, minY, minZ, maxX, maxY, maxZ)
    , m_mineshaftType(mineshaftType)
{}

bool MineshaftPiece::_canPlaceAt(i32 /*x*/, i32 y, i32 /*z*/)
{
    return world::isValidY(y) && y > world::MIN_BUILD_HEIGHT + 4;
}

void MineshaftPiece::_generateSupport(IWorldWriter& world, i32 x, i32 y, i32 z, i32 height, math::IRandom& rng)
{
    const BlockState* fenceState = VanillaBlocks::getState(VanillaBlocks::OAK_LOG); // 使用原木代替栅栏
    const BlockState* planksState = VanillaBlocks::getState(VanillaBlocks::OAK_PLANKS);

    // 生成原木柱（代替栅栏柱）
    for (i32 dy = 0; dy < height; ++dy) {
        world.setBlockState(x, y + dy, z, fenceState);
    }

    // 顶部放置木板
    if (rng.nextFloat() < 0.7f) {
        world.setBlockState(x, y + height, z, planksState);
    }
}

// ============================================================================
// MineshaftRoom 实现
// ============================================================================

namespace {

/**
 * 按 X -> Y -> Z 的顺序抽取房间的三个跨度。
 *
 * 原版是 new BoundingBox(x, 50, z, x+7+nextInt(6), 54+nextInt(6), z+7+nextInt(6))，
 * Java 保证实参从左到右求值；而 C++ 的实参求值顺序未指定，
 * 把三个 nextInt(6) 写在同一构造实参里会让随机序列错位
 * （实测房间高度变成 5，而原版是 10）。
 */
std::array<i32, 3> drawRoomSpans(math::IRandom& rng)
{
    const i32 a = rng.nextInt(6);
    const i32 b = rng.nextInt(6);
    const i32 c2 = rng.nextInt(6);
    return {a, b, c2};
}

} // namespace

MineshaftRoom::MineshaftRoom(i32 componentType, math::IRandom& rng, i32 x, i32 y, i32 z, MineshaftType type)
    : MineshaftRoom(componentType, rng, x, y, z, type, drawRoomSpans(rng))
{}

MineshaftRoom::MineshaftRoom(
    i32 componentType, math::IRandom& rng, i32 x, i32 y, i32 z, MineshaftType type, std::array<i32, 3> spans)
    : MineshaftPiece(componentType, x, 50, z, x + 7 + spans[0], 54 + spans[1], z + 7 + spans[2], type)
{
    // 房间的起点 Y 恒为 50，X/Z 跨度各为 7..12、顶面为 54..59。
    // 传入的 y 在原版里不参与房间包围盒。
    MC_UNUSED(y);
    MC_UNUSED(rng);
    MC_UNUSED(type);
}

void MineshaftRoom::generate(IWorldWriter& world,
    math::IRandom& rng,
    i32 /*chunkX*/,
    i32 /*chunkZ*/,
    const StructureBoundingBox& chunkBounds,
    ChunkPrimer* /*chunk*/,
    IChunkGenerator* /*generator*/)
{
    const BlockState* planksState = VanillaBlocks::getState(VanillaBlocks::OAK_PLANKS);
    const BlockState* fenceState = VanillaBlocks::getState(VanillaBlocks::OAK_LOG); // 使用原木代替栅栏

    // 生成房间地板
    for (i32 x = minX(); x <= maxX(); ++x) {
        for (i32 z = minZ(); z <= maxZ(); ++z) {
            if (chunkBounds.contains(x, minY(), z)) {
                world.setBlockState(x, minY(), z, planksState);
            }
        }
    }

    // 生成角落支撑柱
    for (i32 dx = 0; dx <= 7; dx += 7) {
        for (i32 dz = 0; dz <= 7; dz += 7) {
            i32 x = minX() + dx;
            i32 z = minZ() + dz;
            if (chunkBounds.contains(x, minY() + 1, z)) {
                _generateSupport(world, x, minY() + 1, z, 3, rng);
            }
        }
    }

    // 随机放置一些额外的栅栏
    for (i32 i = 0; i < 4 + rng.nextInt(4); ++i) {
        i32 x = minX() + 1 + rng.nextInt(6);
        i32 z = minZ() + 1 + rng.nextInt(6);
        if (chunkBounds.contains(x, minY() + 1, z)) {
            world.setBlockState(x, minY() + 1, z, fenceState);
        }
    }
}

void MineshaftRoom::buildComponent(
    std::vector<std::unique_ptr<MineshaftPiece>>& pieces, math::IRandom& rng, i32 maxDepth)
{
    MC_UNUSED(maxDepth);

    // 房间沿四条边各开一串出口，每条边都用「随机步进 + 固定 4 格间隔」推进：
    //
    //   j = 0;
    //   while (j < span) {
    //       j += nextInt(span);
    //       if (j + 3 > span) break;      // 剩余跨度不足 3
    //       ...在 j 处开口，y = minY + nextInt(k) + 1...
    //       j += 4;
    //   }
    //
    // 四个循环的顺序固定为 北、南、西、东，各自独立重置 j。
    //
    // 子构件的深度是父深度 + 1，理由同走廊。
    const i32 depth = genDepth() + 1;
    if (depth > maxDepth) {
        return;
    }
    i32 k = (maxY() - minY() + 1) - 3 - 1;
    if (k <= 0) {
        k = 1;
    }

    auto addChild = [&](i32 cx, i32 cy, i32 cz, i32 dir) {
        addMineshaftPiece(this, pieces, rng, cx, cy, cz, dir, depth, m_mineshaftType);
    };

    const i32 xSpan = maxX() - minX() + 1;
    const i32 zSpan = maxZ() - minZ() + 1;

    // 北侧：沿 X 步进，出口在 minZ - 1
    for (i32 offset = 0; offset < xSpan;) {
        offset += rng.nextInt(xSpan);
        if (offset + 3 > xSpan) {
            break;
        }
        const i32 cx = minX() + offset;
        const i32 cy = minY() + rng.nextInt(k) + 1; // 独立语句：实参求值顺序未指定
        addChild(cx, cy, minZ() - 1, 0);
        offset += 4;
    }

    // 南侧：沿 X 步进，出口在 maxZ + 1
    for (i32 offset = 0; offset < xSpan;) {
        offset += rng.nextInt(xSpan);
        if (offset + 3 > xSpan) {
            break;
        }
        const i32 cx = minX() + offset;
        const i32 cy = minY() + rng.nextInt(k) + 1;
        addChild(cx, cy, maxZ() + 1, 1);
        offset += 4;
    }

    // 西侧：沿 Z 步进，出口在 minX - 1
    for (i32 offset = 0; offset < zSpan;) {
        offset += rng.nextInt(zSpan);
        if (offset + 3 > zSpan) {
            break;
        }
        const i32 cz = minZ() + offset;
        const i32 cy = minY() + rng.nextInt(k) + 1;
        addChild(minX() - 1, cy, cz, 2);
        offset += 4;
    }

    // 东侧：沿 Z 步进，出口在 maxX + 1
    for (i32 offset = 0; offset < zSpan;) {
        offset += rng.nextInt(zSpan);
        if (offset + 3 > zSpan) {
            break;
        }
        const i32 cz = minZ() + offset;
        const i32 cy = minY() + rng.nextInt(k) + 1;
        addChild(maxX() + 1, cy, cz, 3);
        offset += 4;
    }
}

// ============================================================================
// MineshaftCorridor 实现
// ============================================================================

MineshaftCorridor::MineshaftCorridor(i32 componentType,
    math::IRandom& rng,
    i32 minX,
    i32 minY,
    i32 minZ,
    i32 maxX,
    i32 maxY,
    i32 maxZ,
    i32 direction,
    MineshaftType type)
    : MineshaftPiece(componentType, minX, minY, minZ, maxX, maxY, maxZ, type)
    , m_direction(direction)
{
    m_hasRails = rng.nextInt(3) == 0;                   // 33% 概率有铁轨
    m_hasSpiders = !m_hasRails && rng.nextInt(23) == 0; // ~4% 概率有蜘蛛刷怪笼

    // 计算段数
    if (direction == 0 || direction == 1) {
        m_sectionCount = (maxZ - minZ) / 5;
    } else {
        m_sectionCount = (maxX - minX) / 5;
    }
}

void MineshaftCorridor::generate(IWorldWriter& world,
    math::IRandom& rng,
    i32 /*chunkX*/,
    i32 /*chunkZ*/,
    const StructureBoundingBox& chunkBounds,
    ChunkPrimer* /*chunk*/,
    IChunkGenerator* /*generator*/)
{
    // 生成地板和天花板
    if (m_direction == 0 || m_direction == 1) {
        // 南北方向
        for (i32 z = minZ(); z <= maxZ(); ++z) {
            _generateFloor(world, minX(), z, maxX(), z, rng, chunkBounds);
            _generateCeiling(world, minX(), z, maxX(), z, rng, chunkBounds);

            // 每5格放置支撑柱
            if ((z - minZ()) % 5 == 2) {
                i32 sectionIndex = (z - minZ()) / 5;
                _generatePillars(world, sectionIndex, rng, chunkBounds);
            }
        }
    } else {
        // 东西方向
        for (i32 x = minX(); x <= maxX(); ++x) {
            _generateFloor(world, x, minZ(), x, maxZ(), rng, chunkBounds);
            _generateCeiling(world, x, minZ(), x, maxZ(), rng, chunkBounds);

            // 每5格放置支撑柱
            if ((x - minX()) % 5 == 2) {
                i32 sectionIndex = (x - minX()) / 5;
                _generatePillars(world, sectionIndex, rng, chunkBounds);
            }
        }
    }

    // 生成铁轨
    if (m_hasRails) {
        _generateRails(world, rng, chunkBounds);
    }

    // 生成蜘蛛刷怪笼
    if (m_hasSpiders && !m_spawnerPlaced && rng.nextInt(3) == 0) {
        i32 sx = (minX() + maxX()) / 2;
        i32 sz = (minZ() + maxZ()) / 2;
        _generateSpawner(world, sx, minY() + 1, sz, chunkBounds);
        m_spawnerPlaced = true;
    }

    // 随机生成宝箱矿车（概率为 1%）
    if (rng.nextInt(100) == 0) {
        i32 cx = minX() + rng.nextInt(maxX() - minX());
        i32 cz = minZ() + rng.nextInt(maxZ() - minZ());
        _generateChestMinecart(world, cx, minY() + 1, cz, rng, chunkBounds);
    }
}

void MineshaftCorridor::_generateFloor(
    IWorldWriter& world, i32 x1, i32 z1, i32 x2, i32 z2, math::IRandom& rng, const StructureBoundingBox& chunkBounds)
{
    const BlockState* planksState = VanillaBlocks::getState(VanillaBlocks::OAK_PLANKS);
    const BlockState* cobblestoneState = VanillaBlocks::getState(VanillaBlocks::COBBLESTONE);

    for (i32 x = x1; x <= x2; ++x) {
        for (i32 z = z1; z <= z2; ++z) {
            if (chunkBounds.contains(x, minY(), z)) {
                // 恶地矿井使用深色橡木
                const BlockState* floorState = (m_mineshaftType == MineshaftType::Mesa)
                    ? VanillaBlocks::getState(VanillaBlocks::DARK_OAK_PLANKS)
                    : planksState;

                // 70% 概率放置地板，模拟损坏效果
                if (rng.nextInt(100) < 70) {
                    world.setBlockState(x, minY(), z, floorState);
                } else {
                    // 使用圆石作为损坏的地板
                    world.setBlockState(x, minY(), z, cobblestoneState);
                }
            }
        }
    }
}

void MineshaftCorridor::_generateCeiling(
    IWorldWriter& world, i32 x1, i32 z1, i32 x2, i32 z2, math::IRandom& rng, const StructureBoundingBox& chunkBounds)
{
    const BlockState* planksState = VanillaBlocks::getState(VanillaBlocks::OAK_PLANKS);

    for (i32 x = x1; x <= x2; ++x) {
        for (i32 z = z1; z <= z2; ++z) {
            if (chunkBounds.contains(x, maxY(), z)) {
                // 60% 概率放置天花板，模拟损坏效果
                if (rng.nextInt(100) < 60) {
                    const BlockState* ceilingState = (m_mineshaftType == MineshaftType::Mesa)
                        ? VanillaBlocks::getState(VanillaBlocks::DARK_OAK_PLANKS)
                        : planksState;
                    world.setBlockState(x, maxY(), z, ceilingState);
                }
            }
        }
    }
}

void MineshaftCorridor::_generatePillars(
    IWorldWriter& world, i32 /*sectionIndex*/, math::IRandom& rng, const StructureBoundingBox& chunkBounds)
{
    if (m_direction == 0 || m_direction == 1) {
        // 南北方向：支撑柱在东西两侧
        i32 z = minZ() + (m_sectionCount / 2) * 5 + 2;
        if (z > maxZ()) {
            z = (minZ() + maxZ()) / 2;
        }

        // 西侧支撑柱
        if (chunkBounds.contains(minX(), minY() + 1, z)) {
            _generateSupport(world, minX(), minY() + 1, z, 2, rng);
        }
        // 东侧支撑柱
        if (chunkBounds.contains(maxX(), minY() + 1, z)) {
            _generateSupport(world, maxX(), minY() + 1, z, 2, rng);
        }
    } else {
        // 东西方向：支撑柱在南北两侧
        i32 x = minX() + (m_sectionCount / 2) * 5 + 2;
        if (x > maxX()) {
            x = (minX() + maxX()) / 2;
        }

        // 北侧支撑柱
        if (chunkBounds.contains(x, minY() + 1, minZ())) {
            _generateSupport(world, x, minY() + 1, minZ(), 2, rng);
        }
        // 南侧支撑柱
        if (chunkBounds.contains(x, minY() + 1, maxZ())) {
            _generateSupport(world, x, minY() + 1, maxZ(), 2, rng);
        }
    }
}

void MineshaftCorridor::_generateRails(
    IWorldWriter& world, math::IRandom& /*rng*/, const StructureBoundingBox& chunkBounds)
{
    const BlockState* railState = VanillaBlocks::getState(VanillaBlocks::RAIL);
    if (!railState) {
        railState = VanillaBlocks::getState(VanillaBlocks::STONE_BRICKS);
    }

    if (m_direction == 0 || m_direction == 1) {
        // 南北方向
        i32 centerX = (minX() + maxX()) / 2;
        for (i32 z = minZ(); z <= maxZ(); ++z) {
            if (chunkBounds.contains(centerX, minY() + 1, z)) {
                world.setBlockState(centerX, minY() + 1, z, railState);
            }
        }
    } else {
        // 东西方向
        i32 centerZ = (minZ() + maxZ()) / 2;
        for (i32 x = minX(); x <= maxX(); ++x) {
            if (chunkBounds.contains(x, minY() + 1, centerZ)) {
                world.setBlockState(x, minY() + 1, centerZ, railState);
            }
        }
    }
}

void MineshaftCorridor::_generateSpawner(
    IWorldWriter& world, i32 x, i32 y, i32 z, const StructureBoundingBox& chunkBounds)
{
    if (!chunkBounds.contains(x, y, z)) {
        return;
    }

    const BlockState* centerState = VanillaBlocks::getState(VanillaBlocks::COBWEB);
    if (!centerState) {
        centerState = VanillaBlocks::getState(VanillaBlocks::MOSSY_COBBLESTONE);
    }
    if (centerState) {
        world.setBlockState(x, y, z, centerState);
    }

    const BlockState* webState = VanillaBlocks::getState(VanillaBlocks::COBWEB);
    if (webState) {
        for (i32 dx = -1; dx <= 1; ++dx) {
            for (i32 dz = -1; dz <= 1; ++dz) {
                if ((dx == 0 && dz == 0) || !chunkBounds.contains(x + dx, y, z + dz)) {
                    continue;
                }
                world.setBlockState(x + dx,
                    y,
                    z + dz,
                    webState,
                    world::BlockUpdateFlags::UPDATE_CLIENTS | world::BlockUpdateFlags::UPDATE_KNOWN_SHAPE);
            }
        }
    }
}

void MineshaftCorridor::_generateChestMinecart(
    IWorldWriter& world, i32 x, i32 y, i32 z, math::IRandom& /*rng*/, const StructureBoundingBox& chunkBounds)
{
    if (!chunkBounds.contains(x, y, z)) {
        return;
    }

    const BlockState* railState = VanillaBlocks::getState(VanillaBlocks::RAIL);
    const BlockState* lootMarker = VanillaBlocks::getState(VanillaBlocks::GOLD_BLOCK);

    if (railState) {
        world.setBlockState(
            x, y, z, railState, world::BlockUpdateFlags::UPDATE_CLIENTS | world::BlockUpdateFlags::UPDATE_KNOWN_SHAPE);
    }
    if (lootMarker) {
        world.setBlockState(x,
            y + 1,
            z,
            lootMarker,
            world::BlockUpdateFlags::UPDATE_CLIENTS | world::BlockUpdateFlags::UPDATE_KNOWN_SHAPE);
    }
}

void MineshaftCorridor::buildComponent(
    std::vector<std::unique_ptr<MineshaftPiece>>& pieces, math::IRandom& rng, i32 maxDepth)
{
    MC_UNUSED(maxDepth);

    // 走廊的展开分两段，顺序不能变：
    //
    // 1) 先抽一次 nextInt(4)，按走廊朝向决定从哪条边开一个「主口」。
    //    走廊的朝向恒为四个水平方向之一，所以整段必然执行。
    // 2) 再在 genDepth < 8 时沿一边逐点步进，每点抽一次 nextInt(5)，命中 0 或 1 才开口。
    //
    // 第 2 段的条件是 `direction != NORTH && direction != SOUTH` 时沿 X 扫描、
    // 否则沿 Z 扫描。走廊朝向不会是 null，因此这里直接按方向判断。
    //
    // 原版 addChildren 传的是 this.getGenDepth()，generateAndAddPiece 内部再 +1；
    // 本项目的 addMineshaftPiece 直接把这个值当新构件的深度，所以要显式 +1，
    // 否则深度不增长、depth > maxDepth 永不成立，整座矿井无界展开。
    const i32 depth = genDepth() + 1;
    if (depth > maxDepth) {
        return;
    }

    auto addChild = [&](i32 cx, i32 cy, i32 cz, i32 dir, i32 childDepth) {
        addMineshaftPiece(this, pieces, rng, cx, cy, cz, dir, childDepth, m_mineshaftType);
    };

    // ---- 第 1 段：主口 ----
    const i32 j = rng.nextInt(4);
    if (j <= 1) {
        const i32 y = minY() - 1 + rng.nextInt(3);
        switch (m_direction) {
            case 1: // 南
                addChild(minX(), y, maxZ() + 1, 1, depth);
                break;
            case 2: // 西
                addChild(minX() - 1, y, minZ(), 2, depth);
                break;
            case 3: // 东
                addChild(maxX() + 1, y, minZ(), 3, depth);
                break;
            case 0: // 北
            default:
                addChild(minX(), y, minZ() - 1, 0, depth);
                break;
        }
    } else if (j == 2) {
        const i32 y = minY() - 1 + rng.nextInt(3);
        switch (m_direction) {
            case 1: // 南
                addChild(minX() - 1, y, maxZ() - 3, 2, depth);
                break;
            case 2: // 西
                addChild(minX(), y, minZ() - 1, 0, depth);
                break;
            case 3: // 东
                addChild(maxX() - 3, y, minZ() - 1, 0, depth);
                break;
            case 0: // 北
            default:
                addChild(minX() - 1, y, minZ(), 2, depth);
                break;
        }
    } else {
        const i32 y = minY() - 1 + rng.nextInt(3);
        switch (m_direction) {
            case 1: // 南
                addChild(maxX() + 1, y, maxZ() - 3, 3, depth);
                break;
            case 2: // 西
                addChild(minX(), y, maxZ() + 1, 1, depth);
                break;
            case 3: // 东
                addChild(maxX() - 3, y, maxZ() + 1, 1, depth);
                break;
            case 0: // 北
            default:
                addChild(maxX() + 1, y, minZ(), 3, depth);
                break;
        }
    }

    // ---- 第 2 段：沿边逐点步进 ----
    // 原版判据是父构件的深度 i < 8，而 depth 已经是子构件的深度，故这里用 depth - 1。
    if (depth - 1 < 8) {
        if (m_direction != 0 && m_direction != 1) {
            // 朝东西：沿 X 扫描，南北两侧开口。
            for (i32 x = minX() + 3; x + 3 <= maxX(); x += 5) {
                const i32 pick = rng.nextInt(5);
                if (pick == 0) {
                    addChild(x, minY(), minZ() - 1, 0, depth);
                } else if (pick == 1) {
                    addChild(x, minY(), maxZ() + 1, 1, depth);
                }
            }
        } else {
            // 朝南北：沿 Z 扫描，东西两侧开口。
            for (i32 z = minZ() + 3; z + 3 <= maxZ(); z += 5) {
                const i32 pick = rng.nextInt(5);
                if (pick == 0) {
                    addChild(minX() - 1, minY(), z, 2, depth);
                } else if (pick == 1) {
                    addChild(maxX() + 1, minY(), z, 3, depth);
                }
            }
        }
    }
}

// ============================================================================
// MineshaftCross 实现
// ============================================================================

MineshaftCross::MineshaftCross(
    i32 componentType, i32 minX, i32 minY, i32 minZ, i32 maxX, i32 maxY, i32 maxZ, i32 direction, MineshaftType type)
    : MineshaftPiece(componentType, minX, minY, minZ, maxX, maxY, maxZ, type)
    , m_direction(direction)
{}

void MineshaftCross::generate(IWorldWriter& world,
    math::IRandom& rng,
    i32 /*chunkX*/,
    i32 /*chunkZ*/,
    const StructureBoundingBox& chunkBounds,
    ChunkPrimer* /*chunk*/,
    IChunkGenerator* /*generator*/)
{
    const BlockState* planksState = VanillaBlocks::getState(VanillaBlocks::OAK_PLANKS);

    // 生成地板
    for (i32 x = minX(); x <= maxX(); ++x) {
        for (i32 z = minZ(); z <= maxZ(); ++z) {
            if (chunkBounds.contains(x, minY(), z)) {
                const BlockState* floorState = (m_mineshaftType == MineshaftType::Mesa)
                    ? VanillaBlocks::getState(VanillaBlocks::DARK_OAK_PLANKS)
                    : planksState;
                if (rng.nextInt(10) < 7) { // 70% 概率
                    world.setBlockState(x, minY(), z, floorState);
                }
            }
        }
    }

    // 生成角落支撑柱
    i32 pillarPositions[][2] = {{0, 0}, {4, 0}, {0, 4}, {4, 4}};
    for (const auto& pos : pillarPositions) {
        i32 px = minX() + pos[0];
        i32 pz = minZ() + pos[1];
        if (chunkBounds.contains(px, minY() + 1, pz)) {
            _generateSupport(world, px, minY() + 1, pz, 2, rng);
        }
    }
}

void MineshaftCross::buildComponent(
    std::vector<std::unique_ptr<MineshaftPiece>>& pieces, math::IRandom& rng, i32 maxDepth)
{
    const i32 depth = genDepth() + 1;
    if (depth > maxDepth) {
        return;
    }

    // 交叉点按朝向固定开出三个方向，坐标由包围盒直接算出，不抽随机数。
    //
    // 原版 MineShaftCrossing.addChildren 的四个分支：
    //   北 -> (minX+1, minY, minZ-1) 北 | (minX-1, minY, minZ+1) 西 | (maxX+1, minY, minZ+1) 东
    //   南 -> (minX+1, minY, maxZ+1) 南 | (minX-1, minY, minZ+1) 西 | (maxX+1, minY, minZ+1) 东
    //   西 -> (minX+1, minY, minZ-1) 北 | (minX+1, minY, maxZ+1) 南 | (minX-1, minY, minZ+1) 西
    //   东 -> (minX+1, minY, minZ-1) 北 | (minX+1, minY, maxZ+1) 南 | (maxX+1, minY, minZ+1) 东
    //
    // 此前这里遍历四个方向、每个方向以 nextInt(3) 决定是否跳过，既不匹配原版的
    // 分支表，又多吃了最多四个随机数。
    auto addChild = [&](i32 nx, i32 ny, i32 nz, i32 dir) {
        addMineshaftPiece(this, pieces, rng, nx, ny, nz, dir, depth, m_mineshaftType);
    };

    switch (m_direction) {
        case 1: // 南
            addChild(minX() + 1, minY(), maxZ() + 1, 1);
            addChild(minX() - 1, minY(), minZ() + 1, 2);
            addChild(maxX() + 1, minY(), minZ() + 1, 3);
            break;
        case 2: // 西
            addChild(minX() + 1, minY(), minZ() - 1, 0);
            addChild(minX() + 1, minY(), maxZ() + 1, 1);
            addChild(minX() - 1, minY(), minZ() + 1, 2);
            break;
        case 3: // 东
            addChild(minX() + 1, minY(), minZ() - 1, 0);
            addChild(minX() + 1, minY(), maxZ() + 1, 1);
            addChild(maxX() + 1, minY(), minZ() + 1, 3);
            break;
        case 0: // 北
        default:
            addChild(minX() + 1, minY(), minZ() - 1, 0);
            addChild(minX() - 1, minY(), minZ() + 1, 2);
            addChild(maxX() + 1, minY(), minZ() + 1, 3);
            break;
    }

    // 双层交叉点（包围盒高度大于 3）还要在上一层再开三个方向，
    // 每个方向由一次 nextBoolean 决定是否生成。
    if (maxY() - minY() + 1 > 3) {
        const i32 upperY = minY() + 4;
        if (rng.nextBoolean()) {
            addChild(minX() + 1, upperY, minZ() - 1, 0);
        }
        if (rng.nextBoolean()) {
            addChild(minX() - 1, upperY, minZ() + 1, 2);
        }
        if (rng.nextBoolean()) {
            addChild(maxX() + 1, upperY, minZ() + 1, 3);
        }
    }
}

// ============================================================================
// MineshaftStairs 实现
// ============================================================================

MineshaftStairs::MineshaftStairs(
    i32 componentType, i32 minX, i32 minY, i32 minZ, i32 maxX, i32 maxY, i32 maxZ, i32 direction, MineshaftType type)
    : MineshaftPiece(componentType, minX, minY, minZ, maxX, maxY, maxZ, type)
    , m_direction(direction)
{}

void MineshaftStairs::generate(IWorldWriter& world,
    math::IRandom& rng,
    i32 /*chunkX*/,
    i32 /*chunkZ*/,
    const StructureBoundingBox& chunkBounds,
    ChunkPrimer* /*chunk*/,
    IChunkGenerator* /*generator*/)
{
    const BlockState* planksState = VanillaBlocks::getState(VanillaBlocks::OAK_PLANKS);

    i32 length = (m_direction == 0 || m_direction == 1) ? (maxZ() - minZ()) : (maxX() - minX());
    i32 stepCount = length / 2; // 每两格下降一格

    for (i32 i = 0; i < stepCount; ++i) {
        i32 stepY = minY() - i;
        i32 x, z;

        if (m_direction == 0 || m_direction == 1) {
            x = (minX() + maxX()) / 2;
            z = minZ() + i * 2;
        } else {
            x = minX() + i * 2;
            z = (minZ() + maxZ()) / 2;
        }

        // 生成楼梯地板
        for (i32 dx = -1; dx <= 1; ++dx) {
            for (i32 dz = -1; dz <= 1; ++dz) {
                i32 bx = x + dx;
                i32 bz = z + dz;
                if (chunkBounds.contains(bx, stepY, bz)) {
                    const BlockState* floorState = (m_mineshaftType == MineshaftType::Mesa)
                        ? VanillaBlocks::getState(VanillaBlocks::DARK_OAK_PLANKS)
                        : planksState;
                    if (rng.nextInt(10) < 7) {
                        world.setBlockState(bx, stepY, bz, floorState);
                    }
                }
            }
        }

        // 生成支撑柱
        if (i % 2 == 0) {
            if (m_direction == 0 || m_direction == 1) {
                if (chunkBounds.contains(minX(), stepY + 1, z)) {
                    _generateSupport(world, minX(), stepY + 1, z, 2, rng);
                }
                if (chunkBounds.contains(maxX(), stepY + 1, z)) {
                    _generateSupport(world, maxX(), stepY + 1, z, 2, rng);
                }
            } else {
                if (chunkBounds.contains(x, stepY + 1, minZ())) {
                    _generateSupport(world, x, stepY + 1, minZ(), 2, rng);
                }
                if (chunkBounds.contains(x, stepY + 1, maxZ())) {
                    _generateSupport(world, x, stepY + 1, maxZ(), 2, rng);
                }
            }
        }
    }
}

void MineshaftStairs::buildComponent(
    std::vector<std::unique_ptr<MineshaftPiece>>& pieces, math::IRandom& rng, i32 maxDepth)
{
    const i32 depth = genDepth() + 1;
    if (depth > maxDepth) {
        return;
    }

    i32 x, y, z;
    i32 length = (m_direction == 0 || m_direction == 1) ? (maxZ() - minZ()) : (maxX() - minX());
    y = minY() - length / 2;

    switch (m_direction) {
        case 0: // 北（向下）
            x = (minX() + maxX()) / 2;
            z = minZ() - 1;
            break;
        case 1: // 南（向下）
            x = (minX() + maxX()) / 2;
            z = maxZ() + 1;
            break;
        case 2: // 西（向下）
            x = minX() - 1;
            z = (minZ() + maxZ()) / 2;
            break;
        case 3: // 东（向下）
        default:
            x = maxX() + 1;
            z = (minZ() + maxZ()) / 2;
            break;
    }

    addMineshaftPiece(this, pieces, rng, x, y, z, m_direction, depth, m_mineshaftType);
}

// ============================================================================
// MineshaftStructure 实现
// ============================================================================

const std::string MineshaftStructure::m_name = "mineshaft";

const std::vector<BiomeId> MineshaftStructure::m_mesaBiomes = {
    Biomes::Badlands, Biomes::BadlandsPlateau, Biomes::ErodedBadlands, Biomes::WoodedBadlandsPlateau};

MineshaftStructure::MineshaftStructure(ResourceLocation id, MineshaftType type)
    : Structure(std::move(id))
{
    m_config.type = type;
    m_config.probability = (type == MineshaftType::Mesa) ? 0.004f : 0.004f;
}

const biome::BiomeTag* MineshaftStructure::defaultBiomeTag() const
{
    return &biome::BiomeTags::HAS_STRUCTURE_MINESHAFT();
}

bool MineshaftStructure::canGenerate(
    IWorld& /*world*/, IChunkGenerator& /*generator*/, math::IRandom& rng, i32 /*chunkX*/, i32 /*chunkZ*/)
{
    // 间距检查已由 StructurePlacement::isStructureChunk() 处理
    // 仅做概率检查
    return rng.nextFloat() < m_config.probability;
}

std::unique_ptr<StructureStart> MineshaftStructure::generate(
    IChunkGenerator& generator, math::IRandom& rng, i32 chunkX, i32 chunkZ) const
{
    // 原版 MineshaftStructure.findGenerationPoint 的第一句就是
    //     context.random().nextDouble();
    // 这个值被直接丢弃，作用是推进随机序列——矿井的房间跨度、走廊长度、
    // 分支选择全部建立在它之后的状态上。漏掉它会让整座矿井的随机序列错一位，
    // 实测房间高度变成 5 而原版是 10。
    (void)rng.nextDouble();

    auto start = std::make_unique<StructureStart>(chunkX, chunkZ);

    // 起点固定在区块内偏移 2 的位置，且这里不消耗任何随机数。
    //
    // 原版 MineshaftStructure.generatePiecesAndAdjust 直接用
    // chunkPos.getBlockX(2) / getBlockZ(2) 建起始房间，房间的 Y 由房间自己定成 50。
    // 此前这里先 rng.nextInt(CHUNK_WIDTH) 抽两次起点、再用
    // getHeight + rng.nextInt 抽一次高度，白吃了三个随机数，
    // 使后续所有构件的方向、类型、尺寸整体错位。
    const i32 baseX = (chunkX << world::CHUNK_SHIFT) + 2;
    const i32 baseZ = (chunkZ << world::CHUNK_SHIFT) + 2;
    const i32 baseY = 50;

    // 创建起始房间
    auto room = std::make_unique<MineshaftRoom>(MineshaftPieceTypes::ROOM, rng, baseX, baseY, baseZ, m_config.type);

    // 构建矿井片段
    std::vector<std::unique_ptr<MineshaftPiece>> pieces;
    pieces.push_back(std::move(room));

    // 递归生成矿井结构
    pieces[0]->buildComponent(pieces, rng, 8); // 最大深度8

    // 将片段添加到起点
    for (auto& piece : pieces) {
        start->addPiece(std::move(piece));
    }

    start->recalculateStructureSize();

    // 把整座矿井下移到海平面以下。
    //
    // 原版 MineshaftStructure.generatePiecesAndAdjust 末尾对非恶地类型调用
    //   moveBelowSeaLevel(seaLevel, minY, random, 10)
    // 其算法是：
    //   int i = seaLevel - padding;                       // 可用的顶面高度上限
    //   BoundingBox bb = 全部构件的包围盒;
    //   int j = bb.getYSpan() + minY + 1;                 // 贴着世界底部的顶面
    //   if (j < i) j += random.nextInt(i - j);            // 剩余空间内随机下移
    //   offsetPiecesVertically(j - bb.maxY());
    // 缺了这一步，矿井会停在房间构造时写死的 Y=50 附近，
    // 而原版的实际房间顶面在 -10 左右（整座结构被下移了 65 格）。
    const i32 seaLevel = ::mc::world::SEA_LEVEL;
    const StructureBoundingBox overall = start->getBoundingBox();

    if (m_config.type == MineshaftType::Mesa) {
        // 恶地矿井贴着地表：以整体包围盒中心的地表高度为准，
        // 地表不高于海平面时对齐到海平面，否则在海平面与地表之间随机取一个高度。
        // 原版 generatePiecesAndAdjust 的 MESA 分支走的就是这条路径，
        // 不走 moveBelowSeaLevel。
        const i32 centerX = (overall.minX() + overall.maxX()) / 2;
        const i32 centerZ = (overall.minZ() + overall.maxZ()) / 2;
        const i32 centerY = (overall.minY() + overall.maxY()) / 2;
        const i32 surfaceY = generator.getHeight(centerX, centerZ, HeightmapType::WorldSurfaceWG);
        i32 targetY = seaLevel;
        if (surfaceY > seaLevel) {
            // Mth.randomBetweenInclusive：闭区间随机。
            targetY = seaLevel + rng.nextInt(surfaceY - seaLevel + 1);
        }
        start->offset(0, targetY - centerY, 0);
        return start;
    }

    // 其余类型整座下移到海平面以下。
    //
    // 原版 StructurePiecesBuilder.moveBelowSeaLevel(seaLevel, minY, random, 10)：
    //   int i = seaLevel - padding;                  // 顶面上限
    //   int j = bb.getYSpan() + minY + 1;            // 贴着世界底部的顶面
    //   if (j < i) j += random.nextInt(i - j);       // 剩余空间内随机上移
    //   offsetPiecesVertically(j - bb.maxY());
    const i32 padding = 10;
    const i32 topLimit = seaLevel - padding;
    i32 topY = (overall.maxY() - overall.minY() + 1) + world::MIN_BUILD_HEIGHT + 1;
    if (topY < topLimit) {
        topY += rng.nextInt(topLimit - topY);
    }
    start->offset(0, topY - overall.maxY(), 0);

    return start;
}

} // namespace mc::world::gen::structure
