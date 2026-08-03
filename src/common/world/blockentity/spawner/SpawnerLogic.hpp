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
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#pragma once

#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/util/nbt/Nbt.hpp"
#include "common/world/block/BlockPos.hpp"
#include <functional>
#include <optional>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>

namespace mc {

class IWorld;

namespace entity {
class EntityType;
enum class EntityClassification : u8;
} // namespace entity

namespace world::spawn {
enum class SpawnReason : u8;
class ISpawnWorldReader;
} // namespace world::spawn

namespace blockentity {

/**
 * @brief 生成数据条目
 *
 * 描述刷怪笼可以生成的一种实体类型及其权重。
 * 对应 MC Java 的 SpawnData + WeightedEntry。
 */
struct SpawnEntry {
    /// 实体类型 ID（如 "minecraft:silverfish"）
    ResourceLocation entityId;
    /// 权重（默认 1）
    i32 weight = 1;
};

/**
 * @brief 自定义生成规则
 *
 * 覆盖刷怪笼默认的生成位置检查，允许自定义光照限制。
 * 对应 MC Java 的 SpawnData.CustomSpawnRules。
 *
 * 当 CustomSpawnRules 存在时，刷怪笼在生成实体前会检查每个生成位置的
 * 方块光照和天空光照是否在指定范围内。如果不在范围内，跳过该位置。
 * 当 CustomSpawnRules 不存在时，刷怪笼会调用
 * EntitySpawnPlacementRegistry::canSpawnEntity() 检查默认生成规则。
 */
struct CustomSpawnRules {
    /// 方块光照范围 [min, max]，默认 [0, 15] 即不限制
    i32 blockLightMin = 0;
    i32 blockLightMax = 15;
    /// 天空光照范围 [min, max]，默认 [0, 15] 即不限制
    i32 skyLightMin = 0;
    i32 skyLightMax = 15;

    /**
     * @brief 检查指定位置的光照条件是否满足自定义生成规则
     *
     * 对应 MC Java 的 SpawnData.CustomSpawnRules.isValidPosition()。
     * 分别检查方块光照和天空光照是否在各自范围内。
     * 注意：此检查使用原始光照值，不应用天空变暗（skyDarkening）。
     *
     * @param blockLight 位置的方块光照值 (0-15)
     * @param skyLight 位置的天空光照值 (0-15)
     * @return 光照条件是否满足
     */
    [[nodiscard]] bool isValidPosition(u8 blockLight, u8 skyLight) const
    {
        return blockLight >= static_cast<u8>(blockLightMin) && blockLight <= static_cast<u8>(blockLightMax) &&
            skyLight >= static_cast<u8>(skyLightMin) && skyLight <= static_cast<u8>(skyLightMax);
    }
};

/**
 * @brief 刷怪笼逻辑（公共类）
 *
 * 从 MobSpawnerBlockEntity 中提取的独立刷怪逻辑类，对应 MC Java 的 BaseSpawner。
 * 可被 MobSpawnerBlockEntity（方块刷怪笼）和 SpawnerMinecartEntity（刷怪笼矿车）共用。
 *
 * 生成逻辑：
 * 1. 检测 requiredPlayerRange 范围内是否有玩家
 * 2. 等待 spawnDelay tick
 * 3. 从 spawnPotentials 中随机选择实体类型
 * 4. 在 spawnRange 范围内尝试生成 spawnCount 个实体
 * 5. 检查附近实体数量不超过 maxNearbyEntities
 * 6. 成功生成后重置延迟
 *
 * 与 MobSpawnerBlockEntity 的关键区别：
 * - MobSpawnerBlockEntity 使用方块位置（BlockPos）作为生成中心
 * - SpawnerMinecartEntity 使用实体坐标（Vector3）作为生成中心
 * - MobSpawnerBlockEntity 通过 world.playEvent() 广播粒子效果
 * - SpawnerMinecartEntity 通过 world.broadcastEntityEvent() 广播粒子效果
 */
class SpawnerLogic {
public:
    /**
     * @brief 粒子事件回调类型
     *
     * 方块刷怪笼使用 world.playEvent(MOB_SPAWNER_PARTICLES, pos, 0)，
     * 矿车刷怪笼使用 world.broadcastEntityEvent(entity, SPAWNER_EVENT)。
     */
    using ParticleEventCallback = std::function<void(IWorld& world)>;

    SpawnerLogic() = default;

    // ========== 服务器端 tick ==========

