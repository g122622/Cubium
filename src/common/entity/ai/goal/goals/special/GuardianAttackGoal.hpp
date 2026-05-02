#pragma once

#include "../../Goal.hpp"
#include "../../../../../core/Types.hpp"

namespace mc {

// 前向声明
class GuardianEntity;
class LivingEntity;

namespace entity::ai::goal {

/**
 * @brief 守卫者激光攻击目标
 *
 * 守卫者特有的激光攻击行为：
 * - 充能阶段：持续 60 tick（3秒）
 * - 发射阶段：造成伤害
 * - 冷却阶段：短暂冷却后可再次攻击
 *
 * 参考 MC 1.16.5 GuardianEntity.AttackGoal
 */
class GuardianAttackGoal : public Goal {
public:
    /**
     * @brief 构造函数
     * @param guardian 守卫者实体
     */
    explicit GuardianAttackGoal(GuardianEntity* guardian);

    [[nodiscard]] bool shouldExecute() override;
    [[nodiscard]] bool shouldContinueExecuting() override;
    void startExecuting() override;
    void resetTask() override;
    void tick() override;

    [[nodiscard]] String getTypeName() const override { return "GuardianAttackGoal"; }

private:
    /**
     * @brief 选择攻击目标
     * MC 1.16.5: 选择最近的玩家或鱿鱼
     * @return 目标实体，如果没有则返回 nullptr
     */
    [[nodiscard]] LivingEntity* selectTarget() const;

    /**
     * @brief 检查目标是否有效
     * @param target 目标实体
     * @return 是否有效
     */
    [[nodiscard]] bool isTargetValid(LivingEntity* target) const;

    /**
     * @brief 更新激光攻击
     */
    void updateLaserAttack();

    /**
     * @brief 执行激光攻击
     * @param target 目标实体
     */
    void performLaserAttack(LivingEntity* target);

    GuardianEntity* m_guardian;
    LivingEntity* m_target = nullptr;
    i32 m_attackTime = 0;          // 攻击计时器
    bool m_isCharging = false;     // 是否正在充能

    // MC 1.16.5 常量
    static constexpr i32 CHARGE_DURATION = 60;  // 充能时间（ticks）
    static constexpr i32 COOLDOWN_DURATION = 20; // 冷却时间（ticks）
    static constexpr f32 ATTACK_RANGE = 15.0f;   // 攻击范围
    static constexpr f32 LASER_DAMAGE = 4.0f;    // 激光伤害（普通守卫者）
};

} // namespace entity::ai::goal
} // namespace mc
