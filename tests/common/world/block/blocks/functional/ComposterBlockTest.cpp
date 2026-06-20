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

#include "world/block/blocks/functional/ComposterBlock.hpp"
#include "common/physics/collision/CollisionShape.hpp"
#include "common/util/property/Properties.hpp"
#include "common/world/block/Material.hpp"
#include <gtest/gtest.h>

using namespace mc;
using namespace mc::blocks;

// ========== ComposterBlock 形状测试 ==========

class ComposterBlockTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        composter_ = std::make_unique<ComposterBlock>(BlockProperties(Material::WOOD).hardness(0.6f).resistance(0.6f));
    }

    std::unique_ptr<ComposterBlock> composter_;
};

// ========== 基本属性测试 ==========

TEST_F(ComposterBlockTest, Create_HasCorrectProperties)
{
    EXPECT_NE(composter_, nullptr);
}

TEST_F(ComposterBlockTest, DefaultState_HasCorrectLevel)
{
    const auto& state = composter_->defaultState();
    EXPECT_EQ(ComposterBlock::getLevel(state), 0);
}

TEST_F(ComposterBlockTest, IsOpaque_ReturnsTrue)
{
    const auto& state = composter_->defaultState();
    // MC Java 中 ComposterBlock 未重写 isOpaque，默认为 true
    // 即使有空腔，方块本身是不透明的（阻挡光线传播）
    EXPECT_TRUE(composter_->isOpaque(state));
}

// ========== 渲染形状测试 ==========

TEST_F(ComposterBlockTest, GetShape_Level0_NotEmpty)
{
    // 等级0：底板2像素 + 四面墙壁
    auto state = composter_->defaultState().with(BlockStateProperties::LEVEL_0_8(), 0);
    const auto& shape = composter_->getShape(state);
    EXPECT_FALSE(shape.isEmpty()) << "Level 0 shape should not be empty";
}

TEST_F(ComposterBlockTest, GetShape_Level0_NotFullBlock)
{
    // 等级0不是完整方块，应该是有空腔的外壁形状
    auto state = composter_->defaultState().with(BlockStateProperties::LEVEL_0_8(), 0);
    const auto& shape = composter_->getShape(state);
    EXPECT_FALSE(shape.isFullBlock()) << "Level 0 shape should NOT be a full block (it has a hollow interior)";
}

TEST_F(ComposterBlockTest, GetShape_AllLevels_NotEmpty)
{
    // 所有等级的形状都应非空
    for (i32 level = 0; level <= 8; ++level) {
        auto state = composter_->defaultState().with(BlockStateProperties::LEVEL_0_8(), level);
        const auto& shape = composter_->getShape(state);
        EXPECT_FALSE(shape.isEmpty()) << "Level " << level << " shape should not be empty";
    }
}

TEST_F(ComposterBlockTest, GetShape_Level7And8_AreIdentical)
{
    // MC Java: avoxelshape[8] = avoxelshape[7]
    auto state7 = composter_->defaultState().with(BlockStateProperties::LEVEL_0_8(), 7);
    auto state8 = composter_->defaultState().with(BlockStateProperties::LEVEL_0_8(), 8);

    const auto& shape7 = composter_->getShape(state7);
    const auto& shape8 = composter_->getShape(state8);

    // 两个形状的包围盒应完全相同
    auto boxes7 = shape7.getWorldBoxes(0, 0, 0);
    auto boxes8 = shape8.getWorldBoxes(0, 0, 0);
    ASSERT_EQ(boxes7.size(), boxes8.size()) << "Level 7 and 8 should have same number of boxes";

    for (size_t i = 0; i < boxes7.size(); ++i) {
        EXPECT_FLOAT_EQ(boxes7[i].minX, boxes8[i].minX) << "Box " << i << " minX differs between level 7 and 8";
        EXPECT_FLOAT_EQ(boxes7[i].minY, boxes8[i].minY) << "Box " << i << " minY differs between level 7 and 8";
        EXPECT_FLOAT_EQ(boxes7[i].minZ, boxes8[i].minZ) << "Box " << i << " minZ differs between level 7 and 8";
        EXPECT_FLOAT_EQ(boxes7[i].maxX, boxes8[i].maxX) << "Box " << i << " maxX differs between level 7 and 8";
        EXPECT_FLOAT_EQ(boxes7[i].maxY, boxes8[i].maxY) << "Box " << i << " maxY differs between level 7 and 8";
        EXPECT_FLOAT_EQ(boxes7[i].maxZ, boxes8[i].maxZ) << "Box " << i << " maxZ differs between level 7 and 8";
    }
}

TEST_F(ComposterBlockTest, GetShape_HasFiveBoxes)
{
    // 外壁形状由5部分组成：底板 + 4面墙
    auto state = composter_->defaultState().with(BlockStateProperties::LEVEL_0_8(), 0);
    const auto& shape = composter_->getShape(state);
    auto boxes = shape.getWorldBoxes(0, 0, 0);
    // 底板 + 北墙 + 南墙 + 西墙 + 东墙 = 5 个AABB
    EXPECT_EQ(boxes.size(), 5u) << "Composter shape should consist of 5 boxes (base + 4 walls)";
}

