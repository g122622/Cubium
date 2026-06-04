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

#include "PathNodeType.hpp"
#include "core/Types.hpp"
#include "world/WorldConstants.hpp"
#include <cmath>

namespace mc::entity::ai::pathfinding {

/**
 * @brief 路径点
 *
 * 表示寻路网格中的一个节点，包含位置和寻路信息。
 */
class PathPoint {
public:
    /**
     * @brief 默认构造函数
     * 用于std::vector等容器
     */
    PathPoint() = default;

    /**
     * @brief 构造函数
     * @param x X坐标
     * @param y Y坐标
     * @param z Z坐标
     */
    PathPoint(i32 x, i32 y, i32 z);

    // ========== 位置访问 ==========

    [[nodiscard]] i32 x() const { return m_x; }
    [[nodiscard]] i32 y() const { return m_y; }
    [[nodiscard]] i32 z() const { return m_z; }

    // ========== 寻路属性 ==========

    [[nodiscard]] f32 costMalus() const { return m_costMalus; }
    void setCostMalus(f32 malus) { m_costMalus = malus; }

    /**
     * @brief 获取从起点到当前节点的代价（g值）
     */
    [[nodiscard]] f32 costFromStart() const { return m_costFromStart; }
    void setCostFromStart(f32 cost)
    {
        m_costFromStart = cost;
        updateTotalCost();
    }

    /**
     * @brief 获取启发式代价（h值，到目标的估算代价）
     */
    [[nodiscard]] f32 heuristic() const { return m_heuristic; }
    void setHeuristic(f32 h)
    {
        m_heuristic = h;
        updateTotalCost();
    }

    /**
     * @brief 获取总代价（f值 = g + h）
     */
    [[nodiscard]] f32 totalCost() const { return m_totalCost; }
    void updateTotalCost() { m_totalCost = m_costFromStart + m_heuristic; }

    /**
     * @brief 获取到下一个路径点的距离
     */
    [[nodiscard]] f32 distanceToNext() const { return m_distanceToNext; }
    void setDistanceToNext(f32 distance) { m_distanceToNext = distance; }

    /**
     * @brief 获取行走距离
     * 用于追踪实际行走的距离
     */
    [[nodiscard]] f32 walkedDistance() const { return m_walkedDistance; }
    void setWalkedDistance(f32 distance) { m_walkedDistance = distance; }

    [[nodiscard]] PathNodeType nodeType() const { return m_nodeType; }
    void setNodeType(PathNodeType type) { m_nodeType = type; }

    // ========== 闭合列表 ==========

    [[nodiscard]] bool isVisited() const { return m_visited; }
    void setVisited(bool visited) { m_visited = visited; }

    /**
     * @brief 检查是否已被分配到堆中
     */
    [[nodiscard]] bool isAssigned() const { return m_heapIndex >= 0; }

    // ========== 父节点 ==========

    /**
     * @brief 获取父节点（用于重建路径）
     */
    [[nodiscard]] PathPoint* parent() { return m_parent; }
    [[nodiscard]] const PathPoint* parent() const { return m_parent; }
    void setParent(PathPoint* parent) { m_parent = parent; }

    // ========== 堆索引 ==========

    [[nodiscard]] i32 heapIndex() const { return m_heapIndex; }
    void setHeapIndex(i32 index) { m_heapIndex = index; }

    // ========== 工具方法 ==========

    /**
     * @brief 计算到另一个点的直线距离（欧几里得距离）
     */
    [[nodiscard]] f32 distanceTo(const PathPoint& other) const
    {
        f32 dx = static_cast<f32>(m_x - other.m_x);
        f32 dy = static_cast<f32>(m_y - other.m_y);
        f32 dz = static_cast<f32>(m_z - other.m_z);
        return std::sqrt(dx * dx + dy * dy + dz * dz);
    }

    /**
     * @brief 计算到另一个点的曼哈顿距离
     */
    [[nodiscard]] i32 distanceManhattan(const PathPoint& other) const
    {
        return std::abs(m_x - other.m_x) + std::abs(m_y - other.m_y) + std::abs(m_z - other.m_z);
    }

