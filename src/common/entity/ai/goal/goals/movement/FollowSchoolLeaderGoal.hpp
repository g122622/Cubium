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
class AbstractGroupFishEntity;

namespace entity::ai::goal {

/**
 * @brief 跟随群体领导者目标
 *
 * 使群游鱼类跟随群体领导者。
 *
 * 群游行为：
 * 1. 如果自己是首领 → 不执行
 * 2. 如果已有首领 → 继续跟随
 * 3. 冷却结束 → 搜索附近鱼群，找可扩群的首领或自己成为首领
 */
class FollowSchoolLeaderGoal : public Goal {
public:
    /**
     * @brief 构造函数
     * @param fish 群游鱼类实体
     */
    explicit FollowSchoolLeaderGoal(AbstractGroupFishEntity* fish);

    [[nodiscard]] bool shouldExecute() override;
    [[nodiscard]] bool shouldContinueExecuting() override;
    void startExecuting() override;
    void resetTask() override;
    void tick() override;

    [[nodiscard]] std::string getTypeName() const override { return "FollowSchoolLeaderGoal"; }

private:
    /**
     * @brief 获取新的冷却时间
     *
     * 结果范围：200~219 ticks（约10~11秒）
     */
    [[nodiscard]] i32 _getNewCooldown() const;

    AbstractGroupFishEntity* m_fish;
    AbstractGroupFishEntity* m_leader = nullptr;
    i32 m_navigateTimer = 0;
    i32 m_cooldown = 0; // 搜索冷却
};

} // namespace entity::ai::goal
} // namespace mc
