#pragma once

#include "../../../../../core/Types.hpp"
#include "../../Goal.hpp"

namespace mc {

// 前向声明
class GuardianEntity;
class LivingEntity;

namespace entity::ai::goal {

/**
 * @brief 守卫者激光攻击目标
 *
 * 守卫者特有的激光攻击行为：
 * - 准备阶段：前 10 tick（tickCounter 从 -10 到 0）
 * - 充能动画：tickCounter 从 0 到 80（此时发送状态21触发音效）
 * - 发射阶段：80 tick 时造成伤害
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

    [[nodiscard]] std::string getTypeName() const override { return "GuardianAttackGoal"; }

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

    GuardianEntity* m_guardian;
    LivingEntity* m_target = nullptr;
    i32 m_tickCounter = 0;  // MC 1.16.5: tickCounter
    bool m_isElder = false; // 是否为远古守卫者

    // MC 1.16.5 常量
    static constexpr i32 ATTACK_DURATION = 80;      // 攻击周期（ticks）
    static constexpr f32 ATTACK_RANGE = 15.0f;      // 攻击范围
    static constexpr f32 LASER_DAMAGE = 4.0f;       // 激光伤害（普通守卫者）
    static constexpr f32 ELDER_BONUS_DAMAGE = 2.0f; // 远古守卫者额外伤害
};

} // namespace entity::ai::goal
} // namespace mc
