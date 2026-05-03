#pragma once

#include "Goal.hpp"
#include "PrioritizedGoal.hpp"
#include "../../../core/Types.hpp"
#include <list>
#include <unordered_map>
#include <functional>
#include <algorithm>

namespace mc::entity::ai {

/**
 * @brief AI目标选择器
 *
 * 管理实体的所有AI目标，负责选择和执行当前应该运行的目标。
 * 通过优先级和互斥标志协调多个AI目标的执行。
 *
 * 参考 MC 1.16.5 GoalSelector
 */
class GoalSelector {
public:
    /**
     * @brief 默认 tick 间隔
     * MC 1.16.5: 默认为 3
     */
    static constexpr int DEFAULT_TICK_RATE = 3;

    /**
     * @brief 构造函数
     */
    GoalSelector() : m_tickRate(DEFAULT_TICK_RATE) {}

    /**
     * @brief 析构函数
     */
    ~GoalSelector() = default;

    /**
     * @brief 添加AI目标
     *
     * MC 1.16.5: 目标按优先级插入，使用 LinkedHashSet 保持插入顺序且不允许重复
     * @param priority 优先级（数值越小优先级越高）
     * @param goal AI目标
     */
    void addGoal(int priority, std::unique_ptr<Goal> goal) {
        if (goal == nullptr) return;
        if (hasGoal(goal.get())) return;
        m_goals.emplace_back(priority, std::move(goal));
    }

    /**
     * @brief 添加AI目标（原始指针版本）
     *
     * MC 1.16.5: 目标按优先级插入，使用 LinkedHashSet 保持插入顺序且不允许重复
     * @param priority 优先级
     * @param goal AI目标（获取所有权）
     */
    void addGoal(int priority, Goal* goal) {
        if (goal == nullptr) return;
        if (hasGoal(goal)) return;
        m_goals.emplace_back(priority, goal);
    }

    /**
     * @brief 移除AI目标
     *
     * MC 1.16.5: 先停止所有运行中的匹配目标，再移除
     *
     * @param goal 要移除的目标指针
     */
    void removeGoal(Goal* goal) {
        // MC 1.16.5: 先停止所有运行中的匹配目标
        for (auto& pg : m_goals) {
            if (pg.getGoal() == goal && pg.isRunning()) {
                pg.resetTask();
            }
        }
        // 再移除所有匹配目标
        m_goals.remove_if([goal](const PrioritizedGoal& pg) { return pg.getGoal() == goal; });
    }

    /**
     * @brief 移除所有AI目标
     */
    void removeAllGoals() {
        for (auto& prioritizedGoal : m_goals) {
            if (prioritizedGoal.isRunning()) {
                prioritizedGoal.resetTask();
            }
        }
        m_goals.clear();
        m_flagGoals.clear();
    }

