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
 * @file PathfindingTests.cpp
 * @brief 寻路基础数据结构的单元测试
 *
 * 覆盖 `PathType` 的代价表、`PathPoint` 的距离 / 状态语义、`PathHeap` 的二叉堆行为，
 * 以及 `Path` 的游标推进与裁剪。这些是与原版 1.21.11 一一对应的最小单元，
 * 上面的求值器与 A* 都建立在它们之上。
 */

#include "common/core/Types.hpp"
#include "common/entity/ai/pathfinding/Path.hpp"
#include "common/entity/ai/pathfinding/PathHeap.hpp"
#include "common/entity/ai/pathfinding/PathPoint.hpp"
#include "common/entity/ai/pathfinding/PathType.hpp"
#include <memory>
#include <vector>
#include <gtest/gtest.h>

using namespace mc;
using namespace mc::entity::ai::pathfinding;

// ==================== PathType ====================

TEST(PathTypeTest, DefaultMalus)
{
    // 可走 / 空着的地形没有额外代价。
    EXPECT_EQ(getPathMalus(PathType::Open), 0.0f);
    EXPECT_EQ(getPathMalus(PathType::Walkable), 0.0f);
    EXPECT_EQ(getPathMalus(PathType::WalkableDoor), 0.0f);

    // 彻底不可通行。
    EXPECT_LT(getPathMalus(PathType::Blocked), 0.0f);
    EXPECT_LT(getPathMalus(PathType::Lava), 0.0f);
    EXPECT_LT(getPathMalus(PathType::PowderSnow), 0.0f);
    EXPECT_LT(getPathMalus(PathType::Fence), 0.0f);
    EXPECT_LT(getPathMalus(PathType::DoorWoodClosed), 0.0f);
    EXPECT_LT(getPathMalus(PathType::UnpassableRail), 0.0f);
    EXPECT_LT(getPathMalus(PathType::DamageOther), 0.0f);
    EXPECT_LT(getPathMalus(PathType::Leaves), 0.0f);

    // 可通行但昂贵或会造成伤害：A* 会绕开但不会完全排除。
    EXPECT_GT(getPathMalus(PathType::Water), 0.0f);
    EXPECT_GT(getPathMalus(PathType::DangerFire), 0.0f);
    EXPECT_GT(getPathMalus(PathType::DangerOther), 0.0f);
    EXPECT_GT(getPathMalus(PathType::StickyHoney), 0.0f);
    // 站在火上比靠近火更贵。
    EXPECT_GT(getPathMalus(PathType::DamageFire), getPathMalus(PathType::DangerFire));
}

TEST(PathTypeTest, EnumOrderMatchesVanilla)
{
    // 顺序本身就是 A* 取舍的一部分（同代价时按枚举顺序比较），因此必须与原版一致。
    EXPECT_EQ(static_cast<u8>(PathType::Blocked), 0u);
    EXPECT_EQ(static_cast<u8>(PathType::Open), 1u);
    EXPECT_EQ(static_cast<u8>(PathType::Walkable), 2u);
    EXPECT_EQ(static_cast<u8>(PathType::UnpassableRail), 12u);
    EXPECT_EQ(pathTypeCount(), 26u);
}

// ==================== PathPoint ====================

TEST(PathPointTest, Construction)
{
    const PathPoint point(10, 64, -5);

    EXPECT_EQ(point.x(), 10);
    EXPECT_EQ(point.y(), 64);
    EXPECT_EQ(point.z(), -5);
    EXPECT_EQ(point.type(), PathType::Blocked);
    EXPECT_EQ(point.heapIndex(), -1);
    EXPECT_FALSE(point.inOpenSet());
    EXPECT_FALSE(point.closed());
}

TEST(PathPointTest, DistanceCalculation)
{
    const PathPoint p1(0, 0, 0);
    const PathPoint p2(3, 4, 0);

    EXPECT_FLOAT_EQ(p1.distanceTo(p2), 5.0f);
    EXPECT_FLOAT_EQ(p1.distanceTo(3, 4, 0), 5.0f);
    // 曼哈顿距离按原版口径逐分量取绝对值后相加（结果仍是 f32）。
    EXPECT_FLOAT_EQ(p1.distanceManhattan(p2), 7.0f);
}

TEST(PathPointTest, CostCalculation)
{
    PathPoint point(0, 0, 0);

    point.setCostFromStart(10.0f);
    point.setHeuristic(5.0f);
    point.setTotalCost(point.costFromStart() + point.heuristic());

    // f = g + h
    EXPECT_FLOAT_EQ(point.totalCost(), 15.0f);
}

TEST(PathPointTest, OpenSetTracking)
{
    PathPoint point(0, 0, 0);
    EXPECT_FALSE(point.inOpenSet());

    point.setHeapIndex(3);
    EXPECT_TRUE(point.inOpenSet());

    point.setHeapIndex(-1);
    EXPECT_FALSE(point.inOpenSet());
}

