/**
 * @file DiscreteVoxelShapeTest.cpp
 * @brief DiscreteVoxelShape 单元测试
 *
 * 测试内容：
 * 1. 基本体素操作（填充、清除、查询）
 * 2. 边界查询
 * 3. Z轴线操作
 * 4. XZ矩形查询
 * 5. 盒子合并算法 (forAllBoxes)
 */

#include "common/physics/shape/DiscreteVoxelShape.hpp"
#include <algorithm>
#include <vector>
#include <gtest/gtest.h>

using namespace mc;

// ============================================================================
// 基本操作测试
// ============================================================================

class DiscreteVoxelShapeTest : public ::testing::Test {
protected:
    void SetUp() override {}

    void TearDown() override {}
};

TEST_F(DiscreteVoxelShapeTest, DefaultConstructor)
{
    DiscreteVoxelShape shape;
    EXPECT_EQ(shape.getXSize(), 0);
    EXPECT_EQ(shape.getYSize(), 0);
    EXPECT_EQ(shape.getZSize(), 0);
    EXPECT_TRUE(shape.isEmpty());
}

TEST_F(DiscreteVoxelShapeTest, SizeConstructor)
{
    DiscreteVoxelShape shape(4, 3, 2);
    EXPECT_EQ(shape.getXSize(), 4);
    EXPECT_EQ(shape.getYSize(), 3);
    EXPECT_EQ(shape.getZSize(), 2);
    EXPECT_TRUE(shape.isEmpty());
}

TEST_F(DiscreteVoxelShapeTest, FillAndIsFull)
{
    DiscreteVoxelShape shape(2, 2, 2);

    // 初始状态为空
    EXPECT_FALSE(shape.isFull(0, 0, 0));
    EXPECT_FALSE(shape.isFull(1, 1, 1));

    // 填充一个体素
    shape.fill(0, 0, 0);
    EXPECT_TRUE(shape.isFull(0, 0, 0));
    EXPECT_FALSE(shape.isFull(1, 1, 1));

    // 填充另一个体素
    shape.fill(1, 1, 1);
    EXPECT_TRUE(shape.isFull(0, 0, 0));
    EXPECT_TRUE(shape.isFull(1, 1, 1));
}

TEST_F(DiscreteVoxelShapeTest, Clear)
{
    DiscreteVoxelShape shape(2, 2, 2);

    shape.fill(0, 0, 0);
    EXPECT_TRUE(shape.isFull(0, 0, 0));

    shape.clear(0, 0, 0);
    EXPECT_FALSE(shape.isFull(0, 0, 0));
}

TEST_F(DiscreteVoxelShapeTest, IsFullWide)
{
    DiscreteVoxelShape shape(2, 2, 2);

    // 边界外的坐标返回 false
    EXPECT_FALSE(shape.isFullWide(-1, 0, 0));
    EXPECT_FALSE(shape.isFullWide(2, 0, 0));
    EXPECT_FALSE(shape.isFullWide(0, -1, 0));
    EXPECT_FALSE(shape.isFullWide(0, 2, 0));
    EXPECT_FALSE(shape.isFullWide(0, 0, -1));
    EXPECT_FALSE(shape.isFullWide(0, 0, 2));

    // 边界内的坐标正常工作
    shape.fill(0, 0, 0);
    EXPECT_TRUE(shape.isFullWide(0, 0, 0));
}

TEST_F(DiscreteVoxelShapeTest, FillAll)
{
    DiscreteVoxelShape shape(2, 2, 2);
    shape.fillAll();

    EXPECT_TRUE(shape.isFull(0, 0, 0));
    EXPECT_TRUE(shape.isFull(0, 0, 1));
    EXPECT_TRUE(shape.isFull(0, 1, 0));
    EXPECT_TRUE(shape.isFull(0, 1, 1));
    EXPECT_TRUE(shape.isFull(1, 0, 0));
    EXPECT_TRUE(shape.isFull(1, 0, 1));
    EXPECT_TRUE(shape.isFull(1, 1, 0));
    EXPECT_TRUE(shape.isFull(1, 1, 1));
    EXPECT_FALSE(shape.isEmpty());
}

