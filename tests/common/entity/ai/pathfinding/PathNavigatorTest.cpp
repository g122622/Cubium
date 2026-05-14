/**
 * @file PathNavigatorTest.cpp
 * @brief PathNavigator 和相关类的单元测试
 *
 * 主要测试 Path::setPoint 方法和 PathPoint::cloneMove 方法，
 * 以及 PathNavigator::trimPath 的基本功能。
 */

#include "common/entity/ai/pathfinding/PathNavigator.hpp"
#include "common/entity/ai/pathfinding/Path.hpp"
#include "common/entity/ai/pathfinding/PathNodeType.hpp"
#include "common/entity/ai/pathfinding/PathPoint.hpp"
#include <gtest/gtest.h>

using namespace mc;
using namespace mc::entity::ai::pathfinding;

namespace {

/**
 * @brief 测试 Path::setPoint 方法 - 基本功能
 */
TEST(PathSetPointTest, SetPointModifiesPath)
{
    // 创建路径点数组
    std::vector<PathPoint> points;
    points.emplace_back(0, 0, 0);
    points.emplace_back(1, 0, 1);
    points.emplace_back(2, 0, 2);

    // 创建路径
    Path path(std::move(points));

    // 验证初始路径点
    const PathPoint* p0 = path.getPoint(0);
    ASSERT_NE(p0, nullptr);
    EXPECT_EQ(p0->x(), 0);
    EXPECT_EQ(p0->y(), 0);
    EXPECT_EQ(p0->z(), 0);

    // 修改第一个路径点
    PathPoint newPoint(5, 5, 5);
    path.setPoint(0, newPoint);

    // 验证修改后的路径点
    const PathPoint* modified = path.getPoint(0);
    ASSERT_NE(modified, nullptr);
    EXPECT_EQ(modified->x(), 5);
    EXPECT_EQ(modified->y(), 5);
    EXPECT_EQ(modified->z(), 5);

    // 验证其他路径点未改变
    const PathPoint* p1 = path.getPoint(1);
    ASSERT_NE(p1, nullptr);
    EXPECT_EQ(p1->x(), 1);
    EXPECT_EQ(p1->y(), 0);
    EXPECT_EQ(p1->z(), 1);
}

/**
 * @brief 测试 Path::setPoint 方法 - 边界检查
 */
TEST(PathSetPointTest, SetPointIndexOutOfRangeDoesNotCrash)
{
    // 创建路径点数组
    std::vector<PathPoint> points;
    points.emplace_back(0, 0, 0);
    points.emplace_back(1, 0, 1);

    Path path(std::move(points));

    // 尝试设置超出范围的索引 - 不应该崩溃
    PathPoint newPoint(10, 10, 10);
    EXPECT_NO_THROW(path.setPoint(5, newPoint));   // 索引 5 超出范围
    EXPECT_NO_THROW(path.setPoint(100, newPoint)); // 更大的索引

    // 验证原路径点未改变
    const PathPoint* p0 = path.getPoint(0);
    ASSERT_NE(p0, nullptr);
    EXPECT_EQ(p0->x(), 0);
    EXPECT_EQ(p0->y(), 0);
    EXPECT_EQ(p0->z(), 0);
}

/**
 * @brief 测试 Path::setPoint 方法 - 移动语义
 */
TEST(PathSetPointTest, SetPointWithMoveSemantics)
{
    // 创建路径点数组
    std::vector<PathPoint> points;
    points.emplace_back(0, 0, 0);
    points.emplace_back(1, 0, 1);

    Path path(std::move(points));

    // 使用移动语义设置路径点
    PathPoint newPoint(10, 20, 30);
    path.setPoint(0, std::move(newPoint));

    // 验证修改后的路径点
    const PathPoint* modified = path.getPoint(0);
    ASSERT_NE(modified, nullptr);
    EXPECT_EQ(modified->x(), 10);
    EXPECT_EQ(modified->y(), 20);
    EXPECT_EQ(modified->z(), 30);
}

/**
 * @brief 测试 Path::setPoint 方法 - 空路径
 */
TEST(PathSetPointTest, SetPointOnEmptyPath)
{
    // 创建空路径
    Path path;

    // 尝试设置路径点 - 不应该崩溃
    PathPoint newPoint(10, 10, 10);
    EXPECT_NO_THROW(path.setPoint(0, newPoint));
}

/**
 * @brief 测试 PathPoint::cloneMove 方法 - 保留状态
 */
TEST(PathPointCloneMoveTest, CloneMovePreservesState)
{
    // 创建原始路径点
    PathPoint original(10, 20, 30);
    original.setCostMalus(2.5f);
    original.setNodeType(PathNodeType::Walkable);
    original.setCostFromStart(5.0f);
    original.setHeuristic(3.0f);

    // 克隆到新位置
    PathPoint cloned = original.cloneMove(100, 200, 300);

    // 验证新位置
    EXPECT_EQ(cloned.x(), 100);
    EXPECT_EQ(cloned.y(), 200);
    EXPECT_EQ(cloned.z(), 300);

    // 验证状态被保留
    EXPECT_EQ(cloned.costMalus(), original.costMalus());
    EXPECT_EQ(cloned.nodeType(), original.nodeType());
    EXPECT_EQ(cloned.costFromStart(), original.costFromStart());
    EXPECT_EQ(cloned.heuristic(), original.heuristic());
    EXPECT_EQ(cloned.totalCost(), original.totalCost());
}

/**
 * @brief 测试 PathPoint::cloneMove 方法 - 多次克隆
 */
TEST(PathPointCloneMoveTest, MultipleCloneMoves)
{
    // 创建原始路径点
    PathPoint original(0, 0, 0);
    original.setNodeType(PathNodeType::Water);

    // 第一次克隆
    PathPoint first = original.cloneMove(10, 5, 10);
    EXPECT_EQ(first.x(), 10);
    EXPECT_EQ(first.y(), 5);
    EXPECT_EQ(first.z(), 10);
    EXPECT_EQ(first.nodeType(), PathNodeType::Water);

    // 第二次克隆（从克隆点）
    PathPoint second = first.cloneMove(20, 10, 20);
    EXPECT_EQ(second.x(), 20);
    EXPECT_EQ(second.y(), 10);
    EXPECT_EQ(second.z(), 20);
    EXPECT_EQ(second.nodeType(), PathNodeType::Water);
}

/**
 * @brief 测试 PathPoint::cloneMove 方法 - 负坐标
 */
TEST(PathPointCloneMoveTest, CloneMoveWithNegativeCoordinates)
{
    PathPoint original(0, 0, 0);
    original.setCostMalus(1.0f);

    // 克隆到负坐标
    PathPoint cloned = original.cloneMove(-100, -50, -200);

    EXPECT_EQ(cloned.x(), -100);
    EXPECT_EQ(cloned.y(), -50);
    EXPECT_EQ(cloned.z(), -200);
    EXPECT_EQ(cloned.costMalus(), 1.0f);
}

/**
 * @brief 测试 Path::setPoint 与 trimPath 场景模拟
 *
 * 模拟 trimPath 对炼药锅的处理场景：
 * 当路径点位于炼药锅位置时，需要将其 Y 坐标 +1
 */
TEST(PathTrimPathScenarioTest, SimulateTrimPathCauldronScenario)
{
    // 创建一个穿过炼药锅的路径
    // 假设 (5, 10, 5) 是炼药锅位置
    std::vector<PathPoint> points;
    points.emplace_back(5, 9, 5);  // 炼药锅下方
    points.emplace_back(5, 10, 5); // 炼药锅位置 - 需要上移
    points.emplace_back(5, 11, 5); // 炼药锅上方

    Path path(std::move(points));

    // 模拟 trimPath 对炼药锅的处理
    // MC 1.16.5: 如果路径点在炼药锅位置，将其 Y+1
    const PathPoint* cauldronPoint = path.getPoint(1);
    ASSERT_NE(cauldronPoint, nullptr);
    EXPECT_EQ(cauldronPoint->y(), 10);

    // 使用 cloneMove 创建新的路径点
    PathPoint newPoint = cauldronPoint->cloneMove(cauldronPoint->x(),
        cauldronPoint->y() + 1, // Y+1
        cauldronPoint->z());

    // 设置新路径点
    path.setPoint(1, newPoint);

    // 验证修改后的路径点
    const PathPoint* modified = path.getPoint(1);
    ASSERT_NE(modified, nullptr);
    EXPECT_EQ(modified->x(), 5);
    EXPECT_EQ(modified->y(), 11); // Y 已上移
    EXPECT_EQ(modified->z(), 5);
}

/**
 * @brief 测试 Path::length 方法
 */
TEST(PathBasicTest, LengthMethod)
{
    // 空路径
    Path emptyPath;
    EXPECT_EQ(emptyPath.length(), 0);

    // 有路径点的路径
    std::vector<PathPoint> points;
    points.emplace_back(0, 0, 0);
    points.emplace_back(1, 0, 1);
    points.emplace_back(2, 0, 2);

    Path path(std::move(points));
    EXPECT_EQ(path.length(), 3);
}

/**
 * @brief 测试 Path::empty 方法
 */
TEST(PathBasicTest, EmptyMethod)
{
    Path emptyPath;
    EXPECT_TRUE(emptyPath.empty());

    std::vector<PathPoint> points;
    points.emplace_back(0, 0, 0);
    Path path(std::move(points));
    EXPECT_FALSE(path.empty());
}

/**
 * @brief 测试 PathNavigator 基本构造
 */
TEST(PathNavigatorBasicTest, ConstructorWithNullPathFinder)
{
    // 使用空寻路器构造
    PathNavigator navigator(nullptr);
    EXPECT_FALSE(navigator.hasPath());
    EXPECT_TRUE(navigator.noPath());
}

/**
 * @brief 测试 PathNavigator 基本状态
 */
TEST(PathNavigatorBasicTest, BasicState)
{
    PathNavigator navigator(nullptr);

    // 初始状态检查
    EXPECT_FALSE(navigator.hasPath());
    EXPECT_TRUE(navigator.noPath());
    EXPECT_TRUE(navigator.isDone());

    // 清除路径（应该不崩溃）
    EXPECT_NO_THROW(navigator.clearPath());
    EXPECT_NO_THROW(navigator.stop());
}

} // anonymous namespace
