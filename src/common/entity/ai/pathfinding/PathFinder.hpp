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

#include "../../../core/Types.hpp"
#include "../../../util/math/MathConstants.hpp"
#include "NodeProcessor.hpp"
#include "Path.hpp"
#include "PathHeap.hpp"
#include "PathPoint.hpp"
#include "Region.hpp"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <memory>
#include <utility>
#include <vector>

namespace mc::entity::ai::pathfinding {

/**
 * @brief 寻路器
 *
 * 实现 A* 算法寻找从起点到终点的最优路径。
 */
class PathFinder {
public:
    /**
     * @brief 构造函数
     * @param processor 节点处理器
     */
    explicit PathFinder(std::unique_ptr<NodeProcessor> processor)
        : m_nodeProcessor(std::move(processor))
    {}

    ~PathFinder() = default;

    // 禁止拷贝
    PathFinder(const PathFinder&) = delete;
    PathFinder& operator=(const PathFinder&) = delete;

    // 允许移动
    PathFinder(PathFinder&&) noexcept = default;
    PathFinder& operator=(PathFinder&&) noexcept = default;

    // ========== 配置 ==========

    /**
     * @brief 设置世界区域
     */
    void setRegion(const Region* region)
    {
        if (m_nodeProcessor) {
            m_nodeProcessor->setRegion(region);
        }
    }

    /**
     * @brief 设置实体尺寸
     */
    void setEntitySize(f32 width, f32 height)
    {
        if (m_nodeProcessor) {
            m_nodeProcessor->setEntitySize(width, height);
        }
    }

    /**
     * @brief 设置最大搜索距离
     */
    void setMaxSearchDistance(i32 distance) { m_maxSearchDistance = distance; }

    /**
     * @brief 设置最大搜索节点数
     */
    void setMaxNodes(i32 maxNodes) { m_maxNodes = maxNodes; }

    /**
     * @brief 设置搜索范围（用于性能优化）
     */
    void setSearchRange(i32 range) { m_searchRange = range; }

    // ========== 寻路 ==========

    /**
     * @brief 寻找到坐标的路径
     * @param startX 起点X
     * @param startY 起点Y
     * @param startZ 起点Z
     * @param targetX 终点X
     * @param targetY 终点Y
     * @param targetZ 终点Z
     * @return 找到的路径，如果找不到返回空路径
     */
    [[nodiscard]] Path findPath(i32 startX, i32 startY, i32 startZ, i32 targetX, i32 targetY, i32 targetZ)
    {
        return findPath(startX, startY, startZ, targetX, targetY, targetZ, m_maxSearchDistance);
    }

    /**
     * @brief 寻找到坐标的路径（带最大距离）
     * @param startX 起点X
     * @param startY 起点Y
     * @param startZ 起点Z
     * @param targetX 终点X
     * @param targetY 终点Y
     * @param targetZ 终点Z
     * @param maxDistance 最大搜索距离
     * @param visitedNodesMultiplier 已访问节点数倍率（默认1.0F），与 m_maxNodes 相乘得到实际搜索节点上限
     * @return 找到的路径，如果找不到返回空路径
     */
    [[nodiscard]] Path findPath(i32 startX,
        i32 startY,
        i32 startZ,
        i32 targetX,
        i32 targetY,
        i32 targetZ,
        i32 maxDistance,
        float visitedNodesMultiplier = 1.0f);

    /**
     * @brief 寻找到目标范围的路径
     * @param startX 起点X
     * @param startY 起点Y
     * @param startZ 起点Z
     * @param targetX 目标中心X
     * @param targetY 目标中心Y
     * @param targetZ 目标中心Z
     * @param range 目标范围（到达范围内任意点即成功）
     * @param visitedNodesMultiplier 已访问节点数倍率（默认1.0F）
     * @return 找到的路径
     */
    [[nodiscard]] Path findPathToRange(i32 startX,
        i32 startY,
        i32 startZ,
        i32 targetX,
        i32 targetY,
        i32 targetZ,
        i32 range,
        float visitedNodesMultiplier = 1.0f);

    // ========== 多目标寻路 ==========

    /**
     * @brief 目标点结构（用于多目标寻路）
     */
    struct TargetPoint {
        i32 x, y, z;

        TargetPoint(i32 x_, i32 y_, i32 z_)
            : x(x_)
            , y(y_)
            , z(z_)
        {}
    };

    /**
     * @brief 寻找到多个目标中最近一个的路径
     * @param startX 起点X
     * @param startY 起点Y
     * @param startZ 起点Z
     * @param targets 目标点列表
     * @param maxDistance 最大搜索距离
     * @param visitedNodesMultiplier 已访问节点数倍率（默认1.0F）
     * @return 找到的路径
     */
    [[nodiscard]] Path findPathToClosest(i32 startX,
        i32 startY,
        i32 startZ,
        const std::vector<TargetPoint>& targets,
        i32 maxDistance,
        float visitedNodesMultiplier = 1.0f);

