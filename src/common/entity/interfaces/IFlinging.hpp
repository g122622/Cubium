#pragma once

#include "../../core/Types.hpp"

namespace mc {

class LivingEntity;

namespace entity {

/**
 * @brief 撞飞型近战生物接口
 */
class IFlinging {
public:
    virtual ~IFlinging() = default;

    /**
     * @brief 获取撞飞攻击动画剩余 tick
     */
    [[nodiscard]] virtual i32 getFlingAnimationTicks() const = 0;

    /**
     * @brief 执行一次带撞飞语义的近战攻击
     */
    [[nodiscard]] static bool attackWithFling(LivingEntity& attacker, LivingEntity& target, bool attackerIsBaby);

    /**
     * @brief 对目标施加撞飞
     */
    static void flingTarget(LivingEntity& attacker, LivingEntity& target);
};

} // namespace entity
} // namespace mc
