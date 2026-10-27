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
#include "common/entity/ai/pathfinding/Path.hpp"
#include "common/entity/core/EntityReference.hpp"

#include <string>

namespace mc {

class FrogEntity;
class LivingEntity;
class IWorld;

namespace entity::ai::pathfinding {
class Path;
}

namespace entity::ai::goal {

/**
 * @brief 青蛙寻找可捕食目标（对齐 vanilla FrogAi.StartAttacking + NEAREST_ATTACKABLE）
 *
 * vanilla 由 NearestAttackableTargetSensor 把最近的 FROG_FOOD 实体写入 NEAREST_ATTACKABLE
 * 记忆，IDLE/SWIM/LAY_SPAWN 活动里的 StartAttacking 再把它升级为 ATTACK_TARGET（TONGUE 活动
 * 触发条件）。本项目无 Frog Brain，以本目标等效：定期搜索范围内最近的可捕食生物，
 * 写入 attackTarget 供 FrogTongueGoal 消费。
 *
 * 优先级：4
 * 互斥标志：Target（与 TargetGoal 体系一致，由攻击目标选择器持有）
 */
class FrogAttackTargetGoal : public Goal {
public:
    /**
     * @param frog 青蛙
     * @param range 搜索半径（vanilla 传感器为 FOLLOW_RANGE=16）
     * @param chance 每 tick 执行搜索的概率（0~1）
     */
    FrogAttackTargetGoal(FrogEntity* frog, f32 range, f32 chance);

    ~FrogAttackTargetGoal() override = default;

    [[nodiscard]] bool shouldExecute() override;
    [[nodiscard]] bool shouldContinueExecuting() override;
    void startExecuting() override;
    void resetTask() override;

    [[nodiscard]] std::string getTypeName() const override
    {
        return "FrogAttackTargetGoal";
    }

private:
    FrogEntity* m_frog;
    f32 m_range;
    f32 m_chance;
    EntityReference<LivingEntity> m_target;
};

/**
 * @brief 青蛙突舌捕食（对齐 vanilla ShootTongue）
 *
 * 状态机：MOVE_TO_TARGET → CATCH_ANIMATION → EAT_ANIMATION → DONE。
 * - 开始：注视目标、写入 DATA_TONGUE_TARGET_ID、以 2.0 倍速走向目标；
 * - 目标进入 1.75 格内：播放 FROG_TONGUE、切入 USING_TONGUE 姿态、把目标以 0.75 倍速
 *   拉向自身、进入捕捉动画（6 tick）；
 * - 捕捉动画结束：播放 FROG_EAT 并结算一次近战攻击（FrogEntity::eatTongueTarget），
 *   进入进食动画（10 tick）后结束；
 * - 无法寻路（路径终点离目标超过 1.75 格）或目标失效：放弃目标并进入 100 tick 冷却。
 *
 * 互斥标志：Move, Look
 * 优先级：3
 */
class FrogTongueGoal : public Goal {
public:
    /// 开始突舌前的寻路重算间隔（对齐 ShootTongue.calculatePathCounter = 10）
    static constexpr i32 PATH_RECALC_INTERVAL = 10;
    /// 目标进入该距离即触发捕食（对齐 ShootTongue.EATING_DISTANCE）
    static constexpr f32 EATING_DISTANCE = 1.75f;
    /// 捕捉后把目标拉向自身的速度（对齐 ShootTongue.EATING_MOVEMENT_FACTOR）
    static constexpr f32 EATING_MOVEMENT_FACTOR = 0.75f;
    /// 捕捉动画时长（对齐 ShootTongue.CATCH_ANIMATION_DURATION）
    static constexpr i32 CATCH_ANIMATION_DURATION = 6;
    /// 使用舌头姿态的总时长（对齐 ShootTongue.TONGUE_ANIMATION_DURATION）
    static constexpr i32 TONGUE_ANIMATION_DURATION = 10;
    /// 放弃目标后的冷却（对齐 ShootTongue.UNREACHABLE_TONGUE_TARGETS_COOLDOWN_DURATION）
    static constexpr i32 COOLDOWN_TICKS = 100;

    explicit FrogTongueGoal(FrogEntity* frog);

    ~FrogTongueGoal() override = default;

    [[nodiscard]] bool shouldExecute() override;
    [[nodiscard]] bool shouldContinueExecuting() override;
    void startExecuting() override;
    void resetTask() override;
    void tick() override;

    [[nodiscard]] std::string getTypeName() const override
    {
        return "FrogTongueGoal";
    }

private:
    enum class State : u8 {
        MoveToTarget,
        CatchAnimation,
        EatAnimation,
        Done,
    };

    /// 目标当前是否可捕食（存活、可感知、路径终点足够近）
    [[nodiscard]] bool _canPathfindToTarget(LivingEntity& target) const;

    /// 放弃目标并进入冷却
    void _abandonTarget();

    FrogEntity* m_frog;
    EntityReference<LivingEntity> m_target;
    State m_state = State::Done;
    i32 m_animationTimer = 0;
    i32 m_pathRecalcTimer = 0;
    i32 m_cooldown = 0;
};

} // namespace entity::ai::goal
} // namespace mc
