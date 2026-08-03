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
class BlazeEntity;
class LivingEntity;

namespace entity::ai::goal {

/**
 * @brief 烈焰人火球攻击目标
 *
 * 烈焰人特有的火球攻击行为：
 * - 阶段1：充能（60 ticks），烈焰人进入燃烧状态
 * - 阶段2：连发火球（最多3个，每个间隔6 ticks）
 * - 阶段3：冷却（100 ticks）
 */
class BlazeFireballAttackGoal : public Goal {
public:
    /**
     * @brief 构造函数
     * @param blaze 烈焰人实体
     */
    explicit BlazeFireballAttackGoal(BlazeEntity* blaze);

    [[nodiscard]] bool shouldExecute() override;
    [[nodiscard]] bool shouldContinueExecuting() override;
    void startExecuting() override;
    void resetTask() override;
    void tick() override;

    [[nodiscard]] std::string getTypeName() const override { return "BlazeFireballAttackGoal"; }

private:
    /**
     * @brief 检查目标是否有效
     * @param target 目标实体
     * @return 是否有效
     */
    [[nodiscard]] bool _isTargetValid(LivingEntity* target) const;

    /**
     * @brief 获取追踪范围
     * @return 追踪范围
     */
    [[nodiscard]] f64 _getFollowDistance() const;

    /**
     * @brief 执行火球攻击
     * @param target 目标实体
     * @param distanceToTargetSq 到目标的距离平方
     */
    void _performFireballAttack(LivingEntity* target, f64 distanceToTargetSq);

    BlazeEntity* m_blaze;
    LivingEntity* m_target = nullptr;
    i32 m_attackStep = 0; // 攻击阶段（0=未开始，1=充能，2-4=连发火球）
    i32 m_attackTime = 0; // 攻击计时器
    i32 m_unseenTime = 0; // 看不到目标的时间

    // 常量
    static constexpr i32 CHARGE_TIME = 60;      // 充能时间（ticks）
    static constexpr i32 FIREBALL_INTERVAL = 6; // 火球间隔（ticks）
    static constexpr i32 COOLDOWN_TIME = 100;   // 冷却时间（ticks）
    static constexpr i32 MAX_FIREBALLS = 3;     // 最多连发火球数
    static constexpr f64 MELEE_RANGE_SQ = 4.0;  // 近战范围平方
};

} // namespace entity::ai::goal
} // namespace mc
