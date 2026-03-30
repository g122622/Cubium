#pragma once

#include "../MonsterEntity.hpp"
#include "../../../../core/Types.hpp"

namespace mc {

/**
 * @brief 幻翼实体
 *
 * 在夜间生成的飞行敌对生物，会俯冲攻击玩家。
 *
 * 参考 MC 1.16.5 PhantomEntity
 */
class PhantomEntity : public MonsterEntity {
public:
    /**
     * @brief 幻翼攻击模式
     */
    enum class AttackPhase {
        CIRCLE,      // 环绕
        SWOOP,       // 俯冲
        NONE         // 无
    };

    PhantomEntity(LegacyEntityType type, EntityId id);
    ~PhantomEntity() override = default;

    // 禁止拷贝
    PhantomEntity(const PhantomEntity&) = delete;
    PhantomEntity& operator=(const PhantomEntity&) = delete;

    // 允许移动
    PhantomEntity(PhantomEntity&&) = default;
    PhantomEntity& operator=(PhantomEntity&&) = default;

    // ========== 飞行特性 ==========

    /**
     * @brief 检查是否可以飞行
     */
    [[nodiscard]] bool canFly() const { return m_canFly; }
    void setCanFly(bool canFly) { m_canFly = canFly; }

    /**
     * @brief 获取当前攻击阶段
     */
    [[nodiscard]] AttackPhase getAttackPhase() const { return m_attackPhase; }

    /**
     * @brief 设置攻击阶段
     */
    void setAttackPhase(AttackPhase phase) { m_attackPhase = phase; }

    /**
     * @brief 获取环绕圆心
     */
    [[nodiscard]] f32 getCircleCenterY() const { return m_circleCenterY; }

    /**
     * @brief 设置环绕圆心Y坐标
     */
    void setCircleCenterY(f32 y) { m_circleCenterY = y; }

    /**
     * @brief 获取环绕角度
     */
    [[nodiscard]] f32 getCircleAngle() const { return m_circleAngle; }

    /**
     * @brief 设置环绕角度
     */
    void setCircleAngle(f32 angle) { m_circleAngle = angle; }

    // ========== 生命周期 ==========

    void tick() override;

protected:
    void registerGoals() override;
    void registerAttributes() override;

    /**
     * @brief 更新飞行状态
     */
    void updateFlight();

private:
    bool m_canFly = true;
    AttackPhase m_attackPhase = AttackPhase::CIRCLE;
    f32 m_circleCenterY = 0.0f;
    f32 m_circleAngle = 0.0f;
    i32 m_attackCooldown = 0;
};

} // namespace mc