// ==================== PathHeap ====================

TEST(PathHeapTest, EmptyHeap)
{
    PathHeap heap;

    EXPECT_TRUE(heap.empty());
    EXPECT_EQ(heap.size(), 0u);
}

TEST(PathHeapTest, InsertAndPop)
{
    PathHeap heap;
    PathPoint p1(0, 0, 0);
    PathPoint p2(0, 0, 0);
    PathPoint p3(0, 0, 0);

    p1.setTotalCost(10.0f);
    p2.setTotalCost(5.0f);
    p3.setTotalCost(15.0f);

    heap.insert(&p1);
    heap.insert(&p2);
    heap.insert(&p3);

    EXPECT_EQ(heap.size(), 3u);
    EXPECT_EQ(heap.peek(), &p2);

    // 按代价从小到大弹出。
    EXPECT_EQ(heap.pop(), &p2);
    EXPECT_EQ(heap.pop(), &p1);
    EXPECT_EQ(heap.pop(), &p3);
    EXPECT_TRUE(heap.empty());
}

TEST(PathHeapTest, ChangeCostMovesNodeUp)
{
    PathHeap heap;
    PathPoint p1(0, 0, 0);
    PathPoint p2(0, 0, 0);

    p1.setTotalCost(10.0f);
    p2.setTotalCost(5.0f);
    heap.insert(&p1);
    heap.insert(&p2);

    // 把 p1 的代价调小后它应当浮到堆顶。
    heap.changeCost(&p1, 2.0f);
    EXPECT_EQ(heap.pop(), &p1);
}

TEST(PathHeapTest, HeapProperty)
{
    PathHeap heap;
    std::vector<std::unique_ptr<PathPoint>> points;

    for (int i = 0; i < 20; ++i) {
        points.push_back(std::make_unique<PathPoint>(i, 0, 0));
        points.back()->setTotalCost(static_cast<f32>(i * 10));
        heap.insert(points.back().get());
    }

    f32 lastCost = -1.0f;
    while (!heap.empty()) {
        const PathPoint* point = heap.pop();
        EXPECT_GE(point->totalCost(), lastCost);
        lastCost = point->totalCost();
    }
}

// ==================== Path ====================

namespace {

/// 造一条沿 +X 方向、Y=0 的直线路径。
Path makeLinePath(i32 count)
{
    std::vector<PathPoint> nodes;
    nodes.reserve(static_cast<size_t>(count));
    for (i32 index = 0; index < count; ++index) {
        nodes.emplace_back(index, 0, 0);
    }
    return Path(std::move(nodes), BlockPos(count - 1, 0, 0), true);
}

} // namespace

TEST(PathCursorTest, EmptyPath)
{
    const Path path;

    EXPECT_TRUE(path.empty());
    EXPECT_EQ(path.nodeCount(), 0u);
    EXPECT_EQ(path.endNode(), nullptr);
    EXPECT_TRUE(path.isDone());
}

TEST(PathTest, NodesAndCursor)
{
    Path path = makeLinePath(3);

    EXPECT_EQ(path.nodeCount(), 3u);
    EXPECT_FALSE(path.empty());
    EXPECT_FALSE(path.isDone());
    EXPECT_TRUE(path.notStarted());
    EXPECT_EQ(path.target(), BlockPos(2, 0, 0));
    EXPECT_TRUE(path.canReach());

    EXPECT_EQ(path.nextNode().x(), 0);
    EXPECT_EQ(path.nextNodePos(), BlockPos(0, 0, 0));

    path.advance();
    EXPECT_EQ(path.nextNode().x(), 1);

    path.advance();
    EXPECT_EQ(path.nextNode().x(), 2);

    // 越过最后一个节点后路径就算走完。
    path.advance();
    EXPECT_TRUE(path.isDone());
}

TEST(PathCursorTest, TruncateNodes)
{
    Path path = makeLinePath(4);
    EXPECT_EQ(path.endNode()->x(), 3);

    path.truncateNodes(2);
    EXPECT_EQ(path.nodeCount(), 2u);
    EXPECT_EQ(path.endNode()->x(), 1);
}

TEST(PathTest, SameAsComparesCoordinates)
{
    Path left = makeLinePath(3);
    Path right = makeLinePath(3);
    EXPECT_TRUE(left.sameAs(&right));

    right.truncateNodes(2);
    EXPECT_FALSE(left.sameAs(&right));
}

TEST(PathCursorTest, EntityPosAtNodeCentresOnBlock)
{
    const Path path = makeLinePath(2);

    // 宽度 0.6 的实体站在方块中心：中心偏移为 floor(1.6) * 0.5 = 0.5。
    const Vector3d pos = path.entityPosAtNode(0.6f, 0);
    EXPECT_FLOAT_EQ(static_cast<f32>(pos.x), 0.5f);
    EXPECT_FLOAT_EQ(static_cast<f32>(pos.z), 0.5f);
}
