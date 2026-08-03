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
#include "common/util/assert/AssertAll.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/tick/base/TickPriority.hpp"
#include <cstddef>

namespace mc::world::tick {

/**
 * @brief 空Tick列表实现
 *
 * 用于客户端或不需要tick处理的场景。
 * 单例模式，所有方法返回空操作。
 *
 * 用法示例:
 * @code
 * // 获取空tick列表实例
 * ITickList<Block>& ticks = EmptyTickList<Block>::get();
 *
 * // 所有操作都是无操作
 * ticks.scheduleTick(pos, block, 10);  // 不做任何事情
 * ticks.isTickScheduled(pos, block);    // 始终返回false
 * @endcode
 *
 * @tparam T 目标类型
 */
template <typename T>
class EmptyTickList : public ITickList<T> {
public:
    /**
     * @brief 获取单例实例
     */
    static EmptyTickList<T>& get() noexcept
    {
        static EmptyTickList<T> instance;
        return instance;
    }

    [[nodiscard]] bool isTickScheduled(const BlockPos& pos, const T& target) const noexcept override
    {
        MC_UNUSED(pos, target);
        return false;
    }

    [[nodiscard]] bool isTickPending(const BlockPos& pos, const T& target) const noexcept override
    {
        MC_UNUSED(pos, target);
        return false;
    }

    void scheduleTick(const BlockPos& pos, const T& target, i32 delay) noexcept override
    {
        MC_UNUSED(pos, target, delay);
        // 空操作
    }

    void scheduleTick(const BlockPos& pos, const T& target, i32 delay, TickPriority priority) noexcept override
    {
        MC_UNUSED(pos, target, delay, priority);
        // 空操作
    }

    [[nodiscard]] size_t pendingCount() const noexcept override { return 0; }

private:
    EmptyTickList() = default;
    EmptyTickList(const EmptyTickList&) = delete;
    EmptyTickList& operator=(const EmptyTickList&) = delete;
};

} // namespace mc::world::tick
