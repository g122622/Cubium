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

#include "Goal.hpp"
#include "PrioritizedGoal.hpp"
#include "common/core/EnumSet.hpp"
#include "common/core/Types.hpp"
#include "common/entity/ai/goal/GoalFlag.hpp"

#include <algorithm>
#include <functional>
#include <iterator>
#include <list>
#include <memory>
#include <unordered_map>
#include <utility>
#include <vector>

namespace mc::entity::ai {

/**
 * @brief AI目标选择器
 *
 * 管理实体的所有AI目标，负责选择和执行当前应该运行的目标。
 * 通过优先级和互斥标志协调多个AI目标的执行。
 */
class GoalSelector {
public:
    /**
     * @brief 默认 tick 间隔
     *
     * 对齐 vanilla Mob.serverAiStep 的 (tickCount+id)%2：goal 的 shouldExecute
     * 每 2 tick 评估一次。vanilla 不同 mob 按 id 错峰，本项目用单一计数器所有 mob
     * 同步评估，但评估频率（1/2）与 vanilla 一致。此值配合 Goal::adjustedTickDelay()
     * 使用：shouldExecute 内的概率/冷却经 adjustedTickDelay 减半补偿半 tick 评估。
     */
    static constexpr i32 DEFAULT_TICK_RATE = 2;

    /**
     * @brief 构造函数
     */
    GoalSelector()
        : m_tickRate(DEFAULT_TICK_RATE)
    {}

    /**
     * @brief 析构函数
     */
    ~GoalSelector() = default;

    /**
     * @brief 添加AI目标
     *
     * 目标按优先级插入，不允许重复。
     *
     * @param priority 优先级（数值越小优先级越高）
     * @param goal AI目标
     */
    void addGoal(i32 priority, std::unique_ptr<Goal> goal)
    {
        if (goal == nullptr) {
            return;
        }
        if (_hasGoal(goal.get())) {
            return;
        }
        m_goals.emplace_back(priority, std::move(goal));
    }

    /**
     * @brief 添加AI目标（原始指针版本）
     *
     * @param priority 优先级
     * @param goal AI目标（获取所有权）
     */
    void addGoal(i32 priority, Goal* goal)
    {
        if (goal == nullptr) {
            return;
        }
        if (_hasGoal(goal)) {
            return;
        }
        m_goals.emplace_back(priority, goal);
    }

    /**
     * @brief 移除AI目标
     *
     * 先停止所有运行中的匹配目标，再移除。
     *
     * @param goal 要移除的目标指针
     */
    void removeGoal(Goal* goal)
    {
        // 先停止所有运行中的匹配目标：先收集再 resetTask（resetTask 可能改动 m_goals）。
        std::vector<PrioritizedGoal*> goalsToStop;
        for (auto& pg : m_goals) {
            if (pg.getGoal() == goal && pg.isRunning()) {
                goalsToStop.push_back(&pg);
            }
        }
        for (PrioritizedGoal* pg : goalsToStop) {
            pg->resetTask();
        }
        // 再移除所有匹配目标（同步清理 flagGoals 悬垂指针）
        m_goals.remove_if([goal](const PrioritizedGoal& pg) {
            return pg.getGoal() == goal;
        });
        _pruneFlagGoals();
    }

    /**
     * @brief 移除所有AI目标
     */
    void removeAllGoals()
    {
        // 先收集再 resetTask：resetTask 中的自定义清理可能改动 m_goals。
        std::vector<PrioritizedGoal*> goalsToStop;
        for (auto& prioritizedGoal : m_goals) {
            if (prioritizedGoal.isRunning()) {
                goalsToStop.push_back(&prioritizedGoal);
            }
        }
        for (PrioritizedGoal* prioritizedGoal : goalsToStop) {
            prioritizedGoal->resetTask();
        }
        m_goals.clear();
        m_flagGoals.clear();
    }

