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
#include "common/world/chunk/base/ChunkPos.hpp"
#include "common/world/chunk/data/ChunkData.hpp"
#include "common/world/chunk/data/Heightmap.hpp"
#include "common/world/chunk/data/IChunk.hpp"
#include "common/world/spawn/MobSpawnInfo.hpp"
#include <cstddef>
#include <functional>
#include <memory>
#include <string>
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
     * 计算公式：sum(charge / sqrt(distSq)) * multiplier。
     * 当某个点电荷与查询位置重合（distSq==0）时返回无穷大，
     * 以阻止在已存在实体的精确位置上堆叠生成。
     *
     * @param pos 目标位置
     * @param multiplier 密度乘数（待生成实体的 charge 值）
     * @return 总密度值；重合时为正无穷
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
 * @brief 本地容量计算器（LocalMobCapCalculator）
 *
 * 每个玩家在其附近区块内对每个分类维护一个本地计数，本地 cap = getMaxInstancesPerChunk(classification)。
 * 跨区块共享同一玩家的本地计数，防止单一区域在同一玩家名下无限堆积。
 */
class LocalMobCapCalculator {
public:
    /**
     * @brief 登记玩家与区块的关联（createState 时为每个玩家登记其附近区块）。
     */
    void addPlayerChunk(u64 playerId, const ChunkPos& chunk);

    /**
     * @brief 登记一个已存在的 Mob（createState 时调用，占用本地配额）。
     */
    void addMob(const ChunkPos& chunk, entity::EntityClassification classification);

    /**
     * @brief 检查在指定区块生成该分类是否不超本地 cap。
     *
     * 本地 cap = getMaxCount(classification)。区块附近所有共享该区块的玩家的本地
     * 计数都必须 < cap 才允许生成。
     */
    [[nodiscard]] bool canSpawn(entity::EntityClassification classification, const ChunkPos& chunk) const;

    /**
     * @brief 清空所有登记（每 tick createState 重建时调用，复用桶容量避免逐节点重分配）。
     */
    void clear()
    {
        m_playerChunks.clear();
        m_chunkCounts.clear();
    }

private:
    // 玩家 -> 该玩家登记的区块集合
    std::unordered_map<u64, std::unordered_set<ChunkPos>> m_playerChunks;
    // 区块 -> 各分类已占用本地计数
    std::unordered_map<ChunkPos, std::unordered_map<entity::EntityClassification, i32>> m_chunkCounts;
};

/**
 * @brief 实体密度管理器
 *
 * 管理各类实体的数量和密度限制。
 */
