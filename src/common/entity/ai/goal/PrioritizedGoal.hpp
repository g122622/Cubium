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
#include "common/core/EnumSet.hpp"
#include "common/core/Types.hpp"
#include "common/entity/ai/goal/GoalFlag.hpp"

#include <limits>
#include <memory>
#include <string>
#include <utility>

namespace mc::entity::ai {

/**
 * @brief 带优先级的目标包装器
 *
 * 包装一个 Goal 并添加优先级信息。
 * 优先级数值越小，优先级越高。
 */
class PrioritizedGoal : public Goal {
public:
    /**
     * @brief 构造函数
     * @param priority 优先级（数值越小优先级越高）
     * @param goal 被包装的目标
     */
    PrioritizedGoal(i32 priority, std::unique_ptr<Goal> goal)
        : m_priority(priority)
        , m_inner(std::move(goal))
        , m_running(false)
    {}

    /**
     * @brief 构造函数（从原始指针）
     * @param priority 优先级
     * @param goal 被包装的目标（获取所有权）
     */
    PrioritizedGoal(i32 priority, Goal* goal)
        : m_priority(priority)
        , m_inner(goal)
        , m_running(false)
    {}

    /**
     * @brief 默认构造函数（创建DUMMY对象）
     * 用于GoalSelector中flagGoals的默认值
     */
    PrioritizedGoal()
        : m_priority(std::numeric_limits<i32>::max())
        , m_inner(nullptr)
        , m_running(false)
    {}

    /**
     * @brief 移动构造函数
     */
    PrioritizedGoal(PrioritizedGoal&& other) noexcept
        : m_priority(other.m_priority)
        , m_inner(std::move(other.m_inner))
        , m_running(other.m_running)
    {
        other.m_priority = std::numeric_limits<i32>::max();
        other.m_running = false;
    }

    /**
     * @brief 移动赋值运算符
     */
    PrioritizedGoal& operator=(PrioritizedGoal&& other) noexcept
    {
        if (this != &other) {
            m_priority = other.m_priority;
            m_inner = std::move(other.m_inner);
            m_running = other.m_running;
            other.m_priority = std::numeric_limits<i32>::max();
            other.m_running = false;
        }
        return *this;
    }

    // 禁用拷贝（因为持有unique_ptr）
    PrioritizedGoal(const PrioritizedGoal&) = delete;
    PrioritizedGoal& operator=(const PrioritizedGoal&) = delete;

    /**
     * @brief 是否可以被另一个目标抢占
     * @param other 要比较的目标
     * @return true 如果当前目标可以被抢占
     */
    [[nodiscard]] bool isPreemptedBy(const PrioritizedGoal& other) const noexcept
    {
        // DUMMY（空目标）总是返回true
        if (!m_inner) {
            return true;
        }
        // 如果不可抢占，返回false
        if (!isPreemptible()) {
            return false;
        }
        // 如果other优先级更高（数值更小），则可被抢占
        return other.m_priority < m_priority;
    }

    // ========== Goal 接口实现 ==========

    [[nodiscard]] bool shouldExecute() override { return m_inner && m_inner->shouldExecute(); }

    [[nodiscard]] bool shouldContinueExecuting() override { return m_inner && m_inner->shouldContinueExecuting(); }

    [[nodiscard]] bool isPreemptible() const noexcept override { return !m_inner || m_inner->isPreemptible(); }

    void startExecuting() override
    {
        if (m_inner && !m_running) {
            m_running = true;
            m_inner->startExecuting();
        }
    }

    void resetTask() override
    {
        if (m_inner && m_running) {
            m_running = false;
            m_inner->resetTask();
        }
    }

    void tick() override
    {
        if (m_inner) {
            m_inner->tick();
        }
    }

    void setMutexFlags(const EnumSet<GoalFlag>& flags)
    {
        if (m_inner) {
            m_inner->setMutexFlags(flags);
        }
    }

    [[nodiscard]] const EnumSet<GoalFlag>& getMutexFlags() const noexcept
    {
        static EnumSet<GoalFlag> emptyFlags;
        return m_inner ? m_inner->getMutexFlags() : emptyFlags;
    }

    [[nodiscard]] std::string getTypeName() const override { return m_inner ? m_inner->getTypeName() : "DUMMY"; }

    // ========== PrioritizedGoal 特有方法 ==========

    /**
     * @brief 获取优先级
     */
    [[nodiscard]] i32 getPriority() const noexcept { return m_priority; }

    /**
     * @brief 是否正在运行
     */
    [[nodiscard]] bool isRunning() const noexcept { return m_running; }

    /**
     * @brief 获取内部目标
     */
    [[nodiscard]] Goal* getGoal() noexcept { return m_inner.get(); }

    /**
     * @brief 获取内部目标（const版本）
     */
    [[nodiscard]] const Goal* getGoal() const noexcept { return m_inner.get(); }

    /**
     * @brief 检查是否为空（DUMMY对象）
     */
    [[nodiscard]] bool isNull() const noexcept { return m_inner == nullptr; }

    /**
     * @brief 相等比较
     * 比较内部Goal是否相同（指针比较）
     */
    [[nodiscard]] bool operator==(const PrioritizedGoal& other) const noexcept
    {
        // 两个null比较返回false
        if (!m_inner && !other.m_inner) {
            return false;
        }
        // 一个null一个非null返回false
        if (!m_inner || !other.m_inner) {
            return false;
        }
        // 比较内部Goal指针
        return m_inner.get() == other.m_inner.get();
    }

    /**
     * @brief 不等比较
     */
    [[nodiscard]] bool operator!=(const PrioritizedGoal& other) const noexcept { return !(*this == other); }

private:
    i32 m_priority;                // 优先级
    std::unique_ptr<Goal> m_inner; // 内部目标
    bool m_running;                // 是否正在运行
};

} // namespace mc::entity::ai
