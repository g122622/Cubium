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
#include "common/resource/ResourceLocation.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/blockentity/BlockEntity.hpp"
#include <memory>
#include <optional>
#include <string>
#include <unordered_set>
#include <vector>
#include <nlohmann/json_fwd.hpp>

namespace mc {

class Player;
class Entity;

/**
 * @brief 生成潜力条目
 *
 * 描述试炼刷怪笼可以生成的一种实体类型及其权重。
 * 对应 MC Java 的 SpawnData + WeightedEntry。
 */
struct TrialSpawnPotential {
    /// 实体类型 ID（如 "minecraft:breeze"）
    ResourceLocation entityId;
    /// 权重（默认 1）
    i32 weight = 1;
};

/**
 * @brief 试炼刷怪笼方块实体
 *
 * 试炼密室核心机制方块，具有6种状态的有限状态机。
 * 检测附近玩家后激活，生成怪物，击杀所有怪物后弹出奖励。
 * 不祥变体增加怪物数量和更强的掉落物。
 *
 * 状态机：
 *   INACTIVE → WAITING_FOR_PLAYERS → ACTIVE → WAITING_FOR_REWARD_EJECTION
 *                                     ↓                        ↓
 *                                EJECTING_REWARD ←─────────────┘
 *                                     ↓
 *                                COOLDOWN → WAITING_FOR_PLAYERS
 *
 * 配置（根据刷怪笼类型）：
 *   旋风人:   baseTotalMobs=2,  baseSimultaneousMobs=1, ticksBetweenSpawn=20
 *   近战型:   baseTotalMobs=6,  baseSimultaneousMobs=3, ticksBetweenSpawn=40
 *   远程型:   baseTotalMobs=6,  baseSimultaneousMobs=3, ticksBetweenSpawn=40
 *   小型近战: baseTotalMobs=12, baseSimultaneousMobs=4, ticksBetweenSpawn=20
 *
 * 红石比较器输出：
 *   Inactive=0, WaitingForPlayers=1, Active=2,
 *   WaitingForRewardEjection=3, EjectingReward=4, Cooldown=4
 *
 * 参考: TrialSpawnerBlockEntity
 */
class TrialSpawnerBlockEntity : public BlockEntity {
public:
    /**
     * @brief 试炼刷怪笼状态
     */
    enum class State : u8 {
        Inactive = 0,
        WaitingForPlayers = 1,
        Active = 2,
        WaitingForRewardEjection = 3,
        EjectingReward = 4,
        Cooldown = 5
    };

    /**
     * @brief 刷怪笼类型配置
     */
    struct Config {
        /// 基础总怪物数
        i32 baseTotalMobs = 6;
        /// 每个额外玩家增加的总怪物数
        i32 totalMobsAddedPerPlayer = 2;
        /// 基础同时存在怪物数
        i32 baseSimultaneousMobs = 3;
        /// 每个额外玩家增加的同时怪物数
        i32 simultaneousMobsAddedPerPlayer = 1;
        /// 生成间隔（ticks）
        i32 ticksBetweenSpawn = 40;
        /// 玩家检测范围
        f32 detectionRange = 14.0f;
        /// 生成范围
        f32 spawnRange = 4.0f;
        /// 冷却时间（ticks）= 30分钟
        i32 cooldownTicks = 36000;
        /// 奖励弹出检测间隔（ticks）
        i32 ejectingRewardTicks = 80;
        /// 补给战利品表
        ResourceLocation supplyLootTable;
        /// 钥匙战利品表
        ResourceLocation keyLootTable;
        /// 不祥补给战利品表
        ResourceLocation ominousSupplyLootTable;
        /// 不祥钥匙战利品表
        ResourceLocation ominousKeyLootTable;
        /// 生成潜力列表（可生成的实体类型及权重）
        std::vector<TrialSpawnPotential> spawnPotentials;
    };