TEST_F(DiscreteVoxelShapeTest, FillRange)
{
    DiscreteVoxelShape shape(4, 4, 4);
    shape.fillRange(1, 1, 1, 3, 3, 3);

    // 内部填充
    EXPECT_TRUE(shape.isFull(1, 1, 1));
    EXPECT_TRUE(shape.isFull(2, 2, 2));

    // 外部为空
    EXPECT_FALSE(shape.isFull(0, 0, 0));
    EXPECT_FALSE(shape.isFull(3, 3, 3));
}

TEST_F(DiscreteVoxelShapeTest, WithFilledBounds)
{
    auto shape = DiscreteVoxelShape::withFilledBounds(4, 4, 4, 1, 1, 1, 3, 3, 3);

    EXPECT_EQ(shape.getXSize(), 4);
    EXPECT_EQ(shape.getYSize(), 4);
    EXPECT_EQ(shape.getZSize(), 4);

    // 内部填充
    EXPECT_TRUE(shape.isFull(1, 1, 1));
    EXPECT_TRUE(shape.isFull(2, 2, 2));

    // 外部为空
    EXPECT_FALSE(shape.isFull(0, 0, 0));
    EXPECT_FALSE(shape.isFull(3, 3, 3));
}

// ============================================================================
// 边界查询测试
// ============================================================================

TEST_F(DiscreteVoxelShapeTest, FirstFullAndLastFull)
{
    DiscreteVoxelShape shape(4, 4, 4);
    shape.fill(1, 2, 3);
    shape.fill(2, 3, 1);

    EXPECT_EQ(shape.firstFull(Axis::X), 1);
    EXPECT_EQ(shape.lastFull(Axis::X), 3);
    EXPECT_EQ(shape.firstFull(Axis::Y), 2);
    EXPECT_EQ(shape.lastFull(Axis::Y), 4);
    EXPECT_EQ(shape.firstFull(Axis::Z), 1);
    EXPECT_EQ(shape.lastFull(Axis::Z), 4);
}

TEST_F(DiscreteVoxelShapeTest, FirstLastFullWithSlice)
{
    DiscreteVoxelShape shape(4, 4, 4);

    // 在 y=1 的平面上填充一些体素
    shape.fill(1, 1, 1);
    shape.fill(2, 1, 2);

    // 查询 y=1 平面上 z=1 处的 x 范围
    EXPECT_EQ(shape.firstFull(Axis::X, 1, 1), 1);
    EXPECT_EQ(shape.lastFull(Axis::X, 1, 1), 2);

    // 查询空平面
    EXPECT_EQ(shape.firstFull(Axis::X, 0, 0), 4);
    EXPECT_EQ(shape.lastFull(Axis::X, 0, 0), 0);
}

// ============================================================================
// Z轴线操作测试
// ============================================================================

TEST_F(DiscreteVoxelShapeTest, IsZAxisLineFull)
{
    DiscreteVoxelShape shape(4, 4, 4);

    // 填充一条 Z 轴线 (x=1, y=1, z=0..2)
    shape.fill(1, 1, 0);
    shape.fill(1, 1, 1);
    shape.fill(1, 1, 2);

    // 检查完整的线段
    EXPECT_TRUE(shape.isZAxisLineFull(0, 3, 1, 1));

    // 检查部分线段
    EXPECT_TRUE(shape.isZAxisLineFull(0, 2, 1, 1));
    EXPECT_TRUE(shape.isZAxisLineFull(1, 3, 1, 1));

    // 检查不完整的线段
    EXPECT_FALSE(shape.isZAxisLineFull(0, 4, 1, 1));
    EXPECT_FALSE(shape.isZAxisLineFull(2, 4, 1, 1));

    // 检查空位置
    EXPECT_FALSE(shape.isZAxisLineFull(0, 3, 0, 0));
    EXPECT_FALSE(shape.isZAxisLineFull(0, 3, 2, 2));
}

