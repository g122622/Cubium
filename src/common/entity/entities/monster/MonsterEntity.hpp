#pragma once

#include "../../core/CreatureEntity.hpp"
#include "../../core/Entity.hpp"
#include "../../interfaces/IMob.hpp"
#include "../../../core/Types.hpp"

namespace mc {

/**
 * @brief 敌对生物基类
 *
 * 所有敌对生物（怪物）的基类，提供敌对行为的基础设施。
 *
 * 特性：
 * - 在黑暗中生成
 * - 在阳光下可能燃烧（亡灵类）
 * - 自动攻击玩家
 * - 敌对目标选择
 *
 * 参考 MC 1.16.5 MonsterEntity
 */
class MonsterEntity : public CreatureEntity, public entity::IMob {
public:
    /**
     * @brief 构造函数
     * @param type 实体类型
     * @param id 实体ID
     */
    MonsterEntity(LegacyEntityType type, EntityId id);

    ~MonsterEntity() override = default;

    // 禁止拷贝
    MonsterEntity(const MonsterEntity&) = delete;
    MonsterEntity& operator=(const MonsterEntity&) = delete;

    // 允许移动
    MonsterEntity(MonsterEntity&&) = default;
    MonsterEntity& operator=(MonsterEntity&&) = default;

    // ========== 光照敏感 ==========

    /**
     * @brief 检查是否应该在阳光下燃烧
     * @return 如果应该在阳光下燃烧返回true
     */
    [[nodiscard]] virtual bool shouldBurnInDaylight() const { return m_burnsInDaylight; }

    /**
     * @brief 设置是否在阳光下燃烧
     * @param burn 是否燃烧
     */
    void setBurnsInDaylight(bool burn) { m_burnsInDaylight = burn; }

protected:
    bool m_burnsInDaylight = true;

    /**
     * @brief 检查是否在阳光下
     * @return 如果暴露在阳光下返回true
     */
    [[nodiscard]] bool isInDaylight() const;

    // ========== 敌对行为 ==========

    /**
     * @brief 检查是否应该攻击目标
     * @param target 目标实体
     * @return 如果应该攻击返回true
     */
    [[nodiscard]] virtual bool shouldAttack(LivingEntity* target) const;

    // ========== 生命周期 ==========

    void tick() override;

protected:
    /**
     * @brief 注册 AI 目标
     *
     * 子类应重写此方法来注册敌对生物的基础行为：
     * - SwimGoal (优先级 0)
     * - HurtByTargetGoal (优先级 1)
     * - NearestAttackableTargetGoal (优先级 2)
     */
    void registerGoals() override;

    /**
     * @brief 处理阳光燃烧
     */
    void handleDaylightBurning();

private:
    i32 m_burnTime = 0;
};

} // namespace mc
