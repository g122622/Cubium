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

#include "PathPoint.hpp"
#include "common/core/Types.hpp"
#include <limits>

namespace mc::entity::ai::pathfinding {

/**
 * @brief 带标记的路径点
 *
 * 用于多目标寻路，包装PathPoint并跟踪最佳距离和前驱。
 */
class FlaggedPathPoint {
public:
    /**
     * @brief 默认构造函数
     */
    FlaggedPathPoint() = default;

    /**
     * @brief 从PathPoint构造
     * @param point 路径点
     */
    explicit FlaggedPathPoint(PathPoint* point)
        : m_point(point)
        , m_bestDistance(std::numeric_limits<f32>::max())
        , m_bestPredecessor(nullptr)
        , m_reached(false)
    {}

    /**
     * @brief 获取内部路径点
     */
    [[nodiscard]] PathPoint* getPoint() const { return m_point; }

    /**
     * @brief 获取最佳距离
     */
    [[nodiscard]] f32 bestDistance() const { return m_bestDistance; }

    /**
     * @brief 获取最佳前驱
     */
    [[nodiscard]] PathPoint* bestPredecessor() const { return m_bestPredecessor; }

    /**
     * @brief 检查是否已到达
     */
    [[nodiscard]] bool isReached() const { return m_reached; }

    /**
     * @brief 标记为已到达
     */
    void markReached() { m_reached = true; }

    /**
     * @brief 更新最佳距离和前驱
     *
     * 如果新距离更小，则更新。
     *
     * @param distance 新距离
     * @param predecessor 前驱节点
     * @return 如果更新了返回true
     */
    bool updateBest(f32 distance, PathPoint* predecessor)
    {
        if (distance < m_bestDistance) {
            m_bestDistance = distance;
            m_bestPredecessor = predecessor;
            return true;
        }
        return false;
    }

    /**
     * @brief 检查路径点是否为空
     */
    [[nodiscard]] bool isNull() const { return m_point == nullptr; }

    // PathPoint代理方法
    [[nodiscard]] i32 x() const { return m_point ? m_point->x() : 0; }
    [[nodiscard]] i32 y() const { return m_point ? m_point->y() : 0; }
    [[nodiscard]] i32 z() const { return m_point ? m_point->z() : 0; }

private:
    PathPoint* m_point = nullptr;
    f32 m_bestDistance = std::numeric_limits<f32>::max();
    PathPoint* m_bestPredecessor = nullptr;
    bool m_reached = false;
};

} // namespace mc::entity::ai::pathfinding
