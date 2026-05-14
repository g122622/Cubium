#pragma once

#include "../../core/Types.hpp"
#include <optional>

namespace mc {

// Forward declarations
class LivingEntity;
class Player;

namespace entity {

/**
 * @brief 愤怒接口 - 用于可愤怒的实体
 *
 * 实现此接口的实体可以在被攻击后记住攻击者并进行反击。
 * 例如：狼、铁傀儡、末影人、僵尸猪灵等。
 *
 * 参考 MC 1.16.5 IAngerable
 */
class IAngerable {
public:
    virtual ~IAngerable() = default;

    /**
     * @brief 设置攻击目标
     * @param target 目标实体，nullptr表示清除目标
     */
    virtual void setAttackTarget(LivingEntity* target) = 0;

    /**
     * @brief 获取当前攻击目标
     * @return 当前攻击目标，可能为nullptr
     */
    virtual LivingEntity* getAttackTarget() const = 0;

    /**
     * @brief 设置复仇目标（最近攻击自己的实体）
     * @param target 攻击者实体
     */
    virtual void setRevengeTarget(LivingEntity* target) = 0;

    /**
     * @brief 检查是否处于愤怒状态
     * @return 如果实体正在愤怒则返回true
     */
    virtual bool isAngry() const = 0;

    /**
     * @brief 设置愤怒状态
     * @param angry 是否愤怒
     */
    virtual void setAngry(bool angry) = 0;

    /**
     * @brief 获取愤怒剩余时间（ticks）
     * @return 愤怒剩余时间
     */
    virtual i32 getAngerTime() const = 0;

    /**
     * @brief 设置愤怒时间
     * @param time 愤怒时间（ticks）
     */
    virtual void setAngerTime(i32 time) = 0;

    /**
     * @brief 每tick更新愤怒计时器
     *
     * 子类应在tick()中调用此方法
     */
    virtual void updateAnger()
    {
        if (getAngerTime() > 0) {
            setAngerTime(getAngerTime() - 1);
            if (getAngerTime() == 0) {
                setAngry(false);
                setAttackTarget(nullptr);
            }
        }
    }
};

} // namespace entity
} // namespace mc
