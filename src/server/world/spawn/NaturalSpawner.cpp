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
#include "../ServerChunkManager.hpp"
#include "../ServerWorld.hpp"
#include "SpawnConditions.hpp"
#include "common/entity/combat/DifficultyHelper.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/core/EntityClassification.hpp"
#include "common/entity/core/EntityRegistry.hpp"
#include "common/entity/core/EntitySpawnPlacementRegistry.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/perfetto/TraceEvents.hpp"
#include "common/util/math/MathUtils.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/WorldConstants.hpp"
#include "common/world/biome/Biome.hpp"
#include "common/world/biome/BiomeRegistry.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/Material.hpp"
#include "common/world/chunk/ChunkLoadTicketManager.hpp"
#include "common/world/entity/EntityManager.hpp"
#include "common/world/spawn/MobSpawnInfo.hpp"
#include <algorithm>
#include <chrono>
#include <utility>
#include <fmt/format.h>
#include <spdlog/spdlog.h>

namespace mc::world::spawn {

// ============================================================================
// 常量定义
// ============================================================================

/// 每次生成尝试的区块数量乘数
static constexpr i32 SPAWN_CHUNK_DIVISOR = 17;

/// 每区块最大生成尝试次数
static constexpr i32 MAX_SPAWN_ATTEMPTS_PER_CHUNK = 3;

/// MC 原版生成区块计算常量
static constexpr i32 MAGIC_NUMBER = 289; // 17^2

// ============================================================================
// MobDensityTracker 实现
// ============================================================================

void MobDensityTracker::addCharge(const Vector3& pos, f64 charge)
{
    m_charges.push_back({pos, charge});
}

f64 MobDensityTracker::getTotalCharge(const Vector3& pos, f64 multiplier) const
{
    // 参考 MC 1.16.5 MobDensityTracker.func_234999_b_
    // 计算公式：sum(charge * multiplier / sqrt(distance))
    if (multiplier == 0.0) {
        return 0.0;
    }

    f64 totalCharge = 0.0;

    for (const auto& entry : m_charges) {
        f64 dx = entry.position.x - pos.x;
        f64 dy = entry.position.y - pos.y;
        f64 dz = entry.position.z - pos.z;
        f64 distSq = dx * dx + dy * dy + dz * dz;

        if (distSq < 0.0001) {
            // MC 原版：距离为 0 时直接累加 multiplier
            totalCharge += multiplier;
        } else {
            // MC 原版：charge * multiplier / sqrt(distance)
            f64 distance = std::sqrt(distSq);
            totalCharge += entry.charge * multiplier / distance;
        }
    }

    return totalCharge;
}

// ============================================================================
// EntityDensityManager 实现
// ============================================================================

EntityDensityManager::EntityDensityManager(i32 viewDistance,
    std::unordered_map<entity::EntityClassification, i32> entityCounts,
    MobDensityTracker& densityTracker)
    : m_viewDistance(viewDistance)
    , m_entityCounts(std::move(entityCounts))
    , m_densityTracker(densityTracker)
{}

bool EntityDensityManager::canSpawn(entity::EntityClassification classification) const
{
    // 获取当前数量
    auto it = m_entityCounts.find(classification);
    i32 currentCount = (it != m_entityCounts.end()) ? it->second : 0;

    // MISC 分类不生成
    if (classification == entity::EntityClassification::Misc) {
        return false;
    }

    // 获取最大实例数
    i32 maxInstances = getMaxCount(classification);

    // 计算刷怪区块数量
    // MC 原版: maxCount * chunkCount / 289
    i32 chunkCount = (2 * m_viewDistance + 1) * (2 * m_viewDistance + 1);
    i32 adjustedMax = maxInstances * chunkCount / MAGIC_NUMBER;

    // 确保至少有一个
    adjustedMax = std::max(adjustedMax, 1);

    return currentCount < adjustedMax;
}

bool EntityDensityManager::canSpawnWithDensity(
    const std::string& entityTypeId, const Vector3& pos, const SpawnCosts& spawnCosts) const
{
    if (!spawnCosts.isValid()) {
        return true; // 无成本限制
    }

    // MC 1.16.5: 检查当前密度是否超过能量预算
    // 原版逻辑：getTotalCharge(pos, charge) <= energyBudget
    f64 currentDensity = m_densityTracker.getTotalCharge(pos, spawnCosts.charge);
    return currentDensity <= spawnCosts.energyBudget;
}

void EntityDensityManager::onSpawn(const std::string& entityTypeId, entity::EntityClassification classification,
    const Vector3& pos, const SpawnCosts& spawnCosts)
{
    // MC 1.16.5: 更新密度追踪器
    if (spawnCosts.isValid()) {
        m_densityTracker.addCharge(pos, spawnCosts.charge);
    }

    // MC 1.16.5: 更新实体分类计数
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
    MC_TRACE_EVENT("server.entity", "NaturalSpawner::spawnInChunk", "chunkX", chunkX, "chunkZ", chunkZ);
    // 获取区块的世界坐标范围
    i32 minX = world::toWorldCoord(chunkX);
    i32 minZ = world::toWorldCoord(chunkZ);

    // 参考 MC 1.16.5 performWorldGenSpawning
    // 仅生成 Creature 分类的实体（被动动物）
    const auto& creatures = spawnInfo.getCreatureSpawns();
    if (creatures.empty()) {
        return;
    }

    // 使用 creature_spawn_probability 进行概率循环
    f32 spawnProbability = spawnInfo.getCreatureSpawnProbability();

    while (random.nextFloat() < spawnProbability) {
        // 选择生成条目
        const SpawnEntry* entry = selectEntry(creatures, random);
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
        i32 baseX = minX + random.nextInt(16);
        i32 baseZ = minZ + random.nextInt(16);

        // 生成群体
        for (i32 i = 0; i < count; ++i) {
            // 在基础位置附近随机偏移
            i32 x = baseX + random.nextInt(5) - random.nextInt(5);
            i32 z = baseZ + random.nextInt(5) - random.nextInt(5);

            // 获取生成高度
            HeightmapType heightmapType = EntitySpawnPlacementRegistry::getHeightmapType(entry->entityTypeId);
            i32 y = getSpawnHeight(world, x, z, heightmapType);
            if (y < world::MIN_BUILD_HEIGHT) {
                continue;
            }

            // 检查是否可以生成
            if (!canSpawnAt(world, x, y, z, *entry)) {
                continue;
            }

            // 尝试生成
            trySpawnAt(world, x, y, z, *entry, random);
        }

        // 每次成功选择后降低概率
        spawnProbability *= 0.9f;
    }
}

void NaturalSpawner::tick(mc::server::ServerWorld& world, bool hostile, bool passive)
{
    MC_TRACE_EVENT("server.entity", "NaturalSpawner::tick", "hostile", hostile, "passive", passive);
    // 参考 MC 1.16.5 WorldEntitySpawner.func_234979_a_

    // 和平模式下不生成敌对生物
    if (!entity::combat::DifficultyHelper::allowsMobSpawning(world.difficulty())) {
        hostile = false;
    }

    // 获取当前时间
    u64 worldTime = world.currentTick();

    // 创建随机数生成器
    math::Random random(static_cast<u64>(worldTime));

    // 获取玩家列表
    auto players = world.entityManager().getPlayers();
    if (players.empty()) {
        return;
    }

    // 获取玩家视距
    i32 viewDistance = 10; // 默认视距
    auto* chunkManager = world.chunkManager();
    if (chunkManager) {
        viewDistance = chunkManager->viewDistance();
    }

    // 创建实体密度管理器
    EntityDensityManager densityManager = createDensityManager(world);

    // 遍历每个玩家周围的区块
    for (Entity* player : players) {
        if (!player || player->isRemoved()) {
            continue;
        }

        Vector3 playerPos = player->position();
        ChunkCoord playerChunkX = static_cast<ChunkCoord>(playerPos.x) >> 4;
        ChunkCoord playerChunkZ = static_cast<ChunkCoord>(playerPos.z) >> 4;

        // 遍历玩家周围的区块
        for (i32 dx = -viewDistance; dx <= viewDistance; ++dx) {
            for (i32 dz = -viewDistance; dz <= viewDistance; ++dz) {
                ChunkCoord chunkX = playerChunkX + dx;
                ChunkCoord chunkZ = playerChunkZ + dz;

                // 获取区块
                const ChunkData* chunk = world.getChunk(chunkX, chunkZ);
                if (!chunk) {
                    continue;
                }

                // 遍历每个分类
                static const entity::EntityClassification classifications[] = {entity::EntityClassification::Monster,
                    entity::EntityClassification::Creature,
                    entity::EntityClassification::Ambient,
                    entity::EntityClassification::WaterCreature,
                    entity::EntityClassification::WaterAmbient};

                for (auto classification : classifications) {
                    // 检查是否应该生成该分类
                    bool isPeaceful = (classification == entity::EntityClassification::Creature ||
                        classification == entity::EntityClassification::WaterCreature ||
                        classification == entity::EntityClassification::WaterAmbient ||
                        classification == entity::EntityClassification::Ambient);

                    if (!hostile && !isPeaceful) continue;
                    if (!passive && isPeaceful) continue;

                    // 检查实例数量限制
                    if (!densityManager.canSpawn(classification)) {
                        continue;
                    }

                    // 动物生成检查 - 每 400 tick 检查一次
                    if (classification == entity::EntityClassification::Creature) {
                        if (!isSpawnCategoryReady(classification, worldTime)) {
                            continue;
                        }
                    }

                    // 随机决定是否生成
                    if (random.nextInt(SPAWN_CHUNK_DIVISOR) != 0) {
                        continue;
                    }

                    // 执行生成
                    spawnForClassificationInChunk(classification, world, chunk, playerPos, densityManager, random);
                }
            }
        }
    }

    // 更新动物生成时间
    if (passive && worldTime - m_lastCreatureSpawnTime >= CREATURE_SPAWN_INTERVAL) {
        m_lastCreatureSpawnTime = worldTime;
    }
}

void NaturalSpawner::spawnForClassificationInChunk(entity::EntityClassification classification,
    mc::server::ServerWorld& world,
    const ChunkData* chunk,
    const Vector3& playerPos,
    EntityDensityManager& densityManager,
    math::Random& random)
{
    MC_TRACE_EVENT("server.entity",
        "NaturalSpawner::spawnForClassificationInChunk",
        "classification",
        static_cast<i32>(classification));
    if (!chunk) {
        return;
    }

    // 获取该分类对应的生成高度图类型
    HeightmapType heightmapType = HeightmapType::MotionBlockingNoLeaves;

    // 获取区块的世界坐标
    i32 chunkMinX = chunk->x() << 4;
    i32 chunkMinZ = chunk->z() << 4;

    // 随机选择区块内位置
    i32 spawnX = chunkMinX + random.nextInt(16);
    i32 spawnZ = chunkMinZ + random.nextInt(16);

    // 获取高度
    i32 spawnY = getSpawnHeight(world, spawnX, spawnZ, heightmapType);
    if (spawnY < 0) {
        return;
    }

    // 检查玩家距离
    f64 dx = static_cast<f64>(spawnX) + 0.5 - playerPos.x;
    f64 dz = static_cast<f64>(spawnZ) + 0.5 - playerPos.z;
    f64 playerDistanceSq = dx * dx + dz * dz;

    // 玩家距离必须 > 24 格
    if (playerDistanceSq < MIN_SPAWN_DISTANCE_SQ) {
        return;
    }

    // 玩家距离必须 <= 128 格
    if (playerDistanceSq > MAX_SPAWN_DISTANCE_SQ) {
        return;
    }

    // 选择生成条目
    Vector3i pos(spawnX, spawnY, spawnZ);
    const SpawnEntry* entry = getRandomSpawnEntry(world, chunk, classification, pos, random);
    if (!entry) {
        return;
    }

    // 从 ChunkData 获取生物群系，再从生物群系获取 SpawnCosts
    const SpawnCosts* spawnCosts = nullptr;
    if (chunk) {
        i32 localX = spawnX & 15;
        i32 localZ = spawnZ & 15;
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
            return;
        }
    }

