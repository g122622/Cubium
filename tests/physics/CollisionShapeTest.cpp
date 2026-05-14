/**
 * @file CollisionShapeTest.cpp
 * @brief CollisionShape 单元测试
 *
 * 测试内容：
 * 1. 基本形状创建（空、完整方块、自定义盒）
 * 2. 形状查询方法
 * 3. 世界坐标转换
 * 4. 碰撞检测
 * 5. 面投影方法 getFaceShape()
 */

#include "common/physics/collision/CollisionShape.hpp"
#include "common/util/Direction.hpp"
#include <cmath>
#include <gtest/gtest.h>

using namespace mc;

// ============================================================================
// 基本形状创建测试
// ============================================================================

class CollisionShapeTest : public ::testing::Test {
protected:
    void SetUp() override {}

    void TearDown() override {}
};

TEST_F(CollisionShapeTest, EmptyShape)
{
    CollisionShape shape = CollisionShape::empty();
    EXPECT_TRUE(shape.isEmpty());
    EXPECT_FALSE(shape.isFullBlock());
    EXPECT_EQ(shape.type(), CollisionShape::Type::Empty);
    EXPECT_EQ(shape.boxCount(), 0);
}

TEST_F(CollisionShapeTest, FullBlockShape)
{
    CollisionShape shape = CollisionShape::fullBlock();
    EXPECT_FALSE(shape.isEmpty());
    EXPECT_TRUE(shape.isFullBlock());
    EXPECT_EQ(shape.type(), CollisionShape::Type::FullBlock);
    EXPECT_EQ(shape.boxCount(), 1);

    const auto& boxes = shape.boxes();
    ASSERT_EQ(boxes.size(), 1);
    EXPECT_FLOAT_EQ(boxes[0].minX, 0.0f);
    EXPECT_FLOAT_EQ(boxes[0].minY, 0.0f);
    EXPECT_FLOAT_EQ(boxes[0].minZ, 0.0f);
    EXPECT_FLOAT_EQ(boxes[0].maxX, 1.0f);
    EXPECT_FLOAT_EQ(boxes[0].maxY, 1.0f);
    EXPECT_FLOAT_EQ(boxes[0].maxZ, 1.0f);
}

TEST_F(CollisionShapeTest, BoxShape)
{
    CollisionShape shape = CollisionShape::box(0.0f, 0.0f, 0.0f, 0.5f, 1.0f, 0.5f);
    EXPECT_FALSE(shape.isEmpty());
    EXPECT_FALSE(shape.isFullBlock());
    EXPECT_EQ(shape.type(), CollisionShape::Type::SimpleBox);
    EXPECT_EQ(shape.boxCount(), 1);

    const auto& boxes = shape.boxes();
    ASSERT_EQ(boxes.size(), 1);
    EXPECT_FLOAT_EQ(boxes[0].minX, 0.0f);
    EXPECT_FLOAT_EQ(boxes[0].maxX, 0.5f);
}

TEST_F(CollisionShapeTest, FromPixelBox)
{
    // 16 像素 = 1 方块
    CollisionShape shape = CollisionShape::fromPixelBox(0, 0, 0, 16, 8, 16);
    EXPECT_EQ(shape.boxCount(), 1);

    const auto& boxes = shape.boxes();
    EXPECT_FLOAT_EQ(boxes[0].minX, 0.0f);
    EXPECT_FLOAT_EQ(boxes[0].maxX, 1.0f);
    EXPECT_FLOAT_EQ(boxes[0].minY, 0.0f);
    EXPECT_FLOAT_EQ(boxes[0].maxY, 0.5f); // 8 像素 = 0.5 方块
    EXPECT_FLOAT_EQ(boxes[0].minZ, 0.0f);
    EXPECT_FLOAT_EQ(boxes[0].maxZ, 1.0f);
}

TEST_F(CollisionShapeTest, AddBox)
{
    CollisionShape shape = CollisionShape::box(0.0f, 0.0f, 0.0f, 1.0f, 0.5f, 1.0f);
    EXPECT_EQ(shape.boxCount(), 1);

    shape.addBox(0.0f, 0.5f, 0.0f, 1.0f, 1.0f, 0.5f);
    EXPECT_EQ(shape.boxCount(), 2);
    EXPECT_EQ(shape.type(), CollisionShape::Type::SimpleBox);
}

// ============================================================================
// 世界坐标转换测试
// ============================================================================

TEST_F(CollisionShapeTest, GetWorldBoxes)
{
    CollisionShape shape = CollisionShape::fullBlock();
    auto worldBoxes = shape.getWorldBoxes(10, 64, 20);

    ASSERT_EQ(worldBoxes.size(), 1);
    EXPECT_FLOAT_EQ(worldBoxes[0].minX, 10.0f);
    EXPECT_FLOAT_EQ(worldBoxes[0].minY, 64.0f);
    EXPECT_FLOAT_EQ(worldBoxes[0].minZ, 20.0f);
    EXPECT_FLOAT_EQ(worldBoxes[0].maxX, 11.0f);
    EXPECT_FLOAT_EQ(worldBoxes[0].maxY, 65.0f);
    EXPECT_FLOAT_EQ(worldBoxes[0].maxZ, 21.0f);
}