    /**
     * @brief 计算到另一个点的直线距离平方
     */
    [[nodiscard]] f32 distanceToSq(const PathPoint& other) const
    {
        f32 dx = static_cast<f32>(m_x - other.m_x);
        f32 dy = static_cast<f32>(m_y - other.m_y);
        f32 dz = static_cast<f32>(m_z - other.m_z);
        return dx * dx + dy * dy + dz * dz;
    }

    /**
     * @brief 计算到指定坐标的直线距离平方
     * 避免创建临时 PathPoint 对象
     */
    [[nodiscard]] f32 distanceToSq(i32 x, i32 y, i32 z) const
    {
        f32 dx = static_cast<f32>(m_x - x);
        f32 dy = static_cast<f32>(m_y - y);
        f32 dz = static_cast<f32>(m_z - z);
        return dx * dx + dy * dy + dz * dz;
    }

    /**
     * @brief 检查是否与另一个点位置相同
     */
    [[nodiscard]] bool equals(const PathPoint& other) const
    {
        return m_x == other.m_x && m_y == other.m_y && m_z == other.m_z;
    }

    /**
     * @brief 克隆此节点（不复制寻路状态）
     */
    [[nodiscard]] PathPoint clone() const
    {
        PathPoint copy(m_x, m_y, m_z);
        copy.m_nodeType = m_nodeType;
        copy.m_costMalus = m_costMalus;
        return copy;
    }

    /**
     * @brief 创建移动克隆
     * 创建一个新位置的克隆，保留所有寻路状态字段。
     */
    [[nodiscard]] PathPoint cloneMove(i32 newX, i32 newY, i32 newZ) const
    {
        PathPoint copy(newX, newY, newZ);
        copy.m_heapIndex = m_heapIndex;
        copy.m_costFromStart = m_costFromStart;
        copy.m_heuristic = m_heuristic;
        copy.m_distanceToNext = m_distanceToNext;
        copy.m_totalCost = m_totalCost;
        copy.m_parent = m_parent;
        copy.m_visited = m_visited;
        copy.m_walkedDistance = m_walkedDistance;
        copy.m_costMalus = m_costMalus;
        copy.m_nodeType = m_nodeType;
        return copy;
    }

    /**
     * @brief 创建一个哈希值用于缓存
     */
    [[nodiscard]] u32 hash() const
    {
        // Y 坐标掩码：假设世界高度范围在 0 到 MAX_BUILD_HEIGHT-1
        constexpr u32 Y_MASK = static_cast<u32>(world::MAX_BUILD_HEIGHT - 1);
        u32 hash = static_cast<u32>(m_y) & Y_MASK;
        hash |= static_cast<u32>(m_x & 32767) << 8;
        hash |= static_cast<u32>(m_z & 32767) << 24;
        // 处理负数的符号位
        if (m_x < 0) hash |= 0x80000000;
        if (m_z < 0) hash |= 0x00008000;
        return hash;
    }

private:
    i32 m_x;
    i32 m_y;
    i32 m_z;
    f32 m_costMalus = 0.0f;                          // 代价惩罚（来自节点类型）
    f32 m_costFromStart = 0.0f;                      // 从起点的代价（g值）
    f32 m_heuristic = 0.0f;                          // 启发式代价（h值）
    f32 m_totalCost = 0.0f;                          // 总代价（f值 = g + h）
    f32 m_walkedDistance = 0.0f;                     // 行走距离
    f32 m_distanceToNext = 0.0f;                     // 到下一个路径点的距离
    PathNodeType m_nodeType = PathNodeType::Blocked; // 默认BLOCKED
    bool m_visited = false;                          // 是否已访问（在闭合列表中）
    PathPoint* m_parent = nullptr;                   // 父节点（用于重建路径）
    i32 m_heapIndex = -1;                            // 在堆中的索引（用于优先队列）
};

} // namespace mc::entity::ai::pathfinding
