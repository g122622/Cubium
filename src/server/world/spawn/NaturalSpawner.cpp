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

#include "NaturalSpawner.hpp"
#include "../ServerWorld.hpp"
#include "SpawnConditions.hpp"
#include "common/entity/combat/DifficultyHelper.hpp"
#include "common/entity/combat/DifficultyInstance.hpp"
#include "common/entity/core/CreatureEntity.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/core/EntityClassification.hpp"
#include "common/entity/core/EntityRegistry.hpp"
#include "common/entity/core/MobEntity.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/profiler/TraceEvents.hpp"
#include "common/util/math/MathUtils.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/WorldConstants.hpp"
#include "common/world/biome/Biome.hpp"
#include "common/world/biome/BiomeRegistry.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/Material.hpp"
#include "common/world/chunk/load/ChunkLoadTicketManager.hpp"
#include "common/world/entity/EntityManager.hpp"
#include "common/world/lighting/InternalLightUtils.hpp"
#include "common/world/spawn/EntitySpawnPlacementRegistry.hpp"
#include "common/world/spawn/MobSpawnInfo.hpp"
#include <cmath>
#include <limits>
#include <utility>
#include <fmt/format.h>

using namespace mc::trace;

namespace mc::world::spawn {

// ============================================================================
// 常量定义
// ============================================================================

/// 每区块生成尝试轮数（spawnCategoryForPosition 外层 3 轮）
static constexpr i32 MAX_SPAWN_ATTEMPTS_PER_CHUNK = 3;

/// 生成区块计数基准 (17^2)
static constexpr i32 MAGIC_NUMBER = 289;

// ============================================================================
// MobDensityTracker 实现
// ============================================================================

void MobDensityTracker::addCharge(const Vector3& pos, f64 charge)
{
    m_charges.push_back({pos, charge});
}

f64 MobDensityTracker::getTotalCharge(const Vector3& pos, f64 multiplier) const
{
    // multiplier 为 0 时直接返回 0
    if (multiplier == 0.0) {
        return 0.0;
    }

    // 先对所有点电荷求和：sum(charge / sqrt(distSq))，距离为 0 处贡献无穷大
    f64 sumCharge = 0.0;
    for (const auto& entry : m_charges) {
        f64 dx = entry.position.x - pos.x;
        f64 dy = entry.position.y - pos.y;
        f64 dz = entry.position.z - pos.z;
        f64 distSq = dx * dx + dy * dy + dz * dz;

        if (distSq == 0.0) {
            // 与查询位置重合的点电荷贡献无穷大，使任何有限预算都被超出
            return std::numeric_limits<f64>::infinity();
        }

        sumCharge += entry.charge / std::sqrt(distSq);
    }

    // 乘以 multiplier 得到最终的密度变化量
    return sumCharge * multiplier;
}

// ============================================================================
// LocalMobCapCalculator 实现
// ============================================================================

void LocalMobCapCalculator::addPlayerChunk(u64 playerId, const ChunkPos& chunk)
{
    m_playerChunks[playerId].insert(chunk);
}

void LocalMobCapCalculator::addMob(const ChunkPos& chunk, entity::EntityClassification classification)
{
    m_chunkCounts[chunk][classification]++;
}

bool LocalMobCapCalculator::canSpawn(entity::EntityClassification classification, const ChunkPos& chunk) const
{
    // 本地 cap = 该分类每区块最大实例数（cap 取 MobCategory.getMaxInstancesPerChunk）。
    const i32 localCap = getMaxCount(classification);

    // 任何共享该区块的玩家，其本地计数达到 cap 即阻止生成。
    for (const auto& [playerId, chunks] : m_playerChunks) {
        if (chunks.find(chunk) == chunks.end()) {
            continue;
        }
        auto countIt = m_chunkCounts.find(chunk);
        i32 used = 0;
        if (countIt != m_chunkCounts.end()) {
            auto classIt = countIt->second.find(classification);
            if (classIt != countIt->second.end()) {
                used = classIt->second;
            }
        }
        if (used >= localCap) {
            return false;
        }
    }
    return true;
}

// ============================================================================
// EntityDensityManager 实现
// ============================================================================

EntityDensityManager::EntityDensityManager(i32 spawnableChunkCount,
    std::unordered_map<entity::EntityClassification, i32> entityCounts,
    MobDensityTracker& densityTracker)
    : m_spawnableChunkCount(spawnableChunkCount)
    , m_entityCounts(std::move(entityCounts))
    , m_densityTracker(densityTracker)
{}

bool EntityDensityManager::canSpawn(entity::EntityClassification classification) const
{
    // MISC 分类不生成
    if (classification == entity::EntityClassification::Misc) {
        return false;
    }

    // 获取当前数量
    auto it = m_entityCounts.find(classification);
    i32 currentCount = (it != m_entityCounts.end()) ? it->second : 0;

    // 全局容量上限：maxInstancesPerChunk * spawnableChunkCount / MAGIC_NUMBER(289)
    // 对应 NaturalSpawner.canSpawnForCategoryGlobal，无下限保护。
    i32 maxInstances = getMaxCount(classification);
    i32 cap = maxInstances * m_spawnableChunkCount / MAGIC_NUMBER;

    return currentCount < cap;
}

bool EntityDensityManager::canSpawnWithDensity(
    const std::string& entityTypeId, const Vector3& pos, const SpawnCosts& spawnCosts) const
{
    if (!spawnCosts.isValid()) {
        return true; // 无成本限制
    }

    // 检查当前密度是否超过能量预算
    // 原版逻辑：getTotalCharge(pos, charge) <= energyBudget
    f64 currentDensity = m_densityTracker.getTotalCharge(pos, spawnCosts.charge);
    return currentDensity <= spawnCosts.energyBudget;
}

void EntityDensityManager::onSpawn(const std::string& entityTypeId,
    entity::EntityClassification classification,
    const Vector3& pos,
    const SpawnCosts& spawnCosts)
{
    // 更新密度追踪器
    if (spawnCosts.isValid()) {
        m_densityTracker.addCharge(pos, spawnCosts.charge);
    }

    // 更新实体分类计数
    auto it = m_entityCounts.find(classification);
    if (it != m_entityCounts.end()) {
        ++it->second;
    } else {
        m_entityCounts[classification] = 1;
    }
}

i32 EntityDensityManager::getCount(entity::EntityClassification classification) const
{
    auto it = m_entityCounts.find(classification);
    return (it != m_entityCounts.end()) ? it->second : 0;
}

// ============================================================================
// NaturalSpawner 实现
// ============================================================================

NaturalSpawner::NaturalSpawner()
    : m_lastCreatureSpawnTime(0)
{}

void NaturalSpawner::spawnInChunk(
    mc::server::ServerWorld& world, i32 chunkX, i32 chunkZ, const MobSpawnInfo& spawnInfo, math::Random& random)
{
    MC_TRACE_SCOPED_EVENT(
        TraceEvents.Server.Entity, "NaturalSpawner::spawnInChunk", "chunkX", chunkX, "chunkZ", chunkZ);
    // 获取区块的世界坐标范围
    i32 minX = world::toWorldCoord(chunkX);
    i32 minZ = world::toWorldCoord(chunkZ);

    // 仅生成 Creature 分类的实体（被动动物）
    const auto& creatures = spawnInfo.getCreatureSpawns();
    if (creatures.empty()) {
        return;
    }

    // 使用 creature_spawn_probability 进行概率循环
    f32 spawnProbability = spawnInfo.getCreatureSpawnProbability();

    while (random.nextFloat() < spawnProbability) {
        // 选择生成条目
        const SpawnEntry* entry = _selectEntry(creatures, random);
        if (!entry) {
            break;
        }

        // 获取实体类型
        auto& registry = entity::EntityRegistry::instance();
        const entity::EntityType* entityType = registry.getType(entry->entityTypeId);
        if (!entityType) {
            continue;
        }

        // 确定生成数量
        i32 count = entry->minCount;
        if (entry->maxCount > entry->minCount) {
            count = random.nextInt(entry->minCount, entry->maxCount);
        }

        // 选择初始位置
        i32 baseX = minX + random.nextInt(world::CHUNK_WIDTH);
        i32 baseZ = minZ + random.nextInt(world::CHUNK_WIDTH);

        // 生成群体
        for (i32 i = 0; i < count; ++i) {
            // 在基础位置附近随机偏移
            i32 x = baseX + random.nextInt(5) - random.nextInt(5);
            i32 z = baseZ + random.nextInt(5) - random.nextInt(5);

            // 获取生成高度
            HeightmapType heightmapType = EntitySpawnPlacementRegistry::getHeightmapType(entry->entityTypeId);
            i32 y = _getSpawnHeight(world, x, z, heightmapType);
            if (y < world::MIN_BUILD_HEIGHT) {
                continue;
            }

            // 检查是否可以生成
            if (!_canSpawnAt(world, x, y, z, *entry)) {
                continue;
            }

            // 尝试生成
            _trySpawnAt(world, x, y, z, *entry, random);
        }

        // 每次成功选择后降低概率
        spawnProbability *= 0.9f;
    }
}

void NaturalSpawner::tick(mc::server::ServerWorld& world, bool hostile, bool passive)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Entity, "NaturalSpawner::tick", "hostile", hostile, "passive", passive);

    // 和平模式下不生成敌对生物
    if (!entity::combat::DifficultyHelper::allowsMobSpawning(world.difficulty())) {
        hostile = false;
    }

    // 获取当前时间
    const u64 worldTime = world.currentTick();

    // 创建随机数生成器
    math::Random random(static_cast<u64>(worldTime));

    // 获取玩家列表
    auto players = world.entityManager().getPlayers();
    if (players.empty()) {
        return;
    }

    // 创建实体密度管理器（同时重建密度追踪器与本地容量计算器，统计刷怪区块数）
    EntityDensityManager densityManager = _createDensityManager(world);

    // 全局预过滤：一次性选出本 tick 仍可生成的分类列表。
    // getFilteredSpawningCategories：friendly/persistent 过滤 + 全局 cap 检查 +
    // 持久化分类（Creature/Misc）的 400tick 节流。
    static const entity::EntityClassification allCategories[] = {entity::EntityClassification::Monster,
        entity::EntityClassification::Creature,
        entity::EntityClassification::Ambient,
        entity::EntityClassification::Axolotls,
        entity::EntityClassification::UndergroundWaterCreature,
        entity::EntityClassification::WaterCreature,
        entity::EntityClassification::WaterAmbient};

    std::vector<entity::EntityClassification> activeCategories;
    for (auto classification : allCategories) {
        const bool isPeaceful = entity::isPeaceful(classification);
        if (!hostile && !isPeaceful) {
            continue;
        }
        if (!passive && isPeaceful) {
            continue;
        }
        // 持久化分类（Creature）每 400tick 才允许
        if (!_isSpawnCategoryReady(classification, worldTime)) {
            continue;
        }
        if (!densityManager.canSpawn(classification)) {
            continue;
        }
        activeCategories.push_back(classification);
    }
    if (activeCategories.empty()) {
        // 仍更新动物生成时间戳
        if (passive && worldTime - m_lastCreatureSpawnTime >= CREATURE_SPAWN_INTERVAL) {
            m_lastCreatureSpawnTime = worldTime;
        }
        return;
    }

    // 收集并打乱刷怪区块（collectSpawningChunks + Util.shuffle，无 1/17 概率丢弃）。
    std::vector<ChunkPos> spawnableChunks = _getSpawnableChunks(world, random);

    for (const ChunkPos& chunkPos : spawnableChunks) {
        const ChunkData* chunk = world.getChunk(chunkPos.x, chunkPos.z);
        if (chunk == nullptr) {
            continue;
        }

        // 取该区块附近最近的玩家作为距离基准
        const Vector3 chunkCenter = chunkPos.center();
        Entity* nearestPlayer = world.getClosestPlayer(chunkCenter);
        if (nearestPlayer == nullptr) {
            continue;
        }
        const Vector3 playerPos = nearestPlayer->position();

        for (auto classification : activeCategories) {
            // 每块每分类重新检查容量（前面块可能已触顶）
            if (!densityManager.canSpawn(classification)) {
                continue;
            }
            if (!m_localMobCap.canSpawn(classification, chunkPos)) {
                continue;
            }

            _spawnForClassificationInChunk(classification, world, chunk, playerPos, densityManager, random);
        }
    }

    // 更新动物生成时间
    if (passive && worldTime - m_lastCreatureSpawnTime >= CREATURE_SPAWN_INTERVAL) {
        m_lastCreatureSpawnTime = worldTime;
    }
}

