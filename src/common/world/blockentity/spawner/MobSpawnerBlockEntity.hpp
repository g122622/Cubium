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

#include "SpawnerLogic.hpp"
#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/util/nbt/Nbt.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/blockentity/BlockEntity.hpp"
#include <memory>
#include <optional>
#include <string>
#include <vector>
#include <nlohmann/json_fwd.hpp>

namespace mc {

class IWorld;

namespace blockentity {

/**
 * @brief 刷怪笼方块实体
 *
 * 自动生成实体（怪物/动物）的方块实体。
 * 当玩家在检测范围内时，刷怪笼会周期性地在附近区域尝试生成实体。
 *
 * 生成逻辑委托给 SpawnerLogic 类（对应 MC Java 的 BaseSpawner）。
 * SpawnerLogic 是独立的公共类，可被 MobSpawnerBlockEntity 和 SpawnerMinecartEntity 共用。
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
    [[nodiscard]] bool triggerEvent(i32 id, i32 type) override;

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

    // 测试子类需要访问 _isValidSpawnPosition
    friend class TestMobSpawnerBlockEntity;
};

} // namespace blockentity
} // namespace mc