    // 获取实体类型
    auto& registry = entity::EntityRegistry::instance();
    const entity::EntityType* entityType = registry.getType(entry->entityTypeId);
    if (!entityType) {
        return;
    }

    // 确定生成数量
    i32 count = entry->minCount;
    if (entry->maxCount > entry->minCount) {
        count = random.nextInt(entry->minCount, entry->maxCount);
    }

    i32 spawned = 0;
    i32 groupSize = std::min(count, MAX_GROUP_SIZE);

    // 尝试在位置周围生成群体
    for (i32 i = 0; i < groupSize; ++i) {
        // 计算偏移位置
        i32 x = spawnX + random.nextInt(6) - random.nextInt(6);
        i32 z = spawnZ + random.nextInt(6) - random.nextInt(6);

        // 获取高度
        i32 y = getSpawnHeight(world, x, z, heightmapType);
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

        // 检查是否可以生成
        if (!canSpawnAt(world, x, y, z, *entry)) {
            continue;
        }

        // 尝试生成
        i32 result = trySpawnAt(world, x, y, z, *entry, random);
        if (result > 0) {
            spawned += result;

            // 更新密度和分类计数
            // MC 1.16.5: onSpawn 时同时更新密度追踪器和分类计数
            densityManager.onSpawn(entry->entityTypeId, classification,
                Vector3(static_cast<f32>(x), static_cast<f32>(y), static_cast<f32>(z)),
                spawnCosts ? *spawnCosts : SpawnCosts());

            // 检查是否达到群体大小限制
            if (spawned >= MAX_GROUP_SIZE) {
                break;
            }
        }
    }
}

