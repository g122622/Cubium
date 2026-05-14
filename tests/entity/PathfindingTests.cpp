#include "core/Types.hpp"
#include "entity/ai/pathfinding/Path.hpp"
#include "entity/ai/pathfinding/PathHeap.hpp"
#include "entity/ai/pathfinding/PathNodeType.hpp"
#include "entity/ai/pathfinding/PathPoint.hpp"
#include <cstdlib>
#include <gtest/gtest.h>

using namespace mc;
using namespace mc::entity::ai::pathfinding;

// ==================== PathNodeType Tests ====================

TEST(PathNodeTypeTest, GetCostPenalty)
{
    // 阻塞类型代价为-1（不可通行）- MC 1.16.5: BLOCKED priority = -1.0F
    EXPECT_EQ(getPathCostPenalty(PathNodeType::Blocked), -1.0f);

    // 可行走类型无惩罚 - MC 1.16.5: WALKABLE priority = 0.0F
    EXPECT_EQ(getPathCostPenalty(PathNodeType::Walkable), 0.0f);
    EXPECT_EQ(getPathCostPenalty(PathNodeType::WalkableDoor), 0.0f);

    // 危险区域有高代价（但不禁止通行） - MC 1.16.5: DANGER_* priority = 8.0F
    EXPECT_EQ(getPathCostPenalty(PathNodeType::DangerFire), 8.0f);
    EXPECT_EQ(getPathCostPenalty(PathNodeType::DangerCactus), 8.0f);

    // 岩浆完全禁止 - MC 1.16.5: LAVA priority = -1.0F
    EXPECT_EQ(getPathCostPenalty(PathNodeType::Lava), -1.0f);

    // 伤害类型完全禁止 - MC 1.16.5: DAMAGE_CACTUS priority = -1.0F
    EXPECT_EQ(getPathCostPenalty(PathNodeType::DamageCactus), -1.0f);
}

TEST(PathNodeTypeTest, IsWalkable)
{
    EXPECT_TRUE(isWalkable(PathNodeType::Walkable));
    EXPECT_TRUE(isWalkable(PathNodeType::WalkableDoor));
    EXPECT_TRUE(isWalkable(PathNodeType::Water));
    EXPECT_TRUE(isWalkable(PathNodeType::Climbable));

    EXPECT_FALSE(isWalkable(PathNodeType::Blocked));
    EXPECT_FALSE(isWalkable(PathNodeType::Open));
    EXPECT_FALSE(isWalkable(PathNodeType::Lava));
}

// ==================== PathPoint Tests ====================

TEST(PathPointTest, Construction)
{
    PathPoint point(10, 64, -5);

    EXPECT_EQ(point.x(), 10);
    EXPECT_EQ(point.y(), 64);
    EXPECT_EQ(point.z(), -5);
    // MC 1.16.5: 默认节点类型是BLOCKED，由NodeProcessor设置实际类型
    EXPECT_EQ(point.nodeType(), PathNodeType::Blocked);
    EXPECT_EQ(point.heapIndex(), -1);
    EXPECT_FALSE(point.isVisited());
}

TEST(PathPointTest, DistanceCalculation)
{
    PathPoint p1(0, 0, 0);
    PathPoint p2(3, 4, 0);

    // 直线距离 (MC 1.16.5: distanceTo)
    EXPECT_FLOAT_EQ(p1.distanceTo(p2), 5.0f);

    // 直线距离平方
    EXPECT_FLOAT_EQ(p1.distanceToSq(p2), 25.0f);

    // 曼哈顿距离 (MC 1.16.5: func_224757_c)
    EXPECT_EQ(p1.distanceManhattan(p2), 7);
}

TEST(PathPointTest, EqualityAndHash)
{
    PathPoint p1(10, 20, 30);
    PathPoint p2(10, 20, 30);
    PathPoint p3(10, 21, 30);

    EXPECT_TRUE(p1.equals(p2));
    EXPECT_FALSE(p1.equals(p3));
    EXPECT_EQ(p1.hash(), p2.hash());
    EXPECT_NE(p1.hash(), p3.hash());
}

