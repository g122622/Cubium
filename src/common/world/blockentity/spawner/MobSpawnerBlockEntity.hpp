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
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT ANY KIND, EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#pragma once

#include "common/resource/ResourceLocation.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/blockentity/BlockEntity.hpp"
#include <optional>
#include <string>
#include <vector>

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
 * @brief 刷怪笼方块实体
 *
 * 自动生成实体（怪物/动物）的方块实体。
 * 当玩家在检测范围内时，刷怪笼会周期性地在附近区域尝试生成实体。
 *
 * 生成逻辑：
 * 1. 检测 requiredPlayerRange 范围内是否有玩家
 * 2. 等待 spawnDelay tick
 * 3. 从 spawnPotentials 中随机选择实体类型
 * 4. 在 spawnRange 范围内尝试生成 spawnCount 个实体
 * 5. 检查附近实体数量不超过 maxNearbyEntities
 * 6. 成功生成后重置延迟
 *
 * 线程安全：tick() 在服务器线程调用，load()/save() 可能在保存线程调用。
 * 当前实现使用 m_mutex 保护共享状态。
 */
class MobSpawnerBlockEntity : public BlockEntity {
public:
    /**
     * @brief 构造函数
     * @param pos 方块位置
     */
    explicit MobSpawnerBlockEntity(const BlockPos& pos);

    // ========== BlockEntity 接口 ==========

    [[nodiscard]] bool needsTick() const noexcept override { return true; }
    void tick(IWorld& world) override;
    bool load(const nlohmann::json& data) override;
    void save(nlohmann::json& data) const override;
    bool loadFromNBT(const nbt::CompoundTag& tag) override;
    void saveToNBT(nbt::CompoundTag& tag) const override;
    [[nodiscard]] bool onlyOpsCanSetNbt() const noexcept override { return true; }
    [[nodiscard]] std::unique_ptr<BlockEntity> clone() const override;

    // ========== 配置接口 ==========

    /**
     * @brief 设置刷怪笼的实体类型
     *
     * 清除 spawnPotentials 并将 nextSpawnData 设置为指定实体类型。
     * 对应 MC Java 的 BaseSpawner.setEntityId()。
     *
     * @param entityId 实体类型 ID（如 "minecraft:silverfish"）
     * @param rng 随机数生成器（用于设置初始延迟）
     */
    void setEntityId(const ResourceLocation& entityId, math::Random& rng);

    /**
     * @brief 添加生成实体候选
     *
     * 向 spawnPotentials 列表中添加一个带权重的生成条目。
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

    [[nodiscard]] const std::vector<SpawnEntry>& getSpawnPotentials() const { return m_spawnPotentials; }

    [[nodiscard]] const ResourceLocation& getNextEntityId() const { return m_nextEntityId; }

private:
    // ========== 生成逻辑 ==========

    /**
     * @brief 服务器端 tick 处理
     */
    void _serverTick(IWorld& world);

    /**
     * @brief 检测附近是否有玩家
     * @param world 世界引用
     * @return 如果在 requiredPlayerRange 范围内有玩家返回 true
     */
    [[nodiscard]] bool _isNearPlayer(IWorld& world) const;

    /**
     * @brief 重置生成延迟
     *
     * 在 [minSpawnDelay, maxSpawnDelay] 范围内随机设置下一个延迟，
     * 并从 spawnPotentials 中随机选择下一个实体类型。
     *
     * @param rng 随机数生成器
     */
    void _delay(math::Random& rng);

    /**
     * @brief 尝试生成实体
     * @param world 世界引用
     * @return 如果成功生成了至少一个实体返回 true
     */
    bool _spawnEntities(IWorld& world);

    /**
     * @brief 计算附近同类型实体数量
     * @param world 世界引用
     * @param entityId 实体类型 ID
     * @return 附近实体数量
     */
    [[nodiscard]] i32 _countNearbyEntities(IWorld& world, const ResourceLocation& entityId) const;

    /**
     * @brief 从 spawnPotentials 中随机选择下一个实体类型
     * @param rng 随机数生成器
     */
    void _selectNextEntity(math::Random& rng);

    /**
     * @brief 检查生成位置是否满足光照和生成规则
     *
     * 当 m_customSpawnRules 存在时，检查方块光照和天空光照是否在指定范围内。
     * 当 m_customSpawnRules 不存在时，调用 EntitySpawnPlacementRegistry::canSpawnEntity()
     * 检查默认生成放置规则（包括地面/水中/岩浆放置类型和自定义谓词）。
     *
     * 对应 MC Java BaseSpawner.serverTick() 中的 CustomSpawnRules/SpawnPlacements 分支。
     *
     * @param world 世界引用
     * @param spawnPos 生成位置（方块坐标）
     * @param entityType 实体类型（用于查询分类和放置规则）
     * @return 是否可以在该位置生成
     */
    [[nodiscard]] bool _isValidSpawnPosition(
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

    // ========== 默认常量 ==========

    static constexpr i32 DEFAULT_SPAWN_DELAY = 20;
    static constexpr i32 DEFAULT_MIN_SPAWN_DELAY = 200;
    static constexpr i32 DEFAULT_MAX_SPAWN_DELAY = 800;
    static constexpr i32 DEFAULT_SPAWN_COUNT = 4;
    static constexpr i32 DEFAULT_MAX_NEARBY_ENTITIES = 6;
    static constexpr i32 DEFAULT_REQUIRED_PLAYER_RANGE = 16;
    static constexpr i32 DEFAULT_SPAWN_RANGE = 4;
};

} // namespace blockentity
} // namespace mc