void NaturalSpawner::_spawnForClassificationInChunk(entity::EntityClassification classification,
    mc::server::ServerWorld& world,
    const ChunkData* chunk,
    const Vector3& playerPos,
    EntityDensityManager& densityManager,
    math::Random& random)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Entity,
        "NaturalSpawner::_spawnForClassificationInChunk",
        "classification",
        static_cast<i32>(classification));
    if (!chunk) {
        return;
    }

    // 获取该分类对应的生成高度图类型
    HeightmapType heightmapType = HeightmapType::MotionBlockingNoLeaves;

    // 区块坐标（用于本地容量检查）
    const ChunkPos chunkPos(chunk->x(), chunk->z());

    // 获取区块的世界坐标
    i32 chunkMinX = chunk->x() << world::CHUNK_SHIFT;
    i32 chunkMinZ = chunk->z() << world::CHUNK_SHIFT;

    // 原版 spawnCategoryForPosition 外层 3 轮：每轮独立选取区块内随机位置、
    // 独立重新检查全局/本地容量上限、独立确定群体规模 k1=ceil(random*4)。
    for (i32 round = 0; round < MAX_SPAWN_ATTEMPTS_PER_CHUNK; ++round) {
        // 每轮重新检查容量上限（上一轮生成可能已触顶）
        if (!densityManager.canSpawn(classification)) {
            return;
        }
        if (!m_localMobCap.canSpawn(classification, chunkPos)) {
            return;
        }

        // 随机选择区块内位置
        i32 spawnX = chunkMinX + random.nextInt(world::CHUNK_WIDTH);
        i32 spawnZ = chunkMinZ + random.nextInt(world::CHUNK_WIDTH);

        // 获取高度
        i32 spawnY = _getSpawnHeight(world, spawnX, spawnZ, heightmapType);
        if (spawnY < 0) {
            continue;
        }

        // 检查玩家距离
        f64 dx = static_cast<f64>(spawnX) + 0.5 - playerPos.x;
        f64 dz = static_cast<f64>(spawnZ) + 0.5 - playerPos.z;
        f64 playerDistanceSq = dx * dx + dz * dz;

        // 玩家距离必须 > 24 格
        if (playerDistanceSq < MIN_SPAWN_DISTANCE_SQ) {
            continue;
        }

        // 玩家距离必须 <= 128 格
        if (playerDistanceSq > MAX_SPAWN_DISTANCE_SQ) {
            continue;
        }

        // 选择生成条目
        Vector3i pos(spawnX, spawnY, spawnZ);
        const SpawnEntry* entry = _getRandomSpawnEntry(world, chunk, classification, pos, random);
        if (!entry) {
            continue;
        }

        // 从 ChunkData 获取生物群系，再从生物群系获取 SpawnCosts
        const SpawnCosts* spawnCosts = nullptr;
        {
            i32 localX = world::toLocalCoord(spawnX);
            i32 localZ = world::toLocalCoord(spawnZ);
            BiomeId biomeId = chunk->getBiomeAtBlock(localX, spawnY, localZ);

            if (BiomeRegistry::instance().hasBiome(biomeId)) {
                const Biome& biome = BiomeRegistry::instance().get(biomeId);
                const MobSpawnInfo& spawnInfo = biome.spawnInfo();
                spawnCosts = spawnInfo.getSpawnCost(entry->entityTypeId);
            }
        }

        if (spawnCosts && spawnCosts->isValid()) {
            if (!densityManager.canSpawnWithDensity(entry->entityTypeId,
                    Vector3(static_cast<f32>(spawnX), static_cast<f32>(spawnY), static_cast<f32>(spawnZ)),
                    *spawnCosts)) {
                continue;
            }
        }

        // 获取实体类型
        auto& registry = entity::EntityRegistry::instance();
        const entity::EntityType* entityType = registry.getType(entry->entityTypeId);
        if (!entityType) {
            continue;
        }

        // 群体规模 k1 = ceil(random*4)。
        i32 groupSize = static_cast<i32>(std::ceil(random.nextFloat() * 4.0f));
        if (groupSize < 1) {
            groupSize = 1;
        }

        i32 spawned = 0;

        // 尝试在位置周围生成群体
        for (i32 i = 0; i < groupSize; ++i) {
            // 计算偏移位置
            i32 x = spawnX + random.nextInt(6) - random.nextInt(6);
            i32 z = spawnZ + random.nextInt(6) - random.nextInt(6);

            // 获取高度
            i32 y = _getSpawnHeight(world, x, z, heightmapType);
            if (y < world::MIN_BUILD_HEIGHT) {
                continue;
            }

            // 检查玩家距离
            f64 odx = static_cast<f64>(x) + 0.5 - playerPos.x;
            f64 odz = static_cast<f64>(z) + 0.5 - playerPos.z;
            f64 oDistSq = odx * odx + odz * odz;

            if (oDistSq < MIN_SPAWN_DISTANCE_SQ || oDistSq > MAX_SPAWN_DISTANCE_SQ) {
                continue;
            }

            // 生成前再次检查容量（群体内逐只检查）
            if (!densityManager.canSpawn(classification)) {
                break;
            }
            if (!m_localMobCap.canSpawn(classification, chunkPos)) {
                break;
            }

            // 检查是否可以生成
            if (!_canSpawnAt(world, x, y, z, *entry)) {
                continue;
            }

            // 尝试生成
            i32 result = _trySpawnAt(world, x, y, z, *entry, random);
            if (result > 0) {
                spawned += result;

                // 更新密度和分类计数
                densityManager.onSpawn(entry->entityTypeId,
                    classification,
                    Vector3(static_cast<f32>(x), static_cast<f32>(y), static_cast<f32>(z)),
                    spawnCosts ? *spawnCosts : SpawnCosts());

                // 登记到本地容量（占用本地配额）
                m_localMobCap.addMob(chunkPos, classification);
            }
        }

        // 本轮已生成即结束（原版 spawnCategoryForPosition 每轮独立，
        // 一旦某轮成功生成群体后通常不再同轮堆叠；这里逐轮独立，符合原版语义）
        if (spawned > 0) {
            return;
        }
    }
}