    /**
     * @brief 刻更新
     *
     * MC 1.16.5 的执行流程：
     * 1. goalCleanup: 停止不应继续的目标
     * 2. goalUpdate: 启动新的目标
     * 3. goalTick: 更新所有运行中的目标
     *
     * tickRate控制: 每N tick执行一次完整的目标选择更新
     */
    void tick() {
        // tickRate 控制：每 N tick 执行一次完整的目标选择
        ++m_tickCounter;
        bool shouldUpdateGoals = (m_tickCounter >= m_tickRate);
        if (shouldUpdateGoals) {
            m_tickCounter = 0;
        }

        // ========== Phase 1: goalCleanup ==========
        // MC 1.16.5: 停止不应继续运行的目标
        for (auto& goal : m_goals) {
            if (goal.isRunning()) {
                // 检查是否应该继续执行
                bool shouldContinue = goal.shouldContinueExecuting();
                // 检查是否有禁用的标志
                bool hasDisabledFlagResult = checkDisabledFlags(goal.getMutexFlags());

                if (!shouldContinue || hasDisabledFlagResult) {
                    goal.resetTask();
                }
            }
        }

        // MC 1.16.5: 清理 flagGoals 中已停止的目标
        for (auto it = m_flagGoals.begin(); it != m_flagGoals.end(); ) {
            if (!it->second->isRunning()) {
                it = m_flagGoals.erase(it);
            } else {
                ++it;
            }
        }

        // ========== Phase 2: goalUpdate ==========
        // MC 1.16.5: 启动新目标（仅当 tickRate 允许时）
        if (shouldUpdateGoals) {
            for (auto& goal : m_goals) {
                if (!goal.isRunning()) {
                    // 检查是否可以启动
                    if (canStartGoal(goal)) {
                        // 抢占共享相同标志的低优先级目标
                        startGoal(goal);
                    }
                }
            }
        }

        // ========== Phase 3: goalTick ==========
        // MC 1.16.5: 更新所有运行中的目标
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
    void disableFlag(GoalFlag flag) {
        m_disabledFlags.set(flag);
    }

    /**
     * @brief 启用指定标志
     *
     * @param flag 要启用的标志
     */
    void enableFlag(GoalFlag flag) {
        m_disabledFlags.reset(flag);
    }

    /**
     * @brief 设置标志启用状态
     *
     * @param flag 标志
     * @param enabled true 为启用，false 为禁用
     */
    void setFlag(GoalFlag flag, bool enabled) {
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
    [[nodiscard]] bool isFlagDisabled(GoalFlag flag) const {
        return m_disabledFlags.test(flag);
    }

    /**
     * @brief 设置更新间隔
     *
     * @param rate tick间隔
     */
    void setTickRate(int rate) {
        m_tickRate = rate;
    }

    /**
     * @brief 获取所有正在运行的目标
     *
     * MC 1.16.5: 返回 Stream<PrioritizedGoal>
     * @tparam Func 可调用类型
     * @param func 对每个目标调用的函数
     */
    template<typename Func>
    void forEachRunningGoal(Func&& func) {
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
    [[nodiscard]] const std::list<PrioritizedGoal>& getAllGoals() const {
        return m_goals;
    }

    /**
     * @brief 检查是否有正在运行的目标
     */
    [[nodiscard]] bool hasRunningGoals() const {
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
     * MC 1.16.5: 检查禁用标志、互斥标志抢占、shouldExecute
     */
    [[nodiscard]] bool canStartGoal(PrioritizedGoal& goal) const {
        // 检查是否有禁用的标志
        if (checkDisabledFlags(goal.getMutexFlags())) {
            return false;
        }

        // MC 1.16.5: 检查是否可以抢占正在运行的共享标志的目标
        const auto& flags = goal.getMutexFlags();
        if (!flags.empty()) {
            bool canStart = true;
            flags.forEach([this, &goal, &canStart](GoalFlag flag) {
                auto it = m_flagGoals.find(flag);
                // MC 1.16.5: 如果没有正在运行的目标，或者当前目标可被新目标抢占
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
     * MC 1.16.5: 停止共享相同标志的目标，更新 flagGoals，调用 startExecuting
     */
    void startGoal(PrioritizedGoal& goal) {
        // 停止共享相同标志的正在运行的目标
        const auto& flags = goal.getMutexFlags();
        flags.forEach([this](GoalFlag flag) {
            auto it = m_flagGoals.find(flag);
            if (it != m_flagGoals.end() && it->second->isRunning()) {
                it->second->resetTask();
            }
        });

        // MC 1.16.5: 更新 flagGoals
        flags.forEach([this, &goal](GoalFlag flag) {
            m_flagGoals[flag] = &goal;
        });

        goal.startExecuting();
    }

    /**
     * @brief 检查是否有禁用的标志
     */
    [[nodiscard]] bool checkDisabledFlags(const EnumSet<GoalFlag>& flags) const {
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
     * MC 1.16.5: LinkedHashSet 不允许重复
     */
    [[nodiscard]] bool hasGoal(const Goal* goal) const {
        for (const auto& pg : m_goals) {
            if (pg.getGoal() == goal) {
                return true;
            }
        }
        return false;
    }

    std::list<PrioritizedGoal> m_goals;                            // 所有目标（使用list确保指针稳定性）
    std::unordered_map<GoalFlag, PrioritizedGoal*> m_flagGoals;    // 标志到正在运行的目标的映射
    EnumSet<GoalFlag> m_disabledFlags;                             // 禁用的标志
    int m_tickRate;                                                 // 更新间隔（每N tick更新目标选择）
    int m_tickCounter = 0;                                          // 当前tick计数器
};

} // namespace mc::entity::ai
