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

#pragma once

#include "common/core/Types.hpp"
#include "common/entity/core/EntityClassification.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/chunk/ChunkData.hpp"
#include "common/world/chunk/ChunkPos.hpp"
#include "common/world/chunk/IChunk.hpp"
#include "common/world/spawn/MobSpawnInfo.hpp"
#include <functional>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace mc {

// 前向声明
class Entity;
class BlockState;

namespace server {
class ServerWorld;
}

namespace world::spawn {

/**
 * @brief 实体密度追踪器
 *
 * 追踪区域内的实体密度，用于 SpawnCosts 系统。
 * 参考 MC 1.16.5 MobDensityTracker
 */
class MobDensityTracker {
public:
    /**
     * @brief 添加实体密度
     * @param pos 实体位置
     * @param charge 该实体的充电值
     */
    void addCharge(const Vector3& pos, f64 charge);

    /**
     * @brief 获取指定位置的总密度
     *
     * 参考 MC 1.16.5 MobDensityTracker.func_234999_b_
     * 计算公式：sum(charge / sqrt(distance))
     *
     * @param pos 目标位置
     * @param multiplier 密度乘数（实体的 charge 值）
     * @return 总密度值
     */
    [[nodiscard]] f64 getTotalCharge(const Vector3& pos, f64 multiplier) const;

    /**
     * @brief 清除所有密度数据
     */
    void clear() { m_charges.clear(); }

    /**
     * @brief 获取密度条目数量
     */
    [[nodiscard]] size_t size() const { return m_charges.size(); }

private:
    struct ChargeEntry {
        Vector3 position;
        f64 charge;
    };

    std::vector<ChargeEntry> m_charges;
};

/**
 * @brief 实体密度管理器
 *
 * 管理各类实体的数量和密度限制。
 * 参考 MC 1.16.5 WorldEntitySpawner.EntityDensityManager
 */
class EntityDensityManager {
public:
    /**
     * @brief 构造密度管理器
     * @param viewDistance 视距（区块）
     * @param entityCounts 各分类实体数量快照
     * @param densityTracker 密度追踪器
     */
    EntityDensityManager(i32 viewDistance,
        std::unordered_map<entity::EntityClassification, i32> entityCounts,
        MobDensityTracker& densityTracker);

    /**
     * @brief 检查是否可以生成指定类型的实体
     * @param classification 实体分类
     * @return 是否可以继续生成
     */
    [[nodiscard]] bool canSpawn(entity::EntityClassification classification) const;

    /**
     * @brief 检查并消耗密度预算
     * @param entityTypeId 实体类型ID
     * @param pos 生成位置
     * @param spawnCosts 生成成本
     * @return 是否可以生成
     */
    [[nodiscard]] bool canSpawnWithDensity(
        const std::string& entityTypeId, const Vector3& pos, const SpawnCosts& spawnCosts) const;

    /**
     * @brief 记录实体生成后的密度变化
     *
     * 参考 MC 1.16.5 WorldEntitySpawner.func_234990_a_
     *
     * @param entityTypeId 实体类型ID
     * @param classification 实体分类
     * @param pos 生成位置
     * @param spawnCosts 生成成本
     */
    void onSpawn(const std::string& entityTypeId, entity::EntityClassification classification,
        const Vector3& pos, const SpawnCosts& spawnCosts);

    /**
     * @brief 获取指定分类的当前实体数量
     */
    [[nodiscard]] i32 getCount(entity::EntityClassification classification) const;

    /**
     * @brief 获取视距
     */
    [[nodiscard]] i32 viewDistance() const { return m_viewDistance; }

private:
    i32 m_viewDistance;
    std::unordered_map<entity::EntityClassification, i32> m_entityCounts;
    MobDensityTracker& m_densityTracker;
};

/**
 * @brief 自然生成器
 *
 * 负责在世界中进行自然实体生成。
 * 每tick检查玩家周围区域，根据生物群系配置和光照条件生成实体。
 *
 * 参考 MC 1.16.5 WorldEntitySpawner (NaturalSpawner)
 *
 * 生成规则：
 * 1. 怪物：黑暗环境（光照 <= 7），距离玩家 24-128 格
 * 2. 动物：光照充足（光照 > 7），距离玩家 24-128 格，每 400 tick 尝试一次
 * 3. 环境生物：黑暗环境，随机概率
 * 4. 水生生物：水中，随机概率
 *
 * 使用方式：
 * @code
 * NaturalSpawner spawner;
 * spawner.tick(world, true, true);  // 每tick调用
 * @endcode
 */
class NaturalSpawner {
public:
    NaturalSpawner();
    ~NaturalSpawner() = default;

    // ========== 区块生成 ==========

    /**
     * @brief 在区块中生成实体
     *
     * 区块生成时调用，用于放置被动动物。
     * 仅生成 Creature 分类的实体。
     *
     * @param world 世界
     * @param chunkX 区块X坐标
     * @param chunkZ 区块Z坐标
     * @param spawnInfo 生物群系生成信息
     * @param random 随机数生成器
     */
    void spawnInChunk(
        mc::server::ServerWorld& world, i32 chunkX, i32 chunkZ, const MobSpawnInfo& spawnInfo, math::Random& random);

    /**
     * @brief 在世界中进行自然生成（每tick调用）
     *
     * 遍历所有玩家周围的区块，尝试生成实体。
     *
     * @param world 世界
     * @param hostile 是否生成敌对生物
     * @param passive 是否生成被动生物
     */
    void tick(mc::server::ServerWorld& world, bool hostile, bool passive);

    // ========== 生成配置 ==========

    /**
     * @brief 设置生成距离（区块）
     */
    void setSpawnDistance(i32 chunks) { m_spawnDistance = chunks; }

