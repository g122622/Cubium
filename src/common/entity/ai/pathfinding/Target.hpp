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
 * @file Target.hpp
 * @brief A* 搜索目标（对应原版 `Target`）。
 *
 * 一个目标除了自己的坐标，还记录「到目前为止离它最近的搜索节点」和它的启发式，
 * 以及是否已被某个取出的节点判定为「已到达」。搜索结束后，若没有任何目标被判定
 * 到达，就用每个目标的 `bestNode` 回溯出一条「尽量靠近」的路径。
 *
 * 目标继承自节点是原版的建模方式（两者共享坐标与距离计算），这里保留同样的结构，
 * 因为 `PathFinder` 会把目标的坐标当作搜索节点来比较距离。
 */

#pragma once

#include "PathPoint.hpp"
#include "common/core/Types.hpp"
#include <limits>

namespace mc::entity::ai::pathfinding {

/**
 * @brief A* 搜索目标
 */
class Target : public PathPoint {
public:
    /**
     * @brief 用已有节点的坐标构造目标
     * @param node 提供坐标的节点
     */
    explicit Target(const PathPoint& node) noexcept
        : PathPoint(node.x(), node.y(), node.z())
    {}

    /**
     * @brief 用坐标构造目标
     */
    Target(i32 x, i32 y, i32 z) noexcept
        : PathPoint(x, y, z)
    {}

    /**
     * @brief 记录一个更靠近本目标的搜索节点
     * @param heuristic 该节点到本目标的距离
     * @param node 该节点
     */
    void updateBest(f32 heuristic, PathPoint* node) noexcept
    {
        if (heuristic < m_bestHeuristic) {
            m_bestHeuristic = heuristic;
            m_bestNode = node;
        }
    }

    /// 离本目标最近的搜索节点（尚无任何节点更靠近时为 nullptr）。
    [[nodiscard]] PathPoint* bestNode() const noexcept
    {
        return m_bestNode;
    }

    /// 标记本目标已被判定为到达。
    void setReached() noexcept
    {
        m_reached = true;
    }

    [[nodiscard]] bool isReached() const noexcept
    {
        return m_reached;
    }

private:
    f32 m_bestHeuristic = std::numeric_limits<f32>::max();
    PathPoint* m_bestNode = nullptr;
    bool m_reached = false;
};

} // namespace mc::entity::ai::pathfinding