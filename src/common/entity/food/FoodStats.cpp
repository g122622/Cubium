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
#include "common/entity/effect/EffectType.hpp"
#include "common/entity/entities/player/Player.hpp"
#include <algorithm>

namespace mc {

// ============================================================================
// 常量
// ============================================================================

namespace {

/// 饥饿值上限
constexpr i32 MAX_FOOD_LEVEL = 20;

/// 消耗值上限（防止溢出）
constexpr f32 MAX_EXHAUSTION = 40.0f;

/// 消耗值阈值（每次消耗的量）
constexpr f32 EXHAUSTION_THRESHOLD = 4.0f;

/// 快速生命恢复间隔（ticks）
constexpr i32 FAST_REGEN_INTERVAL = 10;

/// 慢速生命恢复间隔（ticks）
constexpr i32 SLOW_REGEN_INTERVAL = 80;

/// 饥饿伤害间隔（ticks）
constexpr i32 STARVATION_INTERVAL = 80;

/// 快速恢复每次最大消耗饱和度
constexpr f32 FAST_REGEN_MAX_SATURATION = 6.0f;

/// 慢速恢复每次消耗的饱和度
constexpr f32 SLOW_REGEN_EXHAUSTION = 6.0f;

/// 和平模式生命恢复间隔（ticks）
constexpr i32 PEACEFUL_REGEN_INTERVAL = 20;

/// 和平模式饥饿值恢复间隔（ticks）
constexpr i32 PEACEFUL_FOOD_REGEN_INTERVAL = 10;

/// 和平模式生命恢复量
constexpr f32 PEACEFUL_REGEN_AMOUNT = 1.0f;

} // namespace

// ============================================================================
// FoodStats 实现
// ============================================================================

FoodStats::FoodStats() = default;

void FoodStats::tick(Player& player, Difficulty difficulty, bool naturalRegeneration)
{
    // 保存上一刻的饥饿值（用于 UI 动画）
    m_prevFoodLevel = m_foodLevel;

    // 和平模式特殊处理
    if (difficulty == Difficulty::Peaceful) {
        _handlePeacefulMode(player);
        // 和平模式下仍然消耗饱和度，但不消耗饥饿值
        _consumeExhaustion(difficulty);
        return;
    }

    // 1. 处理消耗值积累
    _consumeExhaustion(difficulty);

    // 检查玩家是否应该恢复生命（不死亡、无饥饿效果）
    bool shouldHeal = player.health() > 0.0f && player.health() < player.maxHealth();
    bool hasHungerEffect = player.hasEffect(entity::effect::EffectType::Hunger);

    // 2. 生命恢复逻辑（使用独立的计时器）
    if (naturalRegeneration && shouldHeal && !hasHungerEffect) {
        // 快速生命恢复（饱和度恢复）
        // 条件：foodLevel >= 20 且 saturation > 0
        if (m_foodLevel >= MAX_FOOD_LEVEL && m_saturationLevel > 0.0f) {
            m_foodTimer++;
            if (m_foodTimer >= FAST_REGEN_INTERVAL) {
                if (_performFastRegeneration(player)) {
                    m_foodTimer = 0;
                }
            }
        }
        // 慢速生命恢复（饥饿值恢复）
        // 条件：foodLevel >= 18
        else if (m_foodLevel >= 18) {
            m_foodTimer++;
            if (m_foodTimer >= SLOW_REGEN_INTERVAL) {
                if (_performSlowRegeneration(player)) {
                    m_foodTimer = 0;
                }
            }
        } else {
            // 饥饿值低于 18 时重置恢复计时器
            m_foodTimer = 0;
        }
    } else {
        // 不满足恢复条件时重置恢复计时器
        m_foodTimer = 0;
    }

    // 3. 饥饿伤害逻辑（使用独立的计时器）
    // 条件：foodLevel <= 0
    if (m_foodLevel <= 0) {
        m_starveTimer++;
        if (m_starveTimer >= STARVATION_INTERVAL) {
            _performStarvationDamage(player, difficulty);
            m_starveTimer = 0;
        }
    } else {
        // 饥饿值恢复后重置饥饿伤害计时器
        m_starveTimer = 0;
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
    // 当消耗值 >= 4.0 时，消耗饱和度或饥饿值
    while (m_exhaustionLevel >= EXHAUSTION_THRESHOLD) {
        m_exhaustionLevel -= EXHAUSTION_THRESHOLD;

        if (m_saturationLevel > 0.0f) {
            // 优先消耗饱和度
            m_saturationLevel = std::max(0.0f, m_saturationLevel - 1.0f);
        } else if (difficulty != Difficulty::Peaceful) {
            // 和平模式不消耗饥饿值
            m_foodLevel = std::max(0, m_foodLevel - 1);
        }
    }
}

bool FoodStats::_performFastRegeneration(Player& player)
{
    // 快速恢复：消耗饱和度来恢复生命
    // 每次恢复 saturation/6 点生命，消耗等量饱和度
    f32 saturationToUse = std::min(m_saturationLevel, FAST_REGEN_MAX_SATURATION);

    if (saturationToUse > 0.0f) {
        f32 healAmount = saturationToUse / 6.0f;
        player.heal(healAmount);
        addExhaustion(saturationToUse);
        return true;
    }

    return false;
}

bool FoodStats::_performSlowRegeneration(Player& player)
{
    // 慢速恢复：消耗饥饿值来恢复生命
    // 每次恢复 1 点生命，消耗 6.0 饱和度
    if (m_foodLevel > 0) {
        player.heal(1.0f);
        addExhaustion(SLOW_REGEN_EXHAUSTION);
        return true;
    }

    return false;
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

void FoodStats::_handlePeacefulMode(Player& player)
{
    // 和平模式：每 20 ticks 恢复 1 点生命
    m_foodTimer++;
    if (m_foodTimer % PEACEFUL_REGEN_INTERVAL == 0) {
        if (player.health() < player.maxHealth() && player.health() > 0.0f) {
            player.heal(PEACEFUL_REGEN_AMOUNT);
        }
    }

    // 和平模式：每 10 ticks 恢复 1 点饥饿值
    if (m_foodTimer % PEACEFUL_FOOD_REGEN_INTERVAL == 0) {
        if (m_foodLevel < MAX_FOOD_LEVEL) {
            m_foodLevel++;
        }
    }
}

} // namespace mc
