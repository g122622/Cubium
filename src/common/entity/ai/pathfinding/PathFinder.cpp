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

#include "PathFinder.hpp"
#include "common/profiler/TraceEvents.hpp"
#include "common/util/assert/AssertAll.hpp"
#include <limits>

namespace mc::entity::ai::pathfinding {

std::optional<Path> PathFinder::findPath(const Region& region,
    const PathfindingEntity& entity,
    const Vector3d& entityPosition,
    const std::vector<BlockPos>& targets,
    f32 maxPathLength,
    i32 reachRange,
    f32 maxVisitedNodesMultiplier)
{
    MC_TRACE_SCOPED_EVENT(::mc::trace::TraceEvents.Game.Entity, "PathFinder::findPath");
    MC_ASSERT_RELEASE_MSG(!targets.empty(), "PathFinder::findPath requires at least one target");

    m_openSet.clear();
    m_neighbors.fill(nullptr);

    m_evaluator->prepare(region, entity, entityPosition);
    PathPoint* start = m_evaluator->getStart();
    if (start == nullptr) {
        m_evaluator->done();
        return std::nullopt;
    }

    auto path = search(start, targets, maxPathLength, reachRange, maxVisitedNodesMultiplier);
    // 会话到此结束：求值器内部缓存在这次搜索之后失效，因此路径必须是节点的拷贝。
    m_evaluator->done();
    return path;
}

std::optional<Path> PathFinder::search(PathPoint* start,
    const std::vector<BlockPos>& targets,
    f32 maxPathLength,
    i32 reachRange,
    f32 maxVisitedNodesMultiplier)
{
    // 目标本身也是节点：它们既用于「到达判定」，也参与启发式计算。
    std::vector<Target> targetNodes;
    targetNodes.reserve(targets.size());
    for (const auto& target : targets) {
        targetNodes.emplace_back(target.x, target.y, target.z);
    }

    start->setCostFromStart(0.0f);
    start->setHeuristic(bestHeuristic(*start, targetNodes));
    start->setTotalCost(start->heuristic());
    m_openSet.clear();
    m_openSet.insert(start);

    // 上限在取出节点之前自增：multiplier 使上限变成 0 时一个节点都不展开。
    const i32 limit = static_cast<i32>(static_cast<f32>(m_maxVisitedNodes) * maxVisitedNodesMultiplier);
    i32 searched = 0;
    std::vector<Target*> reachedTargets;

    while (!m_openSet.empty()) {
        if (++searched >= limit) {
            break;
        }

        PathPoint* node = m_openSet.pop();
        node->setClosed(true);

        for (auto& target : targetNodes) {
            if (node->distanceManhattan(target) <= static_cast<f32>(reachRange)) {
                target.setReached();
                reachedTargets.push_back(&target);
            }
        }
        if (!reachedTargets.empty()) {
            break;
        }

        // 超过行走距离上限就不再展开邻居（但节点本身已标记为已取出于开放集）。
        if (node->walkedDistance() >= maxPathLength) {
            continue;
        }

        const i32 neighborCount = m_evaluator->getNeighbors(m_neighbors.data(), node);
        for (i32 index = 0; index < neighborCount; ++index) {
            PathPoint* neighbor = m_neighbors[static_cast<size_t>(index)];
            const f32 step = distance(*node, *neighbor);
            neighbor->setWalkedDistance(node->walkedDistance() + step);
            const f32 tentative = node->costFromStart() + step + neighbor->costMalus();
            if (neighbor->walkedDistance() >= maxPathLength) {
                continue;
            }
            if (neighbor->inOpenSet() && tentative >= neighbor->costFromStart()) {
                continue;
            }
            neighbor->setCameFrom(node);
            neighbor->setCostFromStart(tentative);
            const f32 heuristic = bestHeuristic(*neighbor, targetNodes);
            neighbor->setHeuristic(heuristic);
            if (neighbor->inOpenSet()) {
                m_openSet.changeCost(neighbor, neighbor->costFromStart() + heuristic * FUDGING);
            } else {
                neighbor->setTotalCost(neighbor->costFromStart() + heuristic * FUDGING);
                m_openSet.insert(neighbor);
            }
        }
    }

    m_lastSearchedNodes = searched;

    if (!reachedTargets.empty()) {
        // 有目标被判定到达：取节点数最少的那条，并标记为「真的到达」。
        std::optional<Path> best;
        for (const Target* target : reachedTargets) {
            const BlockPos& targetPos = targets[static_cast<size_t>(target - targetNodes.data())];
            Path candidate = reconstructPath(target->bestNode(), targetPos, true);
            if (!best.has_value() || candidate.nodeCount() < best->nodeCount()) {
                best = std::move(candidate);
            }
        }
        return best;
    }

    // 一个都没到达：为每个目标回溯「最靠近」的路径，取离目标最近、其次节点最少的。
    std::optional<Path> best;
    f32 bestDistance = std::numeric_limits<f32>::max();
    size_t bestNodes = 0;
    for (size_t index = 0; index < targetNodes.size(); ++index) {
        Path candidate = reconstructPath(targetNodes[index].bestNode(), targets[index], false);
        if (!best.has_value() || candidate.distanceToTarget() < bestDistance ||
            (candidate.distanceToTarget() == bestDistance && candidate.nodeCount() < bestNodes)) {
            bestDistance = candidate.distanceToTarget();
            bestNodes = candidate.nodeCount();
            best = std::move(candidate);
        }
    }
    return best;
}

Path PathFinder::reconstructPath(PathPoint* from, const BlockPos& target, bool reached) const
{
    // 起点本身就可能没有 bestNode（目标离实体太近、根节点尚未参与启发式更新），
    // 此时路径退化为只有一个节点。
    MC_ASSERT_RELEASE_MSG(from != nullptr, "PathFinder::reconstructPath received a null node");

    std::vector<PathPoint> nodes;
    for (const PathPoint* node = from; node != nullptr; node = node->cameFrom()) {
        nodes.push_back(*node);
    }
    std::reverse(nodes.begin(), nodes.end());
    return Path(nodes, target, reached);
}

f32 PathFinder::distance(const PathPoint& from, const PathPoint& to) const
{
    return from.distanceTo(to);
}

f32 PathFinder::bestHeuristic(PathPoint& point, std::vector<Target>& targets) const
{
    f32 best = std::numeric_limits<f32>::max();
    for (auto& target : targets) {
        const f32 distance = point.distanceTo(target);
        target.updateBest(distance, &point);
        best = std::min(distance, best);
    }
    return best;
}

} // namespace mc::entity::ai::pathfinding