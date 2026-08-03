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
#include "common/world/block/BlockPos.hpp"
#include <string>
#include <vector>

namespace mc {

// Forward declarations
class PatrollerEntity;
class IWorld;

namespace entity::ai::goal {

/**
 * @brief 巡逻目标
 *
 * 使巡逻实体（掠夺者等）向巡逻目标移动。
 * 队长负责选择巡逻目标，队员跟随队长。
 *
 * 执行条件:
 * - 实体正在巡逻 (isPatrolling() == true)
 * - 没有攻击目标
 * - 没有被骑乘
 * - 有巡逻目标
 * - 不在冷却期
 *
 * 队长行为:
 * - 到达目标（距离 < 10 格）后重置新目标
 * - 移动成功后同步目标给附近队员
 *
 * 队员行为:
 * - 跟随队长的目标
 *
 * 失败处理:
 * - 移动失败时随机移动，并设置 200 tick 冷却
 */
class PatrolGoal : public Goal {
public:
    /**
     * @brief 构造函数
     * @param patroller 巡逻实体
     * @param memberSpeed 队员移动速度（0.7）
     * @param leaderSpeed 队长移动速度（0.595）
     */
    PatrolGoal(PatrollerEntity* patroller, f64 memberSpeed, f64 leaderSpeed);

    ~PatrolGoal() override = default;

    [[nodiscard]] bool shouldExecute() override;
    [[nodiscard]] bool shouldContinueExecuting() override;
    void startExecuting() override;
    void tick() override;

    [[nodiscard]] std::string getTypeName() const override { return "PatrolGoal"; }

    // MC 1.16.5 常量（公开以便测试）
    static constexpr f64 ARRIVAL_THRESHOLD = 10.0;          // 到达目标阈值（格）
    static constexpr f64 ARRIVAL_THRESHOLD_SQ = 100.0;      // 到达目标阈值平方
    static constexpr f64 NEARBY_PATROLLER_RANGE = 16.0;     // 搜索附近队员范围
    static constexpr f64 NEARBY_PATROLLER_RANGE_SQ = 256.0; // 搜索范围平方
    static constexpr i64 COOLDOWN_TICKS = 200L;             // 移动失败冷却时间
    static constexpr i32 RANDOM_MOVE_RANGE = 8;             // 随机移动范围

private:
    /**
     * @brief 获取附近的巡逻队员
     * @return 附近 16 格内的巡逻队员列表（排除自己）
     */
    [[nodiscard]] std::vector<PatrollerEntity*> _getNearbyPatrollers() const;

    /**
     * @brief 随机移动到附近位置
     * @return 是否成功开始移动
     */
    [[nodiscard]] bool _moveRandomly();

    PatrollerEntity* m_patroller;
    f64 m_memberSpeed;         // 队员速度
    f64 m_leaderSpeed;         // 队长速度
    i64 m_cooldownTime = -1LL; // 冷却时间戳（游戏 tick），初始为 -1 表示无冷却
};

} // namespace entity::ai::goal
} // namespace mc