    /**
     * @brief 移除指定类型的所有AI目标
     *
     * 遍历所有目标，使用 dynamic_cast 检查类型匹配。
     * 先停止所有匹配的运行中目标，再移除。
     *
     * @tparam GoalType 要移除的目标类型
     */
    template <typename GoalType>
    void removeGoalsOfType()
    {
        // 先停止所有匹配的运行中目标：先收集再 resetTask（resetTask 可能改动 m_goals）。
        std::vector<PrioritizedGoal*> goalsToStop;
        for (auto& pg : m_goals) {
            if (dynamic_cast<GoalType*>(pg.getGoal()) != nullptr && pg.isRunning()) {
                goalsToStop.push_back(&pg);
            }
        }
        for (PrioritizedGoal* pg : goalsToStop) {
            pg->resetTask();
        }
        // 移除所有匹配目标（同步清理 flagGoals 悬垂指针）
        m_goals.remove_if([](const PrioritizedGoal& pg) {
            return dynamic_cast<const GoalType*>(pg.getGoal()) != nullptr;
        });
        _pruneFlagGoals();
    }

    /**
     * @brief 半 tick 刻更新（等价 vanilla GoalSelector.tickRunningGoals(false)）
     *
     * 对齐 vanilla Mob.serverAiStep 的 (tickCount+id)%2 门控：
     * - 需完整评估的帧调用 tick()（goalCleanup + goalUpdate + goalTick）；
     * - 半 tick 帧调用本方法，只对那些 requiresUpdateEveryTick() 的运行中目标调 tick()，
     *   其余目标（默认每秒评估一次的那种）保持不动。
     *
     * 调用方（MobEntity）按实体自身的错峰相位选择调用哪一个。
     */
    void tickRunningGoalsEveryTick()
    {
        for (auto& goal : m_goals) {
            if (goal.isRunning() && goal.requiresUpdateEveryTick()) {
                goal.tick();
            }
        }
    }

    /**
     * @brief 刻更新（等价 vanilla GoalSelector.tick，仅在完整评估帧调用）
     *
     * 执行流程：
     * 1. goalCleanup: 停止不应继续的目标
     * 2. goalUpdate: 启动新的目标
     * 3. goalTick: 更新所有运行中的目标
     *
     * tickRate控制: 每N tick执行一次完整的目标选择更新
     */
    void tick()
    {
        // tickRate 控制：每 N tick 执行一次完整的目标选择
        ++m_tickCounter;
        bool shouldUpdateGoals = (m_tickCounter >= m_tickRate);
        if (shouldUpdateGoals) {
            m_tickCounter = 0;
        }

        // ========== Phase 1: goalCleanup ==========
        // 停止不应继续运行的目标
        // 对齐 vanilla GoalSelector.tick 的 goalCleanup：先遍历判定，再统一 stop。
        // vanilla 在 for 遍历中对 availableGoals 调 wrappedgoal.stop()（TreeSet，元素稳定），
        // 本项目的 resetTask() 可能经 removeGoal/removeGoalsOfType 改动 m_goals（std::list），
        // 边遍历边修改会让迭代器失效，故先收集再停止。
        std::vector<PrioritizedGoal*> goalsToStop;
        for (auto& goal : m_goals) {
            if (goal.isRunning() && (!goal.shouldContinueExecuting() || _checkDisabledFlags(goal.getMutexFlags()))) {
                goalsToStop.push_back(&goal);
            }
        }
        for (PrioritizedGoal* goal : goalsToStop) {
            goal->resetTask();
        }

        // 清理 flagGoals 中已停止的目标
        for (auto it = m_flagGoals.begin(); it != m_flagGoals.end();) {
            if (!it->second->isRunning()) {
                it = m_flagGoals.erase(it);
            } else {
                ++it;
            }
        }

        // ========== Phase 2: goalUpdate ==========
        // 启动新目标（仅当 tickRate 允许时）
        //
        // 对齐 vanilla GoalSelector.tick 的 goalUpdate：在同一趟遍历中"判定 → 停止被替换者
        // → 启动"。_canStartGoal 内部会调用 shouldExecute()，其副作用依赖"此刻"的标志占用
        // 情况；若先收集全部候选再统一启动，后序目标的 shouldExecute 会看到前序目标尚未
        // 入住的状态。为兼容"启动过程中 m_goals 被 resetTask 改动"的情形，改用前向游标 +
        // 元素身份校验，而非 list 迭代器直接遍历。
        for (std::list<PrioritizedGoal>::iterator it = m_goals.begin(); it != m_goals.end();) {
            PrioritizedGoal* goal = &*it;
            if (goal->isRunning() || !_canStartGoal(*goal)) {
                ++it;
                continue;
            }
            // 先定位下一个候选元素：_startGoal 的 resetTask 可能移除后续元素，届时按身份在
            // 链表中重新定位（找不到说明已被移除，从表尾继续）。
            PrioritizedGoal* nextGoal = nullptr;
            if (std::next(it) != m_goals.end()) {
                nextGoal = &*std::next(it);
            }
            _startGoal(*goal);
            if (nextGoal == nullptr) {
                it = m_goals.end();
                continue;
            }
            auto found = std::find_if(m_goals.begin(), m_goals.end(), [nextGoal](const PrioritizedGoal& pg) {
                return &pg == nextGoal;
            });
            it = found;
        }

        // ========== Phase 3: goalTick ==========
        // 更新所有运行中的目标
        for (auto& goal : m_goals) {
            if (goal.isRunning()) {
                goal.tick();
            }
        }
    }

