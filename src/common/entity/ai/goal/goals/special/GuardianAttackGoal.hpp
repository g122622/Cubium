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
#include "common/entity/ai/goal/Goal.hpp"
#include <string>

namespace mc {

// 前向声明
class GuardianEntity;
class LivingEntity;

namespace entity::ai::goal {

/**
 * @brief 守卫者激光攻击目标
 *
 * 守卫者特有的激光攻击行为：
 * - 准备阶段：前 10 tick（tickCounter 从 -10 到 0）
 * - 充能动画：tickCounter 从 0 到 80（此时发送状态21触发音效）
 * - 发射阶段：80 tick 时造成伤害
 */
class GuardianAttackGoal : public Goal {
public:
    /**
     * @brief 构造函数
     * @param guardian 守卫者实体
     */
    explicit GuardianAttackGoal(GuardianEntity* guardian);

    [[nodiscard]] bool shouldExecute() override;
    [[nodiscard]] bool shouldContinueExecuting() override;
    void startExecuting() override;
    void resetTask() override;
    void tick() override;

    [[nodiscard]] std::string getTypeName() const override { return "GuardianAttackGoal"; }

private:
    /**
     * @brief 选择攻击目标
     * @return 目标实体，如果没有则返回 nullptr
     */
    [[nodiscard]] LivingEntity* _selectTarget() const;

    /**
     * @brief 检查目标是否有效
     * @param target 目标实体
     * @return 是否有效
     */
    [[nodiscard]] bool _isTargetValid(LivingEntity* target) const;

    GuardianEntity* m_guardian;
    LivingEntity* m_target = nullptr;
    i32 m_tickCounter = 0;
    bool m_isElder = false; // 是否为远古守卫者

    // 攻击常量
    static constexpr i32 ATTACK_DURATION = 80;      // 攻击周期（ticks）
    static constexpr f32 ATTACK_RANGE = 15.0f;      // 攻击范围
    static constexpr f32 LASER_DAMAGE = 4.0f;       // 激光伤害（普通守卫者）
    static constexpr f32 ELDER_BONUS_DAMAGE = 2.0f; // 远古守卫者额外伤害
};

} // namespace entity::ai::goal
} // namespace mc
