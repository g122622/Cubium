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
 * @file PathPoint.hpp
 * @brief A* 搜索节点（对应原版 `Node`）。
 *
 * 一个方块坐标上的节点，携带搜索状态：代价（g/h/f）、父节点、是否已取出于开放集、
 * 本节点的路径类型与代价惩罚。节点的身份由世界坐标决定，由 `NodeEvaluator` 统一分配
 * 并在一次寻路会话内复用，因此同一格在搜索中只存在一个对象，指针可以直接当身份用。
 */

#pragma once

#include "PathType.hpp"
#include "common/core/Types.hpp"
#include <cmath>

namespace mc::entity::ai::pathfinding {

/**
 * @brief A* 搜索节点
 */
class PathPoint {
public:
    /**
     * @brief 构造一个位于原点的节点
     *
     * 供容器使用（裁剪路径、预分配缓冲区），不参与搜索。
     */
    PathPoint() noexcept = default;

    /**
     * @brief 构造一个位于给定方块坐标的节点
     * @param x, y, z 方块坐标
     */
    PathPoint(i32 x, i32 y, i32 z) noexcept
        : m_x(x)
        , m_y(y)
        , m_z(z)
    {}

    // ========== 位置 ==========

    [[nodiscard]] i32 x() const noexcept
    {
        return m_x;
    }

    [[nodiscard]] i32 y() const noexcept
    {
        return m_y;
    }

    [[nodiscard]] i32 z() const noexcept
    {
        return m_z;
    }

    // ========== 搜索状态 ==========

    /// 从起点走到本节点的累计代价（原版 `Node.g`）。
    [[nodiscard]] f32 costFromStart() const noexcept
    {
        return m_costFromStart;
    }

    void setCostFromStart(f32 cost) noexcept
    {
        m_costFromStart = cost;
    }

    /// 到最近目标的启发式估值，尚未乘 1.5（原版 `Node.h`）。
    [[nodiscard]] f32 heuristic() const noexcept
    {
        return m_heuristic;
    }

    void setHeuristic(f32 heuristic) noexcept
    {
        m_heuristic = heuristic;
    }

    /// 开放集排序键（原版 `Node.f = g + h * 1.5`）。
    [[nodiscard]] f32 totalCost() const noexcept
    {
        return m_totalCost;
    }

    void setTotalCost(f32 total) noexcept
    {
        m_totalCost = total;
    }

    /// 从起点沿搜索树走过的几何距离（原版 `Node.walkedDistance`），用于限制搜索范围。
    [[nodiscard]] f32 walkedDistance() const noexcept
    {
        return m_walkedDistance;
    }

    void setWalkedDistance(f32 distance) noexcept
    {
        m_walkedDistance = distance;
    }

    /// 本节点的路径类型（原版 `Node.type`）。
    [[nodiscard]] PathType type() const noexcept
    {
        return m_type;
    }

    void setType(PathType type) noexcept
    {
        m_type = type;
    }

    /// 本节点对该实体的代价惩罚（原版 `Node.costMalus`）。
    [[nodiscard]] f32 costMalus() const noexcept
    {
        return m_costMalus;
    }

    void setCostMalus(f32 malus) noexcept
    {
        m_costMalus = malus;
    }

    /// 是否已经从开放集中取出（原版 `Node.closed`）：取出后不再作为邻居加入。
    [[nodiscard]] bool closed() const noexcept
    {
        return m_closed;
    }

    void setClosed(bool closed) noexcept
    {
        m_closed = closed;
    }

    /// 搜索树的父节点。
    [[nodiscard]] PathPoint* cameFrom() const noexcept
    {
        return m_cameFrom;
    }

    void setCameFrom(PathPoint* parent) noexcept
    {
        m_cameFrom = parent;
    }

    /// 开放集（二叉堆）中的下标；-1 表示不在开放集内。
    [[nodiscard]] i32 heapIndex() const noexcept
    {
        return m_heapIndex;
    }

    void setHeapIndex(i32 index) noexcept
    {
        m_heapIndex = index;
    }

    /// 是否在开放集内（原版 `Node.inOpenSet()`）。
    [[nodiscard]] bool inOpenSet() const noexcept
    {
        return m_heapIndex >= 0;
    }

    // ========== 距离 ==========

    /**
     * @brief 到另一节点的欧氏距离
     *
     * 原版用 `Mth.sqrt`，即 f32 的 sqrt；这里逐字相同，避免 double 往返带来 ulp 差。
     */
    [[nodiscard]] f32 distanceTo(const PathPoint& other) const noexcept
    {
        const f32 dx = static_cast<f32>(other.m_x - m_x);
        const f32 dy = static_cast<f32>(other.m_y - m_y);
        const f32 dz = static_cast<f32>(other.m_z - m_z);
        return std::sqrt(dx * dx + dy * dy + dz * dz);
    }

    /**
     * @brief 到给定方块坐标的欧氏距离
     */
    [[nodiscard]] f32 distanceTo(i32 x, i32 y, i32 z) const noexcept
    {
        const f32 dx = static_cast<f32>(x - m_x);
        const f32 dy = static_cast<f32>(y - m_y);
        const f32 dz = static_cast<f32>(z - m_z);
        return std::sqrt(dx * dx + dy * dy + dz * dz);
    }

    /**
     * @brief 到另一节点的曼哈顿距离
     */
    [[nodiscard]] f32 distanceManhattan(const PathPoint& other) const noexcept
    {
        const f32 dx = static_cast<f32>(std::abs(other.m_x - m_x));
        const f32 dy = static_cast<f32>(std::abs(other.m_y - m_y));
        const f32 dz = static_cast<f32>(std::abs(other.m_z - m_z));
        return dx + dy + dz;
    }

    /**
     * @brief 到给定方块坐标的曼哈顿距离
     */
    [[nodiscard]] f32 distanceManhattan(i32 x, i32 y, i32 z) const noexcept
    {
        const f32 dx = static_cast<f32>(std::abs(x - m_x));
        const f32 dy = static_cast<f32>(std::abs(y - m_y));
        const f32 dz = static_cast<f32>(std::abs(z - m_z));
        return dx + dy + dz;
    }

private:
    i32 m_x = 0;
    i32 m_y = 0;
    i32 m_z = 0;
    f32 m_costFromStart = 0.0f;
    f32 m_heuristic = 0.0f;
    f32 m_totalCost = 0.0f;
    f32 m_walkedDistance = 0.0f;
    f32 m_costMalus = 0.0f;
    PathType m_type = PathType::Blocked;
    bool m_closed = false;
    PathPoint* m_cameFrom = nullptr;
    i32 m_heapIndex = -1;
};

} // namespace mc::entity::ai::pathfinding