i32 NaturalSpawner::_trySpawnAt(
    mc::server::ServerWorld& world, i32 x, i32 y, i32 z, const SpawnEntry& entry, math::Random& random)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Entity,
        "NaturalSpawner::_trySpawnAt",
        "pos",
        fmt::format("({}, {}, {})", x, y, z),
        "entityType",
        entry.entityTypeId);
    // 获取实体类型
    auto& registry = entity::EntityRegistry::instance();
    const entity::EntityType* entityType = registry.getType(entry.entityTypeId);
    if (!entityType) {
        return 0;
    }

    // 检查是否可召唤
    if (!entityType->canSummon()) {
        return 0;
    }

    // 检查生成位置
    if (!_canSpawnAt(world, x, y, z, entry)) {
        return 0;
    }

    // 确定生成数量
    i32 count = entry.minCount;
    if (entry.maxCount > entry.minCount) {
        count = random.nextInt(entry.minCount, entry.maxCount);
    }

    i32 spawned = 0;

    // 获取实体尺寸
    entity::EntitySize size = entityType->size();
    f32 width = size.width();
    f32 height = size.height();

    // 尝试生成多个实体
    for (i32 i = 0; i < count; ++i) {
        // 添加随机偏移，使群体分散
        f32 offsetX = (i % 3 - 1) * width;
        f32 offsetZ = (i / 3 - 1) * width;

        f32 spawnX = static_cast<f32>(x) + 0.5f + offsetX;
        f32 spawnZ = static_cast<f32>(z) + 0.5f + offsetZ;

        // 检查碰撞空间
        AxisAlignedBB entityBox =
            AxisAlignedBB::fromPosition(Vector3(spawnX, static_cast<f32>(y), spawnZ), width, height);

        if (world.hasBlockCollision(entityBox)) {
            continue;
        }

        // 创建实体
        std::unique_ptr<Entity> entity = entityType->create(&world);
        if (!entity) {
            continue;
        }

        // 设置实体位置和旋转
        entity->setPosition(spawnX, static_cast<f32>(y), spawnZ);
        entity->setRotation(random.nextFloat() * 360.0f, 0.0f);

        // 实例级生成规则检查（对应 MC PathfinderMob.checkSpawnRules）
        // 在实体创建后、finalizeSpawn之前，检查该位置是否适合该实体的寻路偏好
        auto* creatureEntity = dynamic_cast<CreatureEntity*>(entity.get());
        if (creatureEntity != nullptr) {
            if (!creatureEntity->canSpawnAt(spawnX, static_cast<f32>(y), spawnZ)) {
                continue;
            }
        }

        // 对 MobEntity 调用 finalizeSpawn 进行基于难度的初始化
        auto* mobEntity = dynamic_cast<MobEntity*>(entity.get());
        if (mobEntity != nullptr) {
            entity::combat::DifficultyInstance difficultyInstance =
                entity::combat::DifficultyInstance::at(world, BlockPos(x, y, z));
            mobEntity->finalizeSpawn(world, difficultyInstance, world::spawn::SpawnReason::Natural);
        }

        // 生成实体到世界
        EntityInstanceId entityId = world.spawnEntity(std::move(entity));
        if (entityId != INVALID_ENTITY_ID) {
            ++spawned;
        }
    }

    return spawned;
}