TEST(PathPointTest, Clone)
{
    PathPoint original(5, 10, 15);
    original.setNodeType(PathNodeType::Water);
    original.setCostMalus(0.5f);
    original.setCostFromStart(10.0f);
    original.setHeapIndex(3);

    PathPoint clone = original.clone();

    EXPECT_EQ(clone.x(), 5);
    EXPECT_EQ(clone.y(), 10);
    EXPECT_EQ(clone.z(), 15);
    EXPECT_EQ(clone.nodeType(), PathNodeType::Water);
    EXPECT_FLOAT_EQ(clone.costMalus(), 0.5f);

    // 克隆不复制寻路状态
    EXPECT_EQ(clone.heapIndex(), -1);
    EXPECT_EQ(clone.costFromStart(), 0.0f);
}

TEST(PathPointTest, CostCalculation)
{
    PathPoint point(0, 0, 0);

    point.setCostFromStart(10.0f);
    point.setHeuristic(5.0f);
    point.updateTotalCost();

    // f = g + h
    EXPECT_FLOAT_EQ(point.totalCost(), 15.0f);
}

// ==================== PathHeap Tests ====================

TEST(PathHeapTest, EmptyHeap)
{
    PathHeap heap;

    EXPECT_TRUE(heap.empty());
    EXPECT_EQ(heap.size(), 0u);
    EXPECT_EQ(heap.pop(), nullptr);
}

TEST(PathHeapTest, InsertAndPop)
{
    PathHeap heap;
    PathPoint p1(0, 0, 0);
    PathPoint p2(0, 0, 0);
    PathPoint p3(0, 0, 0);

    p1.setCostFromStart(0.0f);
    p1.setHeuristic(10.0f);
    p1.updateTotalCost();

    p2.setCostFromStart(0.0f);
    p2.setHeuristic(5.0f);
    p2.updateTotalCost();

    p3.setCostFromStart(0.0f);
    p3.setHeuristic(15.0f);
    p3.updateTotalCost();

    heap.insert(&p1);
    heap.insert(&p2);
    heap.insert(&p3);

    EXPECT_EQ(heap.size(), 3u);

    // 应该按代价从小到大弹出
    EXPECT_EQ(heap.pop(), &p2); // cost=5
    EXPECT_EQ(heap.pop(), &p1); // cost=10
    EXPECT_EQ(heap.pop(), &p3); // cost=15
    EXPECT_TRUE(heap.empty());
}

TEST(PathHeapTest, Update)
{
    PathHeap heap;
    PathPoint p1(0, 0, 0);
    PathPoint p2(0, 0, 0);

    p1.setCostFromStart(0.0f);
    p1.setHeuristic(10.0f);
    p1.updateTotalCost();

    p2.setCostFromStart(0.0f);
    p2.setHeuristic(5.0f);
    p2.updateTotalCost();

    heap.insert(&p1);
    heap.insert(&p2);

    // 更新p1的代价使其变为最小
    p1.setHeuristic(2.0f);
    p1.updateTotalCost();
    heap.update(&p1);

    // p1现在应该是最小的
    EXPECT_EQ(heap.pop(), &p1);
}

TEST(PathHeapTest, HeapProperty)
{
    PathHeap heap;

    // 创建指向堆的指针数组，避免vector重分配导致指针失效
    std::vector<std::unique_ptr<PathPoint>> points;

    // 插入有序代价的点
    for (int i = 0; i < 20; ++i) {
        points.push_back(std::make_unique<PathPoint>(i, 0, 0));
        points.back()->setCostFromStart(0.0f);
        points.back()->setHeuristic(static_cast<f32>(i * 10));
        points.back()->updateTotalCost();
        heap.insert(points.back().get());
    }

    EXPECT_TRUE(heap.isValidHeap());

    // 弹出应该按代价排序
    f32 lastCost = -1.0f;
    while (!heap.empty()) {
        PathPoint* p = heap.pop();
        EXPECT_GE(p->totalCost(), lastCost);
        lastCost = p->totalCost();
    }
}

