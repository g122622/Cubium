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
#include "common/item/core/ItemStack.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/LinkedHashSet.hpp"
#include "common/world/blockentity/BlockEntity.hpp"
#include <memory>
#include <string>
#include <vector>
#include <nlohmann/json_fwd.hpp>

namespace mc {

class Player;
class ItemEntity;

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
 *   从战利品表生成物品列表，每隔一定tick依次弹出每个物品。
 *   弹出完成后再检测玩家回到 ACTIVE 或 INACTIVE。
 *
 * 配置：
 *   普通宝库: keyItem=TrialKey, lootTable=reward
 *   不祥宝库: keyItem=OminousTrialKey, lootTable=reward_ominous
 *
 * 参考: VaultBlockEntity
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
     * 1. 当前状态允许弹出奖励（非INACTIVE）
     * 2. 钥匙类型匹配
     * 3. 玩家未领取过奖励
     */
    bool tryInsertKey(Player& player);

    // ========== 红石比较器 ==========

    /**
     * @brief 获取红石比较器输出信号
     * Active/Inactive=0, Unlocking/Ejecting=15
     */
    [[nodiscard]] i32 getComparatorOutput() const;

private:
    // ========== 常量 ==========

    /// 解锁动画持续时间（ticks）
    static constexpr i32 UNLOCKING_DURATION = 14;

    /// 每个物品弹出间隔（ticks）
    static constexpr i32 EJECTION_INTERVAL = 20;

    /// 最后一个物品弹出后等待时间（ticks）
    static constexpr i32 EJECTION_AFTER_LAST_DURATION = 20;

    /// 状态更新扫描间隔（ticks）
    static constexpr i32 STATE_UPDATE_INTERVAL = 20;

    /// 已领取奖励玩家上限
    static constexpr i32 MAX_REWARDED_PLAYERS = 128;

    /// 插入失败音效最小间隔（ticks），防刷
    static constexpr i32 INSERT_FAIL_SOUND_COOLDOWN = 15;

    /// 物品弹出速度
    static constexpr f32 EJECT_VELOCITY = 2.0f;

    // ========== 状态机方法 ==========

    void tickInactive(IWorld& world);
    void tickActive(IWorld& world);
    void tickUnlocking(IWorld& world);
    void tickEjecting(IWorld& world);

    // ========== 奖励逻辑 ==========

    /**
     * @brief 从战利品表解析待弹出的物品列表
     * @param world 世界引用
     * @param player 解锁的玩家
     * @return 待弹出的物品列表
     */
    std::vector<ItemStack> resolveItemsToEject(IWorld& world, Player& player);

    /**
     * @brief 弹出下一个物品到世界中
     * @param world 世界引用
     */
    void ejectNextItem(IWorld& world);

    /**
     * @brief 检测范围内的未奖励玩家
     * @param world 世界引用
     * @param range 检测范围
     * @return 范围内的玩家列表
     */
    std::vector<Player*> detectPlayers(IWorld& world, f32 range);

    /**
     * @brief 通过UUID查找玩家
     * @param world 世界引用
     * @param uuid 玩家UUID
     * @return 玩家指针，未找到返回nullptr
     */
    static Player* findPlayerByUuid(IWorld& world, const std::string& uuid);

    // ========== 数据成员 ==========

    /// 当前状态
    State m_state = State::Inactive;

    /// 是否为不祥宝库
    bool m_ominous = false;

    /// 配置
    Config m_config;

    /// 已领取奖励的玩家UUID集合（按插入顺序排列，最多MAX_REWARDED_PLAYERS人）
    /// 参考 MC Java VaultServerData 使用 ObjectLinkedOpenHashSet 保持插入顺序，
    /// 超上限时按 FIFO 策略淘汰最早插入的玩家
    LinkedHashSet<std::string> m_rewardedPlayers;

    /// 解锁动画开始tick
    i64 m_unlockingStartTick = 0;

    /// 当前弹出阶段结束tick
    i64 m_ejectionEndTick = 0;

    /// 当前正在解锁的玩家UUID
    std::string m_unlockingPlayerUuid;

    /// 待弹出的物品列表
    std::vector<ItemStack> m_itemsToEject;

    /// 本轮需要弹出的物品总数（用于计算弹出进度）
    i32 m_totalEjectionsNeeded = 0;

    /// 上次插入失败音效时间（防止音效刷屏）
    i64 m_lastInsertFailSoundTick = 0;

    /// 上次状态更新tick（用于控制检测频率）
    i64 m_lastStateUpdateTick = 0;
};

} // namespace mc