    /**
     * @brief 禁用指定标志
     *
     * 禁用后，使用该标志的目标将无法执行。
     *
     * @param flag 要禁用的标志
     */
    void disableFlag(GoalFlag flag) noexcept
    {
        m_disabledFlags.set(flag);
    }

    /**
     * @brief 启用指定标志
     *
     * @param flag 要启用的标志
     */
    void enableFlag(GoalFlag flag) noexcept
    {
        m_disabledFlags.reset(flag);
    }

    /**
     * @brief 设置标志启用状态
     *
     * @param flag 标志
     * @param enabled true 为启用，false 为禁用
     */
    void setFlag(GoalFlag flag, bool enabled)
    {
        if (enabled) {
            enableFlag(flag);
        } else {
            disableFlag(flag);
        }
    }

    /**
     * @brief 检查标志是否被禁用
     *
     * @param flag 标志
     * @return true 如果被禁用
     */
    [[nodiscard]] bool isFlagDisabled(GoalFlag flag) const noexcept
    {
        return m_disabledFlags.test(flag);
    }

    /**
     * @brief 设置更新间隔
     *
     * @param rate tick间隔
     */
    void setTickRate(i32 rate) noexcept
    {
        m_tickRate = rate;
    }

    /**
     * @brief 获取所有正在运行的目标
     *
     * @tparam Func 可调用类型
     * @param func 对每个目标调用的函数
     */
    template <typename Func>
    void forEachRunningGoal(Func&& func)
    {
        for (auto& goal : m_goals) {
            if (goal.isRunning()) {
                func(goal);
            }
        }
    }

    /**
     * @brief 获取所有目标
     *
     * @return 所有目标的常量引用
     */
    [[nodiscard]] const std::list<PrioritizedGoal>& getAllGoals() const noexcept
    {
        return m_goals;
    }

    /**
     * @brief 检查是否有正在运行的目标
     */
    [[nodiscard]] bool hasRunningGoals() const
    {
        for (const auto& goal : m_goals) {
            if (goal.isRunning()) {
                return true;
            }
        }
        return false;
    }

private:
    /**
     * @brief 检查目标是否可以启动
     *
     * 检查禁用标志、互斥标志抢占、shouldExecute
     */
    [[nodiscard]] bool _canStartGoal(PrioritizedGoal& goal) const
    {
        // 检查是否有禁用的标志
        if (_checkDisabledFlags(goal.getMutexFlags())) {
            return false;
        }

        // 检查是否可以抢占正在运行的共享标志的目标
        const auto& flags = goal.getMutexFlags();
        if (!flags.empty()) {
            bool canStart = true;
            flags.forEach([this, &goal, &canStart](GoalFlag flag) {
                auto it = m_flagGoals.find(flag);
                // 如果没有正在运行的目标，或者当前目标可被新目标抢占
                if (it != m_flagGoals.end() && it->second && !it->second->isPreemptedBy(goal)) {
                    canStart = false;
                }
            });
            if (!canStart) {
                return false;
            }
        }

        return goal.shouldExecute();
    }

