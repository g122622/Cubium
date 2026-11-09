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

#include "ITickList.hpp"
#include "common/core/Types.hpp"
#include "common/world/block/base/types/BlockPos.hpp"
#include "common/world/tick/base/TickPriority.hpp"
#include <cstddef>
#include <functional>
#include <utility>

namespace mc::world::tick {

/**
 * @brief 把 tick 调度委托给外部回调的列表实现
 *
 * 用于自身不持有 tick 存储的上下文：本类不排队、不排序、不执行，所有调度/查询
 * 全部转交给构造时提供的回调，由回调决定刻最终落在哪里。
 *
 * 典型用法是世界生成：WorldGenRegion 把调度转发到目标区块的 ChunkPrimer 待执行
 * 刻列表，区块发布后再由 ServerChunkManager 转交真正的世界 TickManager。
 *
 * 回调的生命周期必须覆盖本对象。执行/序列化相关方法沿用 ITickList 的默认空行为。
 *
 * @tparam T 目标类型（Block、Fluid等）
 */
template <typename T>
class DelegatingTickList final : public ITickList<T> {
public:
    /// 调度回调：(位置, 目标, 延迟, 优先级)
    using ScheduleCallback = std::function<void(const BlockPos&, const T&, i32, TickPriority)>;

    /// 查询回调：(位置, 目标) -> 是否已调度/是否本刻待执行
    using QueryCallback = std::function<bool(const BlockPos&, const T&)>;

    /// 取消回调：(位置, 目标) -> 是否成功取消
    using CancelCallback = std::function<bool(const BlockPos&, const T&)>;

    /// 待处理数量查询回调
    using PendingCountCallback = std::function<size_t()>;

    DelegatingTickList(ScheduleCallback schedule,
        QueryCallback isScheduled,
        QueryCallback isPending,
        CancelCallback cancel,
        PendingCountCallback pendingCount)
        : m_schedule(std::move(schedule))
        , m_isScheduled(std::move(isScheduled))
        , m_isPending(std::move(isPending))
        , m_cancel(std::move(cancel))
        , m_pendingCount(std::move(pendingCount))
    {}

    // 引入基类的默认优先级版本
    using ITickList<T>::scheduleTick;

    void scheduleTick(const BlockPos& pos, const T& target, i32 delay, TickPriority priority) override
    {
        m_schedule(pos, target, delay, priority);
    }

    [[nodiscard]] bool isTickScheduled(const BlockPos& pos, const T& target) const override
    {
        return m_isScheduled(pos, target);
    }

    [[nodiscard]] bool isTickPending(const BlockPos& pos, const T& target) const override
    {
        return m_isPending(pos, target);
    }

    bool cancelTick(const BlockPos& pos, const T& target) override
    {
        return m_cancel(pos, target);
    }

    [[nodiscard]] size_t pendingCount() const override
    {
        return m_pendingCount();
    }

private:
    ScheduleCallback m_schedule;
    QueryCallback m_isScheduled;
    QueryCallback m_isPending;
    CancelCallback m_cancel;
    PendingCountCallback m_pendingCount;
};

} // namespace mc::world::tick
