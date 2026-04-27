#pragma once

#include "../../Goal.hpp"
#include "../../../../../core/Types.hpp"

namespace mc {

// Forward declarations
class LivingEntity;
class MobEntity;

namespace entity::ai::goal {

/**
 * @brief 远程攻击目标
 *
 * 使实体能够在一定距离外进行远程攻击。
 * 适用于骷髅、烈焰人等使用远程攻击的怪物。
 *
 * 参考 MC 1.16.5 RangedAttackGoal
 */
class RangedAttackGoal : public Goal {
public:
    /**
     * @brief 构造函数
     * @param mob 拥有此目标的生物
     * @param speed 移动速度倍率
     * @param attackIntervalMin 最小攻击间隔（ticks）
     * @param attackIntervalMax 最大攻击间隔（ticks）
     * @param attackRadius 攻击半径
     */
    RangedAttackGoal(MobEntity* mob, f64 speed, i32 attackIntervalMin, i32 attackIntervalMax, f32 attackRadius);

    ~RangedAttackGoal() override = default;

    [[nodiscard]] bool shouldExecute() override;
    [[nodiscard]] bool shouldContinueExecuting() override;
    void startExecuting() override;
    void resetTask() override;
    void tick() override;

protected:
    /**
     * @brief 执行远程攻击
     * @param target 目标实体
     * @param charge 蓄力程度（0.0-1.0）
     */
    virtual void performAttack(LivingEntity* target, f32 charge);

    MobEntity* m_mob;
    LivingEntity* m_target = nullptr;
    f64 m_speed;
    i32 m_attackIntervalMin;
    i32 m_attackIntervalMax;
    f32 m_attackRadius;
    f32 m_maxAttackDistanceSq;  // MC 1.16.5: 缓存的平方距离
    i32 m_attackTime = -1;      // MC 1.16.5: 初始值为 -1
    i32 m_seenTime = 0;         // MC 1.16.5: 能看到目标的时间

    static constexpr i32 MIN_SEEN_TIME = 5;  // MC 1.16.5: 停止移动前需要看到的tick数
};

/**
 * @brief 弓箭攻击目标
 *
 * 使用弓进行远程攻击。
 * 需要实体实现IRangedAttackMob接口。
 *
 * 参考 MC 1.16.5 RangedBowAttackGoal
 */
class RangedBowAttackGoal : public RangedAttackGoal {
public:
    /**
     * @brief 构造函数
     * @param mob 拥有此目标的生物
     * @param speed 移动速度倍率
     * @param attackIntervalMin 最小攻击间隔
     * @param attackIntervalMax 最大攻击间隔
     */
    RangedBowAttackGoal(MobEntity* mob, f64 speed, i32 attackIntervalMin, i32 attackIntervalMax);

    ~RangedBowAttackGoal() override = default;

    [[nodiscard]] bool shouldExecute() override;
    void startExecuting() override;
    void resetTask() override;
    void tick() override;

protected:
    void performAttack(LivingEntity* target, f32 charge) override;

private:
    bool m_isBowCharging = false;
    i32 m_chargeTime = 0;

    // MC 1.16.5: 走位相关字段
    bool m_strafingClockwise = false;    // 是否顺时针走位
    bool m_strafingBackwards = false;    // 是否向后走位
    i32 m_strafingTime = -1;              // 走位时间计数器

    static constexpr i32 BOW_CHARGE_TIME = 20;      // 弓满蓄力时间
    static constexpr i32 STRAFE_THRESHOLD = 20;     // 走位方向变化阈值（MC 1.16.5）
};

} // namespace entity::ai::goal
} // namespace mc
