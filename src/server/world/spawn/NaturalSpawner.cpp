#include "NaturalSpawner.hpp"
#include "SpawnConditions.hpp"
#include "../ServerWorld.hpp"
#include "common/entity/core/EntityRegistry.hpp"
#include "common/entity/core/EntityClassification.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/spawn/MobSpawnInfo.hpp"
#include "common/world/WorldConstants.hpp"
#include "common/world/biome/BiomeRegistry.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/Material.hpp"
#include "common/entity/core/EntitySpawnPlacementRegistry.hpp"
#include "common/util/math/random/Random.hpp"
#include <spdlog/spdlog.h>

namespace mc::world::spawn {

// ============================================================================
// MobDensityTracker 实现
// ============================================================================

void MobDensityTracker::addCharge(const Vector3& pos, f64 charge) {
    m_charges.push_back({pos, charge});
}

f64 MobDensityTracker::getTotalCharge(const Vector3& pos) const {
    // 计算基于距离的加权密度
    // 参考 MC 1.16.5 MobDensityTracker - 使用衰减函数
    f64 totalCharge = 0.0;

    for (const auto& entry : m_charges) {
        f64 dx = entry.position.x - pos.x;
        f64 dy = entry.position.y - pos.y;
        f64 dz = entry.position.z - pos.z;
        f64 distSq = dx * dx + dy * dy + dz * dz;

        // 使用欧几里得距离计算衰减
        // 距离越近，密度影响越大
        if (distSq < 4096.0) {  // 64^2
            f64 distance = std::sqrt(distSq);
            // 衰减因子：距离越近衰减越小（影响越大）
            f64 falloff = 1.0 - (distance / 64.0);
            if (falloff > 0.0) {
                totalCharge += entry.charge * falloff;
            }
        }
    }

    return totalCharge;
}

// ============================================================================
// EntityDensityManager 实现
// ============================================================================

EntityDensityManager::EntityDensityManager(
    i32 viewDistance,
    const std::unordered_map<entity::EntityClassification, i32>& entityCounts,
    MobDensityTracker& densityTracker)
    : m_viewDistance(viewDistance)
    , m_entityCounts(entityCounts)
    , m_densityTracker(densityTracker)
{
}

bool EntityDensityManager::canSpawn(entity::EntityClassification classification) const {
    // 获取当前数量
    auto it = m_entityCounts.find(classification);
    i32 currentCount = (it != m_entityCounts.end()) ? it->second : 0;

    // 获取最大实例数
    i32 maxInstances = 0;
    switch (classification) {
        case entity::EntityClassification::Monster:
            maxInstances = NaturalSpawner::MAX_MONSTERS;
            break;
        case entity::EntityClassification::Creature:
            maxInstances = NaturalSpawner::MAX_CREATURES;
            break;
        case entity::EntityClassification::Ambient:
            maxInstances = NaturalSpawner::MAX_AMBIENT;
            break;
        case entity::EntityClassification::WaterCreature:
            maxInstances = NaturalSpawner::MAX_WATER_CREATURES;
            break;
        case entity::EntityClassification::WaterAmbient:
            maxInstances = 20;  // MC 默认值
            break;
        case entity::EntityClassification::Misc:
        default:
            return false;  // MISC 分类不生成
    }

    // 检查是否低于最大实例数
    return currentCount < maxInstances;
}

bool EntityDensityManager::canSpawnWithDensity(const String& entityTypeId,
                                                const Vector3& pos,
                                                const SpawnCosts& spawnCosts) const {
    if (!spawnCosts.isValid()) {
        return true;  // 无成本限制
    }

    // 检查当前密度是否超过能量预算
    f64 currentDensity = m_densityTracker.getTotalCharge(pos);
    return currentDensity < spawnCosts.energyBudget;
}

void EntityDensityManager::onSpawn(const String& entityTypeId,
                                    const Vector3& pos,
                                    const SpawnCosts& spawnCosts) {
    if (spawnCosts.isValid()) {
        m_densityTracker.addCharge(pos, spawnCosts.charge);
    }
}

i32 EntityDensityManager::getCount(entity::EntityClassification classification) const {
    auto it = m_entityCounts.find(classification);
    return (it != m_entityCounts.end()) ? it->second : 0;
}

// ============================================================================
// NaturalSpawner 实现
// ============================================================================

NaturalSpawner::NaturalSpawner()
{
    // 初始化密度追踪器
}

void NaturalSpawner::spawnInChunk(mc::server::ServerWorld& world, i32 chunkX, i32 chunkZ,
                                   const MobSpawnInfo& spawnInfo, math::Random& random) {
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
            if (y < 0) {
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

void NaturalSpawner::tick(mc::server::ServerWorld& world, bool hostile, bool passive) {
    // 参考 MC 1.16.5 WorldEntitySpawner.func_234979_a_

    // 创建实体密度管理器
    EntityDensityManager densityManager = createDensityManager(world);

    // 定义密度检查回调
    auto densityCheck = [this, &densityManager](
        const SpawnEntry& entry,
        const Vector3i& pos,
        const ChunkData* chunk) -> bool {

        // 检查位置有效性
        if (!chunk) {
            return false;
        }

        // 检查实体密度预算
        // 注意：这里简化了 MC 的 SpawnCosts 系统
        (void)entry;  // 暂时未使用 SpawnCosts
        (void)pos;
        (void)chunk;
        return true;
    };

    // 定义生成后密度更新回调
    auto onSpawnDensityAdd = [this](const String& entityTypeId, const Vector3& pos) {
        // 获取 SpawnCosts 并更新密度
        // 注意：这需要从 Biome 获取，当前简化处理
        (void)entityTypeId;
        (void)pos;
    };

    // 遍历所有实体分类
    static const entity::EntityClassification classifications[] = {
        entity::EntityClassification::Monster,
        entity::EntityClassification::Creature,
        entity::EntityClassification::Ambient,
        entity::EntityClassification::WaterCreature,
        entity::EntityClassification::WaterAmbient
    };

    for (auto classification : classifications) {
        // 检查是否应该生成该分类
        bool isPeaceful = (classification == entity::EntityClassification::Creature ||
                          classification == entity::EntityClassification::WaterCreature ||
                          classification == entity::EntityClassification::WaterAmbient);

        bool isAnimal = (classification == entity::EntityClassification::Creature);

        if (!hostile && !isPeaceful) continue;
        if (!passive && isPeaceful) continue;

        // 检查实例数量限制
        if (!densityManager.canSpawn(classification)) {
            continue;
        }

        // 获取玩家周围的可加载区块
        // TODO: 实现遍历玩家周围区块的逻辑
        // 当前简化实现：仅在有玩家时生成
        (void)world;
        // spawnForClassification(classification, world, nullptr, densityCheck, onSpawnDensityAdd);
    }
}

EntityDensityManager NaturalSpawner::createDensityManager(mc::server::ServerWorld& world) {
    // 统计当前实体数量
    std::unordered_map<entity::EntityClassification, i32> entityCounts;

    // 初始化所有分类为 0
    entityCounts[entity::EntityClassification::Monster] = 0;
    entityCounts[entity::EntityClassification::Creature] = 0;
    entityCounts[entity::EntityClassification::Ambient] = 0;
    entityCounts[entity::EntityClassification::WaterCreature] = 0;
    entityCounts[entity::EntityClassification::WaterAmbient] = 0;
    entityCounts[entity::EntityClassification::Misc] = 0;

    // TODO: 从 EntityManager 获取实体数量
    // 当前使用默认值
    (void)world;

    return EntityDensityManager(m_spawnDistance, entityCounts, m_densityTracker);
}

void NaturalSpawner::spawnForClassification(
    entity::EntityClassification classification,
    mc::server::ServerWorld& world,
    const ChunkData* chunk,
    const std::function<bool(const SpawnEntry&, const Vector3i&, const ChunkData*)>& densityCheck,
    const std::function<void(const String&, const Vector3&)>& onSpawnDensityAdd)
{
    if (!chunk) {
        return;
    }

    // 获取该分类对应的生成高度图类型
    HeightmapType heightmapType = HeightmapType::MotionBlockingNoLeaves;

    math::Random random(static_cast<u64>(std::chrono::steady_clock::now().time_since_epoch().count()));

    // 获取随机生成位置
    Vector3i spawnPos = getRandomSpawnPosition(world, chunk, heightmapType, random);
    if (spawnPos.y < 1) {
        return;  // 无效位置
    }

    // 选择生成条目
    const SpawnEntry* entry = getRandomSpawnEntry(world, chunk, classification, spawnPos, random);
    if (!entry) {
        return;
    }

    // 检查密度限制
    if (!densityCheck(*entry, spawnPos, chunk)) {
        return;
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
    i32 groupSize = count;

    // 尝试在位置周围生成群体
    for (i32 i = 0; i < groupSize; ++i) {
        // 计算偏移位置
        i32 x = spawnPos.x + random.nextInt(6) - random.nextInt(6);
        i32 z = spawnPos.z + random.nextInt(6) - random.nextInt(6);

        // 获取高度
        i32 y = getSpawnHeight(world, x, z, heightmapType);
        if (y < 0) {
            continue;
        }

        // 检查是否可以生成
        if (!canSpawnAt(world, x, y, z, *entry)) {
            continue;
        }

        // 检查玩家距离
        Vector3d entityPos(static_cast<f64>(x) + 0.5, static_cast<f64>(y), static_cast<f64>(z) + 0.5);
        // TODO: 获取最近玩家距离

        // 尝试生成
        i32 result = trySpawnAt(world, x, y, z, *entry, random);
        if (result > 0) {
            spawned += result;

            // 更新密度
            onSpawnDensityAdd(entry->entityTypeId, Vector3(static_cast<f32>(x), static_cast<f32>(y), static_cast<f32>(z)));

            // 检查是否达到群体大小限制
            if (spawned >= MAX_GROUP_SIZE) {
                break;
            }
        }
    }
}

i32 NaturalSpawner::trySpawnAt(mc::server::ServerWorld& world, i32 x, i32 y, i32 z,
                                const SpawnEntry& entry, math::Random& random) {
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
        AxisAlignedBB entityBox = AxisAlignedBB::fromPosition(
            Vector3(spawnX, static_cast<f32>(y), spawnZ), width, height);

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

const SpawnEntry* NaturalSpawner::selectEntry(
    const std::vector<SpawnEntry>& entries, math::Random& random) const {
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

bool NaturalSpawner::canSpawnAt(mc::server::ServerWorld& world, i32 x, i32 y, i32 z,
                                 const SpawnEntry& entry) const {
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
        explicit ServerWorldAdapter(mc::server::ServerWorld& w) : m_world(w) {}

        [[nodiscard]] const BlockState* getBlockState(i32 bx, i32 by, i32 bz) const override {
            return m_world.getBlockState(bx, by, bz);
        }

        [[nodiscard]] bool isInWorldBounds(i32 bx, i32 by, i32 bz) const override {
            return m_world.isWithinWorldBounds(bx, by, bz);
        }

        [[nodiscard]] i32 getHeight(HeightmapType type, i32 bx, i32 bz) const override {
            (void)type;  // 暂时忽略高度图类型
            return m_world.getHeight(bx, bz);
        }

        [[nodiscard]] BiomeId getBiome(i32 bx, i32 by, i32 bz) const override {
            (void)bx; (void)by; (void)bz;
            // TODO: 实现生物群系查询
            return 0;
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

bool NaturalSpawner::checkLightLevel(mc::server::ServerWorld& world,
                                      i32 x, i32 y, i32 z,
                                      bool isMonster) const {
    // 获取天空光照和方块光照
    u8 skyLight = world.getSkyLight(x, y, z);
    u8 blockLight = world.getBlockLight(x, y, z);

    // 使用 SpawnConditions 的光照检查
    return SpawnConditions::checkLightLevel(static_cast<i32>(skyLight),
                                            static_cast<i32>(blockLight),
                                            isMonster);
}

i32 NaturalSpawner::getSpawnHeight(mc::server::ServerWorld& world, i32 x, i32 z,
                                    HeightmapType heightmapType) const {
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

    // 使用高度图获取高度
    // TODO: 实现从 ChunkData 获取指定高度图类型的值
    // 当前使用世界高度查询
    (void)chunk;
    (void)heightmapType;
    (void)localX;
    (void)localZ;

    // 使用世界的高度查询
    return world.getHeight(x, z);
}

Vector3i NaturalSpawner::getRandomSpawnPosition(mc::server::ServerWorld& world,
                                                  const ChunkData* chunk,
                                                  HeightmapType heightmapType,
                                                  math::Random& random) const {
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

bool NaturalSpawner::isValidSpawnPosition(mc::server::ServerWorld& world,
                                           const Vector3i& pos,
                                           f64 playerDistanceSq) const {
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

const SpawnEntry* NaturalSpawner::getRandomSpawnEntry(
    mc::server::ServerWorld& world,
    const ChunkData* chunk,
    entity::EntityClassification classification,
    const Vector3i& pos,
    math::Random& random) const {
    // 获取生物群系
    // TODO: 实现从区块获取生物群系
    (void)world;
    (void)chunk;
    (void)pos;

    // 使用默认生物群系的生成信息
    // 注意：实际应该从区块获取生物群系，然后获取 MobSpawnInfo
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

} // namespace mc::world::spawn
