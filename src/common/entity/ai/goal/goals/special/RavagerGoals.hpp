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

#include "../MeleeAttackGoal.hpp"
#include "common/core/Types.hpp"
#include <string>

namespace mc {

// Forward declarations
class RavagerEntity;
class LivingEntity;

namespace entity::ai::goal {

/**
 * @brief 劫掠兽近战攻击目标
 *
 * 扩展基础近战攻击目标，添加劫掠兽特有的攻击范围计算。
 *
 * 攻击范围计算：
 * - (width - 0.1) * 2.0 的平方
 * - 加上目标宽度
 */
class RavagerAttackGoal : public MeleeAttackGoal {
public:
    /**
     * @brief 构造函数
     * @param ravager 劫掠兽实体
     */
    explicit RavagerAttackGoal(RavagerEntity* ravager);

    ~RavagerAttackGoal() noexcept override = default;

    [[nodiscard]] std::string getTypeName() const noexcept override { return "RavagerAttackGoal"; }

protected:
    /**
     * @brief 计算攻击距离平方
     *
     * 劫掠兽特有的攻击范围计算：(width - 0.1) * 2 的平方 + 目标宽度
     *
     * @param target 目标实体
     * @return 攻击距离的平方
     */
    [[nodiscard]] f32 getAttackReachSqr(LivingEntity* target) const override;

private:
    RavagerEntity* m_ravager;
};

} // namespace entity::ai::goal
} // namespace mc