TEST_F(DiscreteVoxelShapeTest, SetZAxisLine)
{
    DiscreteVoxelShape shape(4, 4, 4);

    // 设置一条 Z 轴线
    shape.setZAxisLine(1, 3, 2, 2, true);

    EXPECT_TRUE(shape.isFull(2, 2, 1));
    EXPECT_TRUE(shape.isFull(2, 2, 2));
    EXPECT_FALSE(shape.isFull(2, 2, 0));
    EXPECT_FALSE(shape.isFull(2, 2, 3));

    // 清除线段
    shape.setZAxisLine(1, 3, 2, 2, false);

    EXPECT_FALSE(shape.isFull(2, 2, 1));
    EXPECT_FALSE(shape.isFull(2, 2, 2));
}

TEST_F(DiscreteVoxelShapeTest, IsXZRectangleFull)
{
    DiscreteVoxelShape shape(4, 4, 4);

    // 填充一个 XZ 矩形 (x=1..2, z=1..2, y=1)
    for (i32 x = 1; x <= 2; ++x) {
        for (i32 z = 1; z <= 2; ++z) {
            shape.fill(x, 1, z);
        }
    }

    // 检查完整矩形
    EXPECT_TRUE(shape.isXZRectangleFull(1, 3, 1, 3, 1));

    // 检查部分矩形
    EXPECT_TRUE(shape.isXZRectangleFull(1, 2, 1, 3, 1));
    EXPECT_TRUE(shape.isXZRectangleFull(1, 3, 1, 2, 1));

    // 检查超出范围的矩形
    EXPECT_FALSE(shape.isXZRectangleFull(0, 3, 1, 3, 1));
    EXPECT_FALSE(shape.isXZRectangleFull(1, 3, 0, 3, 1));
    EXPECT_FALSE(shape.isXZRectangleFull(1, 3, 1, 3, 0));
    EXPECT_FALSE(shape.isXZRectangleFull(1, 3, 1, 3, 2));
}

// ============================================================================
// 盒子合并测试
// ============================================================================

TEST_F(DiscreteVoxelShapeTest, ForAllBoxesEmpty)
{
    DiscreteVoxelShape shape(4, 4, 4);

    std::vector<std::tuple<i32, i32, i32, i32, i32, i32>> boxes;
    shape.forAllBoxes(
        [&boxes](i32 x1, i32 y1, i32 z1, i32 x2, i32 y2, i32 z2) { boxes.emplace_back(x1, y1, z1, x2, y2, z2); }, true);

    EXPECT_TRUE(boxes.empty());
}

TEST_F(DiscreteVoxelShapeTest, ForAllBoxesSingleVoxel)
{
    DiscreteVoxelShape shape(4, 4, 4);
    shape.fill(1, 1, 1);

    std::vector<std::tuple<i32, i32, i32, i32, i32, i32>> boxes;
    shape.forAllBoxes(
        [&boxes](i32 x1, i32 y1, i32 z1, i32 x2, i32 y2, i32 z2) { boxes.emplace_back(x1, y1, z1, x2, y2, z2); }, true);

    ASSERT_EQ(boxes.size(), 1u);
    EXPECT_EQ(std::get<0>(boxes[0]), 1);
    EXPECT_EQ(std::get<1>(boxes[0]), 1);
    EXPECT_EQ(std::get<2>(boxes[0]), 1);
    EXPECT_EQ(std::get<3>(boxes[0]), 2);
    EXPECT_EQ(std::get<4>(boxes[0]), 2);
    EXPECT_EQ(std::get<5>(boxes[0]), 2);
}