i32 NaturalSpawner::trySpawnAt(
    mc::server::ServerWorld& world, i32 x, i32 y, i32 z, const SpawnEntry& entry, math::Random& random)
{
    MC_TRACE_EVENT("server.entity",
        "NaturalSpawner::trySpawnAt",
        "pos",
        fmt::format("({}, {}, {})", x, y, z),
        "entityType",
        entry.entityTypeId);
    // 获取实体类型
    auto& registry = entity::EntityRegistry::instance();
    const entity::EntityType* entityType = registry.getType(entry.entityTypeId);
    if (!entityType) {
        spdlog::debug("Unknown entity type for spawning: {}", entry.entityTypeId);
        return 0;
    }

    // 检查是否可召唤
    if (!entityType->canSummon()) {
        return 0;
    }

    // 检查生成位置
    if (!canSpawnAt(world, x, y, z, entry)) {
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

        // 生成实体到世界
        EntityId entityId = world.spawnEntity(std::move(entity));
        if (entityId != INVALID_ENTITY_ID) {
            ++spawned;
        }
    }

    return spawned;
}

const SpawnEntry* NaturalSpawner::selectEntry(const std::vector<SpawnEntry>& entries, math::Random& random) const
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

bool NaturalSpawner::canSpawnAt(mc::server::ServerWorld& world, i32 x, i32 y, i32 z, const SpawnEntry& entry) const
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
            ChunkCoord chunkX = static_cast<ChunkCoord>(bx >> 4);
            ChunkCoord chunkZ = static_cast<ChunkCoord>(bz >> 4);
            const ChunkData* chunk = m_world.getChunk(chunkX, chunkZ);
            if (!chunk) {
                return m_world.getHeight(bx, bz);
            }
            i32 localX = bx & 15;
            i32 localZ = bz & 15;
            return chunk->getTopBlockY(type, localX, localZ);
        }

        [[nodiscard]] BiomeId getBiome(i32 bx, i32 by, i32 bz) const override
        {
            ChunkCoord chunkX = static_cast<ChunkCoord>(bx >> 4);
            ChunkCoord chunkZ = static_cast<ChunkCoord>(bz >> 4);
            const ChunkData* chunk = m_world.getChunk(chunkX, chunkZ);
            if (!chunk) {
                return Biomes::Plains;
            }
            i32 localX = bx & 15;
            i32 localZ = bz & 15;
            return chunk->getBiomeAtBlock(localX, by, localZ);
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
            return checkLightLevel(world, x, y, z, true);

        case entity::EntityClassification::Creature:
            // 动物需要足够光照
            return checkLightLevel(world, x, y, z, false);

        case entity::EntityClassification::Ambient:
            // 环境生物（蝙蝠）需要低光照
            return checkLightLevel(world, x, y, z, true);

        case entity::EntityClassification::WaterCreature:
        case entity::EntityClassification::WaterAmbient:
            // 水生生物需要在水中 - 已由 EntitySpawnPlacementRegistry::checkInWaterSpawn 处理
            return true;

        case entity::EntityClassification::Misc:
            // 其他类型无特殊限制
            return true;
    }

    return true;
}

