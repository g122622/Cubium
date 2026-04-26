#pragma once

#include "Goal.hpp"
#include "PrioritizedGoal.hpp"
#include "../../../core/Types.hpp"
#include <vector>
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
     * MC 1.16.5: 目标按优先级排序插入，优先级小的在前
     * @param priority 优先级（数值越小优先级越高）
     * @param goal AI目标
     */
    void addGoal(int priority, std::unique_ptr<Goal> goal) {
        // 按优先级插入到正确位置（保持有序）
        auto it = std::lower_bound(m_goals.begin(), m_goals.end(), priority,
            [](const PrioritizedGoal& pg, int prio) { return pg.getPriority() < prio; });
        m_goals.emplace(it, priority, std::move(goal));
    }

    /**
     * @brief 添加AI目标（原始指针版本）
     *
     * MC 1.16.5: 目标按优先级排序插入
     * @param priority 优先级
     * @param goal AI目标（获取所有权）
     */
    void addGoal(int priority, Goal* goal) {
        // 按优先级插入到正确位置（保持有序）
        auto it = std::lower_bound(m_goals.begin(), m_goals.end(), priority,
            [](const PrioritizedGoal& pg, int prio) { return pg.getPriority() < prio; });
        m_goals.emplace(it, priority, goal);
    }

    /**
     * @brief 移除AI目标
     *
     * 移除所有匹配的目标实例（同一个Goal可能被多次添加）。
     *
     * @param goal 要移除的目标指针
     */
    void removeGoal(Goal* goal) {
        // 先停止所有运行中的匹配目标
        for (auto& pg : m_goals) {
            if (pg.getGoal() == goal && pg.isRunning()) {
                pg.resetTask();
            }
        }
        // 再移除所有匹配目标
        m_goals.erase(
            std::remove_if(m_goals.begin(), m_goals.end(),
                [goal](const PrioritizedGoal& pg) { return pg.getGoal() == goal; }),
            m_goals.end());
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
     * 每tick调用，负责选择和执行AI目标。
     */
    void tick() {
        // 1. 清理已停止的目标（只调用resetTask，不清除flagGames）
        for (auto& goal : m_goals) {
            if (goal.isRunning()) {
                if (!goal.shouldContinueExecuting() ||
                    hasDisabledFlag(goal.getMutexFlags())) {
                    goal.resetTask();
                }
            }
        }

        // 清理 flagGoals 中已停止的目标
        for (auto it = m_flagGoals.begin(); it != m_flagGoals.end(); ) {
            if (!it->second->isRunning()) {
                it = m_flagGoals.erase(it);
            } else {
                ++it;
            }
        }

        // 2. 选择新的目标
        for (auto& goal : m_goals) {
            if (!goal.isRunning() &&
                canStartGoal(goal)) {
                // 抢占共享相同标志的低优先级目标
                startGoal(goal);
            }
        }

        // 3. 更新正在运行的目标
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
    [[nodiscard]] const std::vector<PrioritizedGoal>& getAllGoals() const {
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
     */
    [[nodiscard]] bool canStartGoal(PrioritizedGoal& goal) const {
        // 检查是否有禁用的标志
        if (hasDisabledFlag(goal.getMutexFlags())) {
            return false;
        }

        // 检查是否可以抢占正在运行的共享标志的目标
        const auto& flags = goal.getMutexFlags();
        bool canStart = flags.empty(); // 无标志的目标总是可以启动

        if (!flags.empty()) {
            canStart = true;
            flags.forEach([this, &goal, &canStart](GoalFlag flag) {
                auto it = m_flagGoals.find(flag);
                if (it != m_flagGoals.end() && !it->second->isPreemptedBy(goal)) {
                    canStart = false;
                }
            });
        }

        return canStart && goal.shouldExecute();
    }

    /**
     * @brief 启动目标
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

        // 更新 flagGoals
        flags.forEach([this, &goal](GoalFlag flag) {
            m_flagGoals[flag] = &goal;
        });

        goal.startExecuting();
    }

    /**
     * @brief 清除指定标志的映射
     */
    void clearFlagGoals(const EnumSet<GoalFlag>& flags) {
        flags.forEach([this](GoalFlag flag) {
            m_flagGoals.erase(flag);
        });
    }

    /**
     * @brief 检查是否有禁用的标志
     */
    [[nodiscard]] bool hasDisabledFlag(const EnumSet<GoalFlag>& flags) const {
        bool hasDisabled = false;
        flags.forEach([this, &hasDisabled](GoalFlag flag) {
            if (m_disabledFlags.test(flag)) {
                hasDisabled = true;
            }
        });
        return hasDisabled;
    }

    std::vector<PrioritizedGoal> m_goals;                    // 所有目标
    std::unordered_map<GoalFlag, PrioritizedGoal*> m_flagGoals;  // 标志到正在运行的目标的映射
    EnumSet<GoalFlag> m_disabledFlags;                       // 禁用的标志
    int m_tickRate;                                           // 更新间隔
};

} // namespace mc::entity::ai
