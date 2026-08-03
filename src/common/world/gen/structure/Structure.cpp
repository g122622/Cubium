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
#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/Direction.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/util/property/Properties.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/IWorldWriter.hpp"
#include "common/world/WorldConstants.hpp"
#include "common/world/biome/BiomeTag.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/blockentity/BlockEntity.hpp"
#include "common/world/blockentity/BlockEntityType.hpp"
#include "common/world/blockentity/interactive/DispenserBlockEntity.hpp"
#include "common/world/blockentity/storage/ChestEntity.hpp"
#include "common/world/gen/chunk/IChunkGenerator.hpp"
#include <memory>
#include <utility>
#include <vector>

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

void StructurePiece::placeEndPortalFrames(IWorldWriter& world,
    const StructureBoundingBox& bounds,
    i32 centerX,
    i32 y,
    i32 centerZ,
    const bool eyeStates[12],
    bool allEyesFilled)
{
    if (VanillaBlocks::END_PORTAL_FRAME == nullptr) {
        return;
    }

    const BlockState* frameDefault = &VanillaBlocks::END_PORTAL_FRAME->defaultState();

    // 北边框架（z = centerZ - 2）：3 个框架，凸起朝北（FACING=NORTH）
    const BlockState* frameNorth = &frameDefault->with(BlockStateProperties::HORIZONTAL_FACING(), Direction::North);
    for (i32 dx = -1; dx <= 1; ++dx) {
        i32 idx = dx + 1; // 0, 1, 2
        const BlockState* state = &frameNorth->with(BlockStateProperties::EYE(), eyeStates[idx]);
        setBlockState(world, state, centerX + dx, y, centerZ - 2, bounds);
    }

    // 南边框架（z = centerZ + 2）：3 个框架，凸起朝南（FACING=SOUTH）
    const BlockState* frameSouth = &frameDefault->with(BlockStateProperties::HORIZONTAL_FACING(), Direction::South);
    for (i32 dx = -1; dx <= 1; ++dx) {
        i32 idx = dx + 4; // 3, 4, 5
        const BlockState* state = &frameSouth->with(BlockStateProperties::EYE(), eyeStates[idx]);
        setBlockState(world, state, centerX + dx, y, centerZ + 2, bounds);
    }

    // 西边框架（x = centerX - 2）：3 个框架，凸起朝西（FACING=WEST）
    const BlockState* frameWest = &frameDefault->with(BlockStateProperties::HORIZONTAL_FACING(), Direction::West);
    for (i32 dz = -1; dz <= 1; ++dz) {
        i32 idx = dz + 7; // 6, 7, 8
        const BlockState* state = &frameWest->with(BlockStateProperties::EYE(), eyeStates[idx]);
        setBlockState(world, state, centerX - 2, y, centerZ + dz, bounds);
    }

    // 东边框架（x = centerX + 2）：3 个框架，凸起朝东（FACING=EAST）
    const BlockState* frameEast = &frameDefault->with(BlockStateProperties::HORIZONTAL_FACING(), Direction::East);
    for (i32 dz = -1; dz <= 1; ++dz) {
        i32 idx = dz + 10; // 9, 10, 11
        const BlockState* state = &frameEast->with(BlockStateProperties::EYE(), eyeStates[idx]);
        setBlockState(world, state, centerX + 2, y, centerZ + dz, bounds);
    }

    // 当所有框架都有末影之眼时，在内部 3×3 区域放置末地传送门方块
    if (allEyesFilled) {
        const BlockState* endPortal = VanillaBlocks::getState(VanillaBlocks::END_PORTAL);
        if (endPortal != nullptr) {
            for (i32 dx = -1; dx <= 1; ++dx) {
                for (i32 dz = -1; dz <= 1; ++dz) {
                    setBlockState(world, endPortal, centerX + dx, y, centerZ + dz, bounds);
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

const BlockState* StructurePiece::reorientChest(IWorld& world, const BlockPos& pos, const BlockState* defaultState)
{
    if (defaultState == nullptr) {
        return nullptr;
    }

    Direction solidDirection = Direction::None;

    // 四个水平方向
    static constexpr Direction horizontalDirs[] = {
        Direction::North,
        Direction::South,
        Direction::West,
        Direction::East,
    };

    for (Direction dir : horizontalDirs) {
        BlockPos neighborPos = pos.offset(dir);
        const BlockState* neighborState = world.getBlockState(neighborPos);
        if (neighborState == nullptr) {
            continue;
        }

        // 如果相邻位置是宝箱，保持默认朝向（用于双箱合并）
        if (neighborState->is(VanillaBlocks::CHEST)) {
            return defaultState;
        }

        // 检查是否为不透明完整方块
        if (neighborState->isOpaqueCube(world, neighborPos)) {
            if (solidDirection != Direction::None) {
                // 多于一个方向有不透明完整方块，无法确定朝向，重置并跳出
                solidDirection = Direction::None;
                break;
            }
            solidDirection = dir;
        }
    }

    // 如果恰好有一个方向是不透明完整方块，宝箱面向相反方向（面向开放空间）
    if (solidDirection != Direction::None) {
        return &defaultState->with(BlockStateProperties::HORIZONTAL_FACING(), getOpposite(solidDirection));
    }

    // 没有或有两个以上不透明完整方块：从默认朝向开始寻找非不透明方向
    Direction facing = Direction::North;
    auto currentFacing = defaultState->getOptional(BlockStateProperties::HORIZONTAL_FACING());
    if (currentFacing.has_value()) {
        facing = currentFacing.value();
    }

    BlockPos frontPos = pos.offset(facing);
    const BlockState* frontState = world.getBlockState(frontPos);
    if (frontState != nullptr && frontState->isOpaqueCube(world, frontPos)) {
        facing = getOpposite(facing);
        frontPos = pos.offset(facing);
        frontState = world.getBlockState(frontPos);
    }

    if (frontState != nullptr && frontState->isOpaqueCube(world, frontPos)) {
        facing = Directions::rotateY(facing);
        frontPos = pos.offset(facing);
        frontState = world.getBlockState(frontPos);
    }

    if (frontState != nullptr && frontState->isOpaqueCube(world, frontPos)) {
        facing = getOpposite(facing);
    }

    return &defaultState->with(BlockStateProperties::HORIZONTAL_FACING(), facing);
}

void StructurePiece::generateChest(IWorldWriter& world,
    const StructureBoundingBox& bounds,
    math::Random& rng,
    i32 x,
    i32 y,
    i32 z,
    const ResourceLocation& lootTable)
{
    i32 worldX = getXWithOffset(x, z);
    i32 worldY = getYWithOffset(y);
    i32 worldZ = getZWithOffset(x, z);

    if (!bounds.contains(worldX, worldY, worldZ)) {
        return;
    }

    // 检查该位置是否已有宝箱（避免重复放置）
    IWorld* iworld = dynamic_cast<IWorld*>(&world);
    if (iworld != nullptr) {
        const BlockState* existingState = iworld->getBlockState(worldX, worldY, worldZ);
        if (existingState != nullptr && existingState->is(VanillaBlocks::CHEST)) {
            return;
        }
    }

    // 获取默认宝箱状态，通过 reorientChest 自动确定朝向
    const BlockState* chestState = VanillaBlocks::getState(VanillaBlocks::CHEST);
    if (chestState == nullptr) {
        return;
    }

    // 如果能获取世界接口，自动确定朝向；否则使用默认状态
    if (iworld != nullptr) {
        BlockPos chestPos(worldX, worldY, worldZ);
        const BlockState* orientedState = reorientChest(*iworld, chestPos, chestState);
        setBlockState(world, orientedState, x, y, z, bounds);
    } else {
        setBlockState(world, chestState, x, y, z, bounds);
    }

    // 设置战利品表
    if (iworld != nullptr) {
        BlockPos chestPos(worldX, worldY, worldZ);
        BlockEntity* blockEntity = iworld->getBlockEntity(chestPos);
        if (blockEntity != nullptr) {
            if (blockEntity->getType() == BlockEntityType::Chest ||
                blockEntity->getType() == BlockEntityType::TrappedChest) {
                auto* chestEntity = static_cast<blockentity::ChestEntity*>(blockEntity);
                chestEntity->setLootTable(lootTable, rng.nextLong());
            }
        }
    }
}

void StructurePiece::generateChest(IWorldWriter& world,
    const StructureBoundingBox& bounds,
    math::Random& rng,
    i32 x,
    i32 y,
    i32 z,
    Direction facing,
    const ResourceLocation& lootTable)
{
    i32 worldX = getXWithOffset(x, z);
    i32 worldY = getYWithOffset(y);
    i32 worldZ = getZWithOffset(x, z);

    if (!bounds.contains(worldX, worldY, worldZ)) {
        return;
    }

    // 放置带朝向的宝箱方块
    const BlockState* chestState = VanillaBlocks::getState(VanillaBlocks::CHEST);
    if (chestState != nullptr) {
        BlockState orientedState = chestState->with(BlockStateProperties::HORIZONTAL_FACING(), facing);
        setBlockState(world, &orientedState, x, y, z, bounds);
    }

    // 设置战利品表
    IWorld* iworld = dynamic_cast<IWorld*>(&world);
    if (iworld != nullptr) {
        BlockPos chestPos(worldX, worldY, worldZ);
        BlockEntity* blockEntity = iworld->getBlockEntity(chestPos);
        if (blockEntity != nullptr) {
            if (blockEntity->getType() == BlockEntityType::Chest ||
                blockEntity->getType() == BlockEntityType::TrappedChest) {
                auto* chestEntity = static_cast<blockentity::ChestEntity*>(blockEntity);
                chestEntity->setLootTable(lootTable, rng.nextLong());
            }
        }
    }
}

void StructurePiece::generateDispenser(IWorldWriter& world,
    const StructureBoundingBox& bounds,
    math::Random& rng,
    i32 x,
    i32 y,
    i32 z,
    Direction facing,
    const ResourceLocation& lootTable)
{
    i32 worldX = getXWithOffset(x, z);
    i32 worldY = getYWithOffset(y);
    i32 worldZ = getZWithOffset(x, z);

    if (!bounds.contains(worldX, worldY, worldZ)) {
        return;
    }

    // 放置带朝向的发射器方块
    const BlockState* dispenserState = VanillaBlocks::getState(VanillaBlocks::DISPENSER);
    if (dispenserState != nullptr) {
        BlockState orientedState = dispenserState->with(BlockStateProperties::FACING(), facing);
        setBlockState(world, &orientedState, x, y, z, bounds);
    }

    // 设置战利品表
    IWorld* iworld = dynamic_cast<IWorld*>(&world);
    if (iworld != nullptr) {
        BlockPos dispenserPos(worldX, worldY, worldZ);
        BlockEntity* blockEntity = iworld->getBlockEntity(dispenserPos);
        if (blockEntity != nullptr && blockEntity->getType() == BlockEntityType::Dispenser) {
            auto* dispenserEntity = static_cast<blockentity::DispenserBlockEntity*>(blockEntity);
            dispenserEntity->setLootTable(lootTable, rng.nextLong());
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
    // 使用 BiomeTag 进行 O(1) 查找
    const biome::BiomeTag* tag = biomeTag();
    if (tag) {
        return tag->contains(biomeId);
    }
    // 标签未加载时，无法判断，返回 false
    return false;
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

void Structure::placeInChunk(IWorldWriter& world,
    ChunkPrimer& chunk,
    StructureStart& start,
    i32 chunkX,
    i32 chunkZ,
    IChunkGenerator* generator) const
{
    StructureBoundingBox chunkBounds = StructureBoundingBox::fromChunk(chunkX, chunkZ);

    math::Random rng = createRandom(
        static_cast<i64>(chunkX) * 341873128712LL ^ static_cast<i64>(chunkZ) * 132897987541LL, chunkX, chunkZ, 0);

    for (const auto& piece : start.pieces()) {
        if (piece->intersectsChunk(chunkX, chunkZ)) {
            piece->generate(world, rng, chunkX, chunkZ, chunkBounds, &chunk, generator);
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