TEST_F(ComposterBlockTest, GetShape_Level0_BaseHeight)
{
    // 等级0：fillHeightPixels = max(2, 1+0*2) = 2，底板高度 = 2/16
    constexpr f32 P = 1.0f / 16.0f;
    auto state = composter_->defaultState().with(BlockStateProperties::LEVEL_0_8(), 0);
    const auto& shape = composter_->getShape(state);
    auto boxes = shape.getWorldBoxes(0, 0, 0);

    // 底板应为 (0, 0, 0) -> (1, 2P, 1)
    bool foundBase = false;
    for (const auto& box : boxes) {
        if (box.minX == 0.0f && box.minY == 0.0f && box.minZ == 0.0f && box.maxX == 1.0f && box.maxZ == 1.0f) {
            EXPECT_FLOAT_EQ(box.maxY, 2.0f * P) << "Level 0 base should be 2 pixels tall";
            foundBase = true;
            break;
        }
    }
    EXPECT_TRUE(foundBase) << "Level 0 shape should have a base box at y=0";
}

TEST_F(ComposterBlockTest, GetShape_Level0_WallHeight)
{
    // 等级0：墙壁从 y=2P 延伸到 y=1.0
    constexpr f32 P = 1.0f / 16.0f;
    auto state = composter_->defaultState().with(BlockStateProperties::LEVEL_0_8(), 0);
    const auto& shape = composter_->getShape(state);
    auto boxes = shape.getWorldBoxes(0, 0, 0);

    // 检查北墙：(0, 2P, 0) -> (1, 1, 2P)
    bool foundNorthWall = false;
    for (const auto& box : boxes) {
        if (box.minX == 0.0f && box.minZ == 0.0f && box.maxX == 1.0f && box.maxZ == 2.0f * P) {
            EXPECT_FLOAT_EQ(box.minY, 2.0f * P) << "North wall should start at y=2P";
            EXPECT_FLOAT_EQ(box.maxY, 1.0f) << "North wall should reach y=1.0";
            foundNorthWall = true;
            break;
        }
    }
    EXPECT_TRUE(foundNorthWall) << "Level 0 shape should have a north wall";
}

TEST_F(ComposterBlockTest, GetShape_HigherLevelsHaveHigherFill)
{
    // 更高等级的底板更高，空心区域更小
    // 验证等级7的底板高度 > 等级0的底板高度
    constexpr f32 P = 1.0f / 16.0f;
    auto state0 = composter_->defaultState().with(BlockStateProperties::LEVEL_0_8(), 0);
    auto state7 = composter_->defaultState().with(BlockStateProperties::LEVEL_0_8(), 7);

    const auto& shape0 = composter_->getShape(state0);
    const auto& shape7 = composter_->getShape(state7);

    auto boxes0 = shape0.getWorldBoxes(0, 0, 0);
    auto boxes7 = shape7.getWorldBoxes(0, 0, 0);

    // 找到底板（ minX=0, minZ=0, maxX=1, maxZ=1 的 box）
    f32 baseY0 = 0.0f;
    f32 baseY7 = 0.0f;
    for (const auto& box : boxes0) {
        if (box.minX == 0.0f && box.minZ == 0.0f && box.maxX == 1.0f && box.maxZ == 1.0f) {
            baseY0 = box.maxY;
        }
    }
    for (const auto& box : boxes7) {
        if (box.minX == 0.0f && box.minZ == 0.0f && box.maxX == 1.0f && box.maxZ == 1.0f) {
            baseY7 = box.maxY;
        }
    }

    // 等级7：fillHeightPixels = max(2, 1+7*2) = 15，底板高度 = 15/16
    // 等级0：fillHeightPixels = max(2, 1+0*2) = 2，底板高度 = 2/16
    EXPECT_GT(baseY7, baseY0) << "Level 7 should have a higher base than level 0";
    EXPECT_FLOAT_EQ(baseY0, 2.0f * P) << "Level 0 base height should be 2/16";
    EXPECT_FLOAT_EQ(baseY7, 15.0f * P) << "Level 7 base height should be 15/16";
}

// ========== 碰撞形状测试 ==========

