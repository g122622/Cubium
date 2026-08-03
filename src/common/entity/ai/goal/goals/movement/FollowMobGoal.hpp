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

#include "../../../../../core/Types.hpp"
#include "../../Goal.hpp"
#include "../../GoalFlag.hpp"
#include <string>

namespace mc {

// 前向声明
class MobEntity;
class LivingEntity;

namespace entity::ai::goal {

/**
 * @brief 跟随生物目标
 *
 * 使实体跟随附近的其他生物。
 * 常用于鹦鹉等会跟随其他生物的实体。
 */
class FollowMobGoal : public Goal {
public:
    /**
     * @brief 构造函数
     * @param mob 拥有此目标的生物
     * @param speed 移动速度倍率
     * @param minDistance 最小跟随距离
     * @param maxDistance 最大跟随距离（检测范围）
     */
    FollowMobGoal(MobEntity* mob, f64 speed, f32 minDistance, f32 maxDistance);

    ~FollowMobGoal() override = default;

    [[nodiscard]] bool shouldExecute() override;
    [[nodiscard]] bool shouldContinueExecuting() override;
    void startExecuting() override;
    void resetTask() override;
    void tick() override;

    [[nodiscard]] std::string getTypeName() const override { return "FollowMobGoal"; }

private:
    /**
     * @brief 寻找附近的生物作为跟随目标
     * @return 找到的生物，如果没有则返回 nullptr
     */
    [[nodiscard]] LivingEntity* _findNearbyMob();

    MobEntity* m_mob;
    f64 m_speed;
    f32 m_minDistance;
    f32 m_maxDistance;
    LivingEntity* m_targetMob = nullptr;
    i32 m_delayCounter = 0;

    // 路径重新计算间隔
    static constexpr i32 PATH_RECALC_INTERVAL = 10;
};

} // namespace entity::ai::goal
} // namespace mc
