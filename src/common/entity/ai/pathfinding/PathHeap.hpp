#pragma once

#include "../../../core/Types.hpp"
#include "PathPoint.hpp"
#include <vector>
#include <functional>

namespace mc::entity::ai::pathfinding {

/**
 * @brief 路径点最小堆
 *
 * 用于 A* 算法的开放列表，按总代价排序。
 *
 * 参考 MC 1.16.5 PathHeap
 */
class PathHeap {
public:
    PathHeap() = default;

    /**
     * @brief 构造函数，预分配容量
     * @param capacity 初始容量
     */
    explicit PathHeap(size_t capacity) {
        m_heap.reserve(capacity);
    }

    // ========== 堆操作 ==========

    /**
     * @brief 将路径点插入堆中
     * @param point 要插入的路径点
     */
    void insert(PathPoint* point) {
        m_heap.push_back(point);
        const size_t index = m_heap.size() - 1;
        point->setHeapIndex(static_cast<i32>(index));
        siftUp(index);
    }

    /**
     * @brief 弹出堆顶元素（最小代价）
     * @return 堆顶的路径点，如果堆为空返回nullptr
     */
    PathPoint* pop() {
        if (m_heap.empty()) {
            return nullptr;
        }

        PathPoint* result = m_heap[0];
        result->setHeapIndex(-1);

        if (m_heap.size() == 1) {
            m_heap.pop_back();
            return result;
        }

        // 将最后一个元素移到堆顶
        m_heap[0] = m_heap.back();
        m_heap[0]->setHeapIndex(0);
        m_heap.pop_back();

        // 下沉调整
        siftDown(0);

        return result;
    }

    /**
     * @brief 更新路径点的位置（代价改变后）
     *
     * MC 1.16.5: changeDistance 方法
     * 当代价改变时，需要根据代价变化方向决定上浮还是下沉：
     * - 代价减小：上浮（sortBack）
     * - 代价增大：下沉（sortForward）
     *
     * 由于 PathPoint 的代价修改是通过 setter 直接进行的，
     * 本方法采用同时尝试上浮和下沉的方式，只有实际需要的操作会生效。
     *
     * @param point 要更新的路径点
     */
    void update(PathPoint* point) {
        i32 index = point->heapIndex();
        if (index < 0 || index >= static_cast<i32>(m_heap.size())) {
            return;
        }

        // 尝试上浮（如果代价减小）
        siftUp(static_cast<size_t>(index));
        // 尝试下沉（如果代价增大）
        // 当上浮成功时，节点已经移动到正确位置，下沉不会执行任何操作
        // 当上浮没有执行任何操作时（代价增大或已经在正确位置），尝试下沉
        siftDown(static_cast<size_t>(index));
    }


    /**
     * @brief 检查堆是否为空
     */
    [[nodiscard]] bool empty() const {
        return m_heap.empty();
    }

    /**
     * @brief 获取堆中元素数量
     */
    [[nodiscard]] size_t size() const {
        return m_heap.size();
    }

    /**
     * @brief 清空堆
     */
    void clear() {
        m_heap.clear();
    }

    /**
     * @brief 获取堆顶元素（不弹出）
     */
    [[nodiscard]] PathPoint* peek() const {
        return m_heap.empty() ? nullptr : m_heap[0];
    }

    // ========== 调试方法 ==========

    /**
     * @brief 检查堆属性是否有效
     */
    [[nodiscard]] bool isValidHeap() const {
        for (size_t i = 0; i < m_heap.size(); ++i) {
            size_t left = 2 * i + 1;
            size_t right = 2 * i + 2;

            if (left < m_heap.size() && !compare(m_heap[i], m_heap[left])) {
                return false;
            }
            if (right < m_heap.size() && !compare(m_heap[i], m_heap[right])) {
                return false;
            }
        }
        return true;
    }

private:
    std::vector<PathPoint*> m_heap;

    /**
     * @brief 比较两个路径点的代价
     * @return true 如果 a 的代价小于 b
     */
    [[nodiscard]] static bool compare(const PathPoint* a, const PathPoint* b) {
        return a->totalCost() < b->totalCost();
    }

    /**
     * @brief 上浮调整
     */
    void siftUp(size_t index) {
        while (index > 0) {
            size_t parent = (index - 1) / 2;
            if (compare(m_heap[index], m_heap[parent])) {
                swap(index, parent);
                index = parent;
            } else {
                break;
            }
        }
    }

    /**
     * @brief 下沉调整
     */
    void siftDown(size_t index) {
        while (true) {
            size_t left = 2 * index + 1;
            size_t right = 2 * index + 2;
            size_t smallest = index;

            if (left < m_heap.size() && compare(m_heap[left], m_heap[smallest])) {
                smallest = left;
            }
            if (right < m_heap.size() && compare(m_heap[right], m_heap[smallest])) {
                smallest = right;
            }

            if (smallest != index) {
                swap(index, smallest);
                index = smallest;
            } else {
                break;
            }
        }
    }

    /**
     * @brief 交换两个元素
     */
    void swap(size_t i, size_t j) {
        PathPoint* temp = m_heap[i];
        m_heap[i] = m_heap[j];
        m_heap[j] = temp;

        m_heap[i]->setHeapIndex(static_cast<i32>(i));
        m_heap[j]->setHeapIndex(static_cast<i32>(j));
    }
};

} // namespace mc::entity::ai::pathfinding
