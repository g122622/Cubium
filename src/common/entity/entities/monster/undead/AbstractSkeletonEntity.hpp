#pragma once

#include "../../../interfaces/IRangedAttackMob.hpp"
#include "../MonsterEntity.hpp"

namespace mc {

/**
 * @brief 骷髅系怪物公共中间层
 *
 * 对齐 MC 1.16.5 `AbstractSkeletonEntity`，集中承载：
 * - 远程弓箭攻击接口
 * - 拉弓状态与攻击计时
 * - 骷髅系共通属性与基础目标注册
 */
class AbstractSkeletonEntity : public MonsterEntity, public entity::IRangedAttackMob {
public:
    ~AbstractSkeletonEntity() override = default;

    AbstractSkeletonEntity(const AbstractSkeletonEntity&) = delete;
    AbstractSkeletonEntity& operator=(const AbstractSkeletonEntity&) = delete;
    AbstractSkeletonEntity(AbstractSkeletonEntity&&) = default;
    AbstractSkeletonEntity& operator=(AbstractSkeletonEntity&&) = default;

    void attackEntityWithRangedAttack(LivingEntity* target, f32 charge) override;

    [[nodiscard]] bool isChargingBow() const { return m_chargingBow; }
    void setChargingBow(bool charging) { m_chargingBow = charging; }

    [[nodiscard]] i32 getAttackTimer() const { return m_attackTimer; }
    void setAttackTimer(i32 timer) { m_attackTimer = timer; }

    [[nodiscard]] i32 getAttackCooldown() const { return m_attackCooldown; }
    void setAttackCooldown(i32 cooldown) { m_attackCooldown = cooldown; }

    void tick() override;

protected:
    AbstractSkeletonEntity(LegacyEntityType type, EntityId id);

    void registerGoals() override;
    void registerAttributes() override;

    bool m_chargingBow = false;
    i32 m_attackTimer = 0;
    i32 m_attackCooldown = 0;

    static constexpr i32 ATTACK_COOLDOWN = 60;
    static constexpr f32 ARROW_DAMAGE = 2.0f;
};

} // namespace mc