    /**
     * @brief 获取各类型刷怪笼的默认配置
     */
    static Config getBreezeConfig();
    static Config getMeleeConfig();
    static Config getSmallMeleeConfig();
    static Config getRangedConfig();
    static Config getSlowRangedConfig();

    explicit TrialSpawnerBlockEntity(const BlockPos& pos);

    // ========== BlockEntity 接口 ==========

    void tick(IWorld& world) override;
    [[nodiscard]] bool needsTick() const noexcept override { return true; }
    bool load(const nlohmann::json& data) override;
    void save(nlohmann::json& data) const override;
    [[nodiscard]] std::unique_ptr<BlockEntity> clone() const override;

    /**
     * @brief 试炼刷怪笼的 NBT 仅允许 OP 玩家设置
     *
     * 参考 MC Java: BlockEntityType.OP_ONLY_CUSTOM_DATA 包含 TRIAL_SPAWNER
     */
    [[nodiscard]] bool onlyOpsCanSetNbt() const noexcept override { return true; }

    // ========== 状态访问 ==========

    [[nodiscard]] State getState() const noexcept { return m_state; }
    void setState(State state);

    [[nodiscard]] bool isOminous() const noexcept { return m_ominous; }
    void setOminous(bool ominous);

    [[nodiscard]] const Config& getConfig() const noexcept { return m_config; }
    void setConfig(const Config& config);

    // ========== 玩家检测 ==========

    /**
     * @brief 检测范围内的玩家（排除旁观者模式）
     * @param world 世界引用
     * @param range 检测范围
     * @return 范围内的玩家列表
     */
    std::vector<Player*> detectPlayers(IWorld& world, f32 range);

    // ========== 红石比较器 ==========

    /**
     * @brief 获取红石比较器输出信号
     */
    [[nodiscard]] i32 getComparatorOutput() const;

    // ========== 不祥变体 ==========

    /**
     * @brief 将不祥之兆转化为试炼之兆
     *
     * 当持有不祥之兆效果的玩家进入试炼刷怪笼范围时，
     * 消耗不祥之兆，给予试炼之兆效果，并将刷怪笼转为不祥变体。
     */
    void applyOminous(Player& player);

private:
    // ========== 常量 ==========

    /// 玩家扫描间隔（ticks）
    static constexpr i32 PLAYER_SCAN_INTERVAL = 20;

    /// 新玩家检测后延迟生成缓冲（ticks）
    static constexpr i32 DETECT_PLAYER_SPAWN_BUFFER = 40;

    /// 怪物追踪最大距离
    static constexpr f32 MAX_MOB_TRACKING_DISTANCE = 47.0f;

    /// 奖励弹出间隔（ticks）
    static constexpr i32 TIME_BETWEEN_EJECTIONS = 30;

    /// 每级不祥之兆对应试炼之兆的时长（ticks）
    static constexpr i32 TRIAL_OMEN_PER_BAD_OMEN_LEVEL = 18000;

    // ========== 状态机方法 ==========

    void tickInactive(IWorld& world);
    void tickWaitingForPlayers(IWorld& world);
    void tickActive(IWorld& world);
    void tickWaitingForRewardEjection(IWorld& world);
    void tickEjectingReward(IWorld& world);
    void tickCooldown(IWorld& world);

    // ========== 生成逻辑 ==========

    /**
     * @brief 尝试生成一个怪物
     *
     * 在spawnRange范围内寻找合适的生成位置，
     * 确定生成实体类型，生成实体并添加到世界，
     * 追踪生成的实体UUID。
     */
    void spawnMob(IWorld& world);

    /**
     * @brief 弹出奖励物品给指定玩家
     *
     * 从补给表或钥匙表随机选择一个战利品表，
     * 生成物品并弹出到世界中。
     */
    void ejectRewardForPlayer(IWorld& world, Player& player);

    /**
     * @brief 弹出奖励物品（无指定玩家时使用空上下文）
     */
    void ejectReward(IWorld& world);