bool NaturalSpawner::checkLightLevel(mc::server::ServerWorld& world, i32 x, i32 y, i32 z, bool isMonster) const
{
    // 获取天空光照和方块光照
    u8 skyLight = world.getSkyLight(x, y, z);
    u8 blockLight = world.getBlockLight(x, y, z);

    // 使用 SpawnConditions 的光照检查
    return SpawnConditions::checkLightLevel(static_cast<i32>(skyLight), static_cast<i32>(blockLight), isMonster);
}

i32 NaturalSpawner::getSpawnHeight(mc::server::ServerWorld& world, i32 x, i32 z, HeightmapType heightmapType) const
{
    // 获取区块
    ChunkCoord chunkX = static_cast<ChunkCoord>(x >> 4);
    ChunkCoord chunkZ = static_cast<ChunkCoord>(z >> 4);

    const ChunkData* chunk = world.getChunk(chunkX, chunkZ);
    if (!chunk) {
        return -1;
    }

    // 获取局部坐标
    i32 localX = x & 15;
    i32 localZ = z & 15;

    // 使用指定类型的高度图获取高度
    return chunk->getTopBlockY(heightmapType, localX, localZ);
}

Vector3i NaturalSpawner::getRandomSpawnPosition(
    mc::server::ServerWorld& world, const ChunkData* chunk, HeightmapType heightmapType, math::Random& random) const
{
    if (!chunk) {
        return Vector3i(0, -1, 0);
    }

    // 获取区块坐标范围
    i32 minX = chunk->x() << 4;
    i32 minZ = chunk->z() << 4;

    // 随机选择区块内位置
    i32 x = minX + random.nextInt(16);
    i32 z = minZ + random.nextInt(16);

    // 获取高度
    i32 y = getSpawnHeight(world, x, z, heightmapType);

    return Vector3i(x, y, z);
}

