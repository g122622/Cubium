#pragma once

#include "PathPoint.hpp"
#include <limits>

namespace mc::entity::ai::pathfinding {

/**
 * @brief 带标记的路径点
 *
 * 用于多目标寻路，包装PathPoint并跟踪最佳距离和前驱。
 *
 * 参考 MC 1.16.5 FlaggedPathPoint
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
     * MC 1.16.5: field_224766_a
     */
    [[nodiscard]] f32 bestDistance() const { return m_bestDistance; }

    /**
     * @brief 获取最佳前驱
     * MC 1.16.5: field_224767_b
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
     * MC 1.16.5: updateBest
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