    /**
     * @brief 启动目标
     *
     * 停止共享相同标志的目标，更新 flagGoals，调用 startExecuting
     */
    void _startGoal(PrioritizedGoal& goal)
    {
        // 停止共享相同标志的正在运行的目标（对齐 vanilla goalCanBeReplacedForAllFlags 后逐个 stop）。
        // 正向遍历 m_goals 而非按 flag 逐个停止：resetTask 可能移除其它同标志目标，
        // 使 m_flagGoals 中尚未处理的条目变成悬垂指针。
        const auto& flags = goal.getMutexFlags();
        std::vector<PrioritizedGoal*> goalsToStop;
        for (auto& candidate : m_goals) {
            if (&candidate == &goal || !candidate.isRunning()) {
                continue;
            }
            bool sharesFlag = false;
            flags.forEach([&candidate, &sharesFlag](GoalFlag flag) {
                if (candidate.getMutexFlags().test(flag)) {
                    sharesFlag = true;
                }
            });
            if (sharesFlag) {
                goalsToStop.push_back(&candidate);
            }
        }
        for (PrioritizedGoal* candidate : goalsToStop) {
            candidate->resetTask();
        }

        // 更新 flagGoals
        flags.forEach([this, &goal](GoalFlag flag) {
            m_flagGoals[flag] = &goal;
        });

        goal.startExecuting();
    }

    /**
     * @brief 检查是否有禁用的标志
     */
    [[nodiscard]] bool _checkDisabledFlags(const EnumSet<GoalFlag>& flags) const
    {
        bool hasDisabled = false;
        flags.forEach([this, &hasDisabled](GoalFlag flag) {
            if (m_disabledFlags.test(flag)) {
                hasDisabled = true;
            }
        });
        return hasDisabled;
    }

    /**
     * @brief 检查目标是否已存在
     */
    [[nodiscard]] bool _hasGoal(const Goal* goal) const
    {
        for (const auto& pg : m_goals) {
            if (pg.getGoal() == goal) {
                return true;
            }
        }
        return false;
    }

    /**
     * @brief 清理 flagGoals 中指向已从 m_goals 移除（销毁）目标的悬垂指针
     *
     * UAF 根因（全量集成测试 GoalSelector.hpp:319 间歇崩溃）：
     * m_flagGoals 存 PrioritizedGoal* 指向 m_goals（std::list，指针稳定）的元素。
     * removeGoal/removeGoalsOfType 经 remove_if 销毁运行中的 goal 时，
     * 若该 goal 此前已注册进 m_flagGoals（_startGoal 中登记），映射中残留
     * 指向已析构元素的悬垂指针；下次 GoalSelector::tick → _canStartGoal →
     * it->second->isPreemptedBy(goal) 解引用悬垂指针 → ACCESS_VIOLATION
     * 读 0xffffffffffffffff。
     *
     * 触发链（AbstractSkeletonEntity）：setEquipment（主/副手变更）→
     * setCombatTask → removeGoalsOfType<RangedBowAttackGoal/MeleeAttackGoal>
     * 销毁运行中战斗 goal → m_flagGoals 悬垂。
     *
     * 修复：移除目标后同步清理映射中不再指向 m_goals 存活元素的项。
     * 实现为 O(goals * flagGoals)，仅 mutate 路径调用，goal 数量级 ~10，可忽略。
     */
    void _pruneFlagGoals()
    {
        for (auto it = m_flagGoals.begin(); it != m_flagGoals.end();) {
            const bool alive =
                std::any_of(m_goals.begin(), m_goals.end(), [ptr = it->second](const PrioritizedGoal& pg) {
                    return &pg == ptr;
                });
            if (!alive) {
                it = m_flagGoals.erase(it);
            } else {
                ++it;
            }
        }
    }

    std::list<PrioritizedGoal> m_goals;                         ///< 所有目标（使用list确保指针稳定性）
    std::unordered_map<GoalFlag, PrioritizedGoal*> m_flagGoals; ///< 标志到正在运行的目标的映射
    EnumSet<GoalFlag> m_disabledFlags;                          ///< 禁用的标志
    i32 m_tickRate;                                             ///< 更新间隔（每N tick更新目标选择）
    i32 m_tickCounter = 0;                                      ///< 当前tick计数器
};

} // namespace mc::entity::ai