TEST_F(CollisionShapeTest, GetWorldBoxesMultiBox)
{
    CollisionShape shape = CollisionShape::box(0.0f, 0.0f, 0.0f, 1.0f, 0.5f, 1.0f);
    shape.addBox(0.0f, 0.5f, 0.0f, 1.0f, 1.0f, 0.5f);

    auto worldBoxes = shape.getWorldBoxes(0, 0, 0);
    ASSERT_EQ(worldBoxes.size(), 2);
}

// ============================================================================
// 碰撞检测测试
// ============================================================================

TEST_F(CollisionShapeTest, IntersectsFullBlock)
{
    CollisionShape shape = CollisionShape::fullBlock();

    // 实体在方块内部
    AxisAlignedBB inside(0.5f, 0.5f, 0.5f, 0.6f, 1.8f, 0.6f);
    EXPECT_TRUE(shape.intersects(inside, 0, 0, 0));

    // 实体在方块外部
    AxisAlignedBB outside(2.0f, 0.0f, 2.0f, 2.6f, 1.8f, 2.6f);
    EXPECT_FALSE(shape.intersects(outside, 0, 0, 0));

    // 实体与方块边界相交
    AxisAlignedBB boundary(0.5f, 0.5f, 0.5f, 1.5f, 1.5f, 1.5f);
    EXPECT_TRUE(shape.intersects(boundary, 0, 0, 0));
}

TEST_F(CollisionShapeTest, IntersectsEmpty)
{
    CollisionShape shape = CollisionShape::empty();

    AxisAlignedBB anyBox(0.0f, 0.0f, 0.0f, 1.0f, 2.0f, 1.0f);
    EXPECT_FALSE(shape.intersects(anyBox, 0, 0, 0));
}

TEST_F(CollisionShapeTest, IntersectsHalfBlock)
{
    // 下半砖
    CollisionShape shape = CollisionShape::box(0.0f, 0.0f, 0.0f, 1.0f, 0.5f, 1.0f);

    // 实体在下半砖内
    AxisAlignedBB lower(0.1f, 0.1f, 0.1f, 0.9f, 0.4f, 0.9f);
    EXPECT_TRUE(shape.intersects(lower, 0, 0, 0));

    // 实体在上半部分（不碰撞）
    AxisAlignedBB upper(0.1f, 0.6f, 0.1f, 0.9f, 1.0f, 0.9f);
    EXPECT_FALSE(shape.intersects(upper, 0, 0, 0));
}

// ============================================================================
// 面投影测试 (getFaceShape)
// ============================================================================

class CollisionShapeFaceTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(CollisionShapeFaceTest, EmptyShapeReturnsEmpty)
{
    CollisionShape shape = CollisionShape::empty();

    for (int i = 0; i < 6; ++i) {
        Direction dir = static_cast<Direction>(i);
        CollisionShape face = shape.getFaceShape(dir);
        EXPECT_TRUE(face.isEmpty()) << "Direction " << i << " should return empty";
    }
}

TEST_F(CollisionShapeFaceTest, FullBlockReturnsFullBlock)
{
    CollisionShape shape = CollisionShape::fullBlock();

    for (int i = 0; i < 6; ++i) {
        Direction dir = static_cast<Direction>(i);
        CollisionShape face = shape.getFaceShape(dir);
        EXPECT_TRUE(face.isFullBlock()) << "Direction " << i << " should return full block";
    }
}

TEST_F(CollisionShapeFaceTest, BottomSlabBottomFace)
{
    // 下半砖 (Y: 0.0 - 0.5)
    CollisionShape shape = CollisionShape::box(0.0f, 0.0f, 0.0f, 1.0f, 0.5f, 1.0f);

    // 底面（Down）: 延伸到 Y=0，应该返回投影
    CollisionShape face = shape.getFaceShape(Direction::Down);
    EXPECT_FALSE(face.isEmpty());
    EXPECT_TRUE(face.isFullBlock()); // 投影覆盖整个 XZ 平面
}

TEST_F(CollisionShapeFaceTest, BottomSlabTopFace)
{
    // 下半砖 (Y: 0.0 - 0.5)
    CollisionShape shape = CollisionShape::box(0.0f, 0.0f, 0.0f, 1.0f, 0.5f, 1.0f);

    // 顶面（Up）: maxY=0.5，不延伸到 Y=1.0，应该返回空
    CollisionShape face = shape.getFaceShape(Direction::Up);
    EXPECT_TRUE(face.isEmpty()) << "Top face of bottom slab should be empty";
}

