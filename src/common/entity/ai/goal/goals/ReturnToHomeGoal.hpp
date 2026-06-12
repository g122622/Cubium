/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to Use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software be
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
#include "common/entity/ai/goal/GoalFlag.hpp"
#include "common/util/math/Vector3.hpp"

namespace mc {

// 前向声明
class CreatureEntity;

namespace entity::ai::goal {

/**
 * @brief 返回家园目标
 *
 * 控制生物返回其家园位置的行为。用于村民返回床位、
 * 铁傀儡返回村庄等场景。当生物离开家园范围时，
 * 会导航回家园位置附近。
 *
 * 依赖MobEntity的家园区系统（setHomePosAndDistance/hasHome/isWithinHomeDistanceFromPosition）。
 */
class ReturnToHomeGoal : public Goal {
public:
    /**
     * @brief 构造函数
     * @param creature 拥有此目标的生物
     * @param speed 移动速度倍率
     * @param homeRadius 家园半径（仅当生物未设置家园区时使用）
     */
    ReturnToHomeGoal(CreatureEntity* creature, f64 speed, f32 homeRadius = 16.0f);

    ~ReturnToHomeGoal() override = default;

    [[nodiscard]] bool shouldExecute() override;
    [[nodiscard]] bool shouldContinueExecuting() override;
    void startExecuting() override;
    void resetTask() override;
    void tick() override;

    [[nodiscard]] std::string getTypeName() const override { return "ReturnToHomeGoal"; }

private:
    /**
     * @brief 重新计算路径
     */
    void _recalculatePath();

    CreatureEntity* m_creature;
    f64 m_speed;
    f32 m_homeRadius;
    f64 m_targetX = 0.0;
    f64 m_targetY = 0.0;
    f64 m_targetZ = 0.0;
    i32 m_pathRecalcTimer = 0;

    // 搜索范围常量
    static constexpr i32 HOME_XZ_RANGE = 16;        // 朝向家园位置的水平搜索范围
    static constexpr i32 HOME_Y_RANGE = 7;          // 朝向家园位置的垂直搜索范围
    static constexpr i32 PATH_RECALC_INTERVAL = 10; // 路径重新计算间隔（tick）
};

} // namespace entity::ai::goal
} // namespace mc