bool NaturalSpawner::isValidSpawnPosition(
    mc::server::ServerWorld& world, const Vector3i& pos, f64 playerDistanceSq) const
{
    // 检查距离限制
    // 参考 MC 1.16.5: 玩家必须在 24-128 格范围内
    if (playerDistanceSq < MIN_SPAWN_DISTANCE_SQ) {
        return false;
    }

    // 超过最大距离的怪物会立刻消失
    if (playerDistanceSq > MAX_SPAWN_DISTANCE_SQ) {
        return false;
    }

    // 检查世界边界
    if (!world.isWithinWorldBounds(pos.x, pos.y, pos.z)) {
        return false;
    }

    return true;
}

const SpawnEntry* NaturalSpawner::getRandomSpawnEntry(mc::server::ServerWorld& world,
    const ChunkData* chunk,
    entity::EntityClassification classification,
    const Vector3i& pos,
    math::Random& random) const
{
    // 从 ChunkData 获取生物群系
    BiomeId biomeId = Biomes::Plains;
    if (chunk) {
        i32 localX = pos.x & 15;
        i32 localZ = pos.z & 15;
        biomeId = chunk->getBiomeAtBlock(localX, pos.y, localZ);
    }

    // 检查生物群系是否存在
    if (!BiomeRegistry::instance().hasBiome(biomeId)) {
        // 使用默认生物群系
        static MobSpawnInfo defaultInfo = MobSpawnInfo::createPlains();
        const std::vector<SpawnEntry>* entries = nullptr;
        switch (classification) {
            case entity::EntityClassification::Monster:
                entries = &defaultInfo.getMonsterSpawns();
                break;
            case entity::EntityClassification::Creature:
                entries = &defaultInfo.getCreatureSpawns();
                break;
            case entity::EntityClassification::Ambient:
                entries = &defaultInfo.getAmbientSpawns();
                break;
            case entity::EntityClassification::WaterCreature:
                entries = &defaultInfo.getWaterCreatureSpawns();
                break;
            case entity::EntityClassification::WaterAmbient:
                entries = &defaultInfo.getWaterAmbientSpawns();
                break;
            case entity::EntityClassification::Misc:
            default:
                return nullptr;
        }
        return selectEntry(*entries, random);
    }

    // 获取生物群系信息
    const Biome& biome = BiomeRegistry::instance().get(biomeId);

    // 从生物群系获取生成信息
    const MobSpawnInfo& spawnInfo = biome.spawnInfo();
    const std::vector<SpawnEntry>& entries = spawnInfo.getSpawns(classification);

    if (entries.empty()) {
        return nullptr;
    }

    return selectEntry(entries, random);
}

