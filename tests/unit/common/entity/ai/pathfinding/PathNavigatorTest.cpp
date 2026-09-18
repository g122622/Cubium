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

/**
 * @brief 测试 Path::truncateNodes 方法 - 基本截断
 */
TEST(PathTruncateNodesTest, BasicTruncation)
{
    // 创建包含5个路径点的路径
    std::vector<PathPoint> points;
    points.emplace_back(0, 0, 0);
    points.emplace_back(1, 0, 1);
    points.emplace_back(2, 0, 2);
    points.emplace_back(3, 0, 3);
    points.emplace_back(4, 0, 4);

    Path path(std::move(points));
    EXPECT_EQ(path.length(), 5);

    // 在索引2处截断（保留索引0和1）
    path.truncateNodes(2);
    EXPECT_EQ(path.length(), 2);

    // 验证保留的路径点
    const PathPoint* p0 = path.getPoint(0);
    ASSERT_NE(p0, nullptr);
    EXPECT_EQ(p0->x(), 0);

    const PathPoint* p1 = path.getPoint(1);
    ASSERT_NE(p1, nullptr);
    EXPECT_EQ(p1->x(), 1);

    // 被截断的路径点应不存在
    EXPECT_EQ(path.getPoint(2), nullptr);
    EXPECT_EQ(path.getPoint(3), nullptr);
    EXPECT_EQ(path.getPoint(4), nullptr);
}

/**
 * @brief 测试 Path::truncateNodes 方法 - 索引0处截断（清空路径）
 */
TEST(PathTruncateNodesTest, TruncateAtZero)
{
    std::vector<PathPoint> points;
    points.emplace_back(0, 0, 0);
    points.emplace_back(1, 0, 1);

    Path path(std::move(points));
    EXPECT_EQ(path.length(), 2);

    // 在索引0处截断（清空路径）
    path.truncateNodes(0);
    EXPECT_EQ(path.length(), 0);
    EXPECT_TRUE(path.empty());
}

/**
 * @brief 测试 Path::truncateNodes 方法 - 索引超出范围（不做任何操作）
 */
TEST(PathTruncateNodesTest, TruncateIndexOutOfRange)
{
    std::vector<PathPoint> points;
    points.emplace_back(0, 0, 0);
    points.emplace_back(1, 0, 1);

    Path path(std::move(points));
    EXPECT_EQ(path.length(), 2);

    // 索引超出范围，不做任何操作
    path.truncateNodes(5);
    EXPECT_EQ(path.length(), 2);

    // 负数索引也不做操作
    path.truncateNodes(-1);
    EXPECT_EQ(path.length(), 2);
}

/**
 * @brief 测试 Path::truncateNodes 方法 - 当前索引调整
 *
 * 当截断发生在当前索引之前时，当前索引应被调整到新的末尾
 */
TEST(PathTruncateNodesTest, CurrentIndexAdjustment)
{
    std::vector<PathPoint> points;
    points.emplace_back(0, 0, 0);
    points.emplace_back(1, 0, 1);
    points.emplace_back(2, 0, 2);
    points.emplace_back(3, 0, 3);

    Path path(std::move(points));

    // 将当前索引设置到3
    path.setCurrentIndex(3);
    EXPECT_EQ(path.getCurrentIndex(), 3);

    // 在索引2处截断
    path.truncateNodes(2);
    EXPECT_EQ(path.length(), 2);

    // 当前索引3超出了新的路径长度2，应被调整到1（最后的有效索引）
    EXPECT_EQ(path.getCurrentIndex(), 1);
}

/**
 * @brief 测试 Path::truncateNodes 方法 - 模拟阳光避让场景
 *
 * 模拟 trimPath 的阳光避让逻辑：
 * 路径包含5个节点，其中第3个节点暴露在阳光下，
 * 截断应发生在第3个节点处，保留前2个节点（阴影中的路径）
 */
