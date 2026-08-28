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

#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include <algorithm>

namespace mc {

// 前向声明
class Player;

/**
 * @brief 饥饿/饱食度系统
 *
 * 管理玩家的饥饿值、饱和度和消耗值。
 * 对齐 MC Java 1.21.11 FoodData（net/minecraft/world/food/FoodData.java）。
 *
 * 核心机制：
 * - foodLevel: 饥饿值 (0-20)，显示为 10 个鸡腿
 * - saturationLevel: 饱和度 (0-foodLevel)，先于饥饿值消耗
 * - exhaustionLevel: 累积消耗值，每攒满 4.0（严格大于）消耗 1 饱和度或饥饿值
 *
 * 生命恢复（对齐 FoodData.tick:44-59，受 NATURAL_HEALTH_REGENERATION 游戏规则门控，
 *   不查 Hunger 效果——Hunger 效果仅加速 exhaustion 累积，不阻止回血）：
 * - 快速恢复：foodLevel>=20 且 saturation>0 时，每 10 ticks 恢复 min(sat,6)/6 点生命
 * - 慢速恢复：foodLevel>=18 时，每 80 ticks 恢复 1 点生命
 * - 饥饿伤害：foodLevel<=0 时，每 80 ticks 造成 1 点 starve 伤害（按难度门控血量下限）
 *
 * 计时器：单一 tickTimer（m_foodTimer）共享回血/饿死/else-reset 三分支，对齐 vanilla
 *   FoodData 单 tickTimer 语义（状态切换时进度归零）。m_foodTimer 经 foodTimer()/setFoodTimer()
 *   序列化为 foodTickTimer，与 vanilla NBT 字段一致。
 */
class FoodStats {
public:
    /**
     * @brief 构造饥饿系统
     */
    FoodStats();

    /**
     * @brief 每刻更新
     *
     * 对齐 MC Java 1.21.11 FoodData.tick（FoodData.java:32-72）：
     * 1. 消耗值严格大于 4.0 时，扣 4.0 并消耗 1 饱和度（不足则扣 1 饥饿值，和平不扣饥饿值）
     * 2. 快速生命恢复（满饱 saturation>0，每 10 tick）
     * 3. 慢速生命恢复（foodLevel>=18，每 80 tick）
     * 4. 饥饿伤害（foodLevel<=0，每 80 tick，按难度血量下限门控）
     * 5. 以上不满足时 tickTimer 归零
     * 回血路径不查 Hunger 效果（Hunger 仅加速 exhaustion）。和平难度无特例——
     * foodLevel 恒 20、saturation 会被消耗，走通用回血分支（对齐 vanilla）。
     *
     * @param player 玩家引用
     * @param difficulty 当前难度
     * @param naturalRegeneration 是否启用自然恢复
     */
    void tick(Player& player, Difficulty difficulty, bool naturalRegeneration);

    /**
     * @brief 进食后增加饥饿值和饱和度
     *
     * 饱和度计算公式：saturation += food * saturationModifier * 2.0
     * 上限为当前 foodLevel
     *
     * @param food 饥饿值
     * @param saturationModifier 饱和度修正值
     */
    void addStats(i32 food, f32 saturationModifier);

    /**
     * @brief 增加消耗值
     *
     * 消耗值上限为 40.0，实际消耗发生在 tick() 中
     *
     * @param exhaustion 消耗值增量
     */
    void addExhaustion(f32 exhaustion);

    /**
     * @brief 检查是否需要食物
     * @return 如果饥饿值 < 20 返回 true
     */
    [[nodiscard]] bool needsFood() const { return m_foodLevel < 20; }

    /**
     * @brief 获取饥饿值
     */
    [[nodiscard]] i32 foodLevel() const { return m_foodLevel; }

    /**
     * @brief 设置饥饿值
     */
    void setFoodLevel(i32 level) { m_foodLevel = std::clamp(level, 0, 20); }

    /**
     * @brief 获取饱和度
     */
    [[nodiscard]] f32 saturationLevel() const { return m_saturationLevel; }

    /**
     * @brief 设置饱和度
     */
    void setSaturationLevel(f32 saturation) { m_saturationLevel = std::max(0.0f, saturation); }

    /**
     * @brief 获取消耗值
     */
    [[nodiscard]] f32 exhaustionLevel() const { return m_exhaustionLevel; }

    /**
     * @brief 设置消耗值
     */
    void setExhaustionLevel(f32 exhaustion) { m_exhaustionLevel = std::max(0.0f, exhaustion); }

    /**
     * @brief 获取上一刻的饥饿值（用于 UI 动画）
     */
    [[nodiscard]] i32 prevFoodLevel() const { return m_prevFoodLevel; }

    /**
     * @brief 获取食物计时器
     */
    [[nodiscard]] i32 foodTimer() const { return m_foodTimer; }

    /**
     * @brief 设置食物计时器
     */
    void setFoodTimer(i32 timer) { m_foodTimer = timer; }

private:
    /**
     * @brief 处理消耗值积累
     * @param difficulty 当前难度
     */
    void _consumeExhaustion(Difficulty difficulty);

    /**
     * @brief 执行快速生命恢复（饱和度恢复，对齐 FoodData.java:47-51）
     * @param player 玩家引用
     */
    void _performFastRegeneration(Player& player);

    /**
     * @brief 执行慢速生命恢复（饥饿值恢复，对齐 FoodData.java:55-58）
     * @param player 玩家引用
     */
    void _performSlowRegeneration(Player& player);

    /**
     * @brief 执行饥饿伤害
     * @param player 玩家引用
     * @param difficulty 当前难度
     */
    void _performStarvationDamage(Player& player, Difficulty difficulty);

private:
    i32 m_foodLevel = 20;         ///< 饥饿值 (0-20)
    f32 m_saturationLevel = 5.0f; ///< 饱和度 (0-foodLevel)
    f32 m_exhaustionLevel = 0.0f; ///< 累积消耗值 (0-40)
    i32 m_foodTimer = 0;          ///< 食物计时器（共享回血/饿死，对齐 vanilla tickTimer）
    i32 m_prevFoodLevel = 20;     ///< 上一刻的饥饿值（用于 UI 同步）
};

} // namespace mc