    /**
     * @brief 服务器端 tick 处理
     *
     * 执行完整的刷怪逻辑：玩家检测、延迟倒计时、实体生成、延迟重置。
     *
     * @param world 世界引用
     * @param centerX 生成中心 X 坐标（方块坐标或实体坐标）
     * @param centerY 生成中心 Y 坐标
     * @param centerZ 生成中心 Z 坐标
     * @param particleCallback 成功生成实体后的粒子回调
     */
    void serverTick(
        IWorld& world, f64 centerX, f64 centerY, f64 centerZ, const ParticleEventCallback& particleCallback);

    // ========== 客户端端 tick ==========

    /**
     * @brief 客户端端 tick 处理
     *
     * 处理刷怪笼粒子效果和旋转动画。
     *
     * @param world 世界引用
     * @param centerX 生成中心 X 坐标
     * @param centerY 生成中心 Y 坐标
     * @param centerZ 生成中心 Z 坐标
     */
    void clientTick(IWorld& world, f64 centerX, f64 centerY, f64 centerZ);

    /**
     * @brief 处理实体事件（客户端收到广播事件时调用）
     *
     * 当收到事件 id == 1 时，重置 spawnDelay 为 minSpawnDelay，
     * 触发刷怪笼内的旋转模型重新开始旋转动画。
     *
     * @param eventId 事件 ID
     */
    void onEventTriggered(int eventId);

    // ========== 配置接口 ==========

    /**
     * @brief 设置刷怪笼的实体类型
     *
     * 清除 spawnPotentials 并将 nextEntityId 设置为指定实体类型。
     * 对应 MC Java 的 BaseSpawner.setEntityId()。
     *
     * @param entityId 实体类型 ID（如 "minecraft:silverfish"）
     * @param rng 随机数生成器（用于设置初始延迟）
     */
    void setEntityId(const ResourceLocation& entityId, math::Random& rng);

    /**
     * @brief 添加生成实体候选
     *
     * @param entityId 实体类型 ID
     * @param weight 权重
     */
    void addSpawnPotential(const ResourceLocation& entityId, i32 weight);

    /**
     * @brief 设置自定义生成规则
     * @param rules 自定义生成规则
     */
    void setCustomSpawnRules(const CustomSpawnRules& rules);

    // ========== 参数访问 ==========

    [[nodiscard]] i32 getMinSpawnDelay() const { return m_minSpawnDelay; }
    void setMinSpawnDelay(i32 delay) { m_minSpawnDelay = delay; }

    [[nodiscard]] i32 getMaxSpawnDelay() const { return m_maxSpawnDelay; }
    void setMaxSpawnDelay(i32 delay) { m_maxSpawnDelay = delay; }

    [[nodiscard]] i32 getSpawnCount() const { return m_spawnCount; }
    void setSpawnCount(i32 count) { m_spawnCount = count; }

    [[nodiscard]] i32 getMaxNearbyEntities() const { return m_maxNearbyEntities; }
    void setMaxNearbyEntities(i32 max) { m_maxNearbyEntities = max; }

    [[nodiscard]] i32 getRequiredPlayerRange() const { return m_requiredPlayerRange; }
    void setRequiredPlayerRange(i32 range) { m_requiredPlayerRange = range; }

    [[nodiscard]] i32 getSpawnRange() const { return m_spawnRange; }
    void setSpawnRange(i32 range) { m_spawnRange = range; }

    [[nodiscard]] i32 getSpawnDelay() const { return m_spawnDelay; }
    void setSpawnDelay(i32 delay) { m_spawnDelay = delay; }

    [[nodiscard]] const std::vector<SpawnEntry>& getSpawnPotentials() const { return m_spawnPotentials; }
    [[nodiscard]] const ResourceLocation& getNextEntityId() const { return m_nextEntityId; }
    [[nodiscard]] const std::optional<CustomSpawnRules>& getCustomSpawnRules() const { return m_customSpawnRules; }

    // ========== 序列化 ==========

    /**
     * @brief 从 JSON 加载刷怪笼数据
     * @param data JSON 数据
     */
    void loadFromJson(const nlohmann::json& data);

    /**
     * @brief 保存刷怪笼数据到 JSON
     * @param data JSON 数据输出
     */
    void saveToJson(nlohmann::json& data) const;

    /**
     * @brief 从 NBT 加载刷怪笼数据
     * @param tag NBT 复合标签
     */
    void loadFromNBT(const nbt::CompoundTag& tag);

    /**
     * @brief 保存刷怪笼数据到 NBT
     * @param tag NBT 复合标签输出
     */
    void saveToNBT(nbt::CompoundTag& tag) const;

    // ========== 旋转动画（客户端） ==========

