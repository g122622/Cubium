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
#include "../StructureBoundingBox.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorldWriter.hpp"
#include "common/world/WorldConstants.hpp"
#include "common/world/biome/BiomeIds.hpp"
#include "common/world/biome/BiomeTags.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include <cmath>

namespace mc::world::gen::structure {

// ============================================================================
// 辅助函数实现
// ============================================================================

std::unique_ptr<MineshaftPiece> createMineshaftPiece(std::vector<std::unique_ptr<MineshaftPiece>>& pieces,
    math::Random& rng,
    i32 x,
    i32 y,
    i32 z,
    i32 direction,
    i32 depth,
    MineshaftType type)
{
    i32 chance = rng.nextInt(100);

    if (chance >= 80) {
        // 20% 概率生成交叉点
        i32 minX = x, maxX = x + 4;
        i32 minZ = z, maxZ = z + 4;
        i32 minY = y, maxY = y + 2;

        // 根据方向调整边界
        switch (direction) {
            case 0: // 北
                maxZ = z;
                minZ = z - 4;
                break;
            case 1: // 南
                minZ = z;
                maxZ = z + 4;
                break;
            case 2: // 西
                maxX = x;
                minX = x - 4;
                break;
            case 3: // 东
                minX = x;
                maxX = x + 4;
                break;
        }

        return std::make_unique<MineshaftCross>(
            MineshaftPieceTypes::CROSS, minX, minY, minZ, maxX, maxY, maxZ, direction, type);
    } else if (chance >= 70) {
        // 10% 概率生成楼梯
        i32 length = 8;
        i32 minX = x, maxX = x + 2;
        i32 minZ = z, maxZ = z + 2;
        i32 minY = y - length / 2, maxY = y + 2;

        switch (direction) {
            case 0: // 北
                minZ = z - length;
                maxZ = z;
                break;
            case 1: // 南
                minZ = z;
                maxZ = z + length;
                break;
            case 2: // 西
                minX = x - length;
                maxX = x;
                break;
            case 3: // 东
                minX = x;
                maxX = x + length;
                break;
        }

        return std::make_unique<MineshaftStairs>(
            MineshaftPieceTypes::STAIRS, minX, minY, minZ, maxX, maxY, maxZ, direction, type);
    } else {
        // 70% 概率生成走廊
        i32 length = (rng.nextInt(3) + 2) * 5; // 10-20格
        i32 minX = x, maxX = x + 2;
        i32 minZ = z, maxZ = z + 2;
        i32 minY = y, maxY = y + 2;

        switch (direction) {
            case 0: // 北
                minZ = z - length;
                maxZ = z;
                break;
            case 1: // 南
                minZ = z;
                maxZ = z + length;
                break;
            case 2: // 西
                minX = x - length;
                maxX = x;
                break;
            case 3: // 东
                minX = x;
                maxX = x + length;
                break;
        }

        return std::make_unique<MineshaftCorridor>(
            MineshaftPieceTypes::CORRIDOR, rng, minX, minY, minZ, maxX, maxY, maxZ, direction, type);
    }
}

std::unique_ptr<MineshaftPiece> addMineshaftPiece(MineshaftPiece* parent,
    std::vector<std::unique_ptr<MineshaftPiece>>& pieces,
    math::Random& rng,
    i32 x,
    i32 y,
    i32 z,
    i32 direction,
    i32 depth,
    MineshaftType type)
{
    // 检查深度限制
    if (depth > 8) return nullptr;

    // 检查距离限制（防止无限扩展）
    if (parent) {
        i32 dx = std::abs(x - parent->minX());
        i32 dz = std::abs(z - parent->minZ());
        if (dx > 80 || dz > 80) return nullptr;
    }

    // 创建新片段
    auto piece = createMineshaftPiece(pieces, rng, x, y, z, direction, depth, type);
    if (piece) {
        piece->buildComponent(pieces, rng, 8);
    }

    return piece;
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

void MineshaftPiece::_generateSupport(IWorldWriter& world, i32 x, i32 y, i32 z, i32 height, math::Random& rng)
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

MineshaftRoom::MineshaftRoom(i32 componentType, math::Random& rng, i32 x, i32 y, i32 z, MineshaftType type)
    : MineshaftPiece(componentType, x, y, z, x + 7, y + 3, z + 7, type)
{
    // 随机选择出口方向
    if (rng.nextBoolean()) {
        m_exits.push_back(0); // 北
    }
    if (rng.nextBoolean()) {
        m_exits.push_back(1); // 南
    }
    if (rng.nextBoolean()) {
        m_exits.push_back(2); // 西
    }
    if (rng.nextBoolean()) {
        m_exits.push_back(3); // 东
    }
    // 确保至少有一个出口
    if (m_exits.empty()) {
        m_exits.push_back(rng.nextInt(4));
    }
}

void MineshaftRoom::generate(IWorldWriter& world,
    math::Random& rng,
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
    std::vector<std::unique_ptr<MineshaftPiece>>& pieces, math::Random& rng, i32 maxDepth)
{
    i32 depth = type() + 1;

    for (i32 exitDir : m_exits) {
        i32 x, y, z;
        switch (exitDir) {
            case 0: // 北
                x = minX() + rng.nextInt(4) + 1;
                y = minY();
                z = minZ() - 1;
                break;
            case 1: // 南
                x = minX() + rng.nextInt(4) + 1;
                y = minY();
                z = maxZ() + 1;
                break;
            case 2: // 西
                x = minX() - 1;
                y = minY();
                z = minZ() + rng.nextInt(4) + 1;
                break;
            case 3: // 东
            default:
                x = maxX() + 1;
                y = minY();
                z = minZ() + rng.nextInt(4) + 1;
                break;
        }

        auto piece = addMineshaftPiece(this, pieces, rng, x, y, z, exitDir, depth, m_mineshaftType);
        if (piece) {
            pieces.push_back(std::move(piece));
        }
    }
}

// ============================================================================
// MineshaftCorridor 实现
// ============================================================================

MineshaftCorridor::MineshaftCorridor(i32 componentType,
    math::Random& rng,
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
    math::Random& rng,
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
    IWorldWriter& world, i32 x1, i32 z1, i32 x2, i32 z2, math::Random& rng, const StructureBoundingBox& chunkBounds)
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
    IWorldWriter& world, i32 x1, i32 z1, i32 x2, i32 z2, math::Random& rng, const StructureBoundingBox& chunkBounds)
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
    IWorldWriter& world, i32 /*sectionIndex*/, math::Random& rng, const StructureBoundingBox& chunkBounds)
{
    if (m_direction == 0 || m_direction == 1) {
        // 南北方向：支撑柱在东西两侧
        i32 z = minZ() + (m_sectionCount / 2) * 5 + 2;
        if (z > maxZ()) z = (minZ() + maxZ()) / 2;

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
        if (x > maxX()) x = (minX() + maxX()) / 2;

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
    IWorldWriter& world, math::Random& /*rng*/, const StructureBoundingBox& chunkBounds)
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
    if (!chunkBounds.contains(x, y, z)) return;

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
                world.setBlockState(x + dx, y, z + dz, webState, 18);
            }
        }
    }
}

