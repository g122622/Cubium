#pragma once

#include "../../../core/Types.hpp"
#include "PathNodeType.hpp"
#include <cmath>

namespace mc::entity::ai::pathfinding {

/**
 * @brief 路径点
 *
 * 表示寻路网格中的一个节点，包含位置和寻路信息。
 *
 * 参考 MC 1.16.5 PathPoint
 */
class PathPoint {
public:
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

    [[nodiscard]] f32 costFromStart() const { return m_costFromStart; }
    void setCostFromStart(f32 cost) { m_costFromStart = cost; }

    [[nodiscard]] f32 costToTarget() const { return m_costToTarget; }
    void setCostToTarget(f32 cost) { m_costToTarget = cost; }

    [[nodiscard]] f32 totalCost() const { return m_totalCost; }
    void updateTotalCost() { m_totalCost = m_costFromStart + m_costToTarget; }

    [[nodiscard]] PathNodeType nodeType() const { return m_nodeType; }
    void setNodeType(PathNodeType type) { m_nodeType = type; }

    // ========== 闭合列表 ==========

    [[nodiscard]] bool isVisited() const { return m_visited; }
    void setVisited(bool visited) { m_visited = visited; }

    // ========== 父节点 ==========

    [[nodiscard]] const PathPoint* parent() const { return m_parent; }
    void setParent(const PathPoint* parent) { m_parent = parent; }

    // ========== 堆索引 ==========

    [[nodiscard]] i32 heapIndex() const { return m_heapIndex; }
    void setHeapIndex(i32 index) { m_heapIndex = index; }

    // ========== 工具方法 ==========

    /**
     * @brief 计算到另一个点的曼哈顿距离
     */
    [[nodiscard]] i32 distanceTo(const PathPoint& other) const {
        return std::abs(m_x - other.m_x) + std::abs(m_y - other.m_y) + std::abs(m_z - other.m_z);
    }

    /**
     * @brief 计算到另一个点的直线距离平方
     */
    [[nodiscard]] f32 distanceToSq(const PathPoint& other) const {
        f32 dx = static_cast<f32>(m_x - other.m_x);
        f32 dy = static_cast<f32>(m_y - other.m_y);
        f32 dz = static_cast<f32>(m_z - other.m_z);
        return dx * dx + dy * dy + dz * dz;
    }

    /**
     * @brief 计算到另一个点的直线距离
     */
    [[nodiscard]] f32 distanceToLinear(const PathPoint& other) const {
        return std::sqrt(distanceToSq(other));
    }

    /**
     * @brief 检查是否与另一个点位置相同
     */
    [[nodiscard]] bool equals(const PathPoint& other) const {
        return m_x == other.m_x && m_y == other.m_y && m_z == other.m_z;
    }

    /**
     * @brief 克隆此节点（不复制寻路状态）
     */
    [[nodiscard]] PathPoint clone() const {
        PathPoint copy(m_x, m_y, m_z);
        copy.m_nodeType = m_nodeType;
        copy.m_costMalus = m_costMalus;
        return copy;
    }

    /**
     * @brief 创建一个哈希值用于缓存
     */
    [[nodiscard]] u64 hash() const {
        // Y 在高16位，X 和 Z 各占低32位
         return (static_cast<u64>(static_cast<u16>(m_y)) << 48) |
             (static_cast<u64>(static_cast<u32>(m_x)) << 16) |
             static_cast<u64>(static_cast<u32>(m_z));
    }

private:
    i32 m_x;
    i32 m_y;
    i32 m_z;
    f32 m_costMalus = 0.0f;        // 代价惩罚（来自节点类型）
    f32 m_costFromStart = 0.0f;    // 从起点的代价（g值）
    f32 m_costToTarget = 0.0f;     // 到目标的估算代价（h值）
    f32 m_totalCost = 0.0f;        // 总代价（f值 = g + h）
    PathNodeType m_nodeType = PathNodeType::Walkable;
    bool m_visited = false;        // 是否已访问（在闭合列表中）
    const PathPoint* m_parent = nullptr; // 父节点（用于重建路径）
    i32 m_heapIndex = -1;          // 在堆中的索引（用于优先队列）
};

} // namespace mc::entity::ai::pathfinding
