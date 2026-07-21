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
 * @file DirtPathBlockTest.cpp
 * @brief DirtPathBlock 单元测试
 *
 * 测试草径方块的核心行为：
 * - 方块注册为 DirtPathBlock 类型（非 SimpleBlock）
 * - 渲染形状与碰撞形状高度均为 15/16 格（0.9375），顶部比完整方块矮 1 像素
 * - 形状为 SimpleBox（非 FullBlock），boxCount==1
 */

#include <gtest/gtest.h>

#include "common/world/block/blocks/dirt/DirtPathBlock.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"

using namespace mc;
using namespace mc::blocks;

class DirtPathBlockTest : public ::testing::Test {
protected:
    void SetUp() override { VanillaBlocks::initialize(); }
};

// ============================================================================
// 方块注册与类型测试
// ============================================================================

TEST_F(DirtPathBlockTest, DirtPathIsRegistered)
{
    ASSERT_NE(VanillaBlocks::GRASS_PATH, nullptr);
}

TEST_F(DirtPathBlockTest, DirtPathIsDirtPathBlockType)
{
    // 草径应注册为 DirtPathBlock 类型（非 SimpleBlock），因其有自定义形状
    const Block* dirtPath = VanillaBlocks::GRASS_PATH;
    ASSERT_NE(dirtPath, nullptr);

    auto* dirtPathBlock = dynamic_cast<const DirtPathBlock*>(dirtPath);
    EXPECT_NE(dirtPathBlock, nullptr) << "GRASS_PATH should be registered as DirtPathBlock, not SimpleBlock";
}

// ============================================================================
// 渲染形状测试
// ============================================================================

TEST_F(DirtPathBlockTest, ShapeHeightIs15Over16)
{
    // 草径渲染形状高度应为 15/16 格（0.9375），顶部比完整方块矮 1 像素
    const Block* dirtPath = VanillaBlocks::GRASS_PATH;
    ASSERT_NE(dirtPath, nullptr);

    const BlockState& state = dirtPath->defaultState();
    const CollisionShape& shape = dirtPath->getShape(state);

    ASSERT_EQ(shape.boxCount(), 1u) << "DirtPath shape should be a single box";

    const AxisAlignedBB& box = shape.boxes().front();
    EXPECT_FLOAT_EQ(box.minX, 0.0f);
    EXPECT_FLOAT_EQ(box.minY, 0.0f);
    EXPECT_FLOAT_EQ(box.minZ, 0.0f);
    EXPECT_FLOAT_EQ(box.maxX, 1.0f);
    EXPECT_FLOAT_EQ(box.maxY, 15.0f / 16.0f) << "DirtPath shape maxY should be 15/16 (0.9375)";
    EXPECT_FLOAT_EQ(box.maxZ, 1.0f);
}

// ============================================================================
// 碰撞形状测试
// ============================================================================

TEST_F(DirtPathBlockTest, CollisionShapeEqualsShape)
{
    // 草径碰撞形状与渲染形状一致（均为 15/16 高）
    // 注意：与 FarmlandBlock 不同，FarmlandBlock 的碰撞形状是完整方块
    const Block* dirtPath = VanillaBlocks::GRASS_PATH;
    ASSERT_NE(dirtPath, nullptr);

    const BlockState& state = dirtPath->defaultState();
    const CollisionShape& shape = dirtPath->getShape(state);
    const CollisionShape& collisionShape = dirtPath->getCollisionShape(state);

    ASSERT_EQ(shape.boxCount(), 1u);
    ASSERT_EQ(collisionShape.boxCount(), 1u);

    const AxisAlignedBB& shapeBox = shape.boxes().front();
    const AxisAlignedBB& collisionBox = collisionShape.boxes().front();
    EXPECT_FLOAT_EQ(shapeBox.maxY, collisionBox.maxY)
        << "DirtPath collision shape should equal render shape (15/16 high)";
}

// ============================================================================
// 形状类型测试
// ============================================================================

TEST_F(DirtPathBlockTest, ShapeIsSimpleBoxNotFullBlock)
{
    // 草径形状应为 SimpleBox（非 FullBlock），且仅 1 个 box
    // 这样 ChunkMesher 才会按 shape 的 AABB 渲染非完整高度
    const Block* dirtPath = VanillaBlocks::GRASS_PATH;
    ASSERT_NE(dirtPath, nullptr);

    const BlockState& state = dirtPath->defaultState();
    const CollisionShape& shape = dirtPath->getShape(state);

    EXPECT_FALSE(shape.isEmpty());
    EXPECT_FALSE(shape.isFullBlock())
        << "DirtPath shape should NOT be FullBlock (must be SimpleBox for short-height rendering)";
    EXPECT_EQ(shape.boxCount(), 1u);
}