TEST_F(DiscreteVoxelShapeTest, ForAllBoxesSimpleMerge)
{
    // 创建一个 2x2x2 的立方体，应该合并成一个盒子
    DiscreteVoxelShape shape(4, 4, 4);
    shape.fillRange(1, 1, 1, 3, 3, 3);

    std::vector<std::tuple<i32, i32, i32, i32, i32, i32>> boxes;
    shape.forAllBoxes(
        [&boxes](i32 x1, i32 y1, i32 z1, i32 x2, i32 y2, i32 z2) { boxes.emplace_back(x1, y1, z1, x2, y2, z2); }, true);

    // 合并后应该只有一个盒子
    ASSERT_EQ(boxes.size(), 1u);
    EXPECT_EQ(std::get<0>(boxes[0]), 1);
    EXPECT_EQ(std::get<1>(boxes[0]), 1);
    EXPECT_EQ(std::get<2>(boxes[0]), 1);
    EXPECT_EQ(std::get<3>(boxes[0]), 3);
    EXPECT_EQ(std::get<4>(boxes[0]), 3);
    EXPECT_EQ(std::get<5>(boxes[0]), 3);
}

TEST_F(DiscreteVoxelShapeTest, ForAllBoxesNoMerge)
{
    // 不合并模式
    DiscreteVoxelShape shape(4, 4, 4);
    shape.fillRange(1, 1, 1, 3, 3, 3);

    std::vector<std::tuple<i32, i32, i32, i32, i32, i32>> boxes;
    shape.forAllBoxes(
        [&boxes](i32 x1, i32 y1, i32 z1, i32 x2, i32 y2, i32 z2) { boxes.emplace_back(x1, y1, z1, x2, y2, z2); },
        false);

    // 不合并应该有 8 个盒子 (2x2x2)
    EXPECT_EQ(boxes.size(), 8u);
}

TEST_F(DiscreteVoxelShapeTest, ForAllBoxesTwoSeparateRegions)
{
    // 创建两个分离的区域
    DiscreteVoxelShape shape(8, 4, 4);

    // 第一个区域 (0,0,0) 到 (1,1,1)
    shape.fillRange(0, 0, 0, 2, 2, 2);

    // 第二个区域 (4,0,0) 到 (5,1,1)
    shape.fillRange(4, 0, 0, 6, 2, 2);

    std::vector<std::tuple<i32, i32, i32, i32, i32, i32>> boxes;
    shape.forAllBoxes(
        [&boxes](i32 x1, i32 y1, i32 z1, i32 x2, i32 y2, i32 z2) { boxes.emplace_back(x1, y1, z1, x2, y2, z2); }, true);

    // 应该有两个盒子
    ASSERT_EQ(boxes.size(), 2u);

    // 按位置排序
    std::sort(boxes.begin(), boxes.end());

    // 第一个盒子
    EXPECT_EQ(std::get<0>(boxes[0]), 0);
    EXPECT_EQ(std::get<3>(boxes[0]), 2);

    // 第二个盒子
    EXPECT_EQ(std::get<0>(boxes[1]), 4);
    EXPECT_EQ(std::get<3>(boxes[1]), 6);
}

TEST_F(DiscreteVoxelShapeTest, ForAllBoxesZAxisMerge)
{
    // 创建一条沿 Z 轴的线
    DiscreteVoxelShape shape(4, 4, 8);
    shape.fill(1, 1, 1);
    shape.fill(1, 1, 2);
    shape.fill(1, 1, 3);
    shape.fill(1, 1, 4);

    std::vector<std::tuple<i32, i32, i32, i32, i32, i32>> boxes;
    shape.forAllBoxes(
        [&boxes](i32 x1, i32 y1, i32 z1, i32 x2, i32 y2, i32 z2) { boxes.emplace_back(x1, y1, z1, x2, y2, z2); }, true);

    // 应该合并成一个盒子
    ASSERT_EQ(boxes.size(), 1u);
    EXPECT_EQ(std::get<0>(boxes[0]), 1);
    EXPECT_EQ(std::get<1>(boxes[0]), 1);
    EXPECT_EQ(std::get<2>(boxes[0]), 1);
    EXPECT_EQ(std::get<3>(boxes[0]), 2);
    EXPECT_EQ(std::get<4>(boxes[0]), 2);
    EXPECT_EQ(std::get<5>(boxes[0]), 5);
}

