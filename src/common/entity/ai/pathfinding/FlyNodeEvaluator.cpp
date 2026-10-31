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

#include "FlyNodeEvaluator.hpp"
#include "common/util/AxisAlignedBB.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/fluid/Fluid.hpp"
#include "common/world/fluid/FluidTags.hpp"
#include <algorithm>
#include <cmath>

namespace mc::entity::ai::pathfinding {

void FlyNodeEvaluator::prepare(const Region& region, const PathfindingEntity& entity, const Vector3d& entityPosition)
{
    WalkNodeEvaluator::prepare(region, entity, entityPosition);
    m_flyPathTypeCache.clear();
}

void FlyNodeEvaluator::done()
{
    m_flyPathTypeCache.clear();
    WalkNodeEvaluator::done();
}

PathType FlyNodeEvaluator::getPathType(const PathfindingContext& context, i32 x, i32 y, i32 z) const
{
    const Region& region = context.region();
    PathType type = getPathTypeFromState(region, x, y, z);
    if (type == PathType::Open && y >= region.minBuildHeight() + 1) {
        const PathType below = getPathTypeFromState(region, x, y - 1, z);
        if (below == PathType::DamageFire || below == PathType::Lava) {
            type = PathType::DamageFire;
        } else if (below == PathType::DamageOther) {
            type = PathType::DamageOther;
        } else if (below == PathType::Cocoa) {
            type = PathType::Cocoa;
        } else if (below == PathType::Fence) {
            // 正站在栅栏顶上时不算「栅栏上方」，其余情况贴地飞行会被栅栏挡住。
            const BlockPos entityPos = context.entityBlockPos();
            if (entityPos.x != x || entityPos.y != y - 1 || entityPos.z != z) {
                type = PathType::Fence;
            }
        } else {
            type = (below != PathType::Walkable && below != PathType::Open && below != PathType::Water)
                ? PathType::Walkable
                : PathType::Open;
        }
    }
    if (type == PathType::Walkable || type == PathType::Open) {
        type = checkNeighbourBlocks(region, x, y, z, type);
    }
    return type;
}

PathType FlyNodeEvaluator::cachedPathType(i32 x, i32 y, i32 z)
{
    const u64 key = nodeKey(x, y, z);
    const auto found = m_flyPathTypeCache.find(key);
    if (found != m_flyPathTypeCache.end()) {
        return found->second;
    }
    const PathType type = getPathTypeOfMob(*context(), x, y, z);
    m_flyPathTypeCache.emplace(key, type);
    return type;
}

bool FlyNodeEvaluator::canStartAt(i32 x, i32 y, i32 z) const
{
    const PathType type = const_cast<FlyNodeEvaluator*>(this)->cachedPathType(x, y, z);
    return context()->entity().pathfindingMalus(type) >= 0.0f;
}

PathPoint* FlyNodeEvaluator::getStart()
{
    const PathfindingContext& pathContext = *context();
    const Region& region = pathContext.region();
    const Vector3d& position = pathContext.entityPosition();

    i32 y;
    if (canFloat() && pathContext.entity().pathfindingInWater()) {
        y = static_cast<i32>(std::floor(position.y));
        const i32 blockX = static_cast<i32>(std::floor(position.x));
        const i32 blockZ = static_cast<i32>(std::floor(position.z));
        while (region.getFluidState(blockX, y, blockZ) != nullptr &&
            !region.getFluidState(blockX, y, blockZ)->isEmpty() &&
            fluid::FluidTags::WATER().contains(region.getFluidState(blockX, y, blockZ)->getFluid())) {
            ++y;
        }
    } else {
        y = static_cast<i32>(std::floor(position.y + 0.5));
    }

    const i32 blockX = static_cast<i32>(std::floor(position.x));
    const i32 blockZ = static_cast<i32>(std::floor(position.z));
    if (!canStartAt(blockX, y, blockZ)) {
        // 原版用 `aabb.getSize() < 1.0` 判定「小实体」，getSize 取三个轴尺寸的**最大值**，
        // 而不是只看宽度：宽 0.6、高 1.8 的僵尸在 x 轴上很窄，但整体尺寸 1.8 属于大实体，
        // 走的是四角枚举。只看宽度会把大量正常体型错判成小实体，起点因此偏离原版。
        const AxisAlignedBB box = pathContext.entity().pathfindingBoundingBox();
        const f32 size = std::max(box.width(), std::max(box.height(), box.depth()));
        const bool small = size < SMALL_MOB_SIZE;
        if (small) {
            const f64 inflateZ =
                std::max(0.0, static_cast<f64>(SMALL_MOB_INFLATED_START_NODE_BOUNDING_BOX) - box.depth());
            const f64 inflateX =
                std::max(0.0, static_cast<f64>(SMALL_MOB_INFLATED_START_NODE_BOUNDING_BOX) - box.width());
            const f64 inflateY =
                std::max(0.0, static_cast<f64>(SMALL_MOB_INFLATED_START_NODE_BOUNDING_BOX) - box.height());
            const AxisAlignedBB inflated =
                box.expand(static_cast<f32>(inflateX), static_cast<f32>(inflateY), static_cast<f32>(inflateZ));
            const i32 minX = static_cast<i32>(std::floor(inflated.minX));
            const i32 minY = static_cast<i32>(std::floor(inflated.minY));
            const i32 minZ = static_cast<i32>(std::floor(inflated.minZ));
            const i32 maxX = static_cast<i32>(std::floor(inflated.maxX));
            const i32 maxY = static_cast<i32>(std::floor(inflated.maxY));
            const i32 maxZ = static_cast<i32>(std::floor(inflated.maxZ));
            // 原版在这里抽样 10 个随机候选。这里改为按固定顺序枚举，去掉随机性：
            // 抽样只影响「从哪一点起飞」，而候选区间的确定性枚举保证了同一输入永远得到同一结果。
            i32 tested = 0;
            for (i32 candidateY = minY; candidateY <= maxY && tested < MAX_START_NODE_CANDIDATES; ++candidateY) {
                for (i32 candidateX = minX; candidateX <= maxX && tested < MAX_START_NODE_CANDIDATES; ++candidateX) {
                    for (i32 candidateZ = minZ; candidateZ <= maxZ && tested < MAX_START_NODE_CANDIDATES;
                        ++candidateZ) {
                        ++tested;
                        if (canStartAt(candidateX, candidateY, candidateZ)) {
                            return getStartNode(candidateX, candidateY, candidateZ);
                        }
                    }
                }
            }
        } else {
            const i32 corners[4][2] = {
                {static_cast<i32>(std::floor(box.minX)), static_cast<i32>(std::floor(box.minZ))},
                {static_cast<i32>(std::floor(box.minX)), static_cast<i32>(std::floor(box.maxZ))},
                {static_cast<i32>(std::floor(box.maxX)), static_cast<i32>(std::floor(box.minZ))},
                {static_cast<i32>(std::floor(box.maxX)), static_cast<i32>(std::floor(box.maxZ))},
            };
            for (const auto& corner : corners) {
                if (canStartAt(corner[0], y, corner[1])) {
                    return getStartNode(corner[0], y, corner[1]);
                }
            }
        }
    }
    return getStartNode(blockX, y, blockZ);
}

Target FlyNodeEvaluator::getTarget(f64 x, f64 y, f64 z)
{
    // 飞行寻路不与地面「对齐到方块」：目标直接取下取整，不做地面吸附。
    return _getTargetNodeAt(x, y, z);
}

PathPoint* FlyNodeEvaluator::findFlyNode(i32 x, i32 y, i32 z)
{
    PathPoint* node = nullptr;
    const PathType type = cachedPathType(x, y, z);
    const f32 malus = context()->entity().pathfindingMalus(type);
    if (malus >= 0.0f) {
        node = getNode(x, y, z);
        node->setType(type);
        node->setCostMalus(std::max(node->costMalus(), malus));
        if (type == PathType::Walkable) {
            // 可站立的格子对飞行生物额外 +1：它们更愿意走开阔空域。
            node->setCostMalus(node->costMalus() + 1.0f);
        }
    }
    return node;
}

i32 FlyNodeEvaluator::getNeighbors(PathPoint** neighbors, PathPoint* point)
{
    i32 count = 0;
    const auto push = [&](PathPoint* node) {
        if (isOpen(node)) {
            neighbors[count++] = node;
        }
    };

    // 六个正交邻居
    const PathPoint* north = findFlyNode(point->x(), point->y(), point->z() + 1);
    push(const_cast<PathPoint*>(north));
    const PathPoint* west = findFlyNode(point->x() - 1, point->y(), point->z());
    push(const_cast<PathPoint*>(west));
    const PathPoint* east = findFlyNode(point->x() + 1, point->y(), point->z());
    push(const_cast<PathPoint*>(east));
    const PathPoint* south = findFlyNode(point->x(), point->y(), point->z() - 1);
    push(const_cast<PathPoint*>(south));
    const PathPoint* up = findFlyNode(point->x(), point->y() + 1, point->z());
    push(const_cast<PathPoint*>(up));
    const PathPoint* down = findFlyNode(point->x(), point->y() - 1, point->z());
    push(const_cast<PathPoint*>(down));

    // 斜向邻居：要求「经过的两个正交邻居」都可走，避免从墙角穿过。
    struct DiagonalEdge {
        f32 dx, dy, dz;
        const PathPoint* first;
        const PathPoint* second;
    };

    const DiagonalEdge edges[] = {
        {0, 1, 1, north, up},
        {-1, 1, 0, west, up},
        {1, 1, 0, east, up},
        {0, 1, -1, south, up},
        {0, -1, 1, north, down},
        {-1, -1, 0, west, down},
        {1, -1, 0, east, down},
        {0, -1, -1, south, down},
        {1, 0, -1, south, east},
        {1, 0, 1, north, east},
        {-1, 0, -1, south, west},
        {-1, 0, 1, north, west},
    };
    // 12 个斜向邻居各保留一份引用：角落判定要按名字查其中 6 个。
    const PathPoint* diagonalNodes[12] = {};
    for (size_t index = 0; index < 12; ++index) {
        const DiagonalEdge& edge = edges[index];
        PathPoint* node = findFlyNode(point->x() + static_cast<i32>(edge.dx),
            point->y() + static_cast<i32>(edge.dy),
            point->z() + static_cast<i32>(edge.dz));
        diagonalNodes[index] = node;
        if (isOpen(node) && hasMalus(edge.first) && hasMalus(edge.second)) {
            neighbors[count++] = node;
        }
    }
    const PathPoint* northUp = diagonalNodes[0];
    const PathPoint* westUp = diagonalNodes[1];
    const PathPoint* eastUp = diagonalNodes[2];
    const PathPoint* southUp = diagonalNodes[3];
    const PathPoint* northDown = diagonalNodes[4];
    const PathPoint* westDown = diagonalNodes[5];
    const PathPoint* eastDown = diagonalNodes[6];
    const PathPoint* southDown = diagonalNodes[7];
    const PathPoint* southEast = diagonalNodes[8];
    const PathPoint* northEast = diagonalNodes[9];
    const PathPoint* southWest = diagonalNodes[10];
    const PathPoint* northWest = diagonalNodes[11];

    // 真正的角落（三个轴同时变化）：原版要求六个邻居全部可走——三个正交邻居加上
    // 三个与该角落相邻的斜向邻居。只查三个正交邻居会让飞行实体从「墙角 + 斜向缝隙」
    // 里钻过去，路径会穿过实际堵死的空间。
    struct CornerEdge {
        f32 dx, dy, dz;
        // 原版对每个角落要求六个邻居全部可走，顺序与原版列举次序一致。
        const PathPoint* required[6];
    };

    // 每一项对应原版的一个角落（node18 ~ node25）。
    const CornerEdge corners[] = {
        // node18 = (+1, +1, -1)：node14, node3, node2, node4, node9, node8
        {1, 1, -1, {southEast, south, east, up, southUp, eastUp}},
        // node19 = (+1, +1, +1)：node15, node, node2, node4, node6, node8
        {1, 1, 1, {northEast, north, east, up, northUp, eastUp}},
        // node20 = (-1, +1, -1)：node16, node3, node1, node4, node9, node7
        {-1, 1, -1, {southWest, south, west, up, southUp, westUp}},
        // node21 = (-1, +1, +1)：node17, node, node1, node4, node6, node7
        {-1, 1, 1, {northWest, north, west, up, northUp, westUp}},
        // node22 = (+1, -1, -1)：node14, node3, node2, node5, node13, node12
        {1, -1, -1, {southEast, south, east, down, southDown, eastDown}},
        // node23 = (+1, -1, +1)：node15, node, node2, node5, node10, node12
        {1, -1, 1, {northEast, north, east, down, northDown, eastDown}},
        // node24 = (-1, -1, -1)：node16, node3, node1, node5, node13, node11
        {-1, -1, -1, {southWest, south, west, down, southDown, westDown}},
        // node25 = (-1, -1, +1)：node17, node, node1, node5, node10, node11
        {-1, -1, 1, {northWest, north, west, down, northDown, westDown}},
    };
    for (const auto& corner : corners) {
        PathPoint* node = findFlyNode(point->x() + static_cast<i32>(corner.dx),
            point->y() + static_cast<i32>(corner.dy),
            point->z() + static_cast<i32>(corner.dz));
        if (!isOpen(node)) {
            continue;
        }
        bool reachable = true;
        for (const PathPoint* required : corner.required) {
            if (!hasMalus(required)) {
                reachable = false;
                break;
            }
        }
        if (reachable) {
            neighbors[count++] = node;
        }
    }
    return count;
}

} // namespace mc::entity::ai::pathfinding
