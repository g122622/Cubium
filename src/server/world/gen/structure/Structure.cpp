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
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/util/property/Properties.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/IWorldWriter.hpp"
#include "common/world/WorldConstants.hpp"
#include "common/world/biome/BiomeTag.hpp"
#include "common/world/block/base/types/BlockPos.hpp"
#include "common/world/block/base/types/BlockUpdateFlags.hpp"
#include "common/world/block/core/Block.hpp"
#include "common/world/block/registry/vanilla/VanillaBlocks.hpp"
#include "common/world/blockentity/BlockEntity.hpp"
#include "common/world/blockentity/BlockEntityType.hpp"
#include "common/world/blockentity/interactive/DispenserBlockEntity.hpp"
#include "common/world/blockentity/storage/ChestEntity.hpp"
#include "common/world/chunk/data/Heightmap.hpp"
#include "server/world/gen/chunk/IChunkGenerator.hpp"
#include <memory>
#include <utility>
#include <vector>
#include <spdlog/spdlog.h>

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
    , m_originX(minX)
    , m_originY(minY)
    , m_originZ(minZ)
{}

StructurePiece::StructurePiece(
    i32 type, i32 minX, i32 minY, i32 minZ, i32 maxX, i32 maxY, i32 maxZ, i32 originX, i32 originY, i32 originZ)
    : m_type(type)
    , m_minX(minX)
    , m_minY(minY)
    , m_minZ(minZ)
    , m_maxX(maxX)
    , m_maxY(maxY)
    , m_maxZ(maxZ)
    , m_originX(originX)
    , m_originY(originY)
    , m_originZ(originZ)
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
    m_originX += dx;
    m_originY += dy;
    m_originZ += dz;
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

        world.setBlockState(worldX, worldY, worldZ, transformedState, world::BlockUpdateFlags::UPDATE_CLIENTS);
    } else {
        world.setBlockState(worldX, worldY, worldZ, state, world::BlockUpdateFlags::UPDATE_CLIENTS);
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
                    if (x == minX || x == maxX) {
                        corners++;
                    }
                    if (y == minY || y == maxY) {
                        corners++;
                    }
                    if (z == minZ || z == maxZ) {
                        corners++;
                    }
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
    math::IRandom& rng,
    BlockSelector& selector)
{
    // 当 alwaysReplace=true 时，只替换非空气方块
    // 当 alwaysReplace=false 时，无条件填充
    IWorld* worldReader = alwaysReplace ? dynamic_cast<IWorld*>(&world) : nullptr;

    for (i32 y = minY; y <= maxY; ++y) {
        for (i32 x = minX; x <= maxX; ++x) {
            for (i32 z = minZ; z <= maxZ; ++z) {
                if (worldReader) {
                    // 未初始化的方块（`ChunkData` 里空气即 nullptr）同样是空气：
                    // 原版 `getBlock(...).isAir()` 对空区块为 true，`skipAir=true` 时整块跳过。
                    const BlockState* currentState = getBlockStateFromPos(*worldReader, x, y, z, bounds);
                    if (currentState == nullptr || currentState->isAir()) {
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
    math::IRandom& rng,
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

bool StructurePiece::isInterior(IWorld& world, i32 x, i32 y, i32 z, const StructureBoundingBox& bounds) const
{
    const i32 worldX = getXWithOffset(x, z);
    const i32 worldY = getYWithOffset(y + 1);
    const i32 worldZ = getZWithOffset(x, z);

    if (!bounds.contains(worldX, worldY, worldZ)) {
        return false;
    }

    // 原版用 OCEAN_FLOOR_WG 高度图，且 `LevelReader.getHeight` 返回首个可放置 Y
    // （最高方块 Y + 1）；本项目的 `IWorld::getHeight` 返回最高方块 Y，需要 +1。
    return worldY < world.getHeight(worldX, worldZ, world::chunk::HeightmapType::OceanFloorWG) + 1;
}

void StructurePiece::maybeFillWithBlocks(IWorldWriter& world,
    const StructureBoundingBox& bounds,
    math::IRandom& rng,
    f32 probability,
    i32 minX,
    i32 minY,
    i32 minZ,
    i32 maxX,
    i32 maxY,
    i32 maxZ,
    const BlockState* boundaryBlock,
    const BlockState* insideBlock,
    bool requireNonAir,
    bool requireInterior)
{
    IWorld* reader = dynamic_cast<IWorld*>(&world);
    MC_ASSERT_RELEASE_MSG(reader != nullptr, "maybeFillWithBlocks requires an IWorld");

    for (i32 y = minY; y <= maxY; ++y) {
        for (i32 x = minX; x <= maxX; ++x) {
            for (i32 z = minZ; z <= maxZ; ++z) {
                // 随机数先于其余条件消耗：原版对每个位置都抽一次 nextFloat。
                if (rng.nextFloat() > probability) {
                    continue;
                }

                if (requireNonAir) {
                    const BlockState* current = getBlockStateFromPos(*reader, x, y, z, bounds);
                    if (current == nullptr || current->isAir()) {
                        continue;
                    }
                }

                if (requireInterior && !isInterior(*reader, x, y, z, bounds)) {
                    continue;
                }

                const bool isBoundary = x == minX || x == maxX || y == minY || y == maxY || z == minZ || z == maxZ;
                setBlockState(world, isBoundary ? boundaryBlock : insideBlock, x, y, z, bounds);
            }
        }
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

    const BlockState& frameDefault = VanillaBlocks::END_PORTAL_FRAME->defaultState();

    // 四个方向的框架状态按原版显式配置。`with` 返回注册表里的全局状态引用，
    // 这里落成具名变量是为了让下方「再加 EYE」的两步链路保持可读。
    const BlockState frameNorth = frameDefault.with(BlockStateProperties::HORIZONTAL_FACING(), Direction::North);
    const BlockState frameSouth = frameDefault.with(BlockStateProperties::HORIZONTAL_FACING(), Direction::South);
    // 注意方向：西侧（x = centerX - 2）的框架朝东，东侧（x = centerX + 2）的框架朝西。
    const BlockState frameEast = frameDefault.with(BlockStateProperties::HORIZONTAL_FACING(), Direction::East);
    const BlockState frameWest = frameDefault.with(BlockStateProperties::HORIZONTAL_FACING(), Direction::West);

    // 北边框架（z = centerZ - 2）：3 个框架，凸起朝北（FACING=NORTH）
    for (i32 dx = -1; dx <= 1; ++dx) {
        const BlockState state = frameNorth.with(BlockStateProperties::EYE(), eyeStates[dx + 1]);
        setBlockState(world, &state, centerX + dx, y, centerZ - 2, bounds);
    }

    // 南边框架（z = centerZ + 2）：3 个框架，凸起朝南（FACING=SOUTH）
    for (i32 dx = -1; dx <= 1; ++dx) {
        const BlockState state = frameSouth.with(BlockStateProperties::EYE(), eyeStates[dx + 4]);
        setBlockState(world, &state, centerX + dx, y, centerZ + 2, bounds);
    }

    // 西侧框架（x = centerX - 2）：3 个框架，凸起朝东（FACING=EAST）
    for (i32 dz = -1; dz <= 1; ++dz) {
        const BlockState state = frameEast.with(BlockStateProperties::EYE(), eyeStates[dz + 7]);
        setBlockState(world, &state, centerX - 2, y, centerZ + dz, bounds);
    }

    // 东侧框架（x = centerX + 2）：3 个框架，凸起朝西（FACING=WEST）
    for (i32 dz = -1; dz <= 1; ++dz) {
        const BlockState state = frameWest.with(BlockStateProperties::EYE(), eyeStates[dz + 10]);
        setBlockState(world, &state, centerX + 2, y, centerZ + dz, bounds);
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

void StructurePiece::fillColumnDown(
    IWorld& world, const BlockState* state, i32 x, i32 y, i32 z, const StructureBoundingBox& bounds)
{
    i32 worldX = getXWithOffset(x, z);
    i32 worldY = getYWithOffset(y);
    i32 worldZ = getZWithOffset(x, z);

    if (!bounds.contains(worldX, worldY, worldZ)) {
        return;
    }

    // 下界这类维度的最低高度不是全局的 -64，必须按世界自己的高度取；原版循环到
    // `getMinY() + 1` 为止（最低写入 y = getMinY() + 2）。
    while (worldY > world.getMinBuildHeight() + 1) {
        const BlockState* current = world.getBlockState(worldX, worldY, worldZ);
        if (current == nullptr || isReplaceableByStructures(*current)) {
            world.setBlockState(worldX, worldY, worldZ, state, world::BlockUpdateFlags::UPDATE_CLIENTS);
            --worldY;
        } else {
            break;
        }
    }
}

bool StructurePiece::isReplaceableByStructures(const BlockState& state)
{
    return state.isAir() || state.isLiquid() || state.is(VanillaBlocks::GLOW_LICHEN) ||
        state.is(VanillaBlocks::SEAGRASS) || state.is(VanillaBlocks::TALL_SEAGRASS);
}

const BlockState* StructurePiece::reorientChest(IWorld& world, const BlockPos& pos, const BlockState* defaultState)
{
    if (defaultState == nullptr) {
        return nullptr;
    }

    Direction solidDirection = Direction::None;

    // 四个水平方向：顺序沿用原版 `Direction.Plane.HORIZONTAL`（N/E/S/W）。
    // 「恰好一个方向有完整实心方块」的判定依赖于此遍历顺序。
    static constexpr Direction horizontalDirs[] = {
        Direction::North,
        Direction::East,
        Direction::South,
        Direction::West,
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

bool StructurePiece::createChestAt(IWorldWriter& world,
    const StructureBoundingBox& bounds,
    math::IRandom& rng,
    const BlockPos& pos,
    const ResourceLocation& lootTable)
{
    if (!bounds.contains(pos.x, pos.y, pos.z)) {
        return false;
    }

    // 检查该位置是否已有宝箱（避免重复放置）
    IWorld* iworld = dynamic_cast<IWorld*>(&world);
    if (iworld != nullptr) {
        const BlockState* existingState = iworld->getBlockState(pos);
        if (existingState != nullptr && existingState->is(VanillaBlocks::CHEST)) {
            return false;
        }
    }

    // 获取默认宝箱状态，通过 reorientChest 自动确定朝向
    const BlockState* chestState = VanillaBlocks::getState(VanillaBlocks::CHEST);
    if (chestState == nullptr) {
        return false;
    }

    // 如果能获取世界接口，自动确定朝向；否则使用默认状态。
    // `reorientChest` 的返回值已经是**世界坐标下的朝向**（按四周方块推出的），
    // 不能再经过构件旋转/镜像；原版这里直接 `level.setBlock`，不是 `placeBlock`。
    if (iworld != nullptr) {
        const BlockState* orientedState = reorientChest(*iworld, pos, chestState);
        world.setBlockState(pos.x, pos.y, pos.z, orientedState, world::BlockUpdateFlags::UPDATE_CLIENTS);
    } else {
        world.setBlockState(pos.x, pos.y, pos.z, chestState, world::BlockUpdateFlags::UPDATE_CLIENTS);
    }

    // 设置战利品表：只有方块实体确实是战利品容器时才消耗随机数并挂表。
    if (iworld != nullptr) {
        BlockEntity* blockEntity = iworld->getBlockEntity(pos);
        if (blockEntity != nullptr) {
            if (blockEntity->getType() == BlockEntityType::Chest ||
                blockEntity->getType() == BlockEntityType::TrappedChest) {
                auto* chestEntity = static_cast<blockentity::ChestEntity*>(blockEntity);
                chestEntity->setLootTable(lootTable, rng.nextLong());
            }
        }
    }
    return true;
}

void StructurePiece::generateChest(IWorldWriter& world,
    const StructureBoundingBox& bounds,
    math::IRandom& rng,
    i32 x,
    i32 y,
    i32 z,
    const ResourceLocation& lootTable)
{
    // 模板坐标重载保持原有的 void 语义：越界或已有宝箱时静默跳过。
    (void)createChestAt(
        world, bounds, rng, BlockPos(getXWithOffset(x, z), getYWithOffset(y), getZWithOffset(x, z)), lootTable);
}

void StructurePiece::generateChest(IWorldWriter& world,
    const StructureBoundingBox& bounds,
    math::IRandom& rng,
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
    math::IRandom& rng,
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
    StructurePiece* /*component*/, std::vector<std::unique_ptr<StructurePiece>>& /*pieces*/, math::IRandom& /*rng*/)
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

    // 【必须断言，不得静默返回 false】结构 JSON 的 "biomes" 是标签引用
    // （如 "#minecraft:has_structure/village_plains"）。标签解析失败时 biomeTag()
    // 为空，若此处静默返回 false，调用方的语义会从"该群系不允许本结构"变成
    // "**所有**群系都不允许本结构"——该结构将永不生成，且不产生任何错误。
    // 这与 _hasBiomesForStructureSet 的静默跳过叠加，正是"村庄整片消失却无日志"
    // 的成因。标签缺失属数据/加载错误，必须立刻暴露。
    if (tag == nullptr) {
        spdlog::error(
            "[STRUCT] structure '{}' has an unresolved biome tag; structure tags not loaded?", id().toString());
    }
    MC_ASSERT_RELEASE(tag != nullptr);

    return tag->contains(biomeId);
}

bool Structure::isValidBiomeAt(IChunkGenerator& generator, const BlockPos& pos) const
{
    // 采样点必须是候选生成点本身：原版在 `GenerationStub` 位置按 QuartPos.fromBlock
    // 取噪声群系，路由到 `getNoiseBiome(x>>2, y>>2, z>>2)`，与 `getBiomeAtBlock` 一致。
    const BiomeId biomeId = generator.getBiomeAtBlock(pos.x, pos.y, pos.z);
    return isValidBiome(biomeId);
}

BlockPos Structure::onTopOfChunkCenter(IChunkGenerator& generator, i32 chunkX, i32 chunkZ, HeightmapType type) const
{
    const i32 x = chunkX * world::CHUNK_WIDTH + world::CHUNK_WIDTH / 2;
    const i32 z = chunkZ * world::CHUNK_WIDTH + world::CHUNK_WIDTH / 2;
    return BlockPos(x, generator.getHeight(x, z, type), z);
}

std::unique_ptr<StructureStart> Structure::generate(
    IChunkGenerator& /*generator*/, math::IRandom& /*rng*/, i32 chunkX, i32 chunkZ) const
{
    return makeStructureStart(chunkX, chunkZ);
}

void Structure::placeInChunk(IWorldWriter& world,
    ChunkPrimer& chunk,
    StructureStart& start,
    i32 chunkX,
    i32 chunkZ,
    math::IRandom& rng,
    IChunkGenerator* generator) const
{
    StructureBoundingBox chunkBounds = StructureBoundingBox::fromChunk(chunkX, chunkZ);

    // 随机源由调用方按 `applyBiomeDecoration` 的口径准备：每区块一份 worldgen 随机源，
    // 放置前 `setFeatureSeed(decorSeed, index, stage)`。所有构件共用同一个实例、依序消费，
    // 与逐构件另起随机源的结果不同（宝箱等会用到它）。
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

StructureBoundingBox Structure::adjustBoundingBox(const StructureBoundingBox& box) const
{
    // 原版 `Structure.adjustBoundingBox`：地形适配非 NONE 时每侧外扩 12 格
    // （Beardifier 的核半径），NONE 时原样返回。
    if (terrainAdaptation() == TerrainAdaptation::None || !box.isValid()) {
        return box;
    }
    constexpr i32 TERRAIN_ADJUSTMENT_INFLATION = 12;
    return StructureBoundingBox(box.minX() - TERRAIN_ADJUSTMENT_INFLATION,
        box.minY() - TERRAIN_ADJUSTMENT_INFLATION,
        box.minZ() - TERRAIN_ADJUSTMENT_INFLATION,
        box.maxX() + TERRAIN_ADJUSTMENT_INFLATION,
        box.maxY() + TERRAIN_ADJUSTMENT_INFLATION,
        box.maxZ() + TERRAIN_ADJUSTMENT_INFLATION);
}

std::unique_ptr<StructureStart> Structure::makeStructureStart(i32 chunkX, i32 chunkZ) const
{
    return std::make_unique<StructureStart>(*this, chunkX, chunkZ);
}

// ========== StructureStart ==========

StructureStart::StructureStart(const Structure& owner, i32 chunkX, i32 chunkZ)
    : m_chunkX(chunkX)
    , m_chunkZ(chunkZ)
    , m_owner(&owner)
{}

const StructureBoundingBox& StructureStart::getBoundingBox() const noexcept
{
    if (m_boundingBoxDirty) {
        m_boundingBox = m_owner->adjustBoundingBox(m_pieceBox);
        m_boundingBoxDirty = false;
    }
    return m_boundingBox;
}

void StructureStart::addPiece(std::unique_ptr<StructurePiece> piece)
{
    if (piece) {
        m_pieceBox.expandToInclude(piece->minX(), piece->minY(), piece->minZ());
        m_pieceBox.expandToInclude(piece->maxX(), piece->maxY(), piece->maxZ());
        m_boundingBoxDirty = true;
        m_pieces.push_back(std::move(piece));
    }
}

void StructureStart::recalculateStructureSize()
{
    m_pieceBox = StructureBoundingBox(); // 重置为无效状态
    for (const auto& piece : m_pieces) {
        m_pieceBox.expandToInclude(piece->minX(), piece->minY(), piece->minZ());
        m_pieceBox.expandToInclude(piece->maxX(), piece->maxY(), piece->maxZ());
    }
    // 地形适配外扩在 getBoundingBox() 里按需施加：原版同样从片段并集重算，
    // 再调 `Structure.adjustBoundingBox`，外扩不会被片段位置"吃掉"。
    m_boundingBoxDirty = true;
}

bool StructureStart::isRefCountBelowMax() const noexcept
{
    return getRefCount() < getMaxRefCount();
}

void StructureStart::offset(i32 dx, i32 dy, i32 dz)
{
    for (auto& piece : m_pieces) {
        piece->offset(dx, dy, dz);
    }
    const auto shift = [&](StructureBoundingBox& box) {
        box = StructureBoundingBox(
            box.minX() + dx, box.minY() + dy, box.minZ() + dz, box.maxX() + dx, box.maxY() + dy, box.maxZ() + dz);
    };
    shift(m_pieceBox);
    if (!m_boundingBoxDirty) {
        shift(m_boundingBox);
    }
}

} // namespace mc::world::gen::structure