TEST_F(DiscreteVoxelShapeTest, ForAllBoxesXAxisMerge)
{
    // 创建一条沿 X 轴的线
    DiscreteVoxelShape shape(8, 4, 4);
    shape.fill(1, 1, 1);
    shape.fill(2, 1, 1);
    shape.fill(3, 1, 1);
    shape.fill(4, 1, 1);

    std::vector<std::tuple<i32, i32, i32, i32, i32, i32>> boxes;
    shape.forAllBoxes(
        [&boxes](i32 x1, i32 y1, i32 z1, i32 x2, i32 y2, i32 z2) { boxes.emplace_back(x1, y1, z1, x2, y2, z2); }, true);

    // 应该合并成一个盒子
    ASSERT_EQ(boxes.size(), 1u);
    EXPECT_EQ(std::get<0>(boxes[0]), 1);
    EXPECT_EQ(std::get<1>(boxes[0]), 1);
    EXPECT_EQ(std::get<2>(boxes[0]), 1);
    EXPECT_EQ(std::get<3>(boxes[0]), 5);
    EXPECT_EQ(std::get<4>(boxes[0]), 2);
    EXPECT_EQ(std::get<5>(boxes[0]), 2);
}

TEST_F(DiscreteVoxelShapeTest, ForAllBoxesYAxisMerge)
{
    // 创建一条沿 Y 轴的线
    DiscreteVoxelShape shape(4, 8, 4);
    shape.fill(1, 1, 1);
    shape.fill(1, 2, 1);
    shape.fill(1, 3, 1);
    shape.fill(1, 4, 1);

    std::vector<std::tuple<i32, i32, i32, i32, i32, i32>> boxes;
    shape.forAllBoxes(
        [&boxes](i32 x1, i32 y1, i32 z1, i32 x2, i32 y2, i32 z2) { boxes.emplace_back(x1, y1, z1, x2, y2, z2); }, true);

    // 应该合并成一个盒子
    ASSERT_EQ(boxes.size(), 1u);
    EXPECT_EQ(std::get<0>(boxes[0]), 1);
    EXPECT_EQ(std::get<1>(boxes[0]), 1);
    EXPECT_EQ(std::get<2>(boxes[0]), 1);
    EXPECT_EQ(std::get<3>(boxes[0]), 2);
    EXPECT_EQ(std::get<4>(boxes[0]), 5);
    EXPECT_EQ(std::get<5>(boxes[0]), 2);
}

TEST_F(DiscreteVoxelShapeTest, ForAllBoxesLShapedMerge)
{
    // 创建 L 形状
    DiscreteVoxelShape shape(4, 4, 4);

    // 水平部分 (y=0, z=1)
    shape.fill(0, 0, 1);
    shape.fill(1, 0, 1);
    shape.fill(2, 0, 1);

    // 垂直部分 (x=2)
    shape.fill(2, 0, 1);
    shape.fill(2, 1, 1);
    shape.fill(2, 2, 1);

    std::vector<std::tuple<i32, i32, i32, i32, i32, i32>> boxes;
    shape.forAllBoxes(
        [&boxes](i32 x1, i32 y1, i32 z1, i32 x2, i32 y2, i32 z2) { boxes.emplace_back(x1, y1, z1, x2, y2, z2); }, true);

    // L 形应该被合并成两个盒子
    // 注：具体的合并结果取决于算法的扫描顺序
    EXPECT_GE(boxes.size(), 1u);
    EXPECT_LE(boxes.size(), 3u);

    // 验证所有盒子体积之和等于填充的体素总数
    i32 totalVolume = 0;
    for (const auto& box : boxes) {
        i32 vol = (std::get<3>(box) - std::get<0>(box)) * (std::get<4>(box) - std::get<1>(box)) *
            (std::get<5>(box) - std::get<2>(box));
        totalVolume += vol;
    }
    // 填充了 5 个体素 (水平3个 + 垂直2个额外，因为 (2,0,1) 重复)
    EXPECT_EQ(totalVolume, 5);
}

