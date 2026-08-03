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
#include "common/core/Types.hpp"
#include "common/entity/ai/pathfinding/Path.hpp"
#include "common/entity/ai/pathfinding/PathPoint.hpp"
#include <limits>
#include <vector>

namespace mc::entity::ai::pathfinding {

Path PathFinder::findPath(i32 startX,
    i32 startY,
    i32 startZ,
    i32 targetX,
    i32 targetY,
    i32 targetZ,
    i32 maxDistance,
    float visitedNodesMultiplier)
{
    // 清除上次搜索的缓存
    if (m_nodeProcessor) {
        m_nodeProcessor->clear();
    }
    m_openSet.clear();
    m_lastSearchedNodes = 0;

    if (!m_nodeProcessor) {
        return Path();
    }

    // 获取起始节点
    PathPoint* startNode = m_nodeProcessor->getStartNode(startX, startY, startZ);
    if (!startNode) {
        return Path();
    }

    // 检查起点是否就是终点
    if (_isTargetReached(*startNode, targetX, targetY, targetZ)) {
        Path path;
        path.addPoint(startNode->clone());
        return path;
    }

    // 设置起始节点的代价值
    // totalPathDistance = 0
    // distanceToNext = heuristic * HEURISTIC_MULTIPLIER
    // distanceToTarget = totalPathDistance + distanceToNext (即 f = g + h*1.5)
    startNode->setCostFromStart(0.0f);
    f32 h = _heuristic(*startNode, targetX, targetY, targetZ);
    startNode->setDistanceToNext(h * HEURISTIC_MULTIPLIER);
    startNode->setHeuristic(h);
    startNode->updateTotalCost();

    // 将起点加入开放列表
    m_openSet.insert(startNode);

    // 计算实际最大搜索节点数：基础值 * 倍率
    // 对应 MC Java 的 int j = (int)(this.maxVisitedNodes * maxVisitedNodesMultiplier)
    i32 effectiveMaxNodes = static_cast<i32>(static_cast<float>(m_maxNodes) * visitedNodesMultiplier);
    if (effectiveMaxNodes < 1) {
        effectiveMaxNodes = 1;
    }

    // A* 搜索
    i32 searchedNodes = 0;
    const PathPoint* bestNode = nullptr;
    f32 bestDistance = std::numeric_limits<f32>::max();

    while (!m_openSet.empty() && searchedNodes < effectiveMaxNodes) {
        // 取出代价最小的节点
        PathPoint* current = m_openSet.pop();
        if (!current) {
            break;
        }

        ++searchedNodes;
        current->setVisited(true);

        // 检查是否到达目标
        if (_isTargetReached(*current, targetX, targetY, targetZ)) {
            m_lastSearchedNodes = searchedNodes;
            return Path::buildFromEnd(current);
        }

        // 记录最近的节点（如果找不到精确路径）
        f32 currentDist = _heuristic(*current, targetX, targetY, targetZ);
        if (currentDist < bestDistance) {
            bestDistance = currentDist;
            bestNode = current;
        }

        // 检查是否超出搜索距离
        if (current->costFromStart() > static_cast<f32>(maxDistance)) {
            continue;
        }

        // 扩展相邻节点
        std::vector<PathPoint*> neighbors = m_nodeProcessor->getNeighbors(current);

        for (PathPoint* neighbor : neighbors) {
            if (neighbor->isVisited()) {
                continue;
            }

            // 计算从起点经过当前节点到邻居的代价
            f32 distance = _getMovementCost(*current, *neighbor);

            // walkedDistance 用于限制搜索范围
            f32 newWalkedDistance = current->walkedDistance() + distance;

            // totalPathDistance = previous.totalPathDistance + distance + costMalus
            f32 newCostFromStart = current->costFromStart() + distance + neighbor->costMalus();

            // 检查 walkedDistance 是否超出搜索范围
            if (newWalkedDistance > static_cast<f32>(maxDistance)) {
                continue;
            }

            // 如果找到更短的路径
            if (newCostFromStart < neighbor->costFromStart() || neighbor->heapIndex() == -1) {
                neighbor->setParent(current);
                neighbor->setWalkedDistance(newWalkedDistance);
                neighbor->setCostFromStart(newCostFromStart);
                // distanceToNext = heuristic * HEURISTIC_MULTIPLIER
                f32 neighborH = _heuristic(*neighbor, targetX, targetY, targetZ);
                neighbor->setDistanceToNext(neighborH * HEURISTIC_MULTIPLIER);
                neighbor->setHeuristic(neighborH);
                neighbor->updateTotalCost();

                if (neighbor->heapIndex() == -1) {
                    // 新节点，加入开放列表
                    m_openSet.insert(neighbor);
                } else {
                    // 已在开放列表中，更新位置
                    m_openSet.update(neighbor);
                }
            }
        }
    }

    // 没有找到精确路径，返回到最近节点的路径
    m_lastSearchedNodes = searchedNodes;

    if (bestNode && bestDistance < static_cast<f32>(maxDistance)) {
        return Path::buildFromEnd(bestNode);
    }

    return Path();
}

Path PathFinder::findPathToRange(
    i32 startX, i32 startY, i32 startZ, i32 targetX, i32 targetY, i32 targetZ, i32 range, float visitedNodesMultiplier)
{
    // 与基本寻路相同，但到达范围内任意点即成功
    if (m_nodeProcessor) {
        m_nodeProcessor->clear();
    }
    m_openSet.clear();
    m_lastSearchedNodes = 0;

    if (!m_nodeProcessor) {
        return Path();
    }

    PathPoint* startNode = m_nodeProcessor->getStartNode(startX, startY, startZ);
    if (!startNode) {
        return Path();
    }

    // 检查起点是否已经在范围内
    if (_isTargetReached(*startNode, targetX, targetY, targetZ, range)) {
        Path path;
        path.addPoint(startNode->clone());
        return path;
    }

    // 设置起始节点代价
    startNode->setCostFromStart(0.0f);
    f32 h = _heuristic(*startNode, targetX, targetY, targetZ);
    startNode->setDistanceToNext(h * HEURISTIC_MULTIPLIER);
    startNode->setHeuristic(h);
    startNode->updateTotalCost();

    m_openSet.insert(startNode);

    // 计算实际最大搜索节点数：基础值 * 倍率
    i32 effectiveMaxNodes = static_cast<i32>(static_cast<float>(m_maxNodes) * visitedNodesMultiplier);
    if (effectiveMaxNodes < 1) {
        effectiveMaxNodes = 1;
    }

    i32 searchedNodes = 0;

    while (!m_openSet.empty() && searchedNodes < effectiveMaxNodes) {
        PathPoint* current = m_openSet.pop();
        if (!current) {
            break;
        }

        ++searchedNodes;
        current->setVisited(true);

        // 检查是否在目标范围内
        if (_isTargetReached(*current, targetX, targetY, targetZ, range)) {
            m_lastSearchedNodes = searchedNodes;
            return Path::buildFromEnd(current);
        }

        if (current->costFromStart() > static_cast<f32>(m_maxSearchDistance)) {
            continue;
        }

        std::vector<PathPoint*> neighbors = m_nodeProcessor->getNeighbors(current);

        for (PathPoint* neighbor : neighbors) {
            if (neighbor->isVisited()) {
                continue;
            }

            // 计算距离
            f32 distance = _getMovementCost(*current, *neighbor);
            f32 newWalkedDistance = current->walkedDistance() + distance;

            // 检查 walkedDistance 是否超出搜索范围
            if (newWalkedDistance > static_cast<f32>(m_maxSearchDistance)) {
                continue;
            }

            f32 newCostFromStart = current->costFromStart() + distance + neighbor->costMalus();

            if (newCostFromStart < neighbor->costFromStart() || neighbor->heapIndex() == -1) {
                neighbor->setParent(current);
                neighbor->setWalkedDistance(newWalkedDistance);
                neighbor->setCostFromStart(newCostFromStart);
                f32 neighborH = _heuristic(*neighbor, targetX, targetY, targetZ);
                neighbor->setDistanceToNext(neighborH * HEURISTIC_MULTIPLIER);
                neighbor->setHeuristic(neighborH);
                neighbor->updateTotalCost();

                if (neighbor->heapIndex() == -1) {
                    m_openSet.insert(neighbor);
                } else {
                    m_openSet.update(neighbor);
                }
            }
        }
    }

    m_lastSearchedNodes = searchedNodes;
    return Path();
}

Path PathFinder::findPathToClosest(i32 startX,
    i32 startY,
    i32 startZ,
    const std::vector<TargetPoint>& targets,
    i32 maxDistance,
    float visitedNodesMultiplier)
{
    // 多目标寻路 - 对每个目标点设置标志，搜索时只需到达任意一个目标

    if (targets.empty()) {
        return Path();
    }

    // 清除上次搜索的缓存
    if (m_nodeProcessor) {
        m_nodeProcessor->clear();
    }
    m_openSet.clear();
    m_lastSearchedNodes = 0;

    if (!m_nodeProcessor) {
        return Path();
    }

    // 获取起始节点
    PathPoint* startNode = m_nodeProcessor->getStartNode(startX, startY, startZ);
    if (!startNode) {
        return Path();
    }

    // 检查起点是否就是某个目标
    for (const auto& target : targets) {
        if (_isTargetReached(*startNode, target.x, target.y, target.z)) {
            Path path;
            path.addPoint(startNode->clone());
            return path;
        }
    }

    // 计算到最近目标的启发式
    f32 bestHeuristic = std::numeric_limits<f32>::max();
    for (const auto& target : targets) {
        f32 h = _heuristic(*startNode, target.x, target.y, target.z);
        if (h < bestHeuristic) {
            bestHeuristic = h;
        }
    }

    startNode->setCostFromStart(0.0f);
    startNode->setDistanceToNext(bestHeuristic * HEURISTIC_MULTIPLIER);
    startNode->setHeuristic(bestHeuristic);
    startNode->updateTotalCost();

    m_openSet.insert(startNode);

    // 计算实际最大搜索节点数：基础值 * 倍率
    i32 effectiveMaxNodes = static_cast<i32>(static_cast<float>(m_maxNodes) * visitedNodesMultiplier);
    if (effectiveMaxNodes < 1) {
        effectiveMaxNodes = 1;
    }

    i32 searchedNodes = 0;
    const PathPoint* bestNode = nullptr;
    f32 bestDistance = std::numeric_limits<f32>::max();

    while (!m_openSet.empty() && searchedNodes < effectiveMaxNodes) {
        PathPoint* current = m_openSet.pop();
        if (!current) {
            break;
        }

        ++searchedNodes;
        current->setVisited(true);

        // 检查是否到达任意目标，同时记录最近距离（单次遍历）
        for (const auto& target : targets) {
            if (_isTargetReached(*current, target.x, target.y, target.z)) {
                m_lastSearchedNodes = searchedNodes;
                return Path::buildFromEnd(current);
            }
            f32 currentDist = _heuristic(*current, target.x, target.y, target.z);
            if (currentDist < bestDistance) {
                bestDistance = currentDist;
                bestNode = current;
            }
        }

        // 检查是否超出搜索距离
        if (current->costFromStart() > static_cast<f32>(maxDistance)) {
            continue;
        }

        // 扩展相邻节点
        std::vector<PathPoint*> neighbors = m_nodeProcessor->getNeighbors(current);

        for (PathPoint* neighbor : neighbors) {
            if (neighbor->isVisited()) {
                continue;
            }

            // 计算距离和行走距离
            f32 distance = _getMovementCost(*current, *neighbor);
            f32 newWalkedDistance = current->walkedDistance() + distance;

            // 检查 walkedDistance 是否超出搜索范围
            if (newWalkedDistance > static_cast<f32>(maxDistance)) {
                continue;
            }

            f32 newCostFromStart = current->costFromStart() + distance + neighbor->costMalus();

            // 计算到最近目标的启发式
            f32 bestNeighborHeuristic = std::numeric_limits<f32>::max();
            for (const auto& target : targets) {
                f32 h = _heuristic(*neighbor, target.x, target.y, target.z);
                if (h < bestNeighborHeuristic) {
                    bestNeighborHeuristic = h;
                }
            }

            if (newCostFromStart < neighbor->costFromStart() || neighbor->heapIndex() == -1) {
                neighbor->setParent(current);
                neighbor->setWalkedDistance(newWalkedDistance);
                neighbor->setCostFromStart(newCostFromStart);
                // distanceToNext = heuristic * HEURISTIC_MULTIPLIER
                neighbor->setDistanceToNext(bestNeighborHeuristic * HEURISTIC_MULTIPLIER);
                neighbor->setHeuristic(bestNeighborHeuristic);
                neighbor->updateTotalCost();

                if (neighbor->heapIndex() == -1) {
                    m_openSet.insert(neighbor);
                } else {
                    m_openSet.update(neighbor);
                }
            }
        }
    }

    // 没有找到精确路径，返回到最近节点的路径
    m_lastSearchedNodes = searchedNodes;

    if (bestNode && bestDistance < static_cast<f32>(maxDistance)) {
        return Path::buildFromEnd(bestNode);
    }

    return Path();
}

} // namespace mc::entity::ai::pathfinding