    /**
     * @brief 获取旋转角度（用于客户端渲染）
     * 对应 MC Java BaseSpawner.getSpin()
     */
    [[nodiscard]] f64 getSpin() const { return m_spin; }

    /**
     * @brief 获取上一 tick 旋转角度
     * 对应 MC Java BaseSpawner.getoSpin()
     */
    [[nodiscard]] f64 getOSpin() const { return m_oSpin; }

private:
    // ========== 生成逻辑（内部方法） ==========

    /**
     * @brief 检测附近是否有玩家
     * @param world 世界引用
     * @param centerX 中心 X
     * @param centerY 中心 Y
     * @param centerZ 中心 Z
     * @return 如果在 requiredPlayerRange 范围内有玩家返回 true
     */
    [[nodiscard]] bool isNearPlayer(IWorld& world, f64 centerX, f64 centerY, f64 centerZ) const;

    /**
     * @brief 重置生成延迟
     * @param rng 随机数生成器
     */
    void delay(math::Random& rng);

    /**
     * @brief 尝试生成实体
     * @param world 世界引用
     * @param centerX 中心 X
     * @param centerY 中心 Y
     * @param centerZ 中心 Z
     * @return 如果成功生成了至少一个实体返回 true
     */
    bool spawnEntities(IWorld& world, f64 centerX, f64 centerY, f64 centerZ);

    /**
     * @brief 计算附近同类型实体数量
     * @param world 世界引用
     * @param centerX 中心 X
     * @param centerY 中心 Y
     * @param centerZ 中心 Z
     * @param entityId 实体类型 ID
     * @return 附近实体数量
     */
    [[nodiscard]] i32 countNearbyEntities(
        IWorld& world, f64 centerX, f64 centerY, f64 centerZ, const ResourceLocation& entityId) const;

    /**
     * @brief 从 spawnPotentials 中随机选择下一个实体类型
     * @param rng 随机数生成器
     */
    void selectNextEntity(math::Random& rng);

    /**
     * @brief 检查生成位置是否满足光照和生成规则
     * @param world 世界引用
     * @param spawnPos 生成位置（方块坐标）
     * @param entityType 实体类型
     * @return 是否可以在该位置生成
     */
    [[nodiscard]] bool isValidSpawnPosition(
        IWorld& world, const BlockPos& spawnPos, const entity::EntityType& entityType) const;

    // ========== 生成参数 ==========
    // 默认值对齐 MC Java BaseSpawner

    /// 下一次生成前的延迟 tick 数
    i32 m_spawnDelay = DEFAULT_SPAWN_DELAY;

    /// 可生成的实体类型列表（带权重）
    std::vector<SpawnEntry> m_spawnPotentials;

    /// 下一次生成的实体类型 ID
    ResourceLocation m_nextEntityId;

    /// 自定义生成规则（可选）
    std::optional<CustomSpawnRules> m_customSpawnRules;

    /// 最小生成延迟（tick）
    i32 m_minSpawnDelay = DEFAULT_MIN_SPAWN_DELAY;

    /// 最大生成延迟（tick）
    i32 m_maxSpawnDelay = DEFAULT_MAX_SPAWN_DELAY;

    /// 每次生成尝试的实体数量
    i32 m_spawnCount = DEFAULT_SPAWN_COUNT;

    /// 附近同类型实体最大数量
    i32 m_maxNearbyEntities = DEFAULT_MAX_NEARBY_ENTITIES;

    /// 检测玩家的范围（方块）
    i32 m_requiredPlayerRange = DEFAULT_REQUIRED_PLAYER_RANGE;

    /// 实体生成范围（方块，距刷怪笼的距离）
    i32 m_spawnRange = DEFAULT_SPAWN_RANGE;

    // ========== 客户端旋转动画 ==========

    /// 当前旋转角度
    f64 m_spin = 0.0;

    /// 上一 tick 旋转角度
    f64 m_oSpin = 0.0;

    // ========== 默认常量 ==========

    static constexpr i32 DEFAULT_SPAWN_DELAY = 20;
    static constexpr i32 DEFAULT_MIN_SPAWN_DELAY = 200;
    static constexpr i32 DEFAULT_MAX_SPAWN_DELAY = 800;
    static constexpr i32 DEFAULT_SPAWN_COUNT = 4;
    static constexpr i32 DEFAULT_MAX_NEARBY_ENTITIES = 6;
    static constexpr i32 DEFAULT_REQUIRED_PLAYER_RANGE = 16;
    static constexpr i32 DEFAULT_SPAWN_RANGE = 4;

    /// 附近实体检测范围
    static constexpr f32 NEARBY_ENTITY_RANGE = 16.0f;
};

} // namespace blockentity
} // namespace mc
