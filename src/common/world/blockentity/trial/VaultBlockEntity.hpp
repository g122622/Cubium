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
 * copies of substantial portions of the Software.
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
#include "common/world/blockentity/BlockEntity.hpp"
#include <unordered_set>

namespace mc {

class Player;

/**
 * @brief 宝库方块实体
 *
 * 试炼密室的核心奖励方块，需要对应的钥匙才能解锁。
 * 每个玩家只能解锁一次同一宝库。
 * 解锁后从战利品表中弹出奖励物品。
 *
 * 状态机（4种状态）：
 *   INACTIVE → ACTIVE → UNLOCKING → EJECTING → ACTIVE (循环)
 *
 * 战利品弹出逻辑：
 *   1. 80%概率从稀有表抽1次，20%概率从普通表抽1次
 *   2. 总是从普通表抽1-3次
 *   3. 普通宝库25%概率从独有表抽1次；不祥宝库75%概率
 *
 * 配置：
 *   普通宝库: keyItem=TrialKey, lootTable=reward
 *   不祥宝库: keyItem=OminousTrialKey, lootTable=reward_ominous
 *
 * 参考: net.minecraft.block.entity.VaultBlockEntity
 */
class VaultBlockEntity : public BlockEntity {
public:
    /**
     * @brief 宝库状态
     */
    enum class State : u8 { Inactive = 0, Active = 1, Unlocking = 2, Ejecting = 3 };

    /**
     * @brief 宝库配置
     */
    struct Config {
        /// 战利品表（普通宝库）
        ResourceLocation lootTable = ResourceLocation("minecraft", "chests/trial_chambers/reward");
        /// 不祥宝库战利品表
        ResourceLocation ominousLootTable = ResourceLocation("minecraft", "chests/trial_chambers/reward_ominous");
        /// 玩家激活范围
        f32 activationRange = 4.0f;
        /// 玩家失活范围
        f32 deactivationRange = 4.5f;
        /// 钥匙物品（普通宝库: TrialKey, 不祥宝库: OminousTrialKey）
        const class Item* keyItem = nullptr;
    };

    /**
     * @brief 获取普通宝库配置
     */
    static Config getNormalConfig();

    /**
     * @brief 获取不祥宝库配置
     */
    static Config getOminousConfig();

    explicit VaultBlockEntity(const BlockPos& pos);

    // ========== BlockEntity 接口 ==========

    void tick(IWorld& world) override;
    [[nodiscard]] bool needsTick() const noexcept override { return true; }
    bool load(const nlohmann::json& data) override;
    void save(nlohmann::json& data) const override;
    [[nodiscard]] std::unique_ptr<BlockEntity> clone() const override;

    // ========== 状态访问 ==========

    [[nodiscard]] State getState() const noexcept { return m_state; }
    void setState(State state);

    [[nodiscard]] bool isOminous() const noexcept { return m_ominous; }
    void setOminous(bool ominous);

    [[nodiscard]] const Config& getConfig() const noexcept { return m_config; }
    void setConfig(const Config& config);

    // ========== 钥匙交互 ==========

    /**
     * @brief 尝试插入钥匙
     * @param player 尝试插入钥匙的玩家
     * @return 是否成功插入并开始解锁
     *
     * 检查：
     * 1. 钥匙类型是否匹配
     * 2. 玩家是否已经领取过奖励
     * 3. 当前状态是否允许插入
     */
    bool tryInsertKey(Player& player);

    // ========== 红石比较器 ==========

    /**
     * @brief 获取红石比较器输出信号
     * Active=0, Unlocking/Ejecting=15
     */
    [[nodiscard]] i32 getComparatorOutput() const;

private:
    // ========== 状态机方法 ==========

    void tickInactive(IWorld& world);
    void tickActive(IWorld& world);
    void tickUnlocking(IWorld& world);
    void tickEjecting(IWorld& world);

    // ========== 奖励逻辑 ==========

    /**
     * @brief 弹出战利品奖励
     *
     * 战利品抽取规则：
     * 1. 80%概率从稀有表抽1次，20%概率从普通表抽1次
     * 2. 总是从普通表抽1-3次
     * 3. 普通宝库25%概率从独有表抽1次；不祥宝库75%概率
     */
    void ejectReward(IWorld& world, Player& player);

    /**
     * @brief 检测范围内的玩家
     */
    std::vector<Player*> detectPlayers(IWorld& world);

    // ========== 数据成员 ==========

    /// 当前状态
    State m_state = State::Inactive;

    /// 是否为不祥宝库
    bool m_ominous = false;

    /// 配置
    Config m_config;

    /// 已领取奖励的玩家UUID集合（最多128人）
    std::unordered_set<std::string> m_rewardedPlayers;

    /// 解锁动画开始tick
    i64 m_unlockingStartTick = 0;

    /// 弹出结束tick
    i64 m_ejectingEndTick = 0;

    /// 当前正在解锁的玩家UUID
    std::string m_unlockingPlayerUuid;

    /// 解锁动画持续时间（ticks）
    static constexpr i32 UNLOCKING_DURATION = 40; // 2秒

    /// 弹出动画持续时间（ticks）
    static constexpr i32 EJECTING_DURATION = 60; // 3秒

    /// 最大已领取玩家数
    static constexpr i32 MAX_REWARDED_PLAYERS = 128;
};

} // namespace mc