// ==================== Path Tests ====================

TEST(PathTest, EmptyPath)
{
    Path path;

    EXPECT_TRUE(path.empty());
    EXPECT_EQ(path.length(), 0u);
    EXPECT_EQ(path.getStart(), nullptr);
    EXPECT_EQ(path.getEnd(), nullptr);
    EXPECT_TRUE(path.isFinished());
}

TEST(PathTest, AddPoints)
{
    Path path;

    path.addPoint(PathPoint(0, 0, 0));
    path.addPoint(PathPoint(1, 0, 0));
    path.addPoint(PathPoint(2, 0, 0));

    EXPECT_EQ(path.length(), 3u);
    EXPECT_FALSE(path.empty());
    EXPECT_FALSE(path.isFinished());

    // 验证起点和终点
    EXPECT_NE(path.getStart(), nullptr);
    EXPECT_EQ(path.getStart()->x(), 0);
    EXPECT_NE(path.getEnd(), nullptr);
    EXPECT_EQ(path.getEnd()->x(), 2);
}

TEST(PathTest, Navigation)
{
    Path path;
    path.addPoint(PathPoint(0, 0, 0));
    path.addPoint(PathPoint(1, 0, 0));
    path.addPoint(PathPoint(2, 0, 0));

    // 初始索引为0
    EXPECT_EQ(path.getCurrentIndex(), 0);
    EXPECT_EQ(path.getCurrentTarget()->x(), 0);

    // 前进
    EXPECT_TRUE(path.advance());
    EXPECT_EQ(path.getCurrentIndex(), 1);
    EXPECT_EQ(path.getCurrentTarget()->x(), 1);

    EXPECT_TRUE(path.advance());
    EXPECT_EQ(path.getCurrentIndex(), 2);
    EXPECT_EQ(path.getCurrentTarget()->x(), 2);

    // 到达终点后无法再前进
    EXPECT_FALSE(path.advance());
    EXPECT_TRUE(path.isFinished());

    // 重置
    path.reset();
    EXPECT_EQ(path.getCurrentIndex(), 0);
    EXPECT_FALSE(path.isFinished());
}

TEST(PathTest, BuildFromEnd)
{
    // 创建一个简单的路径点链：0 -> 1 -> 2 -> 3
    PathPoint p0(0, 0, 0);
    PathPoint p1(1, 0, 0);
    PathPoint p2(2, 0, 0);
    PathPoint p3(3, 0, 0);

    p0.setParent(nullptr);
    p1.setParent(&p0);
    p2.setParent(&p1);
    p3.setParent(&p2);

    Path path = Path::buildFromEnd(&p3);

    EXPECT_EQ(path.length(), 4u);
    EXPECT_EQ(path.getStart()->x(), 0);
    EXPECT_EQ(path.getEnd()->x(), 3);
}

TEST(PathTest, ReachesTarget)
{
    Path path;
    path.addPoint(PathPoint(0, 0, 0));
    path.addPoint(PathPoint(5, 10, 15));

    EXPECT_TRUE(path.reachesTarget(5, 10, 15, 0.0f));
    EXPECT_TRUE(path.reachesTarget(5, 10, 15, 1.0f));
    EXPECT_TRUE(path.reachesTarget(6, 10, 15, 2.0f)); // 在容差范围内
    EXPECT_FALSE(path.reachesTarget(10, 10, 15, 1.0f));
}

TEST(PathTest, TrimStart)
{
    Path path;
    path.addPoint(PathPoint(0, 0, 0));
    path.addPoint(PathPoint(1, 0, 0));
    path.addPoint(PathPoint(2, 0, 0));
    path.addPoint(PathPoint(3, 0, 0));

    path.trimStart(2);

    EXPECT_EQ(path.length(), 2u);
    EXPECT_EQ(path.getStart()->x(), 2);
    EXPECT_EQ(path.getEnd()->x(), 3);
}
