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

/**
 * @file PathHeap.hpp
 * @brief A* 开放集（二叉堆，对应原版 `BinaryHeap`）。
 *
 * 排序键是节点的 `f`（`g + h * 1.5`），严格按 `<` 比较。堆的**结构**必须与原版一致：
 * A* 在 `f` 相等时按出堆顺序打破平局，而平局在网格搜索里非常常见（大量对称路径
 * 代价完全相同）。只要上浮/下沉的一步选择不同，最终选出的路径就可能不同，
 * 因此这里的实现逐行对齐原版的比较与推进方式，而不是"写出一个正确的最小堆"。
 *
 * 使用约定：
 *   - 插入前节点的 `heapIndex` 必须是 -1（不在堆中）；
 *   - `pop` 之后节点被移出堆，`heapIndex` 回到 -1；
 *   - 修改节点 `f` 后必须调用 `changeCost` 恢复堆序。
 */

#pragma once

#include "PathPoint.hpp"
#include "common/core/Types.hpp"
#include "common/util/assert/AssertAll.hpp"
#include <limits>
#include <vector>

namespace mc::entity::ai::pathfinding {

/**
 * @brief 开放集：按节点总代价排序的二叉堆
 */
class PathHeap {
public:
    PathHeap() = default;

    /**
     * @brief 插入节点并返回它
     * @param node 尚未在堆中的节点
     */
    PathPoint* insert(PathPoint* node)
    {
        MC_ASSERT_RELEASE_MSG(!node->inOpenSet(), "PathHeap::insert received a node already in the heap");
        m_heap.push_back(node);
        node->setHeapIndex(static_cast<i32>(m_heap.size() - 1));
        _upHeap(static_cast<i32>(m_heap.size() - 1));
        return node;
    }

    /** 清空堆（不移除各节点的下标，因此只应在重新搜索前调用）。 */
    void clear()
    {
        m_heap.clear();
    }

    /** 堆顶（不弹出）；堆为空时不得调用。 */
    [[nodiscard]] PathPoint* peek() const
    {
        return m_heap.front();
    }

    /**
     * @brief 弹出并返回代价最小的节点
     *
     * 把最后一个元素搬到堆顶再下沉。注意原版在 `size == 0` 时仍然会做
     * 「`heap[0] = heap[-1]`」这一步（此时它写进的是已经读出的那个元素本身），
     * 因此这里先取出最后元素、给出节点后再在下标 0 写入最后一个元素，顺序等价。
     */
    PathPoint* pop()
    {
        PathPoint* node = m_heap.front();
        m_heap.front() = m_heap.back();
        m_heap.pop_back();
        if (!m_heap.empty()) {
            _downHeap(0);
        }
        node->setHeapIndex(-1);
        return node;
    }

    /**
     * @brief 修改节点的排序键并恢复堆序
     * @param node 已在堆中的节点
     * @param cost 新的总代价
     */
    void changeCost(PathPoint* node, f32 cost)
    {
        const f32 previous = node->totalCost();
        node->setTotalCost(cost);
        const i32 index = node->heapIndex();
        if (cost < previous) {
            _upHeap(index);
        } else {
            _downHeap(index);
        }
    }

    [[nodiscard]] bool empty() const
    {
        return m_heap.empty();
    }

    [[nodiscard]] size_t size() const
    {
        return m_heap.size();
    }

private:
    /**
     * @brief 上浮
     *
     * 先把待上浮节点提到局部变量，再在循环里「空位下沉到父结点」，最后写回。
     * 与交换式写法相比，这决定了 `f` 相等时节点落在哪个下标，从而影响出堆顺序。
     */
    void _upHeap(i32 index)
    {
        PathPoint* node = m_heap[static_cast<size_t>(index)];
        const f32 cost = node->totalCost();
        while (index > 0) {
            const i32 parentIndex = (index - 1) >> 1;
            PathPoint* parent = m_heap[static_cast<size_t>(parentIndex)];
            if (!(cost < parent->totalCost())) {
                break;
            }
            m_heap[static_cast<size_t>(index)] = parent;
            parent->setHeapIndex(index);
            index = parentIndex;
        }
        m_heap[static_cast<size_t>(index)] = node;
        node->setHeapIndex(index);
    }

    /**
     * @brief 下沉
     *
     * 先比较左右孩子（`f1 < f2` 取左，否则取右且右侧越界记为 +∞），再与当前节点的代价比较。
     */
    void _downHeap(i32 index)
    {
        PathPoint* node = m_heap[static_cast<size_t>(index)];
        const f32 cost = node->totalCost();
        while (true) {
            const i32 leftIndex = 1 + (index << 1);
            const i32 rightIndex = leftIndex + 1;
            if (leftIndex >= static_cast<i32>(m_heap.size())) {
                break;
            }

            PathPoint* left = m_heap[static_cast<size_t>(leftIndex)];
            const f32 leftCost = left->totalCost();
            PathPoint* right = nullptr;
            f32 rightCost = std::numeric_limits<f32>::infinity();
            if (rightIndex < static_cast<i32>(m_heap.size())) {
                right = m_heap[static_cast<size_t>(rightIndex)];
                rightCost = right->totalCost();
            }

            if (leftCost < rightCost) {
                if (!(leftCost < cost)) {
                    break;
                }
                m_heap[static_cast<size_t>(index)] = left;
                left->setHeapIndex(index);
                index = leftIndex;
            } else {
                if (!(rightCost < cost)) {
                    break;
                }
                m_heap[static_cast<size_t>(index)] = right;
                right->setHeapIndex(index);
                index = rightIndex;
            }
        }
        m_heap[static_cast<size_t>(index)] = node;
        node->setHeapIndex(index);
    }

    std::vector<PathPoint*> m_heap;
};

} // namespace mc::entity::ai::pathfinding