    /**
     * @brief 检查已追踪的怪物是否还活着
     *
     * 遍历m_trackedMobs中的UUID，检查对应实体是否仍然存活，
     * 移除已死亡或离开追踪范围的实体UUID，更新m_currentMobsCount。
     */
    void updateTrackedMobs(IWorld& world);

    /**
     * @brief 通过UUID查找实体
     * @param world 世界引用
     * @param uuid 实体UUID
     * @return 实体指针，未找到返回nullptr
     */
    Entity* findEntityByUuid(IWorld& world, const std::string& uuid);

    /**
     * @brief 计算目标总怪物数
     * @param additionalPlayers 额外玩家数（总玩家数-1）
     */
    [[nodiscard]] i32 calculateTargetTotalMobs(i32 additionalPlayers) const;

    /**
     * @brief 计算目标同时怪物数
     * @param additionalPlayers 额外玩家数（总玩家数-1）
     */
    [[nodiscard]] i32 calculateTargetSimultaneousMobs(i32 additionalPlayers) const;

    /**
     * @brief 判断是否已生成完所有怪物
     */
    [[nodiscard]] bool hasFinishedSpawningAllMobs(i32 additionalPlayers) const;

    /**
     * @brief 判断当前存活怪物是否全部死亡
     */
    [[nodiscard]] bool haveAllCurrentMobsDied() const;

    /**
     * @brief 判断是否可以生成下一个怪物
     */
    [[nodiscard]] bool isReadyToSpawnNextMob(IWorld& world, i32 additionalPlayers) const;

    // ========== 生成逻辑辅助方法 ==========

    /**
     * @brief 从生成潜力列表中随机选择一个实体类型
     *
     * 使用加权随机选择算法。如果列表为空返回 nullptr。
     * 选中后自动缓存到 m_nextSpawnEntityId，下次生成时直接使用缓存。
     */
    [[nodiscard]] const ResourceLocation* _selectNextEntity(IWorld& world);

    /**
     * @brief 在刷怪笼附近寻找合适的生成位置
     *
     * 在 spawnRange 范围内随机选择位置，进行碰撞检测和视线检测。
     * 返回生成位置的中心坐标；如果找不到合适位置返回 std::nullopt。
     */
    [[nodiscard]] std::optional<Vector3> _findSpawnPosition(IWorld& world);

    // ========== 数据成员 ==========

    /// 当前状态
    State m_state = State::Inactive;

    /// 是否为不祥变体
    bool m_ominous = false;

    /// 刷怪笼配置
    Config m_config;

    /// 冷却结束tick
    i64 m_cooldownEndsAt = 0;

    /// 奖励弹出结束tick
    i64 m_ejectingRewardEndsAt = 0;

    /// 上次生成tick
    i64 m_lastSpawnTick = 0;

    /// 上次玩家扫描tick
    i64 m_lastPlayerScanTick = 0;

    /// 上次奖励弹出tick
    i64 m_lastEjectionTick = 0;

    /// 缓存的下一个生成实体类型ID（对应 MC Java 的 nextSpawnData）
    ResourceLocation m_nextSpawnEntityId;

    /// 已追踪的玩家UUID
    std::unordered_set<std::string> m_trackedPlayers;

    /// 已生成的怪物UUID
    std::unordered_set<std::string> m_trackedMobs;

    /// 已生成的怪物总数
    i32 m_spawnedMobsCount = 0;

    /// 当前存活的怪物数
    i32 m_currentMobsCount = 0;

    /// 需要生成的总怪物数（根据玩家数量动态计算）
    i32 m_totalMobsToSpawn = 0;

    /// 最大同时存在的怪物数（根据玩家数量动态计算）
    i32 m_maxSimultaneousMobs = 0;

    /// 当前正在弹出奖励的玩家UUID列表（用于EjectingReward状态逐个弹出）
    std::vector<std::string> m_detectedPlayerUuids;

    // 测试子类需要访问私有方法
    friend class TestTrialSpawnerBlockEntity;
};

} // namespace mc
