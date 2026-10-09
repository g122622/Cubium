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
 * @file CombatEntry.hpp
 * @brief 战斗条目 - 记录单次伤害事件
 */

#pragma once

#include "DamageSource.hpp"
#include "common/core/Types.hpp"
#include <memory>
#include <string>

namespace mc {

/**
 * @brief 战斗条目
 *
 * 记录一次伤害事件的详细信息，用于生成死亡消息。
 */
class CombatEntry {
public:
    /**
     * @brief 构造函数
     * @param source 伤害来源（会被移动）
     * @param damage 伤害值
     * @param timestamp 发生时间（tick）
     * @param health 受伤前生命值
     * @param fallSuffix 摔落后缀（如 "fall" 或 "ladder"）
     * @param fallDistance 摔落距离
     * @param trueSourceId 真凶（causingEntity）实体 id，在受伤的同步上下文捕获
     * @param directSourceId 直接来源（directEntity）实体 id，同上
     *
     * 【为什么存 id 而不是直接用 DamageSource 的裸指针】
     * `m_source` 是 DamageSource 的 clone，其中的实体指针是**裸指针**。vanilla 靠 Java GC
     * 保证 `getLastDamageSource().getEntity()` 引用安全；Cubium 无 GC，真凶实体析构后
     * 该指针即悬垂，解引用（取 id / 做 dynamic_cast）就是 UAF 段错误。
     * 故在 trackDamage 的同步上下文（伤害发生瞬间，真凶必活）把 id 捕获下来——id 永不悬垂
     * （EntityInstanceId 单调递增、不复用），使用方经 `IWorld::getEntity(id)` 校验后
     * 才解引用。与 LivingEntity::m_lastDamageSourceTrueId 是同一套 UAF 根治思路。
     */
    CombatEntry(std::unique_ptr<DamageSource> source,
        f32 damage,
        i32 timestamp,
        f32 health,
        const std::string& fallSuffix,
        f32 fallDistance,
        EntityInstanceId trueSourceId,
        EntityInstanceId directSourceId);

    // 移动构造和移动赋值
    CombatEntry(CombatEntry&& other) noexcept;
    CombatEntry& operator=(CombatEntry&& other) noexcept;

    // 禁止拷贝（因为持有 unique_ptr）
    CombatEntry(const CombatEntry&) = delete;
    CombatEntry& operator=(const CombatEntry&) = delete;

    /**
     * @brief 获取伤害来源
     *
     * 【安全警告】返回值内持有真凶/直接来源的**裸指针**，真凶析构后即悬垂。
     * 需要访问伤害来源实体时，请用 `trueSourceId()` / `directSourceId()` 配合
     * `IWorld::getEntity(id)` 校验，不要解引用本对象内部的实体指针。
     * 本方法仅用于访问 DamageSource 自身的属性（type/isFire/deathMessageKey 等）。
     */
    [[nodiscard]] const DamageSource* source() const { return m_source.get(); }

    /**
     * @brief 真凶（causingEntity）实体 id；无实体来源时为 INVALID_ENTITY_ID
     */
    [[nodiscard]] EntityInstanceId trueSourceId() const { return m_trueSourceId; }

    /**
     * @brief 直接来源（directEntity）实体 id；无实体来源时为 INVALID_ENTITY_ID
     *
     * 近战 = 攻击者本身；箭矢 = 箭矢实体（真凶是射击者）。对齐 vanilla
     * `DamageSource.getDirectEntity()` 与 `getEntity()` 的区分。
     */
    [[nodiscard]] EntityInstanceId directSourceId() const { return m_directSourceId; }

    /**
     * @brief 获取伤害值
     */
    [[nodiscard]] f32 damage() const { return m_damage; }

    /**
     * @brief 获取发生时间
     */
    [[nodiscard]] i32 timestamp() const { return m_timestamp; }

    /**
     * @brief 获取受伤前生命值
     */
    [[nodiscard]] f32 health() const { return m_health; }

    /**
     * @brief 获取摔落后缀
     */
    [[nodiscard]] const std::string& fallSuffix() const { return m_fallSuffix; }

    /**
     * @brief 获取摔落距离
     */
    [[nodiscard]] f32 fallDistance() const { return m_fallDistance; }

    /**
     * @brief 是否来自生物
     */
    [[nodiscard]] bool isLivingSource() const;

    /**
     * @brief 是否来自玩家
     */
    [[nodiscard]] bool isPlayerSource() const;

    /**
     * @brief 获取伤害量（用于摔落计算）
     *
     * 虚空伤害返回 FLT_MAX
     */
    [[nodiscard]] f32 getDamageAmount() const;

private:
    std::unique_ptr<DamageSource> m_source;
    f32 m_damage;
    i32 m_timestamp;
    f32 m_health;             // 受伤前生命值
    std::string m_fallSuffix; // 摔落后缀（如 "fall", "ladder"）
    f32 m_fallDistance;       // 摔落距离
    // 实体 id（在 trackDamage 的同步上下文捕获，见构造函数注释）。id 永不悬垂。
    EntityInstanceId m_trueSourceId = INVALID_ENTITY_ID;
    EntityInstanceId m_directSourceId = INVALID_ENTITY_ID;
};

} // namespace mc
