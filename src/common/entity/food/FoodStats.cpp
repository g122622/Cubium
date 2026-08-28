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

#include "FoodStats.hpp"
#include "common/core/Types.hpp"
#include "common/entity/combat/DifficultyHelper.hpp"
#include "common/entity/damage/DamageSource.hpp"
#include "common/entity/entities/player/Player.hpp"
#include <algorithm>

namespace mc {

// ============================================================================
// 常量
// ============================================================================

namespace {

/// 饥饿值上限（对齐 FoodConstants.MAX_FOOD=20）
constexpr i32 MAX_FOOD_LEVEL = 20;

/// 消耗值上限（对齐 FoodData.addExhaustion 第 101 行 Math.min(..., 40.0F)）
constexpr f32 MAX_EXHAUSTION = 40.0f;

/// 消耗值阈值（对齐 FoodConstants.EXHAUSTION_DROP=4.0；触发条件为严格大于）
constexpr f32 EXHAUSTION_THRESHOLD = 4.0f;

/// 快速生命恢复间隔（对齐 FoodConstants.HEALTH_TICK_COUNT_SATURATED=10）
constexpr i32 FAST_REGEN_INTERVAL = 10;

/// 慢速生命恢复间隔（对齐 FoodConstants.HEALTH_TICK_COUNT=80）
constexpr i32 SLOW_REGEN_INTERVAL = 80;

/// 饥饿伤害间隔（对齐 FoodConstants.HEALTH_TICK_COUNT=80）
constexpr i32 STARVATION_INTERVAL = 80;

/// 慢速恢复触发饥饿值下限（对齐 FoodConstants.HEAL_LEVEL=18）
constexpr i32 HEAL_LEVEL = 18;

/// 快速恢复每次最大消耗饱和度（对齐 FoodData.java:48 Math.min(saturation, 6.0F)）
constexpr f32 FAST_REGEN_MAX_SATURATION = 6.0f;

/// 慢速恢复每次消耗的 exhaustion（对齐 FoodData.java:57 addExhaustion(6.0F)）
constexpr f32 SLOW_REGEN_EXHAUSTION = 6.0f;

} // namespace

// ============================================================================
// FoodStats 实现
// ============================================================================

FoodStats::FoodStats() = default;

void FoodStats::tick(Player& player, Difficulty difficulty, bool naturalRegeneration)
{
    // 对齐 MC Java 1.21.11 FoodData.tick（FoodData.java:32-72）。
    // 保存上一刻的饥饿值（用于 UI 动画）
    m_prevFoodLevel = m_foodLevel;

    // 1. 处理消耗值积累（对齐 FoodData.java:35-42）。
    //    严格大于 4.0 才触发（非 >=），一次 tick 最多扣一次（非 while 循环）。
    _consumeExhaustion(difficulty);

    // 2/3/4. 回血 / 饿死（对齐 FoodData.java:44-71，单一 tickTimer 共享三分支）。
    //    回血门控只查 naturalRegeneration + isHurt()（health>0 && health<max），
    //    不查 Hunger 效果——Hunger 效果仅加速 exhaustion 累积，不阻止回血（对齐 vanilla）。
    const bool shouldHeal = player.health() > 0.0f && player.health() < player.maxHealth();

    if (naturalRegeneration && shouldHeal && m_foodLevel >= MAX_FOOD_LEVEL && m_saturationLevel > 0.0f) {
        // 快速生命恢复（满饱 saturation>0，每 10 tick）。对齐 FoodData.java:45-52。
        m_foodTimer++;
        if (m_foodTimer >= FAST_REGEN_INTERVAL) {
            _performFastRegeneration(player);
            m_foodTimer = 0;
        }
    } else if (naturalRegeneration && shouldHeal && m_foodLevel >= HEAL_LEVEL) {
        // 慢速生命恢复（foodLevel>=18，每 80 tick）。对齐 FoodData.java:53-59。
        m_foodTimer++;
        if (m_foodTimer >= SLOW_REGEN_INTERVAL) {
            _performSlowRegeneration(player);
            m_foodTimer = 0;
        }
    } else if (m_foodLevel <= 0) {
        // 饥饿伤害（foodLevel<=0，每 80 tick）。对齐 FoodData.java:60-68。
        m_foodTimer++;
        if (m_foodTimer >= STARVATION_INTERVAL) {
            _performStarvationDamage(player, difficulty);
            m_foodTimer = 0;
        }
    } else {
        // 其余情况（如满血不回血、foodLevel 在 1..17 之间）归零计时器。对齐 FoodData.java:69-70。
        m_foodTimer = 0;
    }
}

void FoodStats::addStats(i32 food, f32 saturationModifier)
{
    // 饥饿值增加，上限 20
    m_foodLevel = std::min(m_foodLevel + food, MAX_FOOD_LEVEL);

    // 饱和度计算公式：saturation += food * saturationModifier * 2.0
    // 上限为当前 foodLevel
    f32 saturationGain = static_cast<f32>(food) * saturationModifier * 2.0f;
    m_saturationLevel = std::min(m_saturationLevel + saturationGain, static_cast<f32>(m_foodLevel));
}

void FoodStats::addExhaustion(f32 exhaustion)
{
    // 累加消耗值，上限 40.0
    m_exhaustionLevel = std::min(m_exhaustionLevel + exhaustion, MAX_EXHAUSTION);
}

void FoodStats::_consumeExhaustion(Difficulty difficulty)
{
    // 对齐 FoodData.java:35-42：exhaustionLevel 严格大于 4.0 时，扣 4.0 并消耗 1 点
    //   饱和度（saturation>0）或 1 点饥饿值（saturation==0 且非和平）。
    // 用单次 if 而非 while：一次 tick 最多扣一次 4.0（对齐 vanilla——消耗节奏为"每攒 4.0 扣 1 点"，
    //   残留的 exhaustion 留待后续 tick 继续扣，不会一次 tick 扣多点）。
    //   旧实现用 while+>= 会一次 tick 扣多点 + 边界 4.0 误扣，偏离 vanilla（消耗过快）。
    if (m_exhaustionLevel > EXHAUSTION_THRESHOLD) {
        m_exhaustionLevel -= EXHAUSTION_THRESHOLD;

        if (m_saturationLevel > 0.0f) {
            // 优先消耗饱和度
            m_saturationLevel = std::max(0.0f, m_saturationLevel - 1.0f);
        } else if (difficulty != Difficulty::Peaceful) {
            // 和平模式不消耗饥饿值（对齐 FoodData.java:39 difficulty != PEACEFUL 门控）
            m_foodLevel = std::max(0, m_foodLevel - 1);
        }
    }
}

void FoodStats::_performFastRegeneration(Player& player)
{
    // 对齐 FoodData.java:47-51：f = min(saturation, 6.0)；heal(f/6.0)；addExhaustion(f)。
    // 进入本函数时 m_foodLevel>=20 && m_saturationLevel>0（见 tick 门控），故 f>0 必然，
    // 无需额外守卫（vanilla 亦无条件执行）。
    const f32 saturationToUse = std::min(m_saturationLevel, FAST_REGEN_MAX_SATURATION);
    player.heal(saturationToUse / 6.0f);
    addExhaustion(saturationToUse);
}

void FoodStats::_performSlowRegeneration(Player& player)
{
    // 对齐 FoodData.java:55-58：heal(1.0)；addExhaustion(6.0)。
    // 进入本函数时 m_foodLevel>=18（见 tick 门控），无需 foodLevel>0 守卫（vanilla 亦无条件执行）。
    player.heal(1.0f);
    addExhaustion(SLOW_REGEN_EXHAUSTION);
}

void FoodStats::_performStarvationDamage(Player& player, Difficulty difficulty)
{
    // 饥饿伤害，根据难度限制最小生命值
    f32 currentHealth = player.health();
    f32 minHealth = entity::combat::DifficultyHelper::getStarvationMinHealth(difficulty);

    // 和平模式不应到达这里
    if (minHealth >= player.maxHealth()) {
        return;
    }

    // 只有当前生命值高于最小生命值时才造成伤害
    if (currentHealth > minHealth) {
        // 使用饥饿伤害源
        auto starveSource = DamageSources::starve();
        player.hurt(starveSource, 1.0f);
    }
}

} // namespace mc
