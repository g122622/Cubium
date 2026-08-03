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

#include "DifficultyHelper.hpp"
#include "common/core/Types.hpp"
#include <algorithm>

namespace mc::entity::combat {

f32 DifficultyHelper::adjustPlayerDamage(Difficulty difficulty, f32 damage)
{
    switch (difficulty) {
        case Difficulty::Peaceful:
            return 0.0f; // 和平模式下玩家不受怪物伤害（由其他逻辑处理）
        case Difficulty::Easy:
            // 伤害减半后加1，不超过原伤害
            // 例如：damage=10 -> min(5+1, 10) = 6
            return std::min(damage / 2.0f + 1.0f, damage);
        case Difficulty::Normal:
            return damage; // 1.0x
        case Difficulty::Hard:
            return damage * HARD_PLAYER_DAMAGE_MULT; // 1.5x
        default:
            return damage;
    }
}

f32 DifficultyHelper::getMobDamageAdjustment(Difficulty difficulty)
{
    switch (difficulty) {
        case Difficulty::Peaceful:
            return 0.0f; // 和平模式下怪物不会攻击
        case Difficulty::Easy:
            return EASY_MOB_DAMAGE_ADJ; // -2
        case Difficulty::Normal:
            return NORMAL_MOB_DAMAGE_ADJ; // 0
        case Difficulty::Hard:
            return HARD_MOB_DAMAGE_ADJ; // +2
        default:
            return NORMAL_MOB_DAMAGE_ADJ;
    }
}

f32 DifficultyHelper::getMobDamageAdjustment(i32 difficultyId)
{
    switch (difficultyId) {
        case 0:
            return 0.0f; // Peaceful
        case 1:
            return EASY_MOB_DAMAGE_ADJ; // Easy: -2
        case 2:
            return NORMAL_MOB_DAMAGE_ADJ; // Normal: 0
        case 3:
            return HARD_MOB_DAMAGE_ADJ; // Hard: +2
        default:
            return NORMAL_MOB_DAMAGE_ADJ;
    }
}

f32 DifficultyHelper::getStarvationMinHealth(Difficulty difficulty)
{
    switch (difficulty) {
        case Difficulty::Peaceful:
            return 20.0f; // 和平模式不受饥饿伤害，返回最大生命值
        case Difficulty::Easy:
            return EASY_STARVATION_MIN; // 10.0 (5颗心)
        case Difficulty::Normal:
            return NORMAL_STARVATION_MIN; // 1.0 (半颗心)
        case Difficulty::Hard:
            return HARD_STARVATION_MIN; // 0.0 (可饿死)
        default:
            return NORMAL_STARVATION_MIN;
    }
}

f32 DifficultyHelper::getFireDurationMultiplier(Difficulty difficulty)
{
    switch (difficulty) {
        case Difficulty::Peaceful:
            return 0.25f;
        case Difficulty::Easy:
            return 0.5f;
        case Difficulty::Normal:
            return 1.0f;
        case Difficulty::Hard:
            return 1.5f;
        default:
            return 1.0f;
    }
}

i32 DifficultyHelper::getFireSpreadBonus(Difficulty difficulty)
{
    // 火焰蔓延概率加成 = difficulty * 7
    return static_cast<i32>(difficulty) * 7;
}

bool DifficultyHelper::canZombieReinforce(Difficulty difficulty)
{
    // 只有困难模式下僵尸才召唤增援
    return difficulty == Difficulty::Hard;
}

f32 DifficultyHelper::getVillagerInfectionChance(Difficulty difficulty)
{
    switch (difficulty) {
        case Difficulty::Peaceful:
        case Difficulty::Easy:
            return 0.0f; // 0%
        case Difficulty::Normal:
            return 0.5f; // 50%
        case Difficulty::Hard:
            return 1.0f; // 100%
        default:
            return 0.0f;
    }
}

i32 DifficultyHelper::getRaidWaves(Difficulty difficulty)
{
    switch (difficulty) {
        case Difficulty::Peaceful:
            return 0; // 和平模式不会发生袭击
        case Difficulty::Easy:
            return 3;
        case Difficulty::Normal:
            return 5;
        case Difficulty::Hard:
            return 7;
        default:
            return 0;
    }
}

bool DifficultyHelper::allowsMobSpawning(Difficulty difficulty)
{
    return difficulty != Difficulty::Peaceful;
}

f32 DifficultyHelper::getRegionalDifficultyBase(Difficulty difficulty)
{
    // 区域难度 = 难度ID * 倍率，这里返回基值
    switch (difficulty) {
        case Difficulty::Peaceful:
            return 0.0f;
        case Difficulty::Easy:
            return 0.75f; // Easy 的基础倍率
        case Difficulty::Normal:
            return 1.0f;
        case Difficulty::Hard:
            return 1.0f;
        default:
            return 1.0f;
    }
}

f32 DifficultyHelper::getRangedAttackInaccuracy(Difficulty difficulty)
{
    // 对应 MC 原版 CrossbowAttackMob.performCrossbowAttack：
    // 14 - difficulty.getId() * 4
    // 难度越高不精确度越低，射击越精准
    return static_cast<f32>(14 - static_cast<i32>(difficulty) * 4);
}

f32 DifficultyHelper::getBreezeWindChargeInaccuracy(Difficulty difficulty)
{
    // 对应 MC 1.21.11 Shoot.tick() 中发射 BreezeWindCharge 的公式：
    // 5 - difficulty.getId() * 4
    // 各难度返回值：Peaceful=5, Easy=1, Normal=-3, Hard=-7
    // Normal/Hard 难度下为负数，由于分布对称性，负值与同绝对值的正值散布效果相同，
    // ProjectileEntity::shoot 内部会取绝对值处理。
    return static_cast<f32>(5 - static_cast<i32>(difficulty) * 4);
}

} // namespace mc::entity::combat
