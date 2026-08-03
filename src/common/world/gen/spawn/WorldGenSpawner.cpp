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

#include "WorldGenSpawner.hpp"
#include "../../../entity/core/EntityClassification.hpp"
#include "../../../entity/core/EntityRegistry.hpp"
#include "../../../util/AxisAlignedBB.hpp"
#include "../../../world/spawn/EntitySpawnPlacementRegistry.hpp"
#include "../../WorldConstants.hpp"
#include "../../lighting/InternalLightUtils.hpp"
#include "../chunk/IChunkGenerator.hpp"
#include "common/core/Types.hpp"
#include "common/entity/core/EntitySize.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/util/math/random/IRandom.hpp"
#include "common/world/biome/Biome.hpp"
#include "common/world/block/Material.hpp"
#include "common/world/chunk/data/Heightmap.hpp"
#include "common/world/spawn/MobSpawnInfo.hpp"
#include <algorithm>
#include <vector>
#include <spdlog/spdlog.h>

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
        : m_region(region)
    {}

    [[nodiscard]] const BlockState* getBlockState(i32 x, i32 y, i32 z) const override
    {
        return m_region.getBlockState(x, y, z);
    }

    [[nodiscard]] bool isInWorldBounds(i32 x, i32 y, i32 z) const override { return world::isValidY(y); }

    [[nodiscard]] i32 getHeight(HeightmapType type, i32 x, i32 z) const override
    {
        return m_region.getTopBlockY(x, z, type);
    }

    [[nodiscard]] BiomeId getBiome(i32 x, i32 y, i32 z) const override { return m_region.getBiome(x, y, z); }

    [[nodiscard]] u64 seed() const override { return m_region.seed(); }

    [[nodiscard]] Difficulty difficulty() const override { return m_region.difficulty(); }

    [[nodiscard]] i64 dayTime() const override { return m_region.dayTime(); }

    [[nodiscard]] i32 getMaxLocalRawBrightness(i32 x, i32 y, i32 z) const override
    {
        const u8 blockLight = m_region.getBlockLight(x, y, z);
        const u8 skyLight = m_region.getSkyLight(x, y, z);
        const i32 skyDarkening = InternalLightUtils::calculateSkyDarkening(
            m_region.dayTimeOfDay(), m_region.isRaining(), m_region.isThundering());
        return InternalLightUtils::calculateRawBrightness(blockLight, skyLight, skyDarkening);
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

i32 WorldGenSpawner::spawnInitialMobs(WorldGenRegion& region,
    const Biome& biome,
    i32 chunkX,
    i32 chunkZ,
    IChunkGenerator& /*generator*/,
    math::IRandom& random,
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
        return 0;
    }

    // 区块世界坐标起点（使用工具函数）
    const i32 startX = world::toWorldCoord(chunkX);
    const i32 startZ = world::toWorldCoord(chunkZ);

    // 使用生物群系的动物生成概率（creatureSpawnProbability，与 NaturalSpawner 同源）
    const f32 spawnProbability = biome.spawnInfo().getCreatureSpawnProbability();

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

        // MC 1.21.11: nextInt(maxCount - minCount) 结果范围 [0, maxCount-minCount)
        // 即 count 范围 [minCount, maxCount)，maxCount 不包含（与 Java Random.nextInt(bound) 一致）
        i32 count = selectedEntry->minCount;
        if (selectedEntry->maxCount > selectedEntry->minCount) {
            count = selectedEntry->minCount + random.nextInt(selectedEntry->maxCount - selectedEntry->minCount);
        }

        // 随机生成位置
        i32 groupX = startX + random.nextInt(world::CHUNK_WIDTH);
        i32 groupZ = startZ + random.nextInt(world::CHUNK_WIDTH);

        // 尝试多次找到合适的生成位置
        for (i32 attempt = 0; attempt < MAX_SPAWN_ATTEMPTS; ++attempt) {
            // 获取生成高度
            i32 spawnY = _getSpawnHeight(region, *entityType, groupX, groupZ);
            if (spawnY < 0) {
                // 尝试新位置
                groupX = startX + random.nextInt(world::CHUNK_WIDTH);
                groupZ = startZ + random.nextInt(world::CHUNK_WIDTH);
                continue;
            }

            // 检查是否可以生成
            if (!_canSpawnAt(region, *entityType, groupX, spawnY, groupZ)) {
                continue;
            }

            // 在组内生成多个实体
            i32 spawned = _spawnGroup(region,
                *entityType,
                static_cast<f32>(groupX) + 0.5f,
                static_cast<f32>(spawnY),
                static_cast<f32>(groupZ) + 0.5f,
                count,
                random,
                outEntities);

            if (spawned > 0) {
                spdlog::info("WorldGenSpawner: Spawned {} x {} at ({}, {}, {})",
                    spawned,
                    entityType->name(),
                    groupX,
                    spawnY,
                    groupZ);
                totalSpawned += spawned;
                break; // 成功生成一组后继续下一组
            }

            // 尝试新位置
            groupX = startX + random.nextInt(world::CHUNK_WIDTH);
            groupZ = startZ + random.nextInt(world::CHUNK_WIDTH);
        }
    }

    return totalSpawned;
}