const SpawnEntry* NaturalSpawner::_selectEntry(const std::vector<SpawnEntry>& entries, math::Random& random) const
{
    if (entries.empty()) {
        return nullptr;
    }

    // 计算总权重
    i32 totalWeight = 0;
    for (const auto& entry : entries) {
        totalWeight += entry.weight;
    }

    if (totalWeight <= 0) {
        return nullptr;
    }

    // 随机选择（加权随机）
    i32 value = random.nextInt(totalWeight);
    i32 current = 0;

    for (const auto& entry : entries) {
        current += entry.weight;
        if (value < current) {
            return &entry;
        }
    }

    return nullptr;
}

bool NaturalSpawner::_canSpawnAt(mc::server::ServerWorld& world, i32 x, i32 y, i32 z, const SpawnEntry& entry) const
{
    // 获取实体类型
    auto& registry = entity::EntityRegistry::instance();
    const entity::EntityType* entityType = registry.getType(entry.entityTypeId);
    if (!entityType) {
        return false;
    }

    // 边界检查
    if (!world::isValidY(y)) {
        return false;
    }

    // 使用 EntitySpawnPlacementRegistry 检查放置条件
    PlacementType placementType = EntitySpawnPlacementRegistry::getPlacementType(entry.entityTypeId);

    // 创建世界读取器适配器
    class ServerWorldAdapter : public ISpawnWorldReader {
    public:
        explicit ServerWorldAdapter(mc::server::ServerWorld& w)
            : m_world(w)
        {}

        [[nodiscard]] const BlockState* getBlockState(i32 bx, i32 by, i32 bz) const override
        {
            return m_world.getBlockState(bx, by, bz);
        }

        [[nodiscard]] bool isInWorldBounds(i32 bx, i32 by, i32 bz) const override
        {
            return m_world.isWithinWorldBounds(bx, by, bz);
        }

        [[nodiscard]] i32 getHeight(HeightmapType type, i32 bx, i32 bz) const override
        {
            ChunkCoord chunkX = world::toChunkCoord(bx);
            ChunkCoord chunkZ = world::toChunkCoord(bz);
            const ChunkData* chunk = m_world.getChunk(chunkX, chunkZ);
            if (!chunk) {
                return m_world.getHeight(bx, bz);
            }
            i32 localX = world::toLocalCoord(bx);
            i32 localZ = world::toLocalCoord(bz);
            return chunk->getTopBlockY(type, localX, localZ);
        }

        [[nodiscard]] BiomeId getBiome(i32 bx, i32 by, i32 bz) const override
        {
            ChunkCoord chunkX = world::toChunkCoord(bx);
            ChunkCoord chunkZ = world::toChunkCoord(bz);
            const ChunkData* chunk = m_world.getChunk(chunkX, chunkZ);
            if (!chunk) {
                return Biomes::Plains;
            }
            i32 localX = world::toLocalCoord(bx);
            i32 localZ = world::toLocalCoord(bz);
            return chunk->getBiomeAtBlock(localX, by, localZ);
        }

        [[nodiscard]] u64 seed() const override { return m_world.seed(); }

        [[nodiscard]] Difficulty difficulty() const override { return m_world.difficulty(); }

        [[nodiscard]] i64 dayTime() const override { return m_world.dayTime(); }

        [[nodiscard]] i32 getMaxLocalRawBrightness(i32 bx, i32 by, i32 bz) const override
        {
            const u8 blockLight = m_world.getBlockLight(bx, by, bz);
            const u8 skyLight = m_world.getSkyLight(bx, by, bz);
            const i32 skyDarkening = InternalLightUtils::calculateSkyDarkening(
                m_world.dayTimeOfDay(), m_world.isRaining(), m_world.isThundering());
            return InternalLightUtils::calculateRawBrightness(blockLight, skyLight, skyDarkening);
        }

    private:
        mc::server::ServerWorld& m_world;
    };

    ServerWorldAdapter adapter(world);
    Vector3i pos(x, y, z);

    // 检查放置类型条件
    if (!EntitySpawnPlacementRegistry::canSpawnAtLocation(placementType, adapter, pos, entry.entityTypeId)) {
        return false;
    }

    // 检查分类特定的条件
    entity::EntityClassification classification = entityType->classification();

    switch (classification) {
        case entity::EntityClassification::Monster:
            // 怪物需要低光照
            return _checkLightLevel(world, x, y, z, true);

        case entity::EntityClassification::Creature:
            // 动物需要足够光照
            return _checkLightLevel(world, x, y, z, false);

        case entity::EntityClassification::Ambient:
            // 环境生物（蝙蝠）需要低光照
            return _checkLightLevel(world, x, y, z, true);

        case entity::EntityClassification::WaterCreature:
        case entity::EntityClassification::WaterAmbient:
        case entity::EntityClassification::Axolotls:
        case entity::EntityClassification::UndergroundWaterCreature:
            // 水生生物需要在水中 - 已由 EntitySpawnPlacementRegistry::checkInWaterSpawn 处理
            return true;

        case entity::EntityClassification::Misc:
            // 其他类型无特殊限制
            return true;
    }

    return true;
}