    // ========== 调试 ==========

    /**
     * @brief 获取节点处理器
     */
    [[nodiscard]] NodeProcessor* getNodeProcessor() { return m_nodeProcessor.get(); }

    /**
     * @brief 获取最后搜索的节点数
     */
    [[nodiscard]] i32 getLastSearchedNodes() const { return m_lastSearchedNodes; }

private:
    std::unique_ptr<NodeProcessor> m_nodeProcessor;
    PathHeap m_openSet;
    i32 m_maxSearchDistance = 100;
    i32 m_maxNodes = 2000;
    i32 m_searchRange = 32;
    i32 m_lastSearchedNodes = 0;

    // ========== 内部方法 ==========

    /**
     * @brief 启发式乘数
     * 计算启发式后会乘以 1.5，存储在 distanceToNext 字段中
     */
    static constexpr f32 HEURISTIC_MULTIPLIER = 1.5f;

    /**
     * @brief 启发式函数：估算从当前点到目标的代价
     * 使用直线距离（欧几里得距离）
     *
     * 注意：此函数直接计算距离，避免创建临时对象
     */
    [[nodiscard]] static f32 _heuristic(const PathPoint& point, i32 targetX, i32 targetY, i32 targetZ)
    {
        f32 dx = static_cast<f32>(point.x() - targetX);
        f32 dy = static_cast<f32>(point.y() - targetY);
        f32 dz = static_cast<f32>(point.z() - targetZ);
        return std::sqrt(dx * dx + dy * dy + dz * dz);
    }

    /**
     * @brief 计算到目标的平方距离（用于比较，避免sqrt）
     */
    [[nodiscard]] static f32 _heuristicSq(const PathPoint& point, i32 targetX, i32 targetY, i32 targetZ)
    {
        f32 dx = static_cast<f32>(point.x() - targetX);
        f32 dy = static_cast<f32>(point.y() - targetY);
        f32 dz = static_cast<f32>(point.z() - targetZ);
        return dx * dx + dy * dy + dz * dz;
    }

    /**
     * @brief 计算两点间的移动代价
     * 考虑对角线移动和垂直移动的实际代价
     */
    [[nodiscard]] static f32 _getMovementCost(const PathPoint& from, const PathPoint& to)
    {
        i32 dx = std::abs(to.x() - from.x());
        i32 dy = std::abs(to.y() - from.y());
        i32 dz = std::abs(to.z() - from.z());

        // 移动代价：水平直线 = 1.0, 水平对角线 = sqrt(2), 垂直移动 = 1.0
        constexpr f32 VERTICAL_COST = 1.0f;

        i32 horizMoves = dx + dz;
        i32 diagonalMoves = std::min(dx, dz);
        i32 straightMoves = horizMoves - 2 * diagonalMoves;

        f32 horizCost = static_cast<f32>(straightMoves) + math::SQRT2 * static_cast<f32>(diagonalMoves);
        f32 vertCost = static_cast<f32>(dy) * VERTICAL_COST;

        return horizCost + vertCost;
    }

    /**
     * @brief 检查是否到达目标
     */
    [[nodiscard]] static bool _isTargetReached(
        const PathPoint& point, i32 targetX, i32 targetY, i32 targetZ, i32 tolerance = 0)
    {
        return std::abs(point.x() - targetX) <= tolerance && std::abs(point.y() - targetY) <= tolerance &&
            std::abs(point.z() - targetZ) <= tolerance;
    }

    /**
     * @brief 检查节点是否在搜索范围内
     */
    [[nodiscard]] bool _isInSearchRange(
        i32 x, i32 y, i32 z, i32 startX, i32 startY, i32 startZ, i32 targetX, i32 targetY, i32 targetZ) const
    {
        // 检查节点是否在起点和终点之间的范围内
        i32 minX = std::min(startX, targetX) - m_searchRange;
        i32 maxX = std::max(startX, targetX) + m_searchRange;
        i32 minY = std::min(startY, targetY) - m_searchRange / 2;
        i32 maxY = std::max(startY, targetY) + m_searchRange / 2;
        i32 minZ = std::min(startZ, targetZ) - m_searchRange;
        i32 maxZ = std::max(startZ, targetZ) + m_searchRange;

        return x >= minX && x <= maxX && y >= minY && y <= maxY && z >= minZ && z <= maxZ;
    }
};

} // namespace mc::entity::ai::pathfinding