TEST_F(DiscreteVoxelShapeTest, ForAllBoxesComplexShape)
{
    // 创建一个复杂的 3D 形状
    DiscreteVoxelShape shape(8, 8, 8);

    // 底层 (y=0): 4x4 实心
    shape.fillRange(2, 0, 2, 6, 1, 6);

    // 中层 (y=1..2): 中空
    // 无填充

    // 顶层 (y=3): 4x4 实心
    shape.fillRange(2, 3, 2, 6, 4, 6);

    std::vector<std::tuple<i32, i32, i32, i32, i32, i32>> boxes;
    shape.forAllBoxes(
        [&boxes](i32 x1, i32 y1, i32 z1, i32 x2, i32 y2, i32 z2) { boxes.emplace_back(x1, y1, z1, x2, y2, z2); }, true);

    // 应该有两个盒子（底层和顶层各一个）
    ASSERT_EQ(boxes.size(), 2u);

    // 计算总体积
    i32 totalVolume = 0;
    for (const auto& box : boxes) {
        i32 vol = (std::get<3>(box) - std::get<0>(box)) * (std::get<4>(box) - std::get<1>(box)) *
            (std::get<5>(box) - std::get<2>(box));
        totalVolume += vol;
    }
    EXPECT_EQ(totalVolume, 16 + 16); // 4x4x1 + 4x4x1
}

TEST_F(DiscreteVoxelShapeTest, ForAllBoxesFullBlock)
{
    // 完整的方块
    DiscreteVoxelShape shape(1, 1, 1);
    shape.fill(0, 0, 0);

    std::vector<std::tuple<i32, i32, i32, i32, i32, i32>> boxes;
    shape.forAllBoxes(
        [&boxes](i32 x1, i32 y1, i32 z1, i32 x2, i32 y2, i32 z2) { boxes.emplace_back(x1, y1, z1, x2, y2, z2); }, true);

    ASSERT_EQ(boxes.size(), 1u);
    EXPECT_EQ(std::get<0>(boxes[0]), 0);
    EXPECT_EQ(std::get<1>(boxes[0]), 0);
    EXPECT_EQ(std::get<2>(boxes[0]), 0);
    EXPECT_EQ(std::get<3>(boxes[0]), 1);
    EXPECT_EQ(std::get<4>(boxes[0]), 1);
    EXPECT_EQ(std::get<5>(boxes[0]), 1);
}

// ============================================================================
// 面遍历测试
// ============================================================================

TEST_F(DiscreteVoxelShapeTest, ForAllFacesSingleVoxel)
{
    DiscreteVoxelShape shape(2, 2, 2);
    shape.fill(1, 1, 1);

    std::vector<std::pair<Direction, std::tuple<i32, i32, i32>>> faces;
    shape.forAllFaces(
        [&faces](Direction dir, i32 x, i32 y, i32 z) { faces.emplace_back(dir, std::make_tuple(x, y, z)); });

    // 单个体素有 6 个面
    EXPECT_EQ(faces.size(), 6u);

    // 统计各方向的面
    i32 countDown = 0, countUp = 0, countNorth = 0, countSouth = 0, countWest = 0, countEast = 0;
    for (const auto& [dir, pos] : faces) {
        switch (dir) {
            case Direction::Down:
                countDown++;
                break;
            case Direction::Up:
                countUp++;
                break;
            case Direction::North:
                countNorth++;
                break;
            case Direction::South:
                countSouth++;
                break;
            case Direction::West:
                countWest++;
                break;
            case Direction::East:
                countEast++;
                break;
            default:
                break;
        }
    }
    EXPECT_EQ(countDown, 1);
    EXPECT_EQ(countUp, 1);
    EXPECT_EQ(countNorth, 1);
    EXPECT_EQ(countSouth, 1);
    EXPECT_EQ(countWest, 1);
    EXPECT_EQ(countEast, 1);
}

TEST_F(DiscreteVoxelShapeTest, ForAllFacesAdjacentVoxels)
{
    // 两个相邻的体素，共享的面不应该出现在外表面
    DiscreteVoxelShape shape(2, 1, 1);
    shape.fillAll();

    std::vector<std::pair<Direction, std::tuple<i32, i32, i32>>> faces;
    shape.forAllFaces(
        [&faces](Direction dir, i32 x, i32 y, i32 z) { faces.emplace_back(dir, std::make_tuple(x, y, z)); });

    // 两个体素相邻，共享一个面，外表面 = 6*2 - 2 = 10
    EXPECT_EQ(faces.size(), 10u);
}

