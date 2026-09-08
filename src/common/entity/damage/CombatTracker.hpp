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

/**
 * @file CombatTracker.hpp
 * @brief 战斗追踪器 - 记录实体的战斗历史
 */

#pragma once

#include "../../core/Types.hpp"
#include "CombatEntry.hpp"
#include "common/entity/damage/DamageSource.hpp"
#include <cstddef>
#include <memory>
#include <string>
#include <vector>

namespace mc {

// 前向声明
class LivingEntity;
class Entity;

/**
 * @brief 战斗追踪器
 *
 * 记录实体受到的所有伤害事件，用于生成死亡消息和统计战斗数据。
 * 每个LivingEntity都有一个CombatTracker实例。
 */
class CombatTracker {
public:
    /**
     * @brief 构造函数
     * @param owner 拥有此追踪器的生物
     */
    explicit CombatTracker(LivingEntity* owner);

    /**
     * @brief 记录伤害事件
     * @param source 伤害来源
     * @param health 受伤前生命值
     * @param damage 伤害值
     */
    void trackDamage(DamageSource& source, f32 health, f32 damage);

    /**
     * @brief 重置追踪器
     *
     * 清除所有记录的战斗数据，通常在重生时调用。
     */
    void reset();

    /**
     * @brief 重新检查战斗状态（对齐 vanilla CombatTracker.recheckStatus）
     *
     * 检查战斗是否超时（战斗中 300 tick / 非战斗中 100 tick）或实体已死亡，
     * 若是则结束战斗状态并清空战斗条目。与 reset() 逻辑一致，但语义上是
     * "重新检查"而非"强制重置"——reset() 由 trackDamage 在记录新伤害前调用，
     * recheckStatus() 由 die()/tick() 在战斗结束后周期性调用以清理过期数据。
     *
     * 对齐 MC Java 1.21.11 CombatTracker.recheckStatus（CombatTracker.java:145-158）。
     */
    void recheckStatus();

    /**
     * @brief 获取最近的伤害来源
     * @return 最近的战斗条目，没有则返回nullptr
     */
    [[nodiscard]] const CombatEntry* getLastEntry() const;

    /**
     * @brief 获取最佳伤害来源（造成最多伤害的来源）
     * @return 最佳战斗条目，没有则返回nullptr
     */
    [[nodiscard]] const CombatEntry* getBestEntry() const;

    /**
     * @brief 获取最近的伤害者实体
     * @return 最近的造成伤害的实体，没有则返回nullptr
     */
    [[nodiscard]] Entity* getLastAttacker() const;

    /**
     * @brief 获取最佳伤害者实体（造成最多伤害的实体）
     *
     * 优先返回玩家，然后返回造成最多伤害的生物。
     *
     * @return 造成最多伤害的实体，没有则返回nullptr
     */
    [[nodiscard]] Entity* getBestAttacker() const;

    /**
     * @brief 获取最佳攻击者实体（用于 Target Goals）
     * @return 最佳攻击者
     */
    [[nodiscard]] LivingEntity* getBestAttackerLiving() const;

    /**
     * @brief 生成死亡消息
     * @return 死亡消息字符串
     */
    [[nodiscard]] std::string getDeathMessage() const;

    /**
     * @brief 检查是否有战斗记录
     */
    [[nodiscard]] bool hasCombat() const { return !m_entries.empty(); }

    /**
     * @brief 获取总承受伤害
     */
    [[nodiscard]] f32 getTotalDamage() const { return m_totalDamage; }

    /**
     * @brief 获取战斗时长（从第一条记录到最后一条）
     * @return 战斗时长（tick）
     */
    [[nodiscard]] i32 getCombatDuration() const;

    /**
     * @brief 获取战斗条目数量
     */
    [[nodiscard]] size_t getEntryCount() const { return m_entries.size(); }

    /**
     * @brief 检查是否在战斗中
     */
    [[nodiscard]] bool inCombat() const { return m_inCombat; }

    /**
     * @brief 获取战斗开始时间
     */
    [[nodiscard]] i32 combatStartTime() const { return m_combatStartTime; }

    /**
     * @brief 获取战斗结束时间
     */
    [[nodiscard]] i32 combatEndTime() const { return m_combatEndTime; }

    // 静态常量
    static constexpr i32 COMBAT_TIMEOUT = 100; // 战斗超时时间（5秒 = 100 tick）

private:
    /**
     * @brief 计算摔落后缀
     *
     * 根据方块类型确定摔落后缀（如梯子、藤蔓等）。
     */
    void _calculateFallSuffix();

    /**
     * @brief 清理过期的战斗条目
     * @param currentTime 当前时间
     */
    void _cleanupOldEntries(i32 currentTime);

    /**
     * @brief 更新最佳伤害记录
     */
    void _updateBestEntry();

    /**
     * @brief 获取最佳战斗条目（复杂逻辑）
     *
     * 优先选择玩家造成的伤害，然后选择最大的伤害。
     */
    [[nodiscard]] CombatEntry* _getBestCombatEntry();

    LivingEntity* m_owner;              // 拥有者
    std::vector<CombatEntry> m_entries; // 战斗记录
    f32 m_totalDamage = 0.0f;           // 总承受伤害
    size_t m_bestEntryIndex = 0;        // 最佳伤害记录索引

    i32 m_lastDamageTime = 0;    // 最后受伤时间
    i32 m_combatStartTime = 0;   // 战斗开始时间
    i32 m_combatEndTime = 0;     // 战斗结束时间
    bool m_inCombat = false;     // 是否在战斗中
    bool m_takingDamage = false; // 是否正在受到伤害
    std::string m_fallSuffix;    // 当前摔落后缀
};

} // namespace mc
