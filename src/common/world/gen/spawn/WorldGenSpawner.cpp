#include "WorldGenSpawner.hpp"
#include "../chunk/IChunkGenerator.hpp"
#include "../../block/BlockRegistry.hpp"
#include "../../block/VanillaBlocks.hpp"
#include "../../WorldConstants.hpp"
#include "../../../entity/core/EntityRegistry.hpp"
#include "../../../entity/core/EntityClassification.hpp"
#include "../../../entity/core/EntitySpawnPlacementRegistry.hpp"
#include "../../../entity/core/MobEntity.hpp"
#include <spdlog/spdlog.h>
#include <cmath>

namespace mc {

// ============================================================================
// WorldGenRegion 适配器 - 实现 ISpawnWorldReader 接口
// ============================================================================

/**
 * @brief WorldGenRegion 的 ISpawnWorldReader 适配器
 *
 * 将 WorldGenRegion 适配为 ISpawnWorldReader 接口，
 * 用于 EntitySpawnPlacementRegistry 的位置检查。
 */
class WorldGenRegionAdapter : public world::spawn::ISpawnWorldReader {
public:
    explicit WorldGenRegionAdapter(WorldGenRegion& region)
        : m_region(region) {}

    [[nodiscard]] const BlockState* getBlockState(i32 x, i32 y, i32 z) const override {
        return m_region.getBlockState(x, y, z);
    }

    [[nodiscard]] bool isInWorldBounds(i32 x, i32 y, i32 z) const override {
        return world::isValidY(y);
    }

    [[nodiscard]] i32 getHeight(HeightmapType type, i32 x, i32 z) const override {
        return m_region.getTopBlockY(x, z, type);
    }

