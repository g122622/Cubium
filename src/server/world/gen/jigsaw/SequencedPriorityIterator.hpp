/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software, and to the following
 * notice and other disclaimer in the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT
 * LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO
 * EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#pragma once

#include "common/core/Types.hpp"

#include <cstddef>
#include <deque>
#include <map>
#include <optional>
#include <utility>

namespace mc {
namespace world {
namespace gen {
namespace jigsaw {

/**
 * @brief 按优先级降序出队的迭代器/队列
 *
 * 对应 MC 1.21 net.minecraft.util.SequencedPriorityIterator<T>。
 *
 * 元素通过 add(item, priority) 入队，相同优先级按入队顺序出队（FIFO），
 * 高优先级的元素先出队。next() 返回当前最高优先级队列的队首元素；
 * 当某优先级队列为空时切换到次高优先级队列。hasNext() 表示是否还有元素。
 *
 * MC 实现使用 Int2ObjectOpenHashMap<Deque<T>>（按优先级分桶），Cubium 用
 * std::map<i32, std::deque<T>>（按 key 升序），用反向迭代器取最高优先级。
 *
 * Jigsaw 组装队列（PendingJoint）用 placementPriority 入队：
 * 起始块的连接点按 selectionPriority 排序后逐个入队，每个 PendingJoint 携带
 * 其源连接点的 placementPriority；放置成功后子块的连接点以同样方式入队。
 * 这使高 placementPriority 的子结构先被扩展，对应 MC JigsawPlacement.Placer.placing。
 */
template <typename T>
class SequencedPriorityIterator {
public:
    /**
     * @brief 入队一个元素
     * @param item 元素（右值，移动入队）
     * @param priority 优先级（数值越大越先出队）
     */
    void add(T item, i32 priority)
    {
        auto& deque = m_queues[priority];
        deque.push_back(std::move(item));
    }

    /**
     * @brief 是否还有元素
     * @return true 表示还有元素可出队
     */
    bool hasNext() const { return !m_queues.empty(); }

    /**
     * @brief 取出并移除当前最高优先级队列的队首元素
     *
     * 相同优先级内按入队顺序出队（FIFO），不同优先级间按优先级降序出队。
     * 队列为空后自动移除空桶。
     *
     * @return 队首元素；调用前必须保证 hasNext() 为 true
     */
    T next()
    {
        // 取最高优先级桶（map 按 key 升序，反向迭代器指向最大 key）
        auto it = m_queues.rbegin();
        auto& deque = it->second;
        T item = std::move(deque.front());
        deque.pop_front();
        if (deque.empty()) {
            // erase 反向迭代器对应的正向位置
            m_queues.erase(std::prev(m_queues.end()));
        }
        return item;
    }

    /**
     * @brief 当前队列中的元素总数（所有优先级桶之和）
     */
    size_t size() const
    {
        size_t total = 0;
        for (const auto& [priority, deque] : m_queues) {
            total += deque.size();
        }
        return total;
    }

private:
    // 按 priority 分桶。map 按 key 升序，rbegin() 指向最高优先级。
    std::map<i32, std::deque<T>> m_queues;
};

} // namespace jigsaw
} // namespace gen
} // namespace world
} // namespace mc