// ============================================================================
// 边遍历测试
// ============================================================================

TEST_F(DiscreteVoxelShapeTest, ForAllEdgesSingleVoxel)
{
    DiscreteVoxelShape shape(2, 2, 2);
    shape.fill(1, 1, 1);

    std::vector<std::tuple<i32, i32, i32, i32, i32, i32>> edges;
    shape.forAllEdges(
        [&edges](i32 x1, i32 y1, i32 z1, i32 x2, i32 y2, i32 z2) { edges.emplace_back(x1, y1, z1, x2, y2, z2); }, true);

    // 单个体素有 12 条边
    EXPECT_EQ(edges.size(), 12u);
}

// ============================================================================
// 拷贝和移动测试
// ============================================================================

TEST_F(DiscreteVoxelShapeTest, CopyConstructor)
{
    DiscreteVoxelShape original(4, 4, 4);
    original.fill(1, 2, 3);

    DiscreteVoxelShape copy(original);

    EXPECT_EQ(copy.getXSize(), 4);
    EXPECT_EQ(copy.getYSize(), 4);
    EXPECT_EQ(copy.getZSize(), 4);
    EXPECT_TRUE(copy.isFull(1, 2, 3));
}

TEST_F(DiscreteVoxelShapeTest, MoveConstructor)
{
    DiscreteVoxelShape original(4, 4, 4);
    original.fill(1, 2, 3);

    DiscreteVoxelShape moved(std::move(original));

    EXPECT_EQ(moved.getXSize(), 4);
    EXPECT_EQ(moved.getYSize(), 4);
    EXPECT_EQ(moved.getZSize(), 4);
    EXPECT_TRUE(moved.isFull(1, 2, 3));
    EXPECT_EQ(original.getXSize(), 0);
}

TEST_F(DiscreteVoxelShapeTest, CopyAssignment)
{
    DiscreteVoxelShape original(4, 4, 4);
    original.fill(1, 2, 3);

    DiscreteVoxelShape copy;
    copy = original;

    EXPECT_EQ(copy.getXSize(), 4);
    EXPECT_TRUE(copy.isFull(1, 2, 3));
}

TEST_F(DiscreteVoxelShapeTest, MoveAssignment)
{
    DiscreteVoxelShape original(4, 4, 4);
    original.fill(1, 2, 3);

    DiscreteVoxelShape moved;
    moved = std::move(original);

    EXPECT_EQ(moved.getXSize(), 4);
    EXPECT_TRUE(moved.isFull(1, 2, 3));
}

// ============================================================================
// AxisCycle 测试
// ============================================================================

TEST_F(DiscreteVoxelShapeTest, IsFullWithAxisCycle)
{
    DiscreteVoxelShape shape(2, 2, 2);
    shape.fill(1, 0, 0); // 填充 (1, 0, 0)

    // NONE 循环：(x, y, z) -> (x, y, z)
    EXPECT_TRUE(shape.isFull(AxisCycle::NONE, 1, 0, 0));
    EXPECT_FALSE(shape.isFull(AxisCycle::NONE, 0, 1, 0));

    // FORWARD 循环：
    // isFull(FORWARD, x', y', z') 查询的是原始坐标 (z', x', y')
    // 所以要查询原始 (1, 0, 0)，需要 (z', x', y') = (1, 0, 0)
    // 即 z' = 1, x' = 0, y' = 0
    EXPECT_TRUE(shape.isFull(AxisCycle::FORWARD, 0, 0, 1));

    // BACKWARD 循环：
    // isFull(BACKWARD, x', y', z') 查询的是原始坐标 (y', z', x')
    // 所以要查询原始 (1, 0, 0)，需要 (y', z', x') = (1, 0, 0)
    // 即 y' = 1, z' = 0, x' = 0
    EXPECT_TRUE(shape.isFull(AxisCycle::BACKWARD, 0, 1, 0));
}
