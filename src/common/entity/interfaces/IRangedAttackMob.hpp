#pragma once

#include "../../core/Types.hpp"

namespace mc {

// Forward declarations
class LivingEntity;

namespace entity {

/**
 * @brief 远程攻击接口 - 用于可以进行远程攻击的实体
 *
 * 实现此接口的实体可以使用远程武器进行攻击。
 * 例如：骷髅（弓箭）、烈焰人（火球）、女巫（药水）等。
 *
 * 参考 MC 1.16.5 IRangedAttackMob
 */
class IRangedAttackMob {
public:
    virtual ~IRangedAttackMob() = default;

    /**
     * @brief 对目标进行远程攻击
     * @param target 攻击目标
     * @param charge 蓄力程度 (0.0 - 1.0)
     *
     * charge值影响箭矢的飞行距离和伤害：
     * - 0.0: 最小蓄力，短距离，低伤害
     * - 1.0: 最大蓄力，远距离，高伤害
     */
    virtual void attackEntityWithRangedAttack(LivingEntity* target, f32 charge) = 0;

    /**
     * @brief 获取攻击间隔时间（ticks）
     * @return 攻击间隔
     */
    virtual i32 getAttackInterval() const { return 20; }

    /**
     * @brief 检查是否可以进行远程攻击
     * @return 如果可以攻击返回true
     */
    virtual bool canRangedAttack() const { return true; }
};

} // namespace entity
} // namespace mc