TEST_F(ComposterBlockTest, GetCollisionShape_IsLevel0Shape)
{
    // MC Java: getCollisionShape() 始终返回 SHAPES[0]（等级0的外壳形状）
    auto state0 = composter_->defaultState().with(BlockStateProperties::LEVEL_0_8(), 0);
    auto state5 = composter_->defaultState().with(BlockStateProperties::LEVEL_0_8(), 5);
    auto state8 = composter_->defaultState().with(BlockStateProperties::LEVEL_0_8(), 8);

    const auto& collisionShape0 = composter_->getCollisionShape(state0);
    const auto& collisionShape5 = composter_->getCollisionShape(state5);
    const auto& collisionShape8 = composter_->getCollisionShape(state8);

    // 所有等级的碰撞形状应该相同（等于等级0的渲染形状）
    auto boxes0 = collisionShape0.getWorldBoxes(0, 0, 0);
    auto boxes5 = collisionShape5.getWorldBoxes(0, 0, 0);
    auto boxes8 = collisionShape8.getWorldBoxes(0, 0, 0);

    EXPECT_EQ(boxes0.size(), boxes5.size()) << "Collision shape should have same box count across levels";
    EXPECT_EQ(boxes0.size(), boxes8.size()) << "Collision shape should have same box count across levels";

    for (size_t i = 0; i < boxes0.size(); ++i) {
        EXPECT_FLOAT_EQ(boxes0[i].minX, boxes5[i].minX) << "Collision box " << i << " minX differs";
        EXPECT_FLOAT_EQ(boxes0[i].minY, boxes5[i].minY) << "Collision box " << i << " minY differs";
        EXPECT_FLOAT_EQ(boxes0[i].minZ, boxes5[i].minZ) << "Collision box " << i << " minZ differs";
        EXPECT_FLOAT_EQ(boxes0[i].maxX, boxes5[i].maxX) << "Collision box " << i << " maxX differs";
        EXPECT_FLOAT_EQ(boxes0[i].maxY, boxes5[i].maxY) << "Collision box " << i << " maxY differs";
        EXPECT_FLOAT_EQ(boxes0[i].maxZ, boxes5[i].maxZ) << "Collision box " << i << " maxZ differs";
    }
}

TEST_F(ComposterBlockTest, GetCollisionShape_NotFullBlock)
{
    // 碰撞形状不应该是完整方块（有空腔可以站进去）
    auto state = composter_->defaultState().with(BlockStateProperties::LEVEL_0_8(), 0);
    const auto& collisionShape = composter_->getCollisionShape(state);
    EXPECT_FALSE(collisionShape.isFullBlock()) << "Collision shape should NOT be a full block";
}

TEST_F(ComposterBlockTest, GetCollisionShape_EqualsLevel0Shape)
{
    // 碰撞形状应与等级0的渲染形状完全相同
    auto state0 = composter_->defaultState().with(BlockStateProperties::LEVEL_0_8(), 0);
    auto state5 = composter_->defaultState().with(BlockStateProperties::LEVEL_0_8(), 5);

    const auto& shape0 = composter_->getShape(state0);
    const auto& collisionShape5 = composter_->getCollisionShape(state5);

    auto shape0Boxes = shape0.getWorldBoxes(0, 0, 0);
    auto collisionBoxes = collisionShape5.getWorldBoxes(0, 0, 0);

    ASSERT_EQ(shape0Boxes.size(), collisionBoxes.size());
    for (size_t i = 0; i < shape0Boxes.size(); ++i) {
        EXPECT_FLOAT_EQ(shape0Boxes[i].minX, collisionBoxes[i].minX);
        EXPECT_FLOAT_EQ(shape0Boxes[i].minY, collisionBoxes[i].minY);
        EXPECT_FLOAT_EQ(shape0Boxes[i].minZ, collisionBoxes[i].minZ);
        EXPECT_FLOAT_EQ(shape0Boxes[i].maxX, collisionBoxes[i].maxX);
        EXPECT_FLOAT_EQ(shape0Boxes[i].maxY, collisionBoxes[i].maxY);
        EXPECT_FLOAT_EQ(shape0Boxes[i].maxZ, collisionBoxes[i].maxZ);
    }
}

// ========== 比较器输出测试 ==========

TEST_F(ComposterBlockTest, HasComparatorInputOverride)
{
    auto state = composter_->defaultState().with(BlockStateProperties::LEVEL_0_8(), 5);
    EXPECT_TRUE(composter_->hasComparatorInputOverride(state));
}

// ========== 各等级底板高度验证 ==========

TEST_F(ComposterBlockTest, GetShape_AllLevelsFillHeightMatchesMC)
{
    // MC Java: Block.column(12.0, clamp(1 + level * 2, 2, 16), 16.0)
    // fillHeightPixels = max(2, 1 + level * 2) for level 0-7
    constexpr f32 P = 1.0f / 16.0f;

    i32 expectedFillHeights[] = {2, 3, 5, 7, 9, 11, 13, 15};

    for (i32 level = 0; level < 8; ++level) {
        auto state = composter_->defaultState().with(BlockStateProperties::LEVEL_0_8(), level);
        const auto& shape = composter_->getShape(state);
        auto boxes = shape.getWorldBoxes(0, 0, 0);

        // 找到底板（覆盖整个 XZ 平面的 box）
        bool foundBase = false;
        for (const auto& box : boxes) {
            if (box.minX == 0.0f && box.minZ == 0.0f && box.maxX == 1.0f && box.maxZ == 1.0f) {
                f32 expectedHeight = static_cast<f32>(expectedFillHeights[level]) * P;
                EXPECT_FLOAT_EQ(box.maxY, expectedHeight)
                    << "Level " << level << " base height should be " << expectedFillHeights[level] << "/16";
                foundBase = true;
                break;
            }
        }
        EXPECT_TRUE(foundBase) << "Level " << level << " should have a base box covering full XZ plane";
    }
}
