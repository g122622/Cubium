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

// Forward declarations
class LivingEntity;

namespace entity {

/**
 * @brief 远程攻击接口 - 用于可以进行远程攻击的实体
 *
 * 实现此接口的实体可以使用远程武器进行攻击。
 * 例如：骷髅（弓箭）、烈焰人（火球）、女巫（药水）等。
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
