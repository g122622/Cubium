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
 * @file PathFinder.hpp
 * @brief A* 寻路器（对应原版 `PathFinder`）。
 *
 * 与教科书 A* 的三处关键差异，都是原版刻意的设计，必须照做：
 *
 *   1. **启发式放大 1.5 倍**（`FUDGING`）：`h = 最近目标距离 * 1.5`，因此搜索不保证
 *      最优路径，但会显著偏向目标方向；
 *   2. **多目标与「尽力而为」**：目标是一个集合，任何一个被判定「到达」（曼哈顿距离
 *      小于到达半径）就立刻停止；若一个都没到达，则用每个目标的 `bestNode` 回溯，
 *      取「离目标最近、其次节点最少」的那条——因此**永远**会返回一条路径（除非起点都
 *      不存在），`canReach()` 区分「真到达」与「尽量靠近」；
 *   3. **访问节点上限**：`maxVisitedNodes * multiplier`，且计数在**取出节点之前**自增，
 *      上限为 1 时一个节点都不会展开。
 *
 * 搜索距离上限是「沿搜索树的行走距离」（`walkedDistance`），不是直线距离。
 */

#pragma once

#include "NodeEvaluator.hpp"
#include "Path.hpp"
#include "PathHeap.hpp"
#include "PathPoint.hpp"
#include "Target.hpp"
#include "common/core/Types.hpp"
#include <memory>
#include <vector>

namespace mc::entity::ai::pathfinding {

/**
 * @brief A* 寻路器
 */
class PathFinder {
public:
    /// 启发式放大倍数（原版 `FUDGING`）。
    static constexpr f32 FUDGING = 1.5f;

    /// 一次搜索默认允许展开的节点数上限。
    static constexpr i32 DEFAULT_VISITED_NODES = 16 * 16;

    /**
     * @brief 构造
     * @param evaluator 求值器（接管所有权）
     * @param maxVisitedNodes 一次搜索的最大访问节点数
     */
    PathFinder(std::unique_ptr<NodeEvaluator> evaluator, i32 maxVisitedNodes)
        : m_maxVisitedNodes(maxVisitedNodes)
        , m_evaluator(std::move(evaluator))
    {}

    virtual ~PathFinder() = default;

    PathFinder(const PathFinder&) = delete;
    PathFinder& operator=(const PathFinder&) = delete;
    PathFinder(PathFinder&&) = delete;
    PathFinder& operator=(PathFinder&&) = delete;

    /**
     * @brief 求解一条路径
     *
     * @param region 世界区域（必须覆盖起点到目标的范围，且比目标多出至少一个区块）
     * @param entity 发起寻路的实体
     * @param entityPosition 实体位置
     * @param targets 目标方块坐标集合；必须非空
     * @param maxPathLength 允许的最大行走距离
     * @param reachRange 判定「到达」的曼哈顿距离半径
     * @param maxVisitedNodesMultiplier 访问节点上限倍率（蜜蜂等实体会临时调整）
     * @return 路径；连起点都无法确定时返回空 optional
     */
    [[nodiscard]] std::optional<Path> findPath(const Region& region,
        const PathfindingEntity& entity,
        const Vector3d& entityPosition,
        const std::vector<BlockPos>& targets,
        f32 maxPathLength,
        i32 reachRange,
        f32 maxVisitedNodesMultiplier);

    /**
     * @brief 直接调整访问节点上限
     */
    void setMaxVisitedNodes(i32 maxVisitedNodes)
    {
        m_maxVisitedNodes = maxVisitedNodes;
    }

    [[nodiscard]] i32 maxVisitedNodes() const
    {
        return m_maxVisitedNodes;
    }

    [[nodiscard]] NodeEvaluator& evaluator()
    {
        return *m_evaluator;
    }

    [[nodiscard]] const NodeEvaluator& evaluator() const
    {
        return *m_evaluator;
    }

private:
    /**
     * @brief 核心搜索（起点已确定）
     *
     * @param start 起点节点
     * @param targets 目标方块坐标集合（非空）
     * @param maxPathLength 允许的最大行走距离
     * @param reachRange 判定到达的曼哈顿半径
     * @param maxVisitedNodesMultiplier 访问节点上限倍率
     */
    [[nodiscard]] std::optional<Path> search(PathPoint* start,
        const std::vector<BlockPos>& targets,
        f32 maxPathLength,
        i32 reachRange,
        f32 maxVisitedNodesMultiplier);

    /**
     * @brief 从某个节点回溯出路径
     */
    [[nodiscard]] Path reconstructPath(PathPoint* from, const BlockPos& target, bool reached) const;

    /**
     * @brief 两个节点之间的移动代价（原版 `distance`，虚方法，供子类改写）
     */
    [[nodiscard]] virtual f32 distance(const PathPoint& from, const PathPoint& to) const;

    /**
     * @brief 到最近目标的启发式，并顺带刷新各目标的 `bestNode`
     *
     * 目标记录「见过的最近节点」是原版的核心记账方式：搜索结束时即使没有目标被判定
     * 到达，也能从这些记录回溯出「尽力而为」的路径。
     */
    [[nodiscard]] f32 bestHeuristic(PathPoint& point, std::vector<Target>& targets) const;

    i32 m_maxVisitedNodes;
    std::unique_ptr<NodeEvaluator> m_evaluator;
    PathHeap m_openSet;
    std::array<PathPoint*, NodeEvaluator::maxNeighbors()> m_neighbors{};

    /// 最近一次搜索实际展开的节点数（诊断 / 性能观测用）。
    i32 m_lastSearchedNodes = 0;

public:
    /// 最近一次搜索展开的节点数。
    [[nodiscard]] i32 lastSearchedNodes() const
    {
        return m_lastSearchedNodes;
    }
};

} // namespace mc::entity::ai::pathfinding