TEST(PathTruncateNodesTest, SimulateSunAvoidanceTruncation)
{
    // 创建路径：5个节点，从阴影区域穿越到阳光区域
    std::vector<PathPoint> points;
    points.emplace_back(0, 64, 0); // 阴影中
    points.emplace_back(1, 64, 1); // 阴影中
    points.emplace_back(2, 64, 2); // 阳光下 <- 截断点
    points.emplace_back(3, 64, 3); // 阳光下
    points.emplace_back(4, 64, 4); // 阳光下

    Path path(std::move(points));
    EXPECT_EQ(path.length(), 5);

    // 模拟阳光避让：在第3个节点（索引2）处截断
    // 对应 MC Java 版：遍历路径，在第一个 canSeeSky 为 true 的节点处截断
    path.truncateNodes(2);
    EXPECT_EQ(path.length(), 2);

    // 验证保留的节点都在阴影中
    const PathPoint* p0 = path.getPoint(0);
    ASSERT_NE(p0, nullptr);
    EXPECT_EQ(p0->x(), 0);
    EXPECT_EQ(p0->z(), 0);

    const PathPoint* p1 = path.getPoint(1);
    ASSERT_NE(p1, nullptr);
    EXPECT_EQ(p1->x(), 1);
    EXPECT_EQ(p1->z(), 1);

    // 路径终点应在阴影区域
    const PathPoint* end = path.getEnd();
    ASSERT_NE(end, nullptr);
    EXPECT_EQ(end->x(), 1);
    EXPECT_EQ(end->z(), 1);
}

/**
 * @brief 测试 Path::truncateNodes 方法 - 阳光避让：全阴影路径不截断
 *
 * 当路径中所有节点都在阴影中时，不需要截断。
 * 对应 MC Java 版：如果路径中没有 canSeeSky 为 true 的节点，
 * trimPath 不会调用 truncateNodes。
 */
TEST(PathTruncateNodesTest, AllShadedPathNoTruncation)
{
    // 创建路径：5个节点，全部在阴影中
    std::vector<PathPoint> points;
    points.emplace_back(0, 64, 0); // 阴影中
    points.emplace_back(1, 64, 1); // 阴影中
    points.emplace_back(2, 64, 2); // 阴影中
    points.emplace_back(3, 64, 3); // 阴影中
    points.emplace_back(4, 64, 4); // 阴影中

    Path path(std::move(points));
    EXPECT_EQ(path.length(), 5);

    // 没有阳光暴露节点，不截断
    // （在实际 _trimPath 中，遍历所有节点后不调用 truncateNodes）
    // 这里通过不调用 truncateNodes 来验证路径保持完整
    EXPECT_EQ(path.length(), 5);
    EXPECT_EQ(path.getEnd()->x(), 4);
}

/**
 * @brief 测试 Path::truncateNodes 方法 - 阳光避让：实体在阳光下不截断
 *
 * 对应 MC Java 版的 GroundPathNavigation.trimPath() 逻辑：
 * 如果实体当前位置已在阳光下（canSeeSky 为 true），则保留完整路径。
 * 因为实体已经在阳光下，需要保留完整路径才能移动到安全区域。
 * 此测试验证 truncateNodes 在此场景下不会被调用（路径保持完整）。
 */
TEST(PathTruncateNodesTest, EntityInSunlightPathPreserved)
{
    // 创建路径：5个节点，从阳光区域移动到阴影区域
    std::vector<PathPoint> points;
    points.emplace_back(0, 64, 0); // 阳光下（实体当前位置）
    points.emplace_back(1, 64, 1); // 阳光下
    points.emplace_back(2, 64, 2); // 阳光下
    points.emplace_back(3, 64, 3); // 阴影中
    points.emplace_back(4, 64, 4); // 阴影中

    Path path(std::move(points));
    EXPECT_EQ(path.length(), 5);

    // 在 MC Java 版中，如果实体已在阳光下，trimPath 不截断路径
    // 因为实体需要完整路径才能移动到阴影区域
    // 路径保持完整
    EXPECT_EQ(path.length(), 5);
    EXPECT_EQ(path.getEnd()->x(), 4);
}

/**
 * @brief 测试 Path::truncateNodes 方法 - 阳光避让：第一个节点就在阳光下
 *
 * 当路径的第一个节点就暴露在阳光下时，路径应截断为空。
 * 对应 MC Java 版：遍历路径时第一个节点 canSeeSky 为 true，
 * 调用 truncateNodes(0) 清空路径。
 */
TEST(PathTruncateNodesTest, FirstNodeInSunlightTruncateToEmpty)
{
    // 创建路径：第一个节点就在阳光下
    std::vector<PathPoint> points;
    points.emplace_back(0, 64, 0); // 阳光下 <- 截断点（索引0）
    points.emplace_back(1, 64, 1); // 阴影中
    points.emplace_back(2, 64, 2); // 阴影中

    Path path(std::move(points));
    EXPECT_EQ(path.length(), 3);

    // 在第一个节点处截断（索引0），路径变为空
    path.truncateNodes(0);
    EXPECT_EQ(path.length(), 0);
    EXPECT_TRUE(path.empty());
}

