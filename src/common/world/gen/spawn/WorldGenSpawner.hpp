#pragma once

#include "../../../core/Types.hpp"
#include "../../../util/math/random/Random.hpp"
#include "../../../util/math/Vector3.hpp"
#include "../../spawn/MobSpawnInfo.hpp"
#include "../../biome/Biome.hpp"
#include "../../../entity/core/EntitySpawnPlacementRegistry.hpp"
#include <vector>
#include <memory>

namespace mc {

// 前向声明
class WorldGenRegion;
class IChunkGenerator;
class Entity;
class MobEntity;

namespace entity {
    class EntityType;
    struct ILivingEntityData;
}

/**
 * @brief 生成的实体数据
 *
 * 用于在区块生成时记录应该生成的实体信息，
 * 后续由 ServerWorld 创建实际的实体对象。
 */
struct SpawnedEntityData {
    /// 实体类型ID（如 "minecraft:pig"）
    String entityTypeId;

    /// 生成位置 X
    f32 x = 0.0f;

    /// 生成位置 Y
    f32 y = 0.0f;

    /// 生成位置 Z
    f32 z = 0.0f;

    /// 生成原因（区块生成）
    static constexpr i32 SPAWN_REASON_CHUNK_GENERATION = 1;

    /// 生成原因
    i32 spawnReason = SPAWN_REASON_CHUNK_GENERATION;

    SpawnedEntityData() = default;

    SpawnedEntityData(String typeId, f32 px, f32 py, f32 pz)
        : entityTypeId(std::move(typeId)), x(px), y(py), z(pz) {}
};

/**
 * @brief 区块生成时的生物放置器
 *
 * 在区块首次生成时放置被动动物（猪、牛、羊等）。
 * 参考 MC 1.16.5 WorldEntitySpawner.performWorldGenSpawning
 *
 * 与 NaturalSpawner 的区别：
 * - WorldGenSpawner: 区块生成时放置动物（仅 Creature 分类）
 * - NaturalSpawner: 运行时自然生成（怪物、动物、环境生物等）
 *
 * 使用方式：
 * @code
 * WorldGenSpawner spawner;
 * std::vector<SpawnedEntityData> entities;
 * spawner.spawnInitialMobs(region, biome, chunkX, chunkZ, generator, random, entities);
 * // 之后由 ServerWorld 创建实体
 * @endcode
 */
class WorldGenSpawner {
public:
    WorldGenSpawner();
    ~WorldGenSpawner();

    /**
     * @brief 在区块生成时放置动物
     *
     * 参考 MC 1.16.5 performWorldGenSpawning
     * 只放置 Creature 分类（被动动物），不生成怪物。
     * 怪物通过 NaturalSpawner 在夜间/黑暗环境生成。
     *
     * @param region 世界生成区域
     * @param biome 区块中心的主要生物群系
     * @param chunkX 区块 X 坐标
     * @param chunkZ 区块 Z 坐标
     * @param generator 区块生成器（用于获取高度）
     * @param random 随机数生成器
     * @param outEntities 输出：生成的实体数据列表
     * @return 实际生成的实体数量
     */
    i32 spawnInitialMobs(
        WorldGenRegion& region,
        const Biome& biome,
        i32 chunkX,
        i32 chunkZ,
        IChunkGenerator& generator,
        math::Random& random,
        std::vector<SpawnedEntityData>& outEntities
    );

    /**
     * @brief 设置是否启用生成
     */
    void setEnabled(bool enabled) { m_enabled = enabled; }

    /**
     * @brief 检查是否启用
     */
    [[nodiscard]] bool isEnabled() const { return m_enabled; }

private:
    bool m_enabled = true;

    /**
     * @brief 尝试在指定位置生成一组实体
     *
     * @param region 世界生成区域
     * @param entityType 实体类型
     * @param x X 坐标
     * @param y Y 坐标
     * @param z Z 坐标
     * @param count 生成数量
     * @param random 随机数生成器
     * @param outEntities 输出：生成的实体数据
     * @return 实际生成的数量
     */
    i32 spawnGroup(
        WorldGenRegion& region,
        const entity::EntityType& entityType,
        f32 x,
        f32 y,
        f32 z,
        i32 count,
        math::Random& random,
        std::vector<SpawnedEntityData>& outEntities
    );

    /**
     * @brief 获取实体类型的生成高度
     *
     * @param region 世界生成区域
     * @param entityType 实体类型
     * @param x X 坐标
     * @param z Z 坐标
     * @return 生成高度，如果无法生成返回 -1
     */
    [[nodiscard]] i32 getSpawnHeight(
        WorldGenRegion& region,
        const entity::EntityType& entityType,
        i32 x,
        i32 z
    ) const;

    /**
     * @brief 检查位置是否可以生成实体
     *
     * @param region 世界生成区域
     * @param entityType 实体类型
     * @param x X 坐标
     * @param y Y 坐标
     * @param z Z 坐标
     * @return 是否可以生成
     */
    [[nodiscard]] bool canSpawnAt(
        WorldGenRegion& region,
        const entity::EntityType& entityType,
        i32 x,
        i32 y,
        i32 z
    ) const;

    /**
     * @brief 检查方块是否允许实体生成
     *
     * 参考 MC EntitySpawnPlacementRegistry.canSpawnAtLocation
     */
    [[nodiscard]] bool checkSpawnRules(
        WorldGenRegion& region,
        const entity::EntityType& entityType,
        i32 x,
        i32 y,
        i32 z
    ) const;
};

} // namespace mc