EntityDensityManager NaturalSpawner::createDensityManager(mc::server::ServerWorld& world)
{
    // 统计当前实体数量快照
    std::unordered_map<entity::EntityClassification, i32> entityCounts =
        world.entityManager().countEntitiesByClassification();

    // 获取视距
    i32 viewDistance = 10;
    auto* chunkManager = world.chunkManager();
    if (chunkManager) {
        viewDistance = chunkManager->viewDistance();
    }

    return EntityDensityManager(viewDistance, std::move(entityCounts), m_densityTracker);
}

std::vector<ChunkPos> NaturalSpawner::getSpawnableChunks(
    mc::server::ServerWorld& world, i32 maxChunks, math::Random& random) const
{
    std::vector<ChunkPos> result;
    std::vector<ChunkPos> allChunks;

    // 获取所有玩家视距内的区块
    auto players = world.entityManager().getPlayers();
    auto* chunkManager = world.chunkManager();
    if (!chunkManager) {
        return result;
    }

    auto& ticketManager = chunkManager->ticketManager();
    i32 viewDistance = ticketManager.viewDistance();

    for (Entity* player : players) {
        if (!player || player->isRemoved()) {
            continue;
        }

        Vector3 pos = player->position();
        ChunkCoord playerChunkX = static_cast<ChunkCoord>(pos.x) >> 4;
        ChunkCoord playerChunkZ = static_cast<ChunkCoord>(pos.z) >> 4;

        // 遍历玩家周围的区块
        for (i32 dx = -viewDistance; dx <= viewDistance; ++dx) {
            for (i32 dz = -viewDistance; dz <= viewDistance; ++dz) {
                ChunkCoord chunkX = playerChunkX + dx;
                ChunkCoord chunkZ = playerChunkZ + dz;

                // 检查区块是否加载
                if (world.hasChunk(chunkX, chunkZ)) {
                    allChunks.emplace_back(chunkX, chunkZ);
                }
            }
        }
    }

    // 随机打乱并选择最多 maxChunks 个
    if (static_cast<i32>(allChunks.size()) > maxChunks) {
        // Fisher-Yates shuffle
        for (size_t i = allChunks.size(); i > 1; --i) {
            size_t j = static_cast<size_t>(random.nextInt(static_cast<i32>(i)));
            std::swap(allChunks[i - 1], allChunks[j]);
        }
        allChunks.resize(maxChunks);
    }

    return allChunks;
}

bool NaturalSpawner::isSpawnCategoryReady(entity::EntityClassification classification, u64 worldTime) const
{
    // 动物生成检查 - 每 400 tick 检查一次
    if (classification == entity::EntityClassification::Creature) {
        return worldTime - m_lastCreatureSpawnTime >= CREATURE_SPAWN_INTERVAL;
    }

    // 其他分类每次都检查
    return true;
}

} // namespace mc::world::spawn
