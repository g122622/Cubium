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

#include "Structure.hpp"
#include "common/util/math/MathUtils.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/IWorldWriter.hpp"
#include "common/world/WorldConstants.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/chunk/data/ChunkPrimer.hpp"
#include "common/world/gen/chunk/IChunkGenerator.hpp"
#include <algorithm>

namespace mc::world::gen::structure {

// ========== StructurePiece::BlockSelector ==========

// 默认实现在子类中提供

// ========== StructurePiece ==========

StructurePiece::StructurePiece(i32 type, i32 minX, i32 minY, i32 minZ, i32 maxX, i32 maxY, i32 maxZ)
    : m_type(type)
    , m_minX(minX)
    , m_minY(minY)
    , m_minZ(minZ)
    , m_maxX(maxX)
    , m_maxY(maxY)
    , m_maxZ(maxZ)
{}

StructureBoundingBox StructurePiece::getBoundingBox() const
{
    return StructureBoundingBox(m_minX, m_minY, m_minZ, m_maxX, m_maxY, m_maxZ);
}

StructureBoundingBox StructurePiece::boundingBox() const
{
    return StructureBoundingBox(m_minX, m_minY, m_minZ, m_maxX, m_maxY, m_maxZ);
}

bool StructurePiece::intersectsChunk(i32 chunkX, i32 chunkZ) const
{
    i32 chunkMinX = chunkX * world::CHUNK_WIDTH;
    i32 chunkMinZ = chunkZ * world::CHUNK_WIDTH;
    i32 chunkMaxX = chunkMinX + world::CHUNK_WIDTH - 1;
    i32 chunkMaxZ = chunkMinZ + world::CHUNK_WIDTH - 1;

    return m_maxX >= chunkMinX && m_minX <= chunkMaxX && m_maxZ >= chunkMinZ && m_minZ <= chunkMaxZ;
}

bool StructurePiece::intersects(const StructureBoundingBox& box) const
{
    return m_maxX >= box.minX() && m_minX <= box.maxX() && m_maxY >= box.minY() && m_minY <= box.maxY() &&
        m_maxZ >= box.minZ() && m_minZ <= box.maxZ();
}

void StructurePiece::offset(i32 dx, i32 dy, i32 dz)
{
    m_minX += dx;
    m_minY += dy;
    m_minZ += dz;
    m_maxX += dx;
    m_maxY += dy;
    m_maxZ += dz;
}

void StructurePiece::setCoordBaseMode(Direction dir)
{
    m_coordBaseMode = dir;
    if (dir == Direction::None) {
        m_rotation = Rotation::None;
        m_mirror = Mirror::None;
    } else {
        switch (dir) {
            case Direction::South:
                m_mirror = Mirror::LeftRight;
                m_rotation = Rotation::None;
                break;
            case Direction::West:
                m_mirror = Mirror::LeftRight;
                m_rotation = Rotation::Clockwise90;
                break;
            case Direction::East:
                m_mirror = Mirror::None;
                m_rotation = Rotation::Clockwise90;
                break;
            default: // North
                m_mirror = Mirror::None;
                m_rotation = Rotation::None;
                break;
        }
    }
}

i32 StructurePiece::getXWithOffset(i32 x, i32 z) const
{
    if (m_coordBaseMode == Direction::None) {
        return x;
    }
    switch (m_coordBaseMode) {
        case Direction::North:
        case Direction::South:
            return m_minX + x;
        case Direction::West:
            return m_maxX - z;
        case Direction::East:
            return m_minX + z;
        default:
            return x;
    }
}

i32 StructurePiece::getYWithOffset(i32 y) const
{
    return m_coordBaseMode == Direction::None ? y : y + m_minY;
}

i32 StructurePiece::getZWithOffset(i32 x, i32 z) const
{
    if (m_coordBaseMode == Direction::None) {
        return z;
    }
    switch (m_coordBaseMode) {
        case Direction::North:
            return m_maxZ - z;
        case Direction::South:
            return m_minZ + z;
        case Direction::West:
        case Direction::East:
            return m_minZ + x;
        default:
            return z;
    }
}

void StructurePiece::setBlockState(
    IWorldWriter& world, const BlockState* state, i32 x, i32 y, i32 z, const StructureBoundingBox& bounds)
{
    i32 worldX = getXWithOffset(x, z);
    i32 worldY = getYWithOffset(y);
    i32 worldZ = getZWithOffset(x, z);

    if (!bounds.contains(worldX, worldY, worldZ)) {
        return;
    }

    // 应用镜像和旋转变换到方块状态
    if (state != nullptr) {
        const BlockState* transformedState = state;

        // 先应用镜像
        if (m_mirror != Mirror::None) {
            transformedState = &transformedState->getBlock().mirror(*transformedState, m_mirror);
        }

        // 再应用旋转
        if (m_rotation != Rotation::None) {
            transformedState = &transformedState->getBlock().rotate(*transformedState, m_rotation);
        }

        world.setBlockState(worldX, worldY, worldZ, transformedState, 2);
    } else {
        world.setBlockState(worldX, worldY, worldZ, state, 2);
    }
}

const BlockState* StructurePiece::getBlockStateFromPos(
    IWorld& world, i32 x, i32 y, i32 z, const StructureBoundingBox& bounds) const
{
    i32 worldX = getXWithOffset(x, z);
    i32 worldY = getYWithOffset(y);
    i32 worldZ = getZWithOffset(x, z);

    if (!bounds.contains(worldX, worldY, worldZ)) {
        return nullptr;
    }

    return world.getBlockState(worldX, worldY, worldZ);
}

void StructurePiece::fillWithAir(
    IWorldWriter& world, const StructureBoundingBox& bounds, i32 minX, i32 minY, i32 minZ, i32 maxX, i32 maxY, i32 maxZ)
{
    for (i32 y = minY; y <= maxY; ++y) {
        for (i32 x = minX; x <= maxX; ++x) {
            for (i32 z = minZ; z <= maxZ; ++z) {
                setBlockState(world, nullptr, x, y, z, bounds);
            }
        }
    }
}

void StructurePiece::fillWithBlocks(IWorldWriter& world,
    const StructureBoundingBox& bounds,
    i32 minX,
    i32 minY,
    i32 minZ,
    i32 maxX,
    i32 maxY,
    i32 maxZ,
    const BlockState* boundaryBlock,
    const BlockState* insideBlock,
    bool excludeCorners)
{
    for (i32 y = minY; y <= maxY; ++y) {
        for (i32 x = minX; x <= maxX; ++x) {
            for (i32 z = minZ; z <= maxZ; ++z) {
                bool isBoundary = (y == minY || y == maxY || x == minX || x == maxX || z == minZ || z == maxZ);
                // 如果排除角落，角落不算边界
                if (excludeCorners && isBoundary) {
                    i32 corners = 0;
                    if (x == minX || x == maxX) corners++;
                    if (y == minY || y == maxY) corners++;
                    if (z == minZ || z == maxZ) corners++;
                    if (corners >= 2) {
                        isBoundary = false;
                    }
                }
                const BlockState* state = isBoundary ? boundaryBlock : insideBlock;
                setBlockState(world, state, x, y, z, bounds);
            }
        }
    }
}

void StructurePiece::fillWithRandomizedBlocks(IWorldWriter& world,
    const StructureBoundingBox& bounds,
    i32 minX,
    i32 minY,
    i32 minZ,
    i32 maxX,
    i32 maxY,
    i32 maxZ,
    bool alwaysReplace,
    math::Random& rng,
    BlockSelector& selector)
{
    // 当 alwaysReplace=true 时，只替换非空气方块
    // 当 alwaysReplace=false 时，无条件填充
    IWorld* worldReader = alwaysReplace ? dynamic_cast<IWorld*>(&world) : nullptr;

    for (i32 y = minY; y <= maxY; ++y) {
        for (i32 x = minX; x <= maxX; ++x) {
            for (i32 z = minZ; z <= maxZ; ++z) {
                if (worldReader) {
                    const BlockState* currentState = getBlockStateFromPos(*worldReader, x, y, z, bounds);
                    if (currentState && currentState->isAir()) {
                        // 跳过空气方块
                        continue;
                    }
                }

                bool isWall = (y == minY || y == maxY || x == minX || x == maxX || z == minZ || z == maxZ);
                selector.selectBlocks(rng, x, y, z, isWall);
                setBlockState(world, selector.getBlockState(), x, y, z, bounds);
            }
        }
    }
}

void StructurePiece::randomlyPlaceBlock(IWorldWriter& world,
    const StructureBoundingBox& bounds,
    math::Random& rng,
    f32 chance,
    i32 x,
    i32 y,
    i32 z,
    const BlockState* state)
{
    if (rng.nextFloat() < chance) {
        setBlockState(world, state, x, y, z, bounds);
    }
}

void StructurePiece::randomlyRareFillWithBlocks(IWorldWriter& world,
    const StructureBoundingBox& bounds,
    i32 minX,
    i32 minY,
    i32 minZ,
    i32 maxX,
    i32 maxY,
    i32 maxZ,
    const BlockState* state)
{
    f32 dx = static_cast<f32>(maxX - minX + 1);
    f32 dy = static_cast<f32>(maxY - minY + 1);
    f32 dz = static_cast<f32>(maxZ - minZ + 1);
    f32 cx = static_cast<f32>(minX) + dx * 0.5f;
    f32 cz = static_cast<f32>(minZ) + dz * 0.5f;
    f32 halfDx = dx * 0.5f;
    f32 halfDz = dz * 0.5f;

    for (i32 y = minY; y <= maxY; ++y) {
        f32 fy = static_cast<f32>(y - minY) / dy;
        for (i32 x = minX; x <= maxX; ++x) {
            f32 fx = (static_cast<f32>(x) - cx) / halfDx;
            for (i32 z = minZ; z <= maxZ; ++z) {
                f32 fz = (static_cast<f32>(z) - cz) / halfDz;

                f32 distSq = fx * fx + fy * fy + fz * fz;
                if (distSq <= 1.05f) {
                    setBlockState(world, state, x, y, z, bounds);
                }
            }
        }
    }
}

void StructurePiece::replaceAirAndLiquidDownwards(
    IWorld& world, const BlockState* state, i32 x, i32 y, i32 z, const StructureBoundingBox& bounds)
{
    i32 worldX = getXWithOffset(x, z);
    i32 worldY = getYWithOffset(y);
    i32 worldZ = getZWithOffset(x, z);

    if (!bounds.contains(worldX, worldY, worldZ)) {
        return;
    }

    while (worldY > world::MIN_BUILD_HEIGHT) {
        const BlockState* current = world.getBlockState(worldX, worldY, worldZ);
        if (current == nullptr || current->isAir() || current->getMaterial().isLiquid()) {
            world.setBlockState(worldX, worldY, worldZ, state, 2);
            --worldY;
        } else {
            break;
        }
    }
}

void StructurePiece::buildComponent(
    StructurePiece* /*component*/, std::vector<std::unique_ptr<StructurePiece>>& /*pieces*/, math::Random& /*rng*/)
{
    // 默认实现为空，子类可以覆盖
}

StructurePiece* StructurePiece::findIntersecting(
    std::vector<std::unique_ptr<StructurePiece>>& pieces, const StructureBoundingBox& bounds)
{
    for (auto& piece : pieces) {
        if (piece && piece->intersects(bounds)) {
            return piece.get();
        }
    }
    return nullptr;
}

// ========== Structure ==========

bool Structure::isValidBiome(BiomeId biomeId) const
{
    // 优先使用 BiomeTag 进行 O(1) 查找
    const biome::BiomeTag* tag = biomeTag();
    if (tag) {
        return tag->contains(biomeId);
    }
    // 标签未加载时，回退到线性搜索（兼容旧代码）
    const auto& biomes = validBiomes();
    return std::find(biomes.begin(), biomes.end(), biomeId) != biomes.end();
}

bool Structure::canGenerate(
    IWorld& /*world*/, IChunkGenerator& /*generator*/, math::Random& /*rng*/, i32 /*chunkX*/, i32 /*chunkZ*/)
{
    return true;
}

std::unique_ptr<StructureStart> Structure::generate(
    IChunkGenerator& /*generator*/, math::Random& /*rng*/, i32 chunkX, i32 chunkZ) const
{
    return std::make_unique<StructureStart>(chunkX, chunkZ);
}

void Structure::placeInChunk(
    IWorldWriter& world, ChunkPrimer& chunk, StructureStart& start, i32 chunkX, i32 chunkZ) const
{
    StructureBoundingBox chunkBounds = StructureBoundingBox::fromChunk(chunkX, chunkZ);

    math::Random rng = createRandom(
        static_cast<i64>(chunkX) * 341873128712LL ^ static_cast<i64>(chunkZ) * 132897987541LL, chunkX, chunkZ, 0);

    for (const auto& piece : start.pieces()) {
        if (piece->intersectsChunk(chunkX, chunkZ)) {
            piece->generate(world, rng, chunkX, chunkZ, chunkBounds);
        }
    }
}

void Structure::afterPlace(IWorldWriter& /*world*/, StructureStart& /*start*/, i32 /*chunkX*/, i32 /*chunkZ*/) const
{
    // 默认实现为空，子类可以覆盖以在放置后执行额外操作
}

math::Random Structure::createRandom(i64 seed, i32 chunkX, i32 chunkZ, i32 salt)
{
    u64 combinedSeed = static_cast<u64>(chunkX) * 341873128712ULL + static_cast<u64>(chunkZ) * 132897987541ULL +
        static_cast<u64>(seed) + static_cast<u64>(salt);
    return math::Random(static_cast<i64>(combinedSeed));
}

ResourceLocation Structure::_typeToId(StructureType type)
{
    switch (type) {
        case StructureType::Temple:
            return ResourceLocation("minecraft", "temple");
        case StructureType::Monument:
            return ResourceLocation("minecraft", "ocean_monument");
        case StructureType::Stronghold:
            return ResourceLocation("minecraft", "stronghold");
        case StructureType::Village:
            return ResourceLocation("minecraft", "village");
        case StructureType::Mineshaft:
            return ResourceLocation("minecraft", "mineshaft");
        case StructureType::RuinedPortal:
            return ResourceLocation("minecraft", "ruined_portal");
        case StructureType::BuriedTreasure:
            return ResourceLocation("minecraft", "buried_treasure");
        case StructureType::Shipwreck:
            return ResourceLocation("minecraft", "shipwreck");
        case StructureType::OceanRuin:
            return ResourceLocation("minecraft", "ocean_ruin");
        case StructureType::WoodlandMansion:
            return ResourceLocation("minecraft", "woodland_mansion");
        case StructureType::Bastion:
            return ResourceLocation("minecraft", "bastion_remnant");
        case StructureType::Fortress:
            return ResourceLocation("minecraft", "fortress");
        case StructureType::EndCity:
            return ResourceLocation("minecraft", "end_city");
        case StructureType::PillagerOutpost:
            return ResourceLocation("minecraft", "pillager_outpost");
        case StructureType::TrialChambers:
            return ResourceLocation("minecraft", "trial_chambers");
        default:
            return ResourceLocation("minecraft", "unknown");
    }
}

bool Structure::findStructureStart(i64 seed,
    i32 chunkX,
    i32 chunkZ,
    const StructureSeparationSettings& settings,
    i32& outStartX,
    i32& outStartZ,
    bool useUniformSpacing)
{
    i32 spacing = settings.spacing;
    i32 separation = settings.separation;

    if (spacing <= 0) {
        return false;
    }

    i32 gridX = math::floorDiv(chunkX, spacing);
    i32 gridZ = math::floorDiv(chunkZ, spacing);

    u64 combinedSeed = static_cast<u64>(gridX) * 341873128712ULL + static_cast<u64>(gridZ) * 132897987541ULL +
        static_cast<u64>(seed) + static_cast<u64>(settings.salt);
    math::Random rng(static_cast<i64>(combinedSeed));

    i32 offsetRange = spacing - separation;

    // 均匀分布 vs 三角分布（两次随机取平均，产生更集中的分布）
    i32 offsetX, offsetZ;
    if (useUniformSpacing) {
        offsetX = rng.nextInt(offsetRange);
        offsetZ = rng.nextInt(offsetRange);
    } else {
        offsetX = (rng.nextInt(offsetRange) + rng.nextInt(offsetRange)) / 2;
        offsetZ = (rng.nextInt(offsetRange) + rng.nextInt(offsetRange)) / 2;
    }

    outStartX = gridX * spacing + offsetX;
    outStartZ = gridZ * spacing + offsetZ;

    return outStartX == chunkX && outStartZ == chunkZ;
}

// ========== StructureStart ==========

StructureStart::StructureStart(i32 chunkX, i32 chunkZ)
    : m_chunkX(chunkX)
    , m_chunkZ(chunkZ)
{}

void StructureStart::addPiece(std::unique_ptr<StructurePiece> piece)
{
    if (piece) {
        m_boundingBox.expandToInclude(piece->minX(), piece->minY(), piece->minZ());
        m_boundingBox.expandToInclude(piece->maxX(), piece->maxY(), piece->maxZ());
        m_pieces.push_back(std::move(piece));
    }
}

void StructureStart::recalculateStructureSize()
{
    m_boundingBox = StructureBoundingBox(); // 重置为无效状态
    for (const auto& piece : m_pieces) {
        m_boundingBox.expandToInclude(piece->minX(), piece->minY(), piece->minZ());
        m_boundingBox.expandToInclude(piece->maxX(), piece->maxY(), piece->maxZ());
    }
}

bool StructureStart::isRefCountBelowMax() const noexcept
{
    return m_references < getMaxRefCount();
}

void StructureStart::offset(i32 dx, i32 dy, i32 dz)
{
    for (auto& piece : m_pieces) {
        piece->offset(dx, dy, dz);
    }
    m_boundingBox = StructureBoundingBox(m_boundingBox.minX() + dx,
        m_boundingBox.minY() + dy,
        m_boundingBox.minZ() + dz,
        m_boundingBox.maxX() + dx,
        m_boundingBox.maxY() + dy,
        m_boundingBox.maxZ() + dz);
}

} // namespace mc::world::gen::structure
