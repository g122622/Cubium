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

#include "TickPriority.hpp"
#include "common/core/Types.hpp"
#include "common/world/block/BlockPos.hpp"
#include <cstddef>
#include <functional>

namespace mc::world::tick {

/**
 * @brief 调度的Tick条目
 *
 * 存储待执行的tick信息，包括位置、目标、调度时间和优先级。
 * 支持按(scheduledTick, priority, tickEntryId)排序。
 *
 * 参考: net.minecraft.world.NextTickListEntry
 *
 * @tparam T 目标类型（Block、Fluid等）
 */
template <typename T>
struct ScheduledTick {
    BlockPos position;     ///< 方块位置
    const T* target;       ///< 目标对象指针
    u64 scheduledTick;     ///< 调度执行的游戏刻
    TickPriority priority; ///< 执行优先级
    u64 tickEntryId;       ///< 唯一ID（用于排序）

    /**
     * @brief 默认构造
     */
    ScheduledTick() noexcept
        : target(nullptr)
        , scheduledTick(0)
        , priority(TickPriority::Normal)
        , tickEntryId(0)
    {}

    /**
     * @brief 创建调度条目
     * @param pos 方块位置
     * @param target 目标对象
     * @param scheduledTick 调度执行的游戏刻
     * @param priority 优先级
     * @param tickEntryId 唯一ID
     */
    ScheduledTick(
        const BlockPos& pos, const T* target, u64 scheduledTick, TickPriority priority, u64 tickEntryId) noexcept
        : position(pos)
        , target(target)
        , scheduledTick(scheduledTick)
        , priority(priority)
        , tickEntryId(tickEntryId)
    {}

    /**
     * @brief 比较器，用于排序
     *
     * 排序顺序: scheduledTick -> priority -> tickEntryId
     * 返回true表示this应该排在other之前（优先执行）
     */
    [[nodiscard]] bool operator<(const ScheduledTick& other) const noexcept
    {
        if (scheduledTick != other.scheduledTick) {
            return scheduledTick < other.scheduledTick;
        }
        if (priority != other.priority) {
            return static_cast<i8>(priority) < static_cast<i8>(other.priority);
        }
        return tickEntryId < other.tickEntryId;
    }

    /**
     * @brief 相等比较，基于位置和目标
     *
     * 注意：用于HashSet去重，只比较位置和目标，
     * 不比较scheduledTick和priority
     */
    [[nodiscard]] bool operator==(const ScheduledTick& other) const noexcept
    {
        return position == other.position && target == other.target;
    }

    /**
     * @brief 哈希值，基于位置和目标
     */
    [[nodiscard]] size_t hashCode() const noexcept
    {
        size_t h = position.toId();
        // 混合目标指针
        h ^= reinterpret_cast<size_t>(target) + 0x9e3779b9 + (h << 6) + (h >> 2);
        return h;
    }
};

/**
 * @brief ScheduledTick的哈希函数
 */
template <typename T>
struct ScheduledTickHash {
    size_t operator()(const ScheduledTick<T>& tick) const noexcept { return tick.hashCode(); }
};

} // namespace mc::world::tick
