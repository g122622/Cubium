#pragma once

#include "../Goal.hpp"
#include "../../../../core/Types.hpp"

namespace mc {

// 前向声明
class CreatureEntity;
class LivingEntity;

namespace entity::ai::goal {

/**
 * @brief 近战攻击目标
 *
 * 使生物攻击目标实体。
 *
 * 参考 MC 1.16.5 MeleeAttackGoal
 */
class MeleeAttackGoal : public Goal {
public:
    /**
     * @brief 构造函数
     * @param creature 生物实体
     * @param speed 移动速度倍率
     * @param useLongMemory 是否使用长期记忆（目标丢失后继续追踪）
     */
    MeleeAttackGoal(CreatureEntity* creature, f64 speed, bool useLongMemory = false);

    [[nodiscard]] bool shouldExecute() override;
    [[nodiscard]] bool shouldContinueExecuting() override;
    void startExecuting() override;
    void resetTask() override;
    void tick() override;

    [[nodiscard]] String getTypeName() const override { return "MeleeAttackGoal"; }

protected:
    /**
     * @brief 检查是否可以攻击目标
     * @param target 目标实体
     * @return 是否可以攻击
     */
    [[nodiscard]] bool canAttack(LivingEntity* target) const;

    /**
     * @brief 检查并执行攻击
     * MC 1.16.5: 检查距离和冷却后执行攻击
     * @param target 目标实体
     * @param distToEnemySqr 到敌人的距离平方
     */
    void checkAndPerformAttack(LivingEntity* target, f64 distToEnemySqr);

    /**
     * @brief 执行攻击
     * @param target 目标实体
     */
    void attackTarget(LivingEntity* target);

    /**
     * @brief 计算攻击距离平方
     * 参考 MC 1.16.5: (this.attacker.getWidth() * 2.0F) * (this.attacker.getWidth() * 2.0F) + target.getWidth()
     */
    [[nodiscard]] f32 getAttackReachSqr(LivingEntity* target) const;

    CreatureEntity* m_creature;
    f64 m_speed;
    bool m_useLongMemory;
    LivingEntity* m_attackTarget = nullptr;
    i32 m_attackCooldown = 0;
    i32 m_pathRecalculateTimer = 0;
    f64 m_targetX = 0.0;
    f64 m_targetY = 0.0;
    f64 m_targetZ = 0.0;
    i32 m_failedPathFindingPenalty = 0;
    bool m_canPenalize = false;  // MC 1.16.5: 路径失败惩罚开关

    static constexpr i32 ATTACK_COOLDOWN_TICKS = 20; // 攻击冷却（ticks）
    static constexpr f32 STOP_ATTACK_DISTANCE = 32.0f; // 停止追踪距离
};

} // namespace entity::ai::goal
} // namespace mc