class EntityDensityManager {
public:
    /**
     * @brief 构造密度管理器
     * @param spawnableChunkCount 刷怪区块计数（玩家固定刷怪距离内的区块数，满载≈289）
     * @param entityCounts 各分类实体数量快照
     * @param densityTracker 密度追踪器
     */
    EntityDensityManager(i32 spawnableChunkCount,
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
     * @param entityTypeId 实体类型ID
     * @param classification 实体分类
     * @param pos 生成位置
     * @param spawnCosts 生成成本
     */
    void onSpawn(const std::string& entityTypeId,
        entity::EntityClassification classification,
        const Vector3& pos,
        const SpawnCosts& spawnCosts);

    /**
     * @brief 获取指定分类的当前实体数量
     */
    [[nodiscard]] i32 getCount(entity::EntityClassification classification) const;

    /**
     * @brief 获取刷怪区块计数
     */
    [[nodiscard]] i32 spawnableChunkCount() const { return m_spawnableChunkCount; }

private:
    i32 m_spawnableChunkCount;
    std::unordered_map<entity::EntityClassification, i32> m_entityCounts;
    MobDensityTracker& m_densityTracker;
};

/**
 * @brief 自然生成器
 *
 * 负责在世界中进行自然实体生成。
 * 每tick检查玩家周围区域，根据生物群系配置和光照条件生成实体。
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

    /**
     * @brief 对单个精确坐标执行一次自然生成判定（对齐 vanilla @VisibleForDebug 单点入口）。
     *
     * 对齐 Java 1.21.11 `NaturalSpawner.spawnCategoryForPosition(MobCategory, ServerLevel, BlockPos)`
     * （NaturalSpawner.java:145-150，vanilla `/debugmobspawning` 命令背后的调试入口）。
     *
     * 与 `tick` 的随机区块选址 + 区块内随机位选址不同：本方法直接用传入坐标作为种子位，
     * 绕过 `_collectSpawnableChunks`/`getRandomPosWithin` 的随机选址——使 GameTest 能对
     * 精确坐标做一次完整条件检查 + 生成，消除小结构 footprint 命中率极低的随机性。
     *
     * 语义对齐 vanilla 3 参版（恒真 SpawnPredicate、空 AfterSpawnCallback）：
     *   - 仍做外层 3 轮 ±5 抖动（vanilla 行 177 `rand(6)-rand(6)`）、最近玩家查找、
     *     距离门控（>24 格）、随机选 SpawnEntry、放置规则/光照/碰撞检查、finalizeSpawn。
     *   - **不检查全局 cap / 本地 cap / SpawnCosts**（vanilla 3 参版不更新 SpawnState，
     *     可反复刷；对齐 vanilla `/debugmobspawning` 语义）。故本方法只适合测试"条件判定"，
     *     不适合测试"cap 节流"行为（后者需走真实 tick）。
     *   - 抖动后 y 用传入坐标的 y（对齐 vanilla 行 177 用原始 i），不重算 heightmap——
     *     传入坐标须已是合法生成位（air 位，由调用方确保）。
     *
     * 前置条件（任一不满足则本方法不生成，返回 0）：
     *   - 世界中存在玩家（最近玩家非空，距离门控基准）；
     *   - 种子位 ±5 抖动后距最近玩家 >24 格且 <=128 格（MONSTER/CREATURE）；
     *   - 抖动位 BlockState 非实心（可生成位）；
     *   - biome SpawnEntry 池非空且抽中条目；
     *   - 通过 `_canSpawnAt`（放置类型 + 光照门槛）；
     *   - 无方块碰撞。
     *
     * @param world 世界
     * @param classification 实体分类（Monster/Creature/Ambient/...）
     * @param pos 种子位（须为合法 air 生成位）
     * @param biomeOverride 非 0 时强制用该 biome 取 SpawnEntry（绕过世界真实 biome 查询），
     *     0 表示用 pos 所在 chunk 的真实 biome（对齐 vanilla）。测试专用：GameTest 结构固定放
     *     世界原点，原点 biome 由世界种子决定不可控（默认 seed=0 原点是 ColdOcean，无陆地动物
     *     SpawnEntry），注入 biome 让测试能稳定验证"plains 能生成动物"等条件判定，不受世界种子
     *     biome 分布制约。仅覆盖"biome→SpawnEntry 选择"一步，光照/距离/放置/finalizeSpawn 仍走
     *     真实世界路径，故不破坏条件判定语义。生产 tick 路径不传此参（用真实 biome）。
     * @return 实际生成的实体数量（0 表示本次未生成）
     */
    static i32 spawnCategoryForPosition(mc::server::ServerWorld& world,
        entity::EntityClassification classification,
        const Vector3i& pos,
        BiomeId biomeOverride = 0);

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

    // ========== 常量 ==========

    /// 最小生成距离（玩家周围）
    static constexpr f64 MIN_SPAWN_DISTANCE_SQ = 24.0 * 24.0;

    /// 最大生成距离（玩家周围）
    static constexpr f64 MAX_SPAWN_DISTANCE_SQ = 128.0 * 128.0;

    /// 固定刷怪距离（区块）
    static constexpr i32 SPAWN_DISTANCE_CHUNK = 8;

private:
    i32 m_spawnDistance = 8; // 生成距离（区块）
    i32 m_spawnRange = 20;   // 玩家周围生成范围（方块）
    i32 m_maxEntities = 200; // 最大实体数量

    /// 密度追踪器
    MobDensityTracker m_densityTracker;

    /// 本地容量计算器（每 tick 在 createState 阶段重建）
    LocalMobCapCalculator m_localMobCap;

    /// 上次动物生成检查时间（游戏刻）
    u64 m_lastCreatureSpawnTime = 0;

