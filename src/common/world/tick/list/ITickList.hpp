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
#include "common/world/block/base/types/BlockPos.hpp"
#include "common/world/tick/base/ScheduledTick.hpp"
#include "common/world/tick/base/TickPriority.hpp"
#include <cstddef>
#include <vector>

namespace mc::world::tick {

/// 单游戏刻最多执行的计划刻数量
inline constexpr size_t MAX_TICKS_PER_TICK = 65536;

/**
 * @brief Tick调度列表接口
 *
 * 定义tick调度的通用接口，支持方块tick、流体tick等。
 * 实现可以是ServerTickList（服务端实际排队执行）、EmptyTickList（客户端/空实现）
 * 或 DelegatingTickList（把调度转交给外部）。
 *
 * 用法示例:
 * @code
 * ITickList<Block>& blockTicks = world.blockTicks();
 *
 * // 调度一个方块tick
 * blockTicks.scheduleTick(pos, block, 10);  // 10 tick延迟
 *
 * // 检查是否有待执行的tick
 * if (blockTicks.isTickScheduled(pos, block)) {
 *     // ...
 * }
 * @endcode
 *
 * @tparam T 目标类型（Block、Fluid等）
 */
template <typename T>
class ITickList {
public:
    virtual ~ITickList() = default;

    /**
     * @brief 是否已调度tick（待执行）
     *
     * 检查指定位置和目标是否已经在调度队列中。
     *
     * @param pos 位置
     * @param target 目标
     * @return 是否已调度
     */
    [[nodiscard]] virtual bool isTickScheduled(const BlockPos& pos, const T& target) const = 0;

    /**
     * @brief 是否在本tick内待执行
     *
     * 检查指定位置和目标是否在本游戏刻内即将执行。
     * 与isTickScheduled的区别：isTickPending只检查当前tick，
     * 而isTickScheduled检查所有未来tick。
     *
     * @param pos 位置
     * @param target 目标
     * @return 是否本tick待执行
     */
    [[nodiscard]] virtual bool isTickPending(const BlockPos& pos, const T& target) const = 0;

    /**
     * @brief 调度tick（普通优先级）
     *
     * @param pos 位置
     * @param target 目标
     * @param delay 延迟tick数（相对于当前游戏刻）
     */
    virtual void scheduleTick(const BlockPos& pos, const T& target, i32 delay)
    {
        scheduleTick(pos, target, delay, TickPriority::Normal);
    }

    /**
     * @brief 调度tick（指定优先级）
     *
     * @param pos 位置
     * @param target 目标
     * @param delay 延迟tick数
     * @param priority 优先级
     */
    virtual void scheduleTick(const BlockPos& pos, const T& target, i32 delay, TickPriority priority) = 0;

    /**
     * @brief 取消tick
     *
     * 移除指定位置和目标的调度。如果不存在则无操作。
     *
     * @param pos 位置
     * @param target 目标
     * @return 是否成功取消
     */
    virtual bool cancelTick(const BlockPos& pos, const T& target)
    {
        // 默认实现不支持取消
        return false;
    }

    /**
     * @brief 获取待处理tick数量
     */
    [[nodiscard]] virtual size_t pendingCount() const = 0;

    // ========== 执行与序列化 ==========
    // 不实际排队执行 tick 的实现（生成期存储、空实现）沿用下面的默认空行为。

    /**
     * @brief 设置当前游戏刻
     *
     * 调度延迟以当前游戏刻为基准，执行 tick 的实现需要保存该值。
     */
    virtual void setCurrentTick(u64 /*currentTick*/) {}

    /**
     * @brief 获取当前游戏刻
     */
    [[nodiscard]] virtual u64 currentTick() const
    {
        return 0;
    }

    /**
     * @brief 执行到期的 tick
     *
     * @param currentTick 当前游戏刻
     * @param maxTicks 本次最多执行的 tick 数
     */
    virtual void tick(u64 /*currentTick*/, size_t /*maxTicks*/) {}

    /**
     * @brief 本游戏刻已执行的 tick 数量
     */
    [[nodiscard]] virtual size_t executedThisTickCount() const
    {
        return 0;
    }

    /**
     * @brief 取出指定区块范围内的待执行 tick
     *
     * 用于区块序列化保存。不存储 tick 的实现返回空列表。
     *
     * @param chunkX 区块X坐标
     * @param chunkZ 区块Z坐标
     * @param remove 是否从列表中移除
     * @param skipCompleted 是否跳过已执行的tick
     */
    [[nodiscard]] virtual std::vector<ScheduledTick<T>> getPendingTicks(
        i32 /*chunkX*/, i32 /*chunkZ*/, bool /*remove*/, bool /*skipCompleted*/)
    {
        return {};
    }
};

} // namespace mc::world::tick
