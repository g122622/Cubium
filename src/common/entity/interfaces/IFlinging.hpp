/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#pragma once

#include "common/core/Types.hpp"

namespace mc {

class LivingEntity;

namespace entity {

/**
 * @brief 撞飞型近战生物接口
 *
 * 对应 vanilla `HoglinBase`：把"随机化伤害 + 施加伤害 + 后置附魔效果 + 抛飞"这一整段
 * 攻击语义收在一处，由所有撞飞型生物（Hoglin/Zoglin）共用。
 *
 * **分工边界**（这是本接口最容易被误用的地方）：
 * - 本接口负责 {@link hurtAndThrowTarget} —— 攻击的**伤害与抛飞**部分，不含动画与音效；
 * - 攻击动画剩余 tick、EntityEvent 广播、攻击音效属于各生物自己的 `attackEntityAsMob`，
 *   由它先做这些表现层动作，最后调用 {@link hurtAndThrowTarget}。
 *
 * 因此 `attackEntityAsMob` 里**不要**再自行调用 `target.hurt(...)`——{@link hurtAndThrowTarget}
 * 已经包含伤害施加，重复调用会造成双重伤害。
 */
class IFlinging {
public:
    /** 攻击动画持续时间（tick），对应 vanilla `HoglinBase.ATTACK_ANIMATION_DURATION` */
    static constexpr i32 ATTACK_ANIMATION_DURATION = 10;

    virtual ~IFlinging() = default;

    /**
     * @brief 获取撞飞攻击动画剩余 tick
     */
    [[nodiscard]] virtual i32 getFlingAnimationTicks() const = 0;

    /**
     * @brief 设置撞飞攻击动画剩余 tick
     *
     * 攻击开始时由生物自己的 `attackEntityAsMob` 置为 {@link ATTACK_ANIMATION_DURATION}，
     * 每 tick 由该生物的 tick 递减。
     */
    virtual void setFlingAnimationTicks(i32 ticks) = 0;

    /**
     * @brief 施加一次带抛飞语义的攻击（伤害 + 后置附魔效果 + 抛飞）
     *
     * 对应 vanilla `HoglinBase.hurtAndThrowTarget`：
     * 1. 计算伤害：成年生物的随机化（`damage / 2 + rand[0, damage)`），幼年取原值；
     * 2. 以 `mobAttack` 来源施加伤害，失败则直接返回 false；
     * 3. 命中后触发后置附魔效果（节肢杀手等，`doPostAttackEffects` 的等价物）；
     * 4. 命中且攻击者非幼年时施加抛飞（{@link flingTarget}）。
     *
     * @param attacker 攻击者（提供攻击力、击退属性与随机源）
     * @param target 目标
     * @return 伤害是否实际生效
     */
    [[nodiscard]] static bool hurtAndThrowTarget(LivingEntity& attacker, LivingEntity& target);

    /**
     * @brief 对目标施加抛飞（只改速度，不造伤害）
     *
     * 对应 vanilla `HoglinBase.throwTarget`：
     * `strength = ATTACK_KNOCKBACK - KNOCKBACK_RESISTANCE`，强度非正时不做任何事；
     * 水平方向取"攻击者指向目标"的单位向量，按 `strength * (rand*0.5 + 0.2)` 缩放后
     * 随机绕 Y 轴旋转 `rand[-10, 10]` 度；垂直方向为 `strength * rand * 0.5`。
     *
     * 供给侧（Hoglin 的 `blockedByItem` 等只抛飞不伤害的场景）也复用本方法。
     */
    static void flingTarget(LivingEntity& attacker, LivingEntity& target);
};

} // namespace entity
} // namespace mc
