#include "DifficultyHelper.hpp"

namespace mc::entity::combat {

f32 DifficultyHelper::adjustPlayerDamage(Difficulty difficulty, f32 damage)
{
    switch (difficulty) {
        case Difficulty::Peaceful:
            return 0.0f; // 和平模式下玩家不受怪物伤害（由其他逻辑处理）
        case Difficulty::Easy:
            // MC 1.16.5 PlayerEntity.java:858-859
            // if (this.world.getDifficulty() == Difficulty.EASY) {
            //     amount = Math.min(amount / 2.0F + 1.0F, amount);
            // }
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
    // MC 1.16.5 FireBlock: difficulty * 7
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
    // 参考 MC 1.16.5 DifficultyInstance
    // 区域难度 = 难度ID * 倍率，其中倍率基于多种因素计算
    // 这里返回基值，实际区域难度需要更多因素
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

} // namespace mc::entity::combat
