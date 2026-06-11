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

#include "IChunkGenerator.hpp"
#include "../../../util/assert/AssertAll.hpp"
#include "../../../util/math/random/Random.hpp"
#include "../../WorldConstants.hpp"
#include "../../biome/BiomeRegistry.hpp"
#include "../../block/BlockRegistry.hpp"
#include "../../fluid/FluidRegistry.hpp"
#include "../spawn/WorldGenSpawner.hpp"
#include "common/world/chunk/data/ChunkPrimer.hpp"

#include <algorithm>
#include <cstdlib>
#include <stdexcept>
#include <spdlog/spdlog.h>

namespace mc {

namespace {

[[nodiscard]] i32 chebyshevDistance(i32 x, i32 z)
{
    return std::max(std::abs(x), std::abs(z));
}

[[nodiscard]] const char* statusName(const ChunkStatus* status)
{
    return status ? status->name().c_str() : "<none>";
}

[[nodiscard]] const ChunkStatus& actualChunkStatus(const IChunk& chunk)
{
    if (const auto* primer = dynamic_cast<const ChunkPrimer*>(&chunk)) {
        return primer->getChunkStatus();
    }
    if (const auto* data = dynamic_cast<const ChunkData*>(&chunk); data != nullptr && data->isFullyGenerated()) {
        return ChunkStatuses::FULL;
    }
    return ChunkStatuses::EMPTY;
}

} // namespace

// ============================================================================
// WorldGenRegion 实现
// ============================================================================

WorldGenRegion::WorldGenRegion(ChunkCoord mainX, ChunkCoord mainZ, i32 chunkRadius, std::vector<IChunk*> chunks)
    : m_mainX(mainX)
    , m_mainZ(mainZ)
    , m_chunkRadius(chunkRadius)
    , m_chunkDiameter(chunkRadius * 2 + 1)
    , m_chunks(std::move(chunks))
    , m_generatingStep(nullptr)
{
    MC_ASSERT_RELEASE(m_chunkRadius >= 0);
    MC_ASSERT_RELEASE(static_cast<i32>(m_chunks.size()) == m_chunkDiameter * m_chunkDiameter);
}

WorldGenRegion::WorldGenRegion(
    ChunkCoord mainX, ChunkCoord mainZ, const ChunkStep& generatingStep, std::vector<IChunk*> chunks)
    : m_mainX(mainX)
    , m_mainZ(mainZ)
    , m_chunkRadius(generatingStep.accumulatedRadius())
    , m_chunkDiameter(m_chunkRadius * 2 + 1)
    , m_chunks(std::move(chunks))
    , m_generatingStep(&generatingStep)
{
    MC_ASSERT_RELEASE(m_chunkRadius >= 0);
    MC_ASSERT_RELEASE(static_cast<i32>(m_chunks.size()) == m_chunkDiameter * m_chunkDiameter);
}

IChunk* WorldGenRegion::getChunkAt(i32 relX, i32 relZ)
{
    // 边界检查
    if (relX < -m_chunkRadius || relX > m_chunkRadius || relZ < -m_chunkRadius || relZ > m_chunkRadius) {
        return nullptr;
    }

    const i32 index = (relZ + m_chunkRadius) * m_chunkDiameter + (relX + m_chunkRadius);
    return m_chunks[index];
}

const IChunk* WorldGenRegion::getChunkAt(i32 relX, i32 relZ) const
{
    if (relX < -m_chunkRadius || relX > m_chunkRadius || relZ < -m_chunkRadius || relZ > m_chunkRadius) {
        return nullptr;
    }

    const i32 index = (relZ + m_chunkRadius) * m_chunkDiameter + (relX + m_chunkRadius);
    return m_chunks[index];
}

IChunk* WorldGenRegion::getIChunk(ChunkCoord x, ChunkCoord z)
{
    return getIChunk(x, z, ChunkStatuses::EMPTY);
}

const IChunk* WorldGenRegion::getIChunk(ChunkCoord x, ChunkCoord z) const
{
    return getIChunk(x, z, ChunkStatuses::EMPTY);
}

IChunk* WorldGenRegion::getIChunk(ChunkCoord x, ChunkCoord z, const ChunkStatus& requestedStatus)
{
    return const_cast<IChunk*>(static_cast<const WorldGenRegion&>(*this).getIChunk(x, z, requestedStatus));
}

const IChunk* WorldGenRegion::getIChunk(ChunkCoord x, ChunkCoord z, const ChunkStatus& requestedStatus) const
{
    const i32 relX = x - m_mainX;
    const i32 relZ = z - m_mainZ;
    const i32 distance = chebyshevDistance(relX, relZ);
    const ChunkStatus* allowedStatus = nullptr;
    if (m_generatingStep != nullptr && distance < m_generatingStep->directDependencies().size()) {
        allowedStatus = m_generatingStep->directDependencies().get(distance);
    }

    const bool statusAllowed = allowedStatus != nullptr && requestedStatus.isOrBefore(*allowedStatus);
    if (m_generatingStep != nullptr && !statusAllowed) {
        spdlog::error("[WorldGenRegion] invalid chunk access: requested=({}, {}), center=({}, {}), distance={}, "
                      "generatingStatus={}, requestedStatus={}, allowedStatus={}",
            x,
            z,
            m_mainX,
            m_mainZ,
            distance,
            statusName(m_generatingStep->targetStatus()),
            requestedStatus.name(),
            statusName(allowedStatus));
        MC_ASSERT_RELEASE_MSG(false, "WorldGenRegion chunk access violates ChunkStep dependencies");
        return nullptr;
    }

    const IChunk* chunk = getChunkAt(relX, relZ);
    if (m_generatingStep != nullptr && chunk == nullptr) {
        spdlog::error("[WorldGenRegion] missing chunk in access window: requested=({}, {}), center=({}, {}), "
                      "distance={}, generatingStatus={}, requestedStatus={}, allowedStatus={}",
            x,
            z,
            m_mainX,
            m_mainZ,
            distance,
            statusName(m_generatingStep->targetStatus()),
            requestedStatus.name(),
            statusName(allowedStatus));
        MC_ASSERT_RELEASE_MSG(false, "WorldGenRegion chunk is missing from generation window");
        return nullptr;
    }

    if (m_generatingStep != nullptr && chunk != nullptr) {
        const ChunkStatus& actualStatus = actualChunkStatus(*chunk);
        if (!actualStatus.isAtLeast(requestedStatus)) {
            spdlog::error(
                "[WorldGenRegion] chunk status below request: requested=({}, {}), center=({}, {}), distance={}, "
                "generatingStatus={}, requestedStatus={}, allowedStatus={}, actualStatus={}",
                x,
                z,
                m_mainX,
                m_mainZ,
                distance,
                statusName(m_generatingStep->targetStatus()),
                requestedStatus.name(),
                statusName(allowedStatus),
                actualStatus.name());
            MC_ASSERT_RELEASE_MSG(false, "WorldGenRegion chunk has not reached requested status");
            return nullptr;
        }
    }

    return chunk;
}

const BlockState* WorldGenRegion::getBlockState(i32 x, i32 y, i32 z) const
{
    // 检查 Y 边界
    if (y < world::MIN_BUILD_HEIGHT || y >= world::MAX_BUILD_HEIGHT) {
        return BlockRegistry::instance().airState();
    }

    // 转换为区块坐标和本地坐标
    const ChunkCoord chunkX = world::toChunkCoord(x);
    const ChunkCoord chunkZ = world::toChunkCoord(z);

    const IChunk* chunk = getIChunk(chunkX, chunkZ, ChunkStatuses::EMPTY);
    if (!chunk) {
        return BlockRegistry::instance().airState();
    }

    const i32 localX = world::toLocalCoord(x);
    const i32 localZ = world::toLocalCoord(z);
    return chunk->getBlockState(localX, y, localZ);
}

bool WorldGenRegion::setBlockState(i32 x, i32 y, i32 z, const BlockState* state)
{
    // 检查 Y 边界
    if (y < world::MIN_BUILD_HEIGHT || y >= world::MAX_BUILD_HEIGHT) {
        return false;
    }

    // 转换为区块坐标和本地坐标
    const ChunkCoord chunkX = world::toChunkCoord(x);
    const ChunkCoord chunkZ = world::toChunkCoord(z);
    const i32 relX = chunkX - m_mainX;
    const i32 relZ = chunkZ - m_mainZ;

    // 检查写入半径限制
    if (m_generatingStep != nullptr) {
        const i32 writeRadius = m_generatingStep->blockStateWriteRadius();
        const i32 dx = std::abs(relX);
        const i32 dz = std::abs(relZ);
        if (writeRadius < 0 || dx > writeRadius || dz > writeRadius) {
            spdlog::error("[WorldGenRegion] blocked setBlockState outside write radius: pos=({}, {}, {}), chunk=({}, "
                          "{}), center=({}, {}), writeRadius={}, generatingStatus={}",
                x,
                y,
                z,
                chunkX,
                chunkZ,
                m_mainX,
                m_mainZ,
                writeRadius,
                statusName(m_generatingStep->targetStatus()));
            MC_ASSERT_RELEASE_MSG(false, "WorldGenRegion setBlockState outside of ChunkStep write radius");
            return false;
        }
    }

    IChunk* chunk = getIChunk(chunkX, chunkZ, ChunkStatuses::EMPTY);
    if (!chunk) {
        return false;
    }

    const i32 localX = world::toLocalCoord(x);
    const i32 localZ = world::toLocalCoord(z);
    const BlockState* oldState = chunk->getBlockState(localX, y, localZ);
    chunk->setBlockState(localX, y, localZ, state);

    auto* primer = dynamic_cast<ChunkPrimer*>(chunk);
    if (primer != nullptr) {
        ChunkData* data = primer->getChunkData();
        const BlockPos pos(x, y, z);
        if (data != nullptr && oldState != nullptr && oldState->getBlock().hasBlockEntity()) {
            data->removeBlockEntity(pos);
        }
        if (data != nullptr && state != nullptr && state->getBlock().hasBlockEntity()) {
            auto& block = const_cast<Block&>(state->getBlock());
            data->setBlockEntity(pos, block.createBlockEntity(pos));
        }
        if (state != nullptr && state->isLiquid()) {
            primer->markPosForPostprocessing(localX, y, localZ);
        }
    }

    return true;
}

BiomeId WorldGenRegion::getBiome(i32 x, i32 y, i32 z) const
{
    const ChunkCoord chunkX = world::toChunkCoord(x);
    const ChunkCoord chunkZ = world::toChunkCoord(z);

    const IChunk* chunk = getIChunk(chunkX, chunkZ, ChunkStatuses::EMPTY);
    if (!chunk) {
        return Biomes::Plains;
    }

    const i32 localX = world::toLocalCoord(x);
    const i32 localZ = world::toLocalCoord(z);
    return chunk->getBiomeAtBlock(localX, y, localZ);
}

i32 WorldGenRegion::getTopBlockY(i32 x, i32 z, HeightmapType type) const
{
    const ChunkCoord chunkX = world::toChunkCoord(x);
    const ChunkCoord chunkZ = world::toChunkCoord(z);

    const IChunk* chunk = getIChunk(chunkX, chunkZ, ChunkStatuses::EMPTY);
    MC_ASSERT_RELEASE(chunk);

    const i32 localX = world::toLocalCoord(x);
    const i32 localZ = world::toLocalCoord(z);
    return chunk->getTopBlockY(type, localX, localZ);
}

i32 WorldGenRegion::_worldToChunkIndex(i32 x, i32 z) const
{
    const ChunkCoord chunkX = world::toChunkCoord(x);
    const ChunkCoord chunkZ = world::toChunkCoord(z);
    const i32 relX = chunkX - m_mainX;
    const i32 relZ = chunkZ - m_mainZ;

    if (relX < -m_chunkRadius || relX > m_chunkRadius || relZ < -m_chunkRadius || relZ > m_chunkRadius) {
        return -1;
    }

    return (relZ + m_chunkRadius) * m_chunkDiameter + (relX + m_chunkRadius);
}

i32 WorldGenRegion::_centerIndex() const
{
    return m_chunkRadius * m_chunkDiameter + m_chunkRadius;
}

void WorldGenRegion::_worldToLocal(i32 worldX, i32 worldZ, i32& localX, i32& localZ)
{
    localX = world::toLocalCoord(worldX);
    localZ = world::toLocalCoord(worldZ);
}

// ============================================================================
// WorldGenRegion - IWorld 接口实现
// ============================================================================

const fluid::FluidState* WorldGenRegion::getFluidState(i32 x, i32 y, i32 z) const
{
    // 从区块获取方块状态，再获取流体状态
    const BlockState* blockState = getBlockState(x, y, z);
    if (!blockState || blockState->isAir()) {
        static const fluid::FluidState emptyState = fluid::FluidRegistry::instance().getFluid(0)->defaultState();
        return &emptyState;
    }
    return blockState->getFluidState();
}

const ChunkData* WorldGenRegion::getChunk(ChunkCoord x, ChunkCoord z) const
{
    const IChunk* chunk = getIChunk(x, z, ChunkStatuses::EMPTY);
    if (!chunk) {
        return nullptr;
    }
    const auto* primer = dynamic_cast<const ChunkPrimer*>(chunk);
    if (primer) {
        return primer->getChunkData();
    }
    return nullptr;
}

bool WorldGenRegion::hasChunk(ChunkCoord x, ChunkCoord z) const
{
    const i32 relX = x - m_mainX;
    const i32 relZ = z - m_mainZ;
    if (m_generatingStep == nullptr) {
        return getChunkAt(relX, relZ) != nullptr;
    }

    const i32 distance = chebyshevDistance(relX, relZ);
    return distance < m_generatingStep->directDependencies().size();
}

i32 WorldGenRegion::getHeight(i32 x, i32 z) const
{
    return getTopBlockY(x, z, HeightmapType::WorldSurfaceWG);
}

u8 WorldGenRegion::getBlockLight(i32 x, i32 y, i32 z) const
{
    // 生成期间光照未计算，返回 0
    (void)x;
    (void)y;
    (void)z;
    return 0;
}

u8 WorldGenRegion::getSkyLight(i32 x, i32 y, i32 z) const
{
    // 生成期间光照未计算，返回 15（最大天空光照）
    (void)x;
    (void)y;
    (void)z;
    return 15;
}

bool WorldGenRegion::hasBlockCollision(const AxisAlignedBB& box) const
{
    // 生成期间不支持碰撞检测
    (void)box;
    return false;
}

std::vector<AxisAlignedBB> WorldGenRegion::getBlockCollisions(const AxisAlignedBB& box) const
{
    // 生成期间不支持碰撞检测
    (void)box;
    return {};
}

bool WorldGenRegion::isWithinWorldBounds(i32 x, i32 y, i32 z) const
{
    return y >= world::MIN_BUILD_HEIGHT && y < world::MAX_BUILD_HEIGHT && x >= -world::WORLD_BORDER &&
        x < world::WORLD_BORDER && z >= -world::WORLD_BORDER && z < world::WORLD_BORDER;
}

bool WorldGenRegion::hasEntityCollision(const AxisAlignedBB& box, const Entity* except) const
{
    // 生成期间没有实体
    (void)box;
    (void)except;
    return false;
}

std::vector<AxisAlignedBB> WorldGenRegion::getEntityCollisions(const AxisAlignedBB& box, const Entity* except) const
{
    // 生成期间没有实体
    (void)box;
    (void)except;
    return {};
}

std::vector<Entity*> WorldGenRegion::getEntitiesInAABB(const AxisAlignedBB& box, const Entity* except) const
{
    // 生成期间没有实体
    (void)box;
    (void)except;
    return {};
}

std::vector<Entity*> WorldGenRegion::getEntitiesInRange(const Vector3& pos, f32 range, const Entity* except) const
{
    // 生成期间没有实体
    (void)pos;
    (void)range;
    (void)except;
    return {};
}

DimensionId WorldGenRegion::dimension() const
{
    // 默认返回主世界
    return 0; // DimensionId::Overworld
}

world::tick::TickManager& WorldGenRegion::tickManager()
{
    // 生成区域不支持 tick 调度
    throw std::logic_error("WorldGenRegion does not support tickManager");
}

const world::tick::TickManager& WorldGenRegion::tickManager() const
{
    throw std::logic_error("WorldGenRegion does not support tickManager");
}

// ============================================================================
// BaseChunkGenerator 实现
// ============================================================================

BaseChunkGenerator::BaseChunkGenerator(u64 seed, DimensionSettings settings)
    : m_seed(seed)
    , m_settings(std::move(settings))
    , m_worldGenSpawner(std::make_unique<WorldGenSpawner>())
{}

void BaseChunkGenerator::generateStructureStarts(WorldGenRegion& /*region*/, ChunkPrimer& chunk)
{
    // 默认实现：不生成任何结构
    // 子类可以覆盖以添加结构生成
    chunk.setChunkStatus(ChunkStatuses::STRUCTURE_STARTS);
}

void BaseChunkGenerator::generateStructureReferences(WorldGenRegion& /*region*/, ChunkPrimer& chunk)
{
    // 默认实现：不处理结构引用
    // 子类可以覆盖以处理结构引用
    chunk.setChunkStatus(ChunkStatuses::STRUCTURE_REFERENCES);
}

void BaseChunkGenerator::generateBiomes(WorldGenRegion& /*region*/, ChunkPrimer& chunk)
{
    // 默认实现：设置默认生物群系
    BiomeContainer& biomes = chunk.getBiomes();

    for (i32 sectionIndex = 0; sectionIndex < BiomeContainer::SECTION_COUNT; ++sectionIndex) {
        for (i32 y = 0; y < BiomeContainer::VERT_SIZE; ++y) {
            for (i32 z = 0; z < BiomeContainer::HORIZ_SIZE; ++z) {
                for (i32 x = 0; x < BiomeContainer::HORIZ_SIZE; ++x) {
                    biomes.setBiome(sectionIndex, x, y, z, m_defaultBiome);
                }
            }
        }
    }

    chunk.setChunkStatus(ChunkStatuses::BIOMES);
}

void BaseChunkGenerator::applyCarvers(WorldGenRegion& /*region*/, ChunkPrimer& chunk)
{
    // 默认实现：无雕刻
    // 子类可以覆盖以添加洞穴和峡谷生成
    chunk.setChunkStatus(ChunkStatuses::CARVERS);
}

void BaseChunkGenerator::placeFeatures(WorldGenRegion& /*region*/, ChunkPrimer& chunk)
{
    // 默认实现：无特性
    // 子类可以覆盖以添加树木、矿石等
    chunk.setChunkStatus(ChunkStatuses::FEATURES);
}

i32 BaseChunkGenerator::spawnInitialMobs(
    WorldGenRegion& region, ChunkPrimer& chunk, std::vector<SpawnedEntityData>& outEntities)
{
    // 默认实现：使用 WorldGenSpawner 放置被动动物
    if (!m_worldGenSpawner || !m_worldGenSpawner->isEnabled()) {
        return 0;
    }

    // 获取区块中心位置的生物群系
    const BiomeId biomeId = chunk.getBiomeAtBlock(8, 64, 8);
    const Biome& biome = BiomeRegistry::instance().get(biomeId);

    // 使用种子创建随机数生成器
    math::Random rng;
    rng.setSeed(static_cast<u64>(chunk.x()) * 341873128712ULL + static_cast<u64>(chunk.z()) * 132897987541ULL + m_seed);

    return m_worldGenSpawner->spawnInitialMobs(region, biome, chunk.x(), chunk.z(), *this, rng, outEntities);
}

} // namespace mc