void MineshaftCorridor::_generateChestMinecart(
    IWorldWriter& world, i32 x, i32 y, i32 z, math::Random& /*rng*/, const StructureBoundingBox& chunkBounds)
{
    if (!chunkBounds.contains(x, y, z)) return;

    const BlockState* railState = VanillaBlocks::getState(VanillaBlocks::RAIL);
    const BlockState* lootMarker = VanillaBlocks::getState(VanillaBlocks::GOLD_BLOCK);

    if (railState) {
        world.setBlockState(x, y, z, railState, 18);
    }
    if (lootMarker) {
        world.setBlockState(x, y + 1, z, lootMarker, 18);
    }
}

void MineshaftCorridor::buildComponent(
    std::vector<std::unique_ptr<MineshaftPiece>>& pieces, math::Random& rng, i32 maxDepth)
{
    i32 depth = type() + 1;
    if (depth > maxDepth) return;

    i32 x, y, z;

    // 根据方向确定出口位置
    switch (m_direction) {
        case 0: // 北
            x = minX() + rng.nextInt(2);
            y = minY() - 1 + rng.nextInt(3);
            z = minZ() - 1;
            break;
        case 1: // 南
            x = minX() + rng.nextInt(2);
            y = minY() - 1 + rng.nextInt(3);
            z = maxZ() + 1;
            break;
        case 2: // 西
            x = minX() - 1;
            y = minY() - 1 + rng.nextInt(3);
            z = minZ() + rng.nextInt(2);
            break;
        case 3: // 东
        default:
            x = maxX() + 1;
            y = minY() - 1 + rng.nextInt(3);
            z = minZ() + rng.nextInt(2);
            break;
    }

    auto piece = addMineshaftPiece(this, pieces, rng, x, y, z, m_direction, depth, m_mineshaftType);
    if (piece) {
        pieces.push_back(std::move(piece));
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
    math::Random& rng,
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
    std::vector<std::unique_ptr<MineshaftPiece>>& pieces, math::Random& rng, i32 maxDepth)
{
    i32 depth = type() + 1;
    if (depth > maxDepth) return;

    // 交叉点可以向三个方向延伸（除了来的方向）
    for (i32 dir = 0; dir < 4; ++dir) {
        // 跳过来的方向
        if ((m_direction == 0 && dir == 1) || (m_direction == 1 && dir == 0) || (m_direction == 2 && dir == 3) ||
            (m_direction == 3 && dir == 2)) {
            continue;
        }

        if (rng.nextInt(3) != 0) continue; // 67% 概率跳过

        i32 x, y, z;
        switch (dir) {
            case 0: // 北
                x = minX() + 2;
                y = minY();
                z = minZ() - 1;
                break;
            case 1: // 南
                x = minX() + 2;
                y = minY();
                z = maxZ() + 1;
                break;
            case 2: // 西
                x = minX() - 1;
                y = minY();
                z = minZ() + 2;
                break;
            case 3: // 东
            default:
                x = maxX() + 1;
                y = minY();
                z = minZ() + 2;
                break;
        }

        auto piece = addMineshaftPiece(this, pieces, rng, x, y, z, dir, depth, m_mineshaftType);
        if (piece) {
            pieces.push_back(std::move(piece));
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
    math::Random& rng,
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
    std::vector<std::unique_ptr<MineshaftPiece>>& pieces, math::Random& rng, i32 maxDepth)
{
    i32 depth = type() + 1;
    if (depth > maxDepth) return;

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

    auto piece = addMineshaftPiece(this, pieces, rng, x, y, z, m_direction, depth, m_mineshaftType);
    if (piece) {
        pieces.push_back(std::move(piece));
    }
}

// ============================================================================
// MineshaftStructure 实现
// ============================================================================

const std::string MineshaftStructure::m_name = "mineshaft";

const std::vector<BiomeId> MineshaftStructure::m_mesaBiomes = {
    Biomes::Badlands, Biomes::BadlandsPlateau, Biomes::ErodedBadlands, Biomes::WoodedBadlandsPlateau};

MineshaftStructure::MineshaftStructure(MineshaftType type)
    : Structure(ResourceLocation("minecraft", "mineshaft"))
{
    m_config.type = type;
    m_config.probability = (type == MineshaftType::Mesa) ? 0.004f : 0.004f;
}

const biome::BiomeTag* MineshaftStructure::defaultBiomeTag() const
{
    return &biome::BiomeTags::HAS_STRUCTURE_MINESHAFT();
}

bool MineshaftStructure::canGenerate(
    IWorld& /*world*/, IChunkGenerator& /*generator*/, math::Random& rng, i32 /*chunkX*/, i32 /*chunkZ*/)
{
    // 间距检查已由 StructurePlacement::isStructureChunk() 处理
    // 仅做概率检查
    return rng.nextFloat() < m_config.probability;
}

std::unique_ptr<StructureStart> MineshaftStructure::generate(
    IChunkGenerator& generator, math::Random& rng, i32 chunkX, i32 chunkZ) const
{
    auto start = std::make_unique<StructureStart>(chunkX, chunkZ);

    // 确定矿井起点位置
    i32 baseX = (chunkX << world::CHUNK_SHIFT) + rng.nextInt(world::CHUNK_WIDTH);
    i32 baseZ = (chunkZ << world::CHUNK_SHIFT) + rng.nextInt(world::CHUNK_WIDTH);

    // 获取高度（在地下）
    i32 surfaceY = generator.getHeight(baseX, baseZ, HeightmapType::WorldSurfaceWG);
    i32 minY = world::MIN_BUILD_HEIGHT + 10; // 最低高度（底部往上10格）
    i32 maxY = surfaceY - 20;                // 最高高度（地表下20格）
    if (maxY < minY + 10) maxY = minY + 10;

    i32 baseY = minY + rng.nextInt(maxY - minY);

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

    return start;
}

} // namespace mc::world::gen::structure