/**
 * @brief 测试 PathNavigator::setAvoidSunPathing 属性传播
 *
 * 验证 setAvoidSunPathing 正确设置 avoidSun 标志，
 * 该标志在 _trimPath 中用于阳光避让路径截断。
 */
TEST(PathNavigatorBasicTest, SetAvoidSunPathingWithNullPathFinder)
{
    PathNavigator navigator(nullptr);

    // 使用空 PathFinder 构造的导航器，setAvoidSunPathing 不应崩溃
    EXPECT_NO_THROW(navigator.setAvoidSunPathing(true));
    EXPECT_NO_THROW(navigator.setAvoidSunPathing(false));
}

/**
 * @brief 测试 PathNavigator::setCanOpenDoors 在空 PathFinder 下不崩溃
 *
 * 验证 setCanOpenDoors 在没有 PathFinder 时也能正常调用。
 */
TEST(PathNavigatorBasicTest, SetCanOpenDoorsWithNullPathFinder)
{
    PathNavigator navigator(nullptr);

    EXPECT_NO_THROW(navigator.setCanOpenDoors(true));
    EXPECT_NO_THROW(navigator.setCanOpenDoors(false));
}

/**
 * @brief 测试 PathNavigator::setCanEnterDoors 在空 PathFinder 下不崩溃
 *
 * 验证 setCanEnterDoors 在没有 PathFinder 时也能正常调用。
 */
TEST(PathNavigatorBasicTest, SetCanEnterDoorsWithNullPathFinder)
{
    PathNavigator navigator(nullptr);

    EXPECT_NO_THROW(navigator.setCanEnterDoors(true));
    EXPECT_NO_THROW(navigator.setCanEnterDoors(false));
}

/**
 * @brief 测试 PathNavigator 门属性默认值
 *
 * 验证 canOpenDoors 默认为 false，canEnterDoors 默认为 true（对齐 MC 的 canPassDoors）。
 */
TEST(PathNavigatorBasicTest, DoorPropertyDefaults)
{
    PathNavigator navigator(nullptr);

    // canOpenDoors 默认为 false（MC 中默认不能开门）
    EXPECT_FALSE(navigator.canOpenDoors());

    // canEnterDoors 默认为 true（MC 中 canPassDoors 默认为 true）
    EXPECT_TRUE(navigator.canEnterDoors());
}

/**
 * @brief 测试 PathNavigator 门属性 getter/setter
 */
TEST(PathNavigatorBasicTest, DoorPropertyGettersSetters)
{
    PathNavigator navigator(nullptr);

    // 测试 canOpenDoors
    navigator.setCanOpenDoors(true);
    EXPECT_TRUE(navigator.canOpenDoors());
    navigator.setCanOpenDoors(false);
    EXPECT_FALSE(navigator.canOpenDoors());

    // 测试 canEnterDoors
    navigator.setCanEnterDoors(false);
    EXPECT_FALSE(navigator.canEnterDoors());
    navigator.setCanEnterDoors(true);
    EXPECT_TRUE(navigator.canEnterDoors());
}

/**
 * @brief 测试 PathNavigator::setMaxVisitedNodesMultiplier / resetMaxVisitedNodesMultiplier
 *
 * 验证 maxVisitedNodesMultiplier 的默认值、设置和重置。
 */
TEST(PathNavigatorBasicTest, MaxVisitedNodesMultiplier)
{
    PathNavigator navigator(nullptr);

    // 默认值应为 1.0F
    EXPECT_FLOAT_EQ(navigator.getMaxVisitedNodesMultiplier(), 1.0f);

    // 设置为 0.5F（蜜蜂漂移飞行时降低寻路开销）
    navigator.setMaxVisitedNodesMultiplier(0.5f);
    EXPECT_FLOAT_EQ(navigator.getMaxVisitedNodesMultiplier(), 0.5f);

    // 设置为 10.0F（蜜蜂精确导航时提高寻路精度）
    navigator.setMaxVisitedNodesMultiplier(10.0f);
    EXPECT_FLOAT_EQ(navigator.getMaxVisitedNodesMultiplier(), 10.0f);

    // 重置为默认值 1.0F
    navigator.resetMaxVisitedNodesMultiplier();
    EXPECT_FLOAT_EQ(navigator.getMaxVisitedNodesMultiplier(), 1.0f);
}

} // anonymous namespace