bool NaturalSpawner::_checkLightLevel(mc::server::ServerWorld& world, i32 x, i32 y, i32 z, bool isMonster) const
{
    // 获取天空光照和方块光照
    u8 skyLight = world.getSkyLight(x, y, z);
    u8 blockLight = world.getBlockLight(x, y, z);

    // 使用 SpawnConditions 的光照检查
    return SpawnConditions::checkLightLevel(static_cast<i32>(skyLight), static_cast<i32>(blockLight), isMonster);
}

i32 NaturalSpawner::_getSpawnHeight(mc::server::ServerWorld& world, i32 x, i32 z, HeightmapType heightmapType) const
{
    // 获取区块
    ChunkCoord chunkX = world::toChunkCoord(x);
    ChunkCoord chunkZ = world::toChunkCoord(z);

    const ChunkData* chunk = world.getChunk(chunkX, chunkZ);
    if (!chunk) {
        return -1;
    }

    // 获取局部坐标
    i32 localX = world::toLocalCoord(x);
    i32 localZ = world::toLocalCoord(z);

    // 使用指定类型的高度图获取高度
    return chunk->getTopBlockY(heightmapType, localX, localZ);
}

const SpawnEntry* NaturalSpawner::_getRandomSpawnEntry(mc::server::ServerWorld& world,
    const ChunkData* chunk,
    entity::EntityClassification classification,
    const Vector3i& pos,
    math::Random& random) const
{
    // 从 ChunkData 获取生物群系
    BiomeId biomeId = Biomes::Plains;
    if (chunk) {
        i32 localX = world::toLocalCoord(pos.x);
        i32 localZ = world::toLocalCoord(pos.z);
        biomeId = chunk->getBiomeAtBlock(localX, pos.y, localZ);
    }

    // 检查生物群系是否存在
    if (!BiomeRegistry::instance().hasBiome(biomeId)) {
        // 使用默认生物群系
        static MobSpawnInfo defaultInfo = MobSpawnInfo::createPlains();
        return _selectEntry(defaultInfo.getSpawns(classification), random);
    }

    // 获取生物群系信息
    const Biome& biome = BiomeRegistry::instance().get(biomeId);

    // 从生物群系获取生成信息
    const MobSpawnInfo& spawnInfo = biome.spawnInfo();
    const std::vector<SpawnEntry>& entries = spawnInfo.getSpawns(classification);

    if (entries.empty()) {
        return nullptr;
    }

    return _selectEntry(entries, random);
}