TEST_F(CollisionShapeFaceTest, BottomSlabSideFaces)
{
    // 下半砖 (Y: 0.0 - 0.5)
    CollisionShape shape = CollisionShape::box(0.0f, 0.0f, 0.0f, 1.0f, 0.5f, 1.0f);

    // 侧面都应该延伸到边界
    for (int i = 2; i < 6; ++i) { // North, South, West, East
        Direction dir = static_cast<Direction>(i);
        CollisionShape face = shape.getFaceShape(dir);
        EXPECT_FALSE(face.isEmpty()) << "Direction " << i << " should not be empty";

        // 投影应该有一个盒子
        const auto& boxes = face.boxes();
        ASSERT_EQ(boxes.size(), 1);

        // 投影轴应该被压缩到 [0, 1]
        // 对于 North/South，Z 轴被压缩
        // 对于 West/East，X 轴被压缩
        // Y 轴保持原样 (0.0 - 0.5)，所以不是完整方块
        EXPECT_FALSE(face.isFullBlock()) << "Direction " << i << " should not be full block (Y range is 0-0.5)";
    }
}

TEST_F(CollisionShapeFaceTest, TopSlabTopFace)
{
    // 上半砖 (Y: 0.5 - 1.0)
    CollisionShape shape = CollisionShape::box(0.0f, 0.5f, 0.0f, 1.0f, 1.0f, 1.0f);

    // 顶面（Up）: 延伸到 Y=1.0，应该返回投影
    CollisionShape face = shape.getFaceShape(Direction::Up);
    EXPECT_FALSE(face.isEmpty());
    EXPECT_TRUE(face.isFullBlock());
}

TEST_F(CollisionShapeFaceTest, TopSlabBottomFace)
{
    // 上半砖 (Y: 0.5 - 1.0)
    CollisionShape shape = CollisionShape::box(0.0f, 0.5f, 0.0f, 1.0f, 1.0f, 1.0f);

    // 底面（Down）: minY=0.5，不延伸到 Y=0，应该返回空
    CollisionShape face = shape.getFaceShape(Direction::Down);
    EXPECT_TRUE(face.isEmpty()) << "Bottom face of top slab should be empty";
}

TEST_F(CollisionShapeFaceTest, PartialXShape)
{
    // X 方向部分方块 (X: 0.25 - 0.75)
    CollisionShape shape = CollisionShape::box(0.25f, 0.0f, 0.0f, 0.75f, 1.0f, 1.0f);

    // 西面（West, X-）: minX=0.25，不延伸到 X=0，应该返回空
    CollisionShape westFace = shape.getFaceShape(Direction::West);
    EXPECT_TRUE(westFace.isEmpty()) << "West face should be empty (minX=0.25)";

    // 东面（East, X+）: maxX=0.75，不延伸到 X=1，应该返回空
    CollisionShape eastFace = shape.getFaceShape(Direction::East);
    EXPECT_TRUE(eastFace.isEmpty()) << "East face should be empty (maxX=0.75)";

    // 其他面都应该有投影
    CollisionShape upFace = shape.getFaceShape(Direction::Up);
    EXPECT_FALSE(upFace.isEmpty());
}

TEST_F(CollisionShapeFaceTest, PartialZShape)
{
    // Z 方向部分方块 (Z: 0.3 - 0.7)
    CollisionShape shape = CollisionShape::box(0.0f, 0.0f, 0.3f, 1.0f, 1.0f, 0.7f);

    // 北面（North, Z-）: minZ=0.3，不延伸到 Z=0，应该返回空
    CollisionShape northFace = shape.getFaceShape(Direction::North);
    EXPECT_TRUE(northFace.isEmpty()) << "North face should be empty (minZ=0.3)";

    // 南面（South, Z+）: maxZ=0.7，不延伸到 Z=1，应该返回空
    CollisionShape southFace = shape.getFaceShape(Direction::South);
    EXPECT_TRUE(southFace.isEmpty()) << "South face should be empty (maxZ=0.7)";
}

TEST_F(CollisionShapeFaceTest, StairsShape)
{
    // 楼梯形状：下半部分 + 后半部分
    CollisionShape shape = CollisionShape::box(0.0f, 0.0f, 0.0f, 1.0f, 0.5f, 1.0f);
    shape.addBox(0.0f, 0.5f, 0.0f, 1.0f, 1.0f, 0.5f);

    // 底面应该有投影
    CollisionShape bottomFace = shape.getFaceShape(Direction::Down);
    EXPECT_FALSE(bottomFace.isEmpty());

    // 顶面也应该有投影（后半部分延伸到顶部）
    CollisionShape topFace = shape.getFaceShape(Direction::Up);
    EXPECT_FALSE(topFace.isEmpty());

    // 检查顶面投影是否正确（只有后半部分）
    const auto& boxes = topFace.boxes();
    ASSERT_EQ(boxes.size(), 1);
    // Y 轴应该被投影为 [0, 1]
    EXPECT_FLOAT_EQ(boxes[0].minY, 0.0f);
    EXPECT_FLOAT_EQ(boxes[0].maxY, 1.0f);
}