    [[nodiscard]] BiomeId getBiome(i32 x, i32 y, i32 z) const override {
        return m_region.getBiome(x, y, z);
    }

private:
    WorldGenRegion& m_region;
};

// ============================================================================
// 常量定义
// ============================================================================

/// 最大生成尝试次数
static constexpr i32 MAX_SPAWN_ATTEMPTS = 4;

/// 生成位置搜索半径
static constexpr f64 SPAWN_SPREAD_RADIUS = 8.0;

WorldGenSpawner::WorldGenSpawner() = default;
WorldGenSpawner::~WorldGenSpawner() = default;

i32 WorldGenSpawner::spawnInitialMobs(
    WorldGenRegion& region,
    const Biome& biome,
    i32 chunkX,
    i32 chunkZ,
    IChunkGenerator& /*generator*/,
    math::Random& random,
    std::vector<SpawnedEntityData>& outEntities)
{
    if (!m_enabled) {
        spdlog::info("WorldGenSpawner: Disabled, skipping spawn");
        return 0;
    }

    i32 totalSpawned = 0;

    // 获取生物群系的生成配置
    const world::spawn::MobSpawnInfo& spawnInfo = biome.spawnInfo();

    // 只生成 Creature 分类（被动动物）
    // 怪物通过 NaturalSpawner 在夜间/黑暗环境生成
    const std::vector<world::spawn::SpawnEntry>& creatures = spawnInfo.getCreatureSpawns();
    if (creatures.empty()) {
        // spdlog::info("WorldGenSpawner: No creature spawns for biome {}", biome.name());
        return 0;
    }

    // spdlog::info("WorldGenSpawner: Biome {} has {} creature types, probability {:.4f}",
    //               biome.name(), creatures.size(), biome.creatureSpawnProbability());

    // 区块世界坐标起点（使用工具函数）
    const i32 startX = world::toWorldCoord(chunkX);
    const i32 startZ = world::toWorldCoord(chunkZ);

    // 参考 MC 1.16.5 performWorldGenSpawning
    // 使用生物群系的生成概率 (creatureSpawnProbability)
    // 默认概率是 10/128 或者从 biome.getSpawningChance() 获取
    const f32 spawnProbability = biome.creatureSpawnProbability();

    // 预计算总权重（creatures 列表不会改变，可以提前计算）
    i32 totalWeight = 0;
    for (const auto& entry : creatures) {
        totalWeight += entry.weight;
    }

    if (totalWeight <= 0) {
        return 0;
    }

    // 尝试生成多组动物
    // 每组有 spawnProbability 概率生成
    while (random.nextFloat() < spawnProbability) {
        // 随机选择一种动物类型（加权随机选择）
        i32 weightValue = random.nextInt(totalWeight);
        const world::spawn::SpawnEntry* selectedEntry = nullptr;
        i32 currentWeight = 0;

        for (const auto& entry : creatures) {
            currentWeight += entry.weight;
            if (weightValue < currentWeight) {
                selectedEntry = &entry;
                break;
            }
        }

        if (!selectedEntry) {
            spdlog::warn("WorldGenSpawner: Failed to select spawn entry for biome {}", biome.name());
            continue;
        }

        // 获取实体类型
        entity::EntityRegistry& registry = entity::EntityRegistry::instance();
        const entity::EntityType* entityType = registry.getType(selectedEntry->entityTypeId);
        if (!entityType) {
            spdlog::warn("WorldGenSpawner: Unknown entity type: {}", selectedEntry->entityTypeId);
            continue;
        }

        // 确定生成数量（使用 nextInt(min, max) 包含两端）
        i32 count = selectedEntry->minCount;
        if (selectedEntry->maxCount > selectedEntry->minCount) {
            count = random.nextInt(selectedEntry->minCount, selectedEntry->maxCount);
        }

        // 随机生成位置
        i32 groupX = startX + random.nextInt(16);
        i32 groupZ = startZ + random.nextInt(16);

        // 尝试多次找到合适的生成位置
        bool spawnedAny = false;
        for (i32 attempt = 0; attempt < MAX_SPAWN_ATTEMPTS; ++attempt) {
            // 获取生成高度
            i32 spawnY = getSpawnHeight(region, *entityType, groupX, groupZ);
            if (spawnY < 0) {
                // 尝试新位置
                groupX = startX + random.nextInt(16);
                groupZ = startZ + random.nextInt(16);
                continue;
            }

            // 检查是否可以生成
            if (!canSpawnAt(region, *entityType, groupX, spawnY, groupZ)) {
                continue;
            }

            // 在组内生成多个实体
            i32 spawned = spawnGroup(region, *entityType,
                static_cast<f32>(groupX) + 0.5f,
                static_cast<f32>(spawnY),
                static_cast<f32>(groupZ) + 0.5f,
                count, random, outEntities);

            if (spawned > 0) {
                spdlog::info("WorldGenSpawner: Spawned {} x {} at ({}, {}, {})",
                              spawned, entityType->name(), groupX, spawnY, groupZ);
                totalSpawned += spawned;
                spawnedAny = true;
                break;  // 成功生成一组后继续下一组
            }

            // 尝试新位置
            groupX = startX + random.nextInt(16);
            groupZ = startZ + random.nextInt(16);
        }
    }

    if (totalSpawned > 0) {
        // spdlog::info("WorldGenSpawner: Spawned {} entities at chunk ({}, {})", totalSpawned, chunkX, chunkZ);
    }

    return totalSpawned;
}

i32 WorldGenSpawner::spawnGroup(
    WorldGenRegion& region,
    const entity::EntityType& entityType,
    f32 x,
    [[maybe_unused]] f32 y,
    f32 z,
    i32 count,
    math::Random& random,
    std::vector<SpawnedEntityData>& outEntities)
{
    i32 spawned = 0;

    // 获取实体尺寸
    const entity::EntitySize size = entityType.size();
    const f32 width = size.width();

    for (i32 i = 0; i < count; ++i) {
        // 添加随机偏移使群体分散
        // 参考 MC 的群体分散逻辑
        f32 spawnX = x + (random.nextFloat() - 0.5f) * width * 2.0f;
        f32 spawnZ = z + (random.nextFloat() - 0.5f) * width * 2.0f;

        // 确保 X 和 Z 在区块内（限制扩散范围）
        spawnX = std::clamp(spawnX, x - static_cast<f32>(SPAWN_SPREAD_RADIUS), x + static_cast<f32>(SPAWN_SPREAD_RADIUS));
        spawnZ = std::clamp(spawnZ, z - static_cast<f32>(SPAWN_SPREAD_RADIUS), z + static_cast<f32>(SPAWN_SPREAD_RADIUS));

        // 检查碰撞空间
        i32 spawnY = getSpawnHeight(region, entityType,
            static_cast<i32>(spawnX), static_cast<i32>(spawnZ));
        if (spawnY < 0) {
            continue;
        }

        if (!canSpawnAt(region, entityType,
            static_cast<i32>(spawnX), spawnY, static_cast<i32>(spawnZ))) {
            continue;
        }

        // 记录生成的实体数据
        outEntities.emplace_back(
            entityType.name(),
            spawnX,
            static_cast<f32>(spawnY),
            spawnZ
        );

        ++spawned;
    }

    return spawned;
}

i32 WorldGenSpawner::getSpawnHeight(
    WorldGenRegion& region,
    const entity::EntityType& entityType,
    i32 x,
    i32 z) const
{
    // 获取实体类型的高度图类型
    HeightmapType heightmapType =
        world::spawn::EntitySpawnPlacementRegistry::getHeightmapType(entityType.name());

    // 获取实体脚下位置：Chunk 的 topBlockY 是顶层方块 Y，这里需要上移一格到空气层。
    const i32 topY = region.getTopBlockY(x, z, heightmapType) + 1;

    if (topY <= 0) {
        return -1;
    }

    // 获取实体放置类型
    world::spawn::PlacementType placementType =
        world::spawn::EntitySpawnPlacementRegistry::getPlacementType(entityType.name());

    // 对于地面生物，需要在地面上一格生成
    if (placementType == world::spawn::PlacementType::OnGround) {
        // 检查脚下方块是否是实心方块
        const BlockState* groundBlock = region.getBlockState(x, topY - 1, z);
        if (!groundBlock || groundBlock->isAir()) {
            return -1;
        }

        // 检查生成位置是否为空气或可通过方块
        const BlockState* spawnBlock = region.getBlockState(x, topY, z);
        if (spawnBlock && spawnBlock->isSolid()) {
            return -1;  // 头顶被堵住
        }

        // 再检查上一格（对于高度 > 1 的生物）
        const BlockState* aboveBlock = region.getBlockState(x, topY + 1, z);
        if (aboveBlock && aboveBlock->isSolid()) {
            return -1;  // 头顶被堵住
        }
    }
    // 对于水中生物，需要找到水
    else if (placementType == world::spawn::PlacementType::InWater) {
        // 在高度位置向下搜索水
        for (i32 y = topY; y > 0; --y) {
            const BlockState* state = region.getBlockState(x, y, z);
            if (state && state->getMaterial().isLiquid()) {
                return y;
            }
        }
        return -1;  // 没找到水
    }
    // 对于岩浆中生物，需要找到岩浆
    else if (placementType == world::spawn::PlacementType::InLava) {
        // 在高度位置向下搜索岩浆
        for (i32 y = topY; y > 0; --y) {
            const BlockState* state = region.getBlockState(x, y, z);
            if (state && &state->getMaterial() == &Material::LAVA) {
                return y;
            }
        }
        return -1;  // 没找到岩浆
    }
    // 无限制
    else if (placementType == world::spawn::PlacementType::NoRestrictions) {
        // do nothing 不拦截
    } else {
        // 类型未知，报错
        MC_ASSERT_RELEASE_MSG(false, "Unsupported placement type");
    }

    return topY;
}

bool WorldGenSpawner::canSpawnAt(
    WorldGenRegion& region,
    const entity::EntityType& entityType,
    i32 x,
    i32 y,
    i32 z) const
{
    // WorldGenSpawner 只处理 Creature 分类（被动动物）的区块生成
    // 怪物通过 NaturalSpawner 在夜间/黑暗环境生成
    // 水生生物和环境生物有单独的生成逻辑
    const entity::EntityClassification classification = entityType.classification();
    if (classification != entity::EntityClassification::Creature) {
        return false;
    }

    // 检查基本位置规则
    if (!world::isValidY(y)) {
        return false;
    }

    // 获取放置类型
    const world::spawn::PlacementType placementType =
        world::spawn::EntitySpawnPlacementRegistry::getPlacementType(entityType.name());

    // 创建适配器用于位置检查
    WorldGenRegionAdapter adapter(region);
    const Vector3i pos(x, y, z);

    // 使用 EntitySpawnPlacementRegistry 检查位置是否适合生成
    if (!world::spawn::EntitySpawnPlacementRegistry::canSpawnAtLocation(
            placementType, adapter, pos, entityType.name())) {
        return false;
    }

    // 检查特定实体的生成规则（如果有）
    return checkSpawnRules(region, entityType, x, y, z);
}

bool WorldGenSpawner::checkSpawnRules(
    WorldGenRegion& region,
    const entity::EntityType& entityType,
    i32 x,
    i32 y,
    i32 z) const
{
    const BlockState* ground = region.getBlockState(x, y - 1, z);
    if (!ground || ground->isAir() || ground->isLiquid()) {
        return false;
    }

    const String& typeName = entityType.name();

    // 参考原版常见被动生物规则：大多数生物偏好草方块
    if (typeName == "minecraft:sheep" ||
        typeName == "minecraft:cow" ||
        typeName == "minecraft:horse" ||
        typeName == "minecraft:donkey") {
        if (!ground->is(VanillaBlocks::GRASS_BLOCK)) {
            return false;
        }
    }

    // 马与驴对地形平坦度要求更高
    if (typeName == "minecraft:horse" || typeName == "minecraft:donkey") {
        const i32 centerTop = region.getTopBlockY(x, z, HeightmapType::MotionBlockingNoLeaves);
        for (i32 dx = -1; dx <= 1; ++dx) {
            for (i32 dz = -1; dz <= 1; ++dz) {
                const i32 h = region.getTopBlockY(x + dx, z + dz, HeightmapType::MotionBlockingNoLeaves);
                if (std::abs(h - centerTop) > 1) {
                    return false;
                }
            }
        }
    }

    return true;
}

} // namespace mc