EntityDensityManager NaturalSpawner::_createDensityManager(mc::server::ServerWorld& world)
{
    // 每 tick 重建密度追踪器：与原版 createState 重建 PotentialCalculator 一致，
    // 避免上 tick 残留的 charge 跨 tick 无限累积。
    m_densityTracker.clear();

    // 每 tick 重建本地容量计算器（createState 重建 LocalMobCapCalculator）。
    // 用 clear 复用桶容量，避免每 tick 逐节点堆分配。
    m_localMobCap.clear();

    auto& entityManager = world.entityManager();

    // 登记每个玩家固定刷怪距离内的区块到本地容量计算器。
    auto players = entityManager.getPlayers();
    for (const Entity* player : players) {
        if (player == nullptr || player->isRemoved()) {
            continue;
        }
        const u64 playerId = static_cast<u64>(static_cast<const Player*>(player)->playerId());
        const Vector3 pos = player->position();
        const ChunkCoord pcx = world::toChunkCoord(static_cast<i32>(pos.x));
        const ChunkCoord pcz = world::toChunkCoord(static_cast<i32>(pos.z));
        for (i32 dx = -SPAWN_DISTANCE_CHUNK; dx <= SPAWN_DISTANCE_CHUNK; ++dx) {
            for (i32 dz = -SPAWN_DISTANCE_CHUNK; dz <= SPAWN_DISTANCE_CHUNK; ++dz) {
                m_localMobCap.addPlayerChunk(playerId, ChunkPos(pcx + dx, pcz + dz));
            }
        }
    }

    // 统计当前实体数量快照（countEntitiesByClassification 已跳过持久化 Mob）。
    // 同时把每个非持久化 Mob 登记到其所在区块的本地容量。
    std::unordered_map<entity::EntityClassification, i32> entityCounts = entityManager.countEntitiesByClassification();

    entityManager.forEachEntity([&](const Entity* entity) {
        if (entity == nullptr || entity->isRemoved()) {
            return true;
        }
        const auto* mob = dynamic_cast<const MobEntity*>(entity);
        if (mob == nullptr) {
            return true;
        }
        if (mob->isNoDespawnRequired() || mob->preventDespawn()) {
            return true; // 持久化 Mob 不占用本地配额
        }
        const std::string& typeId = entity->getTypeId();
        const entity::EntityType* type = entity::EntityRegistry::instance().getType(typeId);
        if (type == nullptr) {
            return true;
        }
        const entity::EntityClassification classification = type->classification();
        if (classification == entity::EntityClassification::Misc) {
            return true;
        }
        const Vector3 pos = entity->position();
        const ChunkPos chunk(
            world::toChunkCoord(static_cast<i32>(pos.x)), world::toChunkCoord(static_cast<i32>(pos.z)));
        m_localMobCap.addMob(chunk, classification);
        return true;
    });

    // 刷怪区块计数：玩家固定刷怪距离（SPAWN_DISTANCE_CHUNK=8）内已加载区块数。
    // 与原版 DistanceManager.getNaturalSpawnChunkCount 一致，满载约 289，与视距无关。
    const i32 spawnableChunkCount = _countSpawnableChunks(world);

    return EntityDensityManager(spawnableChunkCount, std::move(entityCounts), m_densityTracker);
}