TEST_F(CollisionShapeFaceTest, BoxTouchesBoundary)
{
    // 测试刚好接触边界的方块
    // 使用底部边界测试（minY 接近 0）
    // EPSILON = 1.0e-5f，所以 minY = 1e-6f < EPSILON 应该被接受
    CollisionShape shape = CollisionShape::box(0.0f, 0.000001f, 0.0f, 1.0f, 1.0f, 1.0f);

    // 底面应该被识别为接触边界
    CollisionShape bottomFace = shape.getFaceShape(Direction::Down);
    EXPECT_FALSE(bottomFace.isEmpty()) << "Face with minY=0.000001 should touch boundary (within epsilon)";
}

TEST_F(CollisionShapeFaceTest, BoxAtExactBoundary)
{
    // 测试刚好等于边界值的方块
    // 这是最可靠的测试方式
    CollisionShape shape = CollisionShape::box(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f);

    // 所有面都应该有投影（完整方块会返回 fullBlock）
    for (int i = 0; i < 6; ++i) {
        Direction dir = static_cast<Direction>(i);
        CollisionShape face = shape.getFaceShape(dir);
        EXPECT_TRUE(face.isFullBlock()) << "Direction " << i << " should be full block";
    }
}

TEST_F(CollisionShapeFaceTest, BoxNotTouchesBoundary)
{
    // 不接触边界的方块
    CollisionShape shape = CollisionShape::box(0.0f, 0.0f, 0.0f, 1.0f, 0.99f, 1.0f);

    // 顶面不应该被识别为接触边界
    CollisionShape topFace = shape.getFaceShape(Direction::Up);
    EXPECT_TRUE(topFace.isEmpty()) << "Face with maxY=0.99 should not touch boundary";
}

TEST_F(CollisionShapeFaceTest, MultipleBoxesProjectingToSameFace)
{
    // 两个不相连的碰撞箱都在顶面有投影
    CollisionShape shape = CollisionShape::box(0.0f, 0.5f, 0.0f, 0.5f, 1.0f, 1.0f);
    shape.addBox(0.5f, 0.5f, 0.0f, 1.0f, 1.0f, 1.0f);

    // 顶面投影应该有两个盒子
    CollisionShape topFace = shape.getFaceShape(Direction::Up);
    EXPECT_FALSE(topFace.isEmpty());
    EXPECT_EQ(topFace.boxCount(), 2);
}

TEST_F(CollisionShapeFaceTest, ProjectionAxisExpansion)
{
    // 验证投影后的盒子在投影轴上覆盖 [0, 1]
    CollisionShape shape = CollisionShape::box(0.1f, 0.0f, 0.1f, 0.9f, 1.0f, 0.9f);

    // 顶面投影
    CollisionShape topFace = shape.getFaceShape(Direction::Up);
    const auto& boxes = topFace.boxes();
    ASSERT_EQ(boxes.size(), 1);

    // Y 轴应该被投影为 [0, 1]
    EXPECT_FLOAT_EQ(boxes[0].minY, 0.0f);
    EXPECT_FLOAT_EQ(boxes[0].maxY, 1.0f);

    // X 和 Z 应该保持原样
    EXPECT_FLOAT_EQ(boxes[0].minX, 0.1f);
    EXPECT_FLOAT_EQ(boxes[0].maxX, 0.9f);
    EXPECT_FLOAT_EQ(boxes[0].minZ, 0.1f);
    EXPECT_FLOAT_EQ(boxes[0].maxZ, 0.9f);
}

// ============================================================================
// 形状合并测试
// ============================================================================

TEST_F(CollisionShapeTest, CombineOr)
{
    CollisionShape a = CollisionShape::box(0.0f, 0.0f, 0.0f, 0.5f, 1.0f, 1.0f);
    CollisionShape b = CollisionShape::box(0.5f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f);

    CollisionShape combined = CollisionShape::combine(a, b, CollisionShape::CombineOp::OR);
    EXPECT_EQ(combined.boxCount(), 2);
}

TEST_F(CollisionShapeTest, CombineOrWithFullBlock)
{
    CollisionShape a = CollisionShape::fullBlock();
    CollisionShape b = CollisionShape::box(0.0f, 0.0f, 0.0f, 0.5f, 0.5f, 0.5f);

    CollisionShape combined = CollisionShape::combine(a, b, CollisionShape::CombineOp::OR);
    EXPECT_TRUE(combined.isFullBlock());
}