i32 WorldGenSpawner::_spawnGroup(WorldGenRegion& region,
    const entity::EntityType& entityType,
    f32 x,
    [[maybe_unused]] f32 y,
    f32 z,
    i32 count,
    math::IRandom& random,
    std::vector<SpawnedEntityData>& outEntities)
{
    i32 spawned = 0;

    // 检查 isSummonable() - 实体类型是否可以被生成
    if (!entityType.canSummon()) {
        return 0;
    }

    // 获取实体尺寸
    const entity::EntitySize size = entityType.size();
    const f32 width = size.width();
    const f32 height = size.height();

    // 区块起点
    const i32 chunkStartX = static_cast<i32>(x) & ~(world::CHUNK_WIDTH - 1);
    const i32 chunkStartZ = static_cast<i32>(z) & ~(world::CHUNK_WIDTH - 1);

    for (i32 i = 0; i < count; ++i) {
        // 添加随机偏移使群体分散
        f32 spawnX = x + (random.nextFloat() - 0.5f) * width * 2.0f;
        f32 spawnZ = z + (random.nextFloat() - 0.5f) * width * 2.0f;

        // clamp 确保实体在区块边界内，考虑实体宽度
        spawnX = std::clamp(
            spawnX, static_cast<f32>(chunkStartX) + width, static_cast<f32>(chunkStartX + world::CHUNK_WIDTH) - width);
        spawnZ = std::clamp(
            spawnZ, static_cast<f32>(chunkStartZ) + width, static_cast<f32>(chunkStartZ + world::CHUNK_WIDTH) - width);

        // 检查碰撞空间
        i32 spawnY = _getSpawnHeight(region, entityType, static_cast<i32>(spawnX), static_cast<i32>(spawnZ));
        if (spawnY < 0) {
            continue;
        }

        const i32 spawnXi = static_cast<i32>(spawnX);
        const i32 spawnZi = static_cast<i32>(spawnZ);

        if (!_canSpawnAt(region, entityType, spawnXi, spawnY, spawnZi)) {
            continue;
        }

        // 检查碰撞
        const AxisAlignedBB entityBox =
            AxisAlignedBB::fromPosition(Vector3(spawnX, static_cast<f32>(spawnY), spawnZ), width, height);

        if (region.hasBlockCollision(entityBox)) {
            continue; // 有碰撞，跳过此位置
        }

        // 检查实体特定的生成规则（如蝙蝠需要光照<4等）
        WorldGenRegionAdapter adapter(region);
        const Vector3i checkPos(spawnXi, spawnY, spawnZi);
        if (!world::spawn::EntitySpawnPlacementRegistry::canSpawnEntity(
                entityType.name(), adapter, world::spawn::SpawnReason::ChunkGeneration, checkPos, random)) {
            continue;
        }

        // 记录生成的实体数据
        outEntities.emplace_back(entityType.name(), spawnX, static_cast<f32>(spawnY), spawnZ);

        ++spawned;
    }

    return spawned;
}

i32 WorldGenSpawner::_getSpawnHeight(WorldGenRegion& region, const entity::EntityType& entityType, i32 x, i32 z) const
{
    // 获取实体类型的高度图类型
    HeightmapType heightmapType = world::spawn::EntitySpawnPlacementRegistry::getHeightmapType(entityType.name());

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
            return -1; // 头顶被堵住
        }

        // 再检查上一格（对于高度 > 1 的生物）
        const BlockState* aboveBlock = region.getBlockState(x, topY + 1, z);
        if (aboveBlock && aboveBlock->isSolid()) {
            return -1; // 头顶被堵住
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
        return -1; // 没找到水
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
        return -1; // 没找到岩浆
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

bool WorldGenSpawner::_canSpawnAt(
    WorldGenRegion& region, const entity::EntityType& entityType, i32 x, i32 y, i32 z) const
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
    // MC 1.21.11: 初始区块生物生成仅使用 EntitySpawnPlacementRegistry 数据驱动的检查，
    // 不执行额外生成规则（如羊/牛偏好草方块等硬编码逻辑）
    return true;
}

} // namespace mc