    /**
     * @brief 获取生成距离
     */
    [[nodiscard]] i32 getSpawnDistance() const { return m_spawnDistance; }

    /**
     * @brief 设置玩家周围生成范围
     */
    void setSpawnRange(i32 range) { m_spawnRange = range; }

    /**
     * @brief 获取玩家周围生成范围
     */
    [[nodiscard]] i32 getSpawnRange() const { return m_spawnRange; }

    /**
     * @brief 设置最大实体数量
     */
    void setMaxEntities(i32 max) { m_maxEntities = max; }

    /**
     * @brief 获取最大实体数量
     */
    [[nodiscard]] i32 getMaxEntities() const { return m_maxEntities; }

    // ========== 常量（public 供 EntityDensityManager 使用） ==========

    /// 最小生成距离（玩家周围）
    static constexpr f64 MIN_SPAWN_DISTANCE_SQ = 24.0 * 24.0;

    /// 最大生成距离（玩家周围）
    static constexpr f64 MAX_SPAWN_DISTANCE_SQ = 128.0 * 128.0;

    /// 即刻消失距离
    static constexpr f64 INSTANT_DESPAWN_DISTANCE_SQ = 128.0 * 128.0;

    /// 每次生成尝试的组大小上限
    static constexpr i32 MAX_GROUP_SIZE = 4;

    /// 怪物最大实例数
    static constexpr i32 MAX_MONSTERS = 70;

    /// 动物最大实例数
    static constexpr i32 MAX_CREATURES = 10;

    /// 环境生物最大实例数
    static constexpr i32 MAX_AMBIENT = 15;

    /// 水生生物最大实例数
    static constexpr i32 MAX_WATER_CREATURES = 5;

private:
    i32 m_spawnDistance = 8; // 生成距离（区块）
    i32 m_spawnRange = 20;   // 玩家周围生成范围（方块）
    i32 m_maxEntities = 200; // 最大实体数量

    /// 密度追踪器
    MobDensityTracker m_densityTracker;

    /// 上次动物生成检查时间（游戏刻）
    u64 m_lastCreatureSpawnTime = 0;

    /// 动物生成间隔（游戏刻）- MC 默认 400 tick
    static constexpr u64 CREATURE_SPAWN_INTERVAL = 400;

    // ========== 内部方法 ==========

    /**
     * @brief 在指定区块中为指定分类执行生成
     */
    void spawnForClassificationInChunk(entity::EntityClassification classification,
        mc::server::ServerWorld& world,
        const ChunkData* chunk,
        const Vector3& playerPos,
        EntityDensityManager& densityManager,
        math::Random& random);

    /**
     * @brief 在指定位置尝试生成实体
     * @return 生成的实体数量
     */
    i32 trySpawnAt(mc::server::ServerWorld& world, i32 x, i32 y, i32 z, const SpawnEntry& entry, math::Random& random);

    /**
     * @brief 随机选择生成条目
     */
    [[nodiscard]] const SpawnEntry* selectEntry(const std::vector<SpawnEntry>& entries, math::Random& random) const;

    /**
     * @brief 检查位置是否可以生成
     */
    [[nodiscard]] bool canSpawnAt(mc::server::ServerWorld& world, i32 x, i32 y, i32 z, const SpawnEntry& entry) const;

    /**
     * @brief 检查光照条件
     */
    [[nodiscard]] bool checkLightLevel(mc::server::ServerWorld& world, i32 x, i32 y, i32 z, bool isMonster) const;

    /**
     * @brief 获取生成高度
     */
    [[nodiscard]] i32 getSpawnHeight(mc::server::ServerWorld& world, i32 x, i32 z, HeightmapType heightmapType) const;

    /**
     * @brief 获取区块内的随机高度位置
     */
    [[nodiscard]] Vector3i getRandomSpawnPosition(mc::server::ServerWorld& world,
        const ChunkData* chunk,
        HeightmapType heightmapType,
        math::Random& random) const;

    /**
     * @brief 检查位置是否在玩家附近的有效生成区域
     * @param world 世界
     * @param pos 位置
     * @param playerDistanceSq 玩家距离的平方
     * @return 是否可以生成
     */
    [[nodiscard]] bool isValidSpawnPosition(
        mc::server::ServerWorld& world, const Vector3i& pos, f64 playerDistanceSq) const;

    /**
     * @brief 选择指定分类的生成条目
     *
     * 参考 MC 1.16.5 WorldEntitySpawner.getRandomSpawnEntry
     */
    [[nodiscard]] const SpawnEntry* getRandomSpawnEntry(mc::server::ServerWorld& world,
        const ChunkData* chunk,
        entity::EntityClassification classification,
        const Vector3i& pos,
        math::Random& random) const;

    /**
     * @brief 创建实体密度管理器
     */
    EntityDensityManager createDensityManager(mc::server::ServerWorld& world);

    /**
     * @brief 获取可生成区块列表
     *
     * 从玩家视距内的区块中随机选择可生成区块。
     *
     * @param world 世界
     * @param maxChunks 最大区块数量
     * @param random 随机数生成器
     * @return 可生成区块的坐标列表
     */
    [[nodiscard]] std::vector<ChunkPos> getSpawnableChunks(
        mc::server::ServerWorld& world, i32 maxChunks, math::Random& random) const;

    /**
     * @brief 检查实体类型是否应该在当前条件下生成
     *
     * 参考 MC 1.16.5 NaturalSpawner.isSpawnCategoryReady
     *
     * @param classification 实体分类
     * @param worldTime 世界时间（游戏刻）
     * @return 是否可以生成该分类
     */
    [[nodiscard]] bool isSpawnCategoryReady(entity::EntityClassification classification, u64 worldTime) const;
};

} // namespace world::spawn
} // namespace mc