// 收集所有玩家固定刷怪距离（SPAWN_DISTANCE_CHUNK=8）内的已加载区块，去重。
// 返回的集合未被排序；调用方可据此计数或打乱后逐块生成。
std::vector<ChunkPos> NaturalSpawner::_collectSpawnableChunks(mc::server::ServerWorld& world) const
{
    std::vector<ChunkPos> allChunks;
    std::unordered_set<ChunkPos> seen;

    auto players = world.entityManager().getPlayers();
    for (const Entity* player : players) {
        if (player == nullptr || player->isRemoved()) {
            continue;
        }
        const Vector3 pos = player->position();
        const ChunkCoord playerChunkX = world::toChunkCoord(static_cast<i32>(pos.x));
        const ChunkCoord playerChunkZ = world::toChunkCoord(static_cast<i32>(pos.z));

        for (i32 dx = -SPAWN_DISTANCE_CHUNK; dx <= SPAWN_DISTANCE_CHUNK; ++dx) {
            for (i32 dz = -SPAWN_DISTANCE_CHUNK; dz <= SPAWN_DISTANCE_CHUNK; ++dz) {
                const ChunkCoord chunkX = playerChunkX + dx;
                const ChunkCoord chunkZ = playerChunkZ + dz;
                if (!world.hasChunk(chunkX, chunkZ)) {
                    continue;
                }
                const ChunkPos chunk(chunkX, chunkZ);
                if (seen.insert(chunk).second) {
                    allChunks.push_back(chunk);
                }
            }
        }
    }

    return allChunks;
}

i32 NaturalSpawner::_countSpawnableChunks(mc::server::ServerWorld& world) const
{
    return static_cast<i32>(_collectSpawnableChunks(world).size());
}

std::vector<ChunkPos> NaturalSpawner::_getSpawnableChunks(mc::server::ServerWorld& world, math::Random& random) const
{
    std::vector<ChunkPos> chunks = _collectSpawnableChunks(world);

    // 打乱（Util.shuffle），保证遍历顺序随机。
    random.shuffle(chunks);

    return chunks;
}

bool NaturalSpawner::_isSpawnCategoryReady(entity::EntityClassification classification, u64 worldTime) const
{
    // 持久化分类（Creature 等）每 400 tick 才参与一次生成。
    // getFilteredSpawningCategories 第三条件：
    //   flag1(gameTime%400==0) || !mobcategory.isPersistent()
    if (entity::isPersistent(classification)) {
        return worldTime - m_lastCreatureSpawnTime >= CREATURE_SPAWN_INTERVAL;
    }

    // 非持久化分类每次都检查
    return true;
}

} // namespace mc::world::spawn
