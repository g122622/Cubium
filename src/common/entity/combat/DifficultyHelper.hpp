#pragma once

#include "../../core/Types.hpp"

namespace mc {

// 前向声明
class IWorld;

namespace entity::combat {

/**
 * @brief 难度相关工具类
 *
 * 提供难度相关的游戏机制计算：
 * - 玩家受到的伤害缩放
 * - 怪物攻击伤害调整
 * - 火焰持续时间调整
 * - 饥饿伤害限制
 *
 * 参考 MC 1.16.5 Difficulty 和相关类
 */
class DifficultyHelper {
public:
    // ========== 伤害缩放 ==========

    /**
     * @brief 获取玩家受伤的伤害倍率
     *
     * MC 1.16.5 PlayerEntity.attackEntityFrom():
     * - Peaceful: 不受怪物伤害（由其他逻辑处理）
     * - Easy: min(damage/2 + 1, damage) - 伤害减半后加1，不超过原伤害
     * - Normal: 1.0x
     * - Hard: 1.5x
     *
     * 注意：Easy 难度不是简单的 0.5 倍率！
     * 例如：damage=10 -> min(5+1, 10) = 6，而不是 5
     *
     * @param difficulty 难度
     * @param damage 原始伤害值（仅 Easy 难度需要）
     * @return 调整后的伤害值
     */
    [[nodiscard]] static f32 adjustPlayerDamage(Difficulty difficulty, f32 damage);

    /**
     * @brief 获取怪物攻击的伤害调整值
     *
     * MC 1.16.5 中怪物攻击伤害根据难度调整：
     * - Peaceful: 不攻击（由其他逻辑处理）
     * - Easy: -2
     * - Normal: 0
     * - Hard: +2
     *
     * @param difficulty 难度
     * @return 伤害调整值
     */
    [[nodiscard]] static f32 getMobDamageAdjustment(Difficulty difficulty);

    /**
     * @brief 获取怪物攻击的伤害调整值（简单枚举版本）
     *
     * @param difficultyId 难度ID (0-3)
     * @return 伤害调整值
     */
    [[nodiscard]] static f32 getMobDamageAdjustment(i32 difficultyId);

    // ========== 饥饿系统 ==========

    /**
     * @brief 获取饥饿伤害的最小生命值
     *
     * MC 1.16.5 中饥饿伤害不能低于此值：
     * - Peaceful: 不受饥饿伤害
     * - Easy: 10.0 (5颗心)
     * - Normal: 1.0 (半颗心)
     * - Hard: 0.0 (可饿死)
     *
     * @param difficulty 难度
     * @return 最小生命值
     */
    [[nodiscard]] static f32 getStarvationMinHealth(Difficulty difficulty);

    // ========== 火焰 ==========

    /**
     * @brief 获取火焰燃烧的持续时间倍率
     *
     * MC 1.16.5 中火焰持续时间受难度影响：
     * - Peaceful: 0.25x
     * - Easy: 0.5x
     * - Normal: 1.0x
     * - Hard: 1.5x
     *
     * 注意：这个倍率主要用于火焰蔓延，玩家着火时间由其他逻辑控制
     *
     * @param difficulty 难度
     * @return 持续时间倍率
     */
    [[nodiscard]] static f32 getFireDurationMultiplier(Difficulty difficulty);

    /**
     * @brief 获取火焰蔓延的额外概率
     *
     * MC 1.16.5 FireBlock 中火焰蔓延概率与难度相关
     * 公式: (base + 40 + difficulty * 7) / (age + 30)
     *
     * @param difficulty 难度
     * @return 额外概率加成 (0, 7, 14, 21)
     */
    [[nodiscard]] static i32 getFireSpreadBonus(Difficulty difficulty);

    // ========== 特殊机制 ==========

    /**
     * @brief 检查僵尸是否可以召唤增援
     *
     * 只有 Hard 难度下僵尸才召唤增援
     *
     * @param difficulty 难度
     * @return 是否可以召唤增援
     */
    [[nodiscard]] static bool canZombieReinforce(Difficulty difficulty);

    /**
     * @brief 获取村民感染概率
     *
     * MC 1.16.5 中僵尸杀死村民时的感染概率：
     * - Peaceful: 0%
     * - Easy: 0%
     * - Normal: 50%
     * - Hard: 100%
     *
     * @param difficulty 难度
     * @return 感染概率 (0.0 - 1.0)
     */
    [[nodiscard]] static f32 getVillagerInfectionChance(Difficulty difficulty);

    /**
     * @brief 获取袭击的波次数
     *
     * MC 1.16.5 Raid.getWaves()
     * - Peaceful: 0
     * - Easy: 3
     * - Normal: 5
     * - Hard: 7
     *
     * @param difficulty 难度
     * @return 波次数
     */
    [[nodiscard]] static i32 getRaidWaves(Difficulty difficulty);

    /**
     * @brief 检查是否允许怪物生成
     *
     * Peaceful 难度不允许怪物生成
     *
     * @param difficulty 难度
     * @return 是否允许怪物生成
     */
    [[nodiscard]] static bool allowsMobSpawning(Difficulty difficulty);

    /**
     * @brief 获取区域难度基值
     *
     * MC 1.16.5 DifficultyInstance 中使用
     * 区域难度 = 难度ID * 倍率
     *
     * @param difficulty 难度
     * @return 基值 (0.0, 0.75, 1.0, 1.0)
     */
    [[nodiscard]] static f32 getRegionalDifficultyBase(Difficulty difficulty);

private:
    // 常量
    static constexpr f32 NORMAL_PLAYER_DAMAGE_MULT = 1.0f;
    static constexpr f32 HARD_PLAYER_DAMAGE_MULT = 1.5f;

    static constexpr f32 EASY_MOB_DAMAGE_ADJ = -2.0f;
    static constexpr f32 NORMAL_MOB_DAMAGE_ADJ = 0.0f;
    static constexpr f32 HARD_MOB_DAMAGE_ADJ = 2.0f;

    static constexpr f32 EASY_STARVATION_MIN = 10.0f;
    static constexpr f32 NORMAL_STARVATION_MIN = 1.0f;
    static constexpr f32 HARD_STARVATION_MIN = 0.0f;
};

} // namespace entity::combat
} // namespace mc
