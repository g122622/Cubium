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