    /// 动物生成间隔（游戏刻）
    static constexpr u64 CREATURE_SPAWN_INTERVAL = 400;

    // ========== 内部方法 ==========

    /**
     * @brief 在指定区块中为指定分类执行生成
     */
    void _spawnForClassificationInChunk(entity::EntityClassification classification,
        mc::server::ServerWorld& world,
        const ChunkData* chunk,
        const Vector3& playerPos,
        EntityDensityManager& densityManager,
        math::Random& random);

    /**
     * @brief 在指定位置尝试生成实体
     * @return 生成的实体数量
     *
     * static：本方法不依赖任何实例状态（world/entry/random 均由参数传入），改为 static 以便
     * `spawnCategoryForPosition` 单点入口（static）复用，同时是无状态方法的正确归类。
     */
    static i32 _trySpawnAt(
        mc::server::ServerWorld& world, i32 x, i32 y, i32 z, const SpawnEntry& entry, math::Random& random);

    /**
     * @brief 随机选择生成条目
     */
    [[nodiscard]] static const SpawnEntry* _selectEntry(const std::vector<SpawnEntry>& entries, math::Random& random);

    /**
     * @brief 检查位置是否可以生成
     */
    [[nodiscard]] static bool _canSpawnAt(mc::server::ServerWorld& world, i32 x, i32 y, i32 z, const SpawnEntry& entry);

    /**
     * @brief 检查光照条件
     */
    [[nodiscard]] static bool _checkLightLevel(mc::server::ServerWorld& world, i32 x, i32 y, i32 z, bool isMonster);

    /**
     * @brief 获取生成高度
     */
    [[nodiscard]] static i32 _getSpawnHeight(mc::server::ServerWorld& world, i32 x, i32 z, HeightmapType heightmapType);

    /**
     * @brief 选择指定分类的生成条目
     *
     * @param biomeOverride 非 0 时强制用该 biome 取 SpawnEntry（绕过 chunk 真实 biome 查询），
     *     0 表示用 pos 所在 chunk 的真实 biome。测试专用（见 spawnCategoryForPosition 注释）。
     */
    [[nodiscard]] static const SpawnEntry* _getRandomSpawnEntry(mc::server::ServerWorld& world,
        const ChunkData* chunk,
        entity::EntityClassification classification,
        const Vector3i& pos,
        math::Random& random,
        BiomeId biomeOverride = 0);

    /**
     * @brief 创建实体密度管理器
     *
     * 同时重建密度追踪器（清空上 tick 残留）与本地容量计算器，
     * 并以固定刷怪距离内的区块数作为 spawnableChunkCount。
     */
    EntityDensityManager _createDensityManager(mc::server::ServerWorld& world);

    /**
     * @brief 统计刷怪区块数（玩家固定刷怪距离内已加载区块数，去重）
     *
     * 统计玩家固定刷怪距离内已加载区块数（去重）。对应 DistanceManager.getNaturalSpawnChunkCount，满载约 289。
     */
    [[nodiscard]] i32 _countSpawnableChunks(mc::server::ServerWorld& world) const;

    /**
     * @brief 收集所有玩家固定刷怪距离内的已加载区块（去重，未打乱）
     */
    [[nodiscard]] std::vector<ChunkPos> _collectSpawnableChunks(mc::server::ServerWorld& world) const;

    /**
     * @brief 获取可生成区块列表
     *
     * 收集所有玩家固定刷怪距离（SPAWN_DISTANCE_CHUNK=8）内的已加载区块，
     * 去重后随机打乱（collectSpawningChunks + Util.shuffle）。
     *
     * @param world 世界
     * @param random 随机数生成器
     * @return 打乱后的可生成区块坐标列表
     */
    [[nodiscard]] std::vector<ChunkPos> _getSpawnableChunks(mc::server::ServerWorld& world, math::Random& random) const;

    /**
     * @brief 检查实体类型是否应该在当前条件下生成
     *
     * @param classification 实体分类
     * @param worldTime 世界时间（游戏刻）
     * @return 是否可以生成该分类
     */
    [[nodiscard]] bool _isSpawnCategoryReady(entity::EntityClassification classification, u64 worldTime) const;
};

} // namespace world::spawn
} // namespace mc
