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

#include "GoalFlag.hpp"
#include "common/core/EnumSet.hpp"
#include "common/core/Types.hpp"
#include <string>

namespace mc::entity::ai {

/**
 * @brief AI目标基类
 *
 * 所有AI行为（如游泳、漫步、攻击等）都继承自此类。
 * AI目标通过优先级和互斥标志进行协调。
 */
class Goal {
public:
    virtual ~Goal() = default;

    /**
     * @brief 是否应该开始执行
     *
     * 检查当前条件是否满足执行此AI目标。
     * 在此方法中可以缓存执行所需的状态。
     *
     * @return true 如果应该开始执行
     */
    [[nodiscard]] virtual bool shouldExecute() = 0;

    /**
     * @brief 是否应该继续执行
     *
     * 检查当前正在执行的AI目标是否应该继续。
     * 默认实现返回 shouldExecute() 的结果。
     *
     * @return true 如果应该继续执行
     */
    [[nodiscard]] virtual bool shouldContinueExecuting() { return shouldExecute(); }

    /**
     * @brief 是否可以被抢占
     *
     * 如果返回 true，则更高优先级的目标可以抢占此目标。
     * 默认返回 true。
     *
     * @return true 如果可以被抢占
     */
    [[nodiscard]] virtual bool isPreemptible() const { return true; }

    /**
     * @brief 是否每 tick 都需要更新评估
     *
     * 默认 false：goal 的 shouldExecute 受 GoalSelector 节流（每 2 tick 评估一次，
     * 对齐 vanilla Mob.serverAiStep 的 (tickCount+id)%2），故 shouldExecute 内的
     * 概率/冷却应经 adjustedTickDelay() 减半补偿。
     * 个别 goal（如 vanilla RandomLookAroundGoal）override 为 true，使其退化为
     * 每 tick 评估，adjustedTickDelay() 返回原值（不减半）。
     *
     * 对齐 vanilla Goal.requiresUpdateEveryTick()。
     *
     * @return true 表示该 goal 每 tick 都评估（adjustedTickDelay 不补偿）
     */
    [[nodiscard]] virtual bool requiresUpdateEveryTick() const { return false; }

    /**
     * @brief 调整 tick 延迟/概率门槛以补偿半 tick 评估
     *
     * shouldExecute 内的"每隔 N tick 触发一次"概率/冷却应调用本方法传入 N：
     * - requiresUpdateEveryTick() 为 true 时返回 N（该 goal 每 tick 评估，无需补偿）；
     * - 否则返回 reducedTickDelay(N)（GoalSelector 每 2 tick 评估一次，故门槛减半）。
     *
     * 对齐 vanilla Goal.adjustedTickDelay(int)。
     *
     * @param serverTicks vanilla 原始 tick 数
     * @return 补偿后的门槛值
     */
    [[nodiscard]] i32 adjustedTickDelay(i32 serverTicks) const
    {
        return requiresUpdateEveryTick() ? serverTicks : reducedTickDelay(serverTicks);
    }

    /**
     * @brief 将 tick 数减半（向上取整）以补偿 GoalSelector 的半 tick 评估
     *
     * vanilla Mob AI 每 2 tick 才评估一次 goal 的 canUse（Mob.serverAiStep 用
     * (tickCount+id)%2），故 goal 内部"每隔 N tick 触发"的概率门槛需除以 2，
     * 才能在真实时间上保持与"每 tick 评估"等价的触发频率。
     *
     * 等价于 vanilla Mth.positiveCeilDiv(n, 2) = (n + 1) / 2（n 为正数时向上取整）。
     *
     * 对齐 vanilla Goal.reducedTickDelay(int)。
     *
     * @param serverTicks vanilla 原始 tick 数
     * @return ceil(serverTicks / 2)
     */
    [[nodiscard]] static constexpr i32 reducedTickDelay(i32 serverTicks) noexcept { return (serverTicks + 1) / 2; }

    // TODO: 全仓 goal 概率/tick 计时对齐 vanilla adjustedTickDelay/reducedTickDelay 的工作尚未完成。
    // 已对齐核心 goal（EatGrass/RandomWalking/RandomSwimming/LookAt/LookRandomly/MeleeAttack/
    // Breed/FollowParent/Tempt/Swim/NearestAttackableTarget/TargetGoal.unseenMemory）。
    // special 类 goal（FoxGoals/BeeGoals/CatGoals/DolphinGoals/PhantomGoals/SlimeGoals/ShulkerGoals/
    // VexGoals/TurtleGoals/EndermanGoals/IronGolemGoals/BatGoals/RangedAttackGoals 及 villager 系列）
    // 多为项目自创逻辑或与 vanilla 对应关系复杂，其 shouldExecute 概率与 tick 计时器仍用裸值，
    // 待后续逐一核对 vanilla 源码后用 adjustedTickDelay/reducedTickDelay 补偿。

    /**
     * @brief 开始执行
     *
     * 当目标开始执行时调用。
     * 用于初始化执行状态。
     */
    virtual void startExecuting() {}

    /**
     * @brief 重置任务
     *
     * 当目标被中断时调用。
     * 用于清理执行状态。
     */
    virtual void resetTask() {}

    /**
     * @brief 刻更新
     *
     * 每tick调用，用于更新正在执行的目标。
     */
    virtual void tick() {}

    /**
     * @brief 设置互斥标志
     *
     * 设置此目标使用的互斥标志。
     * 共享相同标志的目标不能同时运行。
     *
     * @param flags 互斥标志集合
     */
    void setMutexFlags(const EnumSet<GoalFlag>& flags) { m_flags = flags; }

    /**
     * @brief 获取互斥标志
     *
     * @return 当前设置的互斥标志
     */
    [[nodiscard]] const EnumSet<GoalFlag>& getMutexFlags() const { return m_flags; }

    /**
     * @brief 获取类型名称（用于调试）
     */
    [[nodiscard]] virtual std::string getTypeName() const { return "Goal"; }

protected:
    Goal() = default;

    /**
     * @brief 构造并设置互斥标志
     */
    explicit Goal(const EnumSet<GoalFlag>& flags)
        : m_flags(flags)
    {}

    EnumSet<GoalFlag> m_flags; // 互斥标志
};

} // namespace mc::entity::ai
