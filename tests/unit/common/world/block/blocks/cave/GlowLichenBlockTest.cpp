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
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN THE EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include <gtest/gtest.h>

#include "common/util/property/Properties.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/Material.hpp"
#include "common/world/block/blocks/cave/GlowLichenBlock.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"

using namespace mc;
using namespace mc::blocks;

// ========== GlowLichenBlock 测试 ==========

class GlowLichenBlockTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        block_ = std::make_unique<GlowLichenBlock>(
            BlockProperties(Material::REPLACEABLE_PLANT).noCollision().notSolid().hardness(0.2f).resistance(0.2f));
    }

    std::unique_ptr<GlowLichenBlock> block_;
};

// ============================================================================
// 构造与默认状态测试
// ============================================================================

TEST_F(GlowLichenBlockTest, Create_HasCorrectProperties)
{
    EXPECT_NE(block_, nullptr);
}

TEST_F(GlowLichenBlockTest, DefaultState_AllFacesFalse)
{
    const BlockState& state = block_->defaultState();
    EXPECT_FALSE(state.get(BlockStateProperties::NORTH()));
    EXPECT_FALSE(state.get(BlockStateProperties::SOUTH()));
    EXPECT_FALSE(state.get(BlockStateProperties::EAST()));
    EXPECT_FALSE(state.get(BlockStateProperties::WEST()));
    EXPECT_FALSE(state.get(BlockStateProperties::UP()));
    EXPECT_FALSE(state.get(BlockStateProperties::DOWN()));
    EXPECT_FALSE(state.get(BlockStateProperties::WATERLOGGED()));
}

TEST_F(GlowLichenBlockTest, DefaultState_HasCorrectBlockType)
{
    EXPECT_TRUE(dynamic_cast<const GlowLichenBlock*>(&block_->defaultState().getBlock()) != nullptr);
}

// ============================================================================
// 状态属性测试
// ============================================================================

TEST_F(GlowLichenBlockTest, FaceProperties_CanBeToggled)
{
    auto state = block_->defaultState();

    // 北面
    state = state.with(BlockStateProperties::NORTH(), true);
    EXPECT_TRUE(state.get(BlockStateProperties::NORTH()));
    state = state.with(BlockStateProperties::NORTH(), false);
    EXPECT_FALSE(state.get(BlockStateProperties::NORTH()));

    // 南面
    state = state.with(BlockStateProperties::SOUTH(), true);
    EXPECT_TRUE(state.get(BlockStateProperties::SOUTH()));

    // 东面
    state = state.with(BlockStateProperties::EAST(), true);
    EXPECT_TRUE(state.get(BlockStateProperties::EAST()));

    // 西面
    state = state.with(BlockStateProperties::WEST(), true);
    EXPECT_TRUE(state.get(BlockStateProperties::WEST()));

    // 上面
    state = state.with(BlockStateProperties::UP(), true);
    EXPECT_TRUE(state.get(BlockStateProperties::UP()));

    // 下面
    state = state.with(BlockStateProperties::DOWN(), true);
    EXPECT_TRUE(state.get(BlockStateProperties::DOWN()));
}

TEST_F(GlowLichenBlockTest, WaterloggedProperty_CanBeToggled)
{
    auto state = block_->defaultState();
    EXPECT_FALSE(state.get(BlockStateProperties::WATERLOGGED()));
    state = state.with(BlockStateProperties::WATERLOGGED(), true);
    EXPECT_TRUE(state.get(BlockStateProperties::WATERLOGGED()));
}

// ============================================================================
// 形状索引测试（_getShapeIndex 位编码验证）
// ============================================================================

TEST_F(GlowLichenBlockTest, ShapeIndex_NoFacesActive_IsZero)
{
    // 没有任何面激活时索引为0
    const CollisionShape& shape = block_->getShape(block_->defaultState());
    // 没有任何面时回退到 fullBlock
    EXPECT_TRUE(shape.isFullBlock());
}

TEST_F(GlowLichenBlockTest, ShapeIndex_SingleFaceActive)
{
    // 逐个测试单面激活
    // NORTH
    {
        auto state = block_->defaultState().with(BlockStateProperties::NORTH(), true);
        const CollisionShape& shape = block_->getShape(state);
        EXPECT_FALSE(shape.isEmpty());
        EXPECT_FALSE(shape.isFullBlock());
        EXPECT_EQ(shape.boxCount(), 1u);
    }
    // SOUTH
    {
        auto state = block_->defaultState().with(BlockStateProperties::SOUTH(), true);
        const CollisionShape& shape = block_->getShape(state);
        EXPECT_FALSE(shape.isEmpty());
        EXPECT_FALSE(shape.isFullBlock());
        EXPECT_EQ(shape.boxCount(), 1u);
    }
    // EAST
    {
        auto state = block_->defaultState().with(BlockStateProperties::EAST(), true);
        const CollisionShape& shape = block_->getShape(state);
        EXPECT_FALSE(shape.isEmpty());
        EXPECT_FALSE(shape.isFullBlock());
        EXPECT_EQ(shape.boxCount(), 1u);
    }
    // WEST
    {
        auto state = block_->defaultState().with(BlockStateProperties::WEST(), true);
        const CollisionShape& shape = block_->getShape(state);
        EXPECT_FALSE(shape.isEmpty());
        EXPECT_FALSE(shape.isFullBlock());
        EXPECT_EQ(shape.boxCount(), 1u);
    }
    // UP
    {
        auto state = block_->defaultState().with(BlockStateProperties::UP(), true);
        const CollisionShape& shape = block_->getShape(state);
        EXPECT_FALSE(shape.isEmpty());
        EXPECT_FALSE(shape.isFullBlock());
        EXPECT_EQ(shape.boxCount(), 1u);
    }
    // DOWN
    {
        auto state = block_->defaultState().with(BlockStateProperties::DOWN(), true);
        const CollisionShape& shape = block_->getShape(state);
        EXPECT_FALSE(shape.isEmpty());
        EXPECT_FALSE(shape.isFullBlock());
        EXPECT_EQ(shape.boxCount(), 1u);
    }
}

TEST_F(GlowLichenBlockTest, ShapeIndex_TwoFacesActive)
{
    // 两个面激活时应该有2个碰撞盒
    auto state =
        block_->defaultState().with(BlockStateProperties::NORTH(), true).with(BlockStateProperties::SOUTH(), true);
    const CollisionShape& shape = block_->getShape(state);
    EXPECT_EQ(shape.boxCount(), 2u);
}

TEST_F(GlowLichenBlockTest, ShapeIndex_AllSixFacesActive)
{
    // 六个面全部激活
    auto state = block_->defaultState()
                     .with(BlockStateProperties::NORTH(), true)
                     .with(BlockStateProperties::SOUTH(), true)
                     .with(BlockStateProperties::EAST(), true)
                     .with(BlockStateProperties::WEST(), true)
                     .with(BlockStateProperties::UP(), true)
                     .with(BlockStateProperties::DOWN(), true);
    const CollisionShape& shape = block_->getShape(state);
    EXPECT_EQ(shape.boxCount(), 6u);
}

TEST_F(GlowLichenBlockTest, ShapeIndex_ThreeFacesActive)
{
    // 三个面激活：北、上、下
    auto state = block_->defaultState()
                     .with(BlockStateProperties::NORTH(), true)
                     .with(BlockStateProperties::UP(), true)
                     .with(BlockStateProperties::DOWN(), true);
    const CollisionShape& shape = block_->getShape(state);
    EXPECT_EQ(shape.boxCount(), 3u);
}

// ============================================================================
// 64种形状组合遍历测试
// ============================================================================

TEST_F(GlowLichenBlockTest, All64Combinations_HaveCorrectBoxCount)
{
    // 遍历所有64种面激活组合，验证每种组合的碰撞盒数量等于激活面数
    int testedCount = 0;

    for (int down = 0; down <= 1; ++down) {
        for (int up = 0; up <= 1; ++up) {
            for (int north = 0; north <= 1; ++north) {
                for (int south = 0; south <= 1; ++south) {
                    for (int east = 0; east <= 1; ++east) {
                        for (int west = 0; west <= 1; ++west) {
                            auto state = block_->defaultState()
                                             .with(BlockStateProperties::DOWN(), down != 0)
                                             .with(BlockStateProperties::UP(), up != 0)
                                             .with(BlockStateProperties::NORTH(), north != 0)
                                             .with(BlockStateProperties::SOUTH(), south != 0)
                                             .with(BlockStateProperties::EAST(), east != 0)
                                             .with(BlockStateProperties::WEST(), west != 0);

                            const CollisionShape& shape = block_->getShape(state);
                            int activeFaces = down + up + north + south + east + west;

                            if (activeFaces == 0) {
                                // 没有面激活时回退到 fullBlock
                                EXPECT_TRUE(shape.isFullBlock()) << "No faces active should be full block";
                            } else {
                                // 有面激活时碰撞盒数等于激活面数
                                EXPECT_EQ(shape.boxCount(), static_cast<size_t>(activeFaces))
                                    << "Box count mismatch for combination: D=" << down << " U=" << up << " N=" << north
                                    << " S=" << south << " E=" << east << " W=" << west;
                            }

                            ++testedCount;
                        }
                    }
                }
            }
        }
    }

    EXPECT_EQ(testedCount, 64) << "Should test all 64 combinations";
}

// ============================================================================
// 单面碰撞盒坐标精确验证
// ============================================================================

TEST_F(GlowLichenBlockTest, SingleFaceBoxCoordinates_North)
{
    // NORTH: (0,0,0)-(16,16,1) 像素 → (0,0,0)-(1,1,1/16) 方块坐标
    auto state = block_->defaultState().with(BlockStateProperties::NORTH(), true);
    const CollisionShape& shape = block_->getShape(state);
    ASSERT_EQ(shape.boxCount(), 1u);

    const auto& box = shape.boxes()[0];
    constexpr f32 P = 1.0f / 16.0f; // 像素到方块的转换比
    EXPECT_FLOAT_EQ(box.minX, 0.0f);
    EXPECT_FLOAT_EQ(box.minY, 0.0f);
    EXPECT_FLOAT_EQ(box.minZ, 0.0f);
    EXPECT_FLOAT_EQ(box.maxX, 1.0f);
    EXPECT_FLOAT_EQ(box.maxY, 1.0f);
    EXPECT_FLOAT_EQ(box.maxZ, P); // 1像素 = 1/16
}

TEST_F(GlowLichenBlockTest, SingleFaceBoxCoordinates_South)
{
    // SOUTH: (0,0,15)-(16,16,16) 像素 → (0,0,15/16)-(1,1,1) 方块坐标
    auto state = block_->defaultState().with(BlockStateProperties::SOUTH(), true);
    const CollisionShape& shape = block_->getShape(state);
    ASSERT_EQ(shape.boxCount(), 1u);

    const auto& box = shape.boxes()[0];
    constexpr f32 P = 1.0f / 16.0f;
    EXPECT_FLOAT_EQ(box.minX, 0.0f);
    EXPECT_FLOAT_EQ(box.minY, 0.0f);
    EXPECT_FLOAT_EQ(box.minZ, 15.0f * P);
    EXPECT_FLOAT_EQ(box.maxX, 1.0f);
    EXPECT_FLOAT_EQ(box.maxY, 1.0f);
    EXPECT_FLOAT_EQ(box.maxZ, 1.0f);
}

TEST_F(GlowLichenBlockTest, SingleFaceBoxCoordinates_East)
{
    // EAST: (15,0,0)-(16,16,16) 像素 → (15/16,0,0)-(1,1,1) 方块坐标
    auto state = block_->defaultState().with(BlockStateProperties::EAST(), true);
    const CollisionShape& shape = block_->getShape(state);
    ASSERT_EQ(shape.boxCount(), 1u);

    const auto& box = shape.boxes()[0];
    constexpr f32 P = 1.0f / 16.0f;
    EXPECT_FLOAT_EQ(box.minX, 15.0f * P);
    EXPECT_FLOAT_EQ(box.minY, 0.0f);
    EXPECT_FLOAT_EQ(box.minZ, 0.0f);
    EXPECT_FLOAT_EQ(box.maxX, 1.0f);
    EXPECT_FLOAT_EQ(box.maxY, 1.0f);
    EXPECT_FLOAT_EQ(box.maxZ, 1.0f);
}

TEST_F(GlowLichenBlockTest, SingleFaceBoxCoordinates_West)
{
    // WEST: (0,0,0)-(1,16,16) 像素 → (0,0,0)-(1/16,1,1) 方块坐标
    auto state = block_->defaultState().with(BlockStateProperties::WEST(), true);
    const CollisionShape& shape = block_->getShape(state);
    ASSERT_EQ(shape.boxCount(), 1u);

    const auto& box = shape.boxes()[0];
    constexpr f32 P = 1.0f / 16.0f;
    EXPECT_FLOAT_EQ(box.minX, 0.0f);
    EXPECT_FLOAT_EQ(box.minY, 0.0f);
    EXPECT_FLOAT_EQ(box.minZ, 0.0f);
    EXPECT_FLOAT_EQ(box.maxX, P);
    EXPECT_FLOAT_EQ(box.maxY, 1.0f);
    EXPECT_FLOAT_EQ(box.maxZ, 1.0f);
}

TEST_F(GlowLichenBlockTest, SingleFaceBoxCoordinates_Up)
{
    // UP: (0,15,0)-(16,16,16) 像素 → (0,15/16,0)-(1,1,1) 方块坐标
    auto state = block_->defaultState().with(BlockStateProperties::UP(), true);
    const CollisionShape& shape = block_->getShape(state);
    ASSERT_EQ(shape.boxCount(), 1u);

    const auto& box = shape.boxes()[0];
    constexpr f32 P = 1.0f / 16.0f;
    EXPECT_FLOAT_EQ(box.minX, 0.0f);
    EXPECT_FLOAT_EQ(box.minY, 15.0f * P);
    EXPECT_FLOAT_EQ(box.minZ, 0.0f);
    EXPECT_FLOAT_EQ(box.maxX, 1.0f);
    EXPECT_FLOAT_EQ(box.maxY, 1.0f);
    EXPECT_FLOAT_EQ(box.maxZ, 1.0f);
}

TEST_F(GlowLichenBlockTest, SingleFaceBoxCoordinates_Down)
{
    // DOWN: (0,0,0)-(16,1,16) 像素 → (0,0,0)-(1,1/16,1) 方块坐标
    auto state = block_->defaultState().with(BlockStateProperties::DOWN(), true);
    const CollisionShape& shape = block_->getShape(state);
    ASSERT_EQ(shape.boxCount(), 1u);

    const auto& box = shape.boxes()[0];
    constexpr f32 P = 1.0f / 16.0f;
    EXPECT_FLOAT_EQ(box.minX, 0.0f);
    EXPECT_FLOAT_EQ(box.minY, 0.0f);
    EXPECT_FLOAT_EQ(box.minZ, 0.0f);
    EXPECT_FLOAT_EQ(box.maxX, 1.0f);
    EXPECT_FLOAT_EQ(box.maxY, P);
    EXPECT_FLOAT_EQ(box.maxZ, 1.0f);
}

// ============================================================================
// CollisionShape::combine 从 empty 开始累加验证
// ============================================================================

TEST_F(GlowLichenBlockTest, CombineFromEmpty_NorthThenSouth)
{
    // 验证从 empty 开始逐步 combine 的结果与直接设置两面属性的结果一致
    auto stateNS =
        block_->defaultState().with(BlockStateProperties::NORTH(), true).with(BlockStateProperties::SOUTH(), true);
    const CollisionShape& shapeNS = block_->getShape(stateNS);

    // 应该有2个碰撞盒
    EXPECT_EQ(shapeNS.boxCount(), 2u);

    // 验证每个盒的坐标
    const auto& boxes = shapeNS.boxes();
    bool foundNorth = false, foundSouth = false;
    constexpr f32 P = 1.0f / 16.0f;

    for (const auto& box : boxes) {
        // 北面: z范围 [0, 1/16]
        if (box.minZ == 0.0f && box.maxZ == P) {
            foundNorth = true;
            EXPECT_FLOAT_EQ(box.minX, 0.0f);
            EXPECT_FLOAT_EQ(box.maxX, 1.0f);
            EXPECT_FLOAT_EQ(box.minY, 0.0f);
            EXPECT_FLOAT_EQ(box.maxY, 1.0f);
        }
        // 南面: z范围 [15/16, 1]
        if (box.minZ == 15.0f * P && box.maxZ == 1.0f) {
            foundSouth = true;
            EXPECT_FLOAT_EQ(box.minX, 0.0f);
            EXPECT_FLOAT_EQ(box.maxX, 1.0f);
            EXPECT_FLOAT_EQ(box.minY, 0.0f);
            EXPECT_FLOAT_EQ(box.maxY, 1.0f);
        }
    }
    EXPECT_TRUE(foundNorth) << "North face box should be present";
    EXPECT_TRUE(foundSouth) << "South face box should be present";
}

// ============================================================================
// 光照等级测试
// ============================================================================

TEST_F(GlowLichenBlockTest, LightLevel_NoFacesActive_ReturnsZero)
{
    const BlockState& state = block_->defaultState();
    EXPECT_EQ(block_->getLightLevel(state), 0u);
}

TEST_F(GlowLichenBlockTest, LightLevel_AnyFaceActive_ReturnsSeven)
{
    // 每个面激活都应该让光照等级为7
    auto stateN = block_->defaultState().with(BlockStateProperties::NORTH(), true);
    EXPECT_EQ(block_->getLightLevel(stateN), 7u);

    auto stateS = block_->defaultState().with(BlockStateProperties::SOUTH(), true);
    EXPECT_EQ(block_->getLightLevel(stateS), 7u);

    auto stateE = block_->defaultState().with(BlockStateProperties::EAST(), true);
    EXPECT_EQ(block_->getLightLevel(stateE), 7u);

    auto stateW = block_->defaultState().with(BlockStateProperties::WEST(), true);
    EXPECT_EQ(block_->getLightLevel(stateW), 7u);

    auto stateU = block_->defaultState().with(BlockStateProperties::UP(), true);
    EXPECT_EQ(block_->getLightLevel(stateU), 7u);

    auto stateD = block_->defaultState().with(BlockStateProperties::DOWN(), true);
    EXPECT_EQ(block_->getLightLevel(stateD), 7u);
}

TEST_F(GlowLichenBlockTest, LightLevel_MultipleFacesActive_ReturnsSeven)
{
    auto state = block_->defaultState()
                     .with(BlockStateProperties::NORTH(), true)
                     .with(BlockStateProperties::SOUTH(), true)
                     .with(BlockStateProperties::UP(), true);
    EXPECT_EQ(block_->getLightLevel(state), 7u);
}

// ============================================================================
// IWaterLoggable 接口测试
// ============================================================================

TEST_F(GlowLichenBlockTest, IsWaterlogged_DefaultFalse)
{
    const BlockState& state = block_->defaultState();
    EXPECT_FALSE(block_->isWaterlogged(state));
}

TEST_F(GlowLichenBlockTest, IsWaterlogged_WhenSetTrue)
{
    auto state = block_->defaultState().with(BlockStateProperties::WATERLOGGED(), true);
    EXPECT_TRUE(block_->isWaterlogged(state));
}

// ============================================================================
// useShapeForLightOcclusion 测试
// ============================================================================

TEST_F(GlowLichenBlockTest, UseShapeForLightOcclusion_AlwaysTrue)
{
    const BlockState& state = block_->defaultState();
    EXPECT_TRUE(block_->useShapeForLightOcclusion(state));

    auto stateWithFaces =
        block_->defaultState().with(BlockStateProperties::NORTH(), true).with(BlockStateProperties::UP(), true);
    EXPECT_TRUE(block_->useShapeForLightOcclusion(stateWithFaces));
}

// ============================================================================
// 不同面组合的形状独立性测试
// ============================================================================

TEST_F(GlowLichenBlockTest, DifferentFaceCombinations_DifferentShapes)
{
    // 不同的面组合应该产生不同的形状（不同碰撞盒数量或坐标）
    auto stateN = block_->defaultState().with(BlockStateProperties::NORTH(), true);
    auto stateS = block_->defaultState().with(BlockStateProperties::SOUTH(), true);
    auto stateNS =
        block_->defaultState().with(BlockStateProperties::NORTH(), true).with(BlockStateProperties::SOUTH(), true);

    const CollisionShape& shapeN = block_->getShape(stateN);
    const CollisionShape& shapeS = block_->getShape(stateS);
    const CollisionShape& shapeNS = block_->getShape(stateNS);

    // 单面形状各有1个盒
    EXPECT_EQ(shapeN.boxCount(), 1u);
    EXPECT_EQ(shapeS.boxCount(), 1u);
    // 双面形状有2个盒
    EXPECT_EQ(shapeNS.boxCount(), 2u);

    // 北面和南面的单面盒坐标应该不同（z方向不同）
    const auto& boxN = shapeN.boxes()[0];
    const auto& boxS = shapeS.boxes()[0];
    EXPECT_NE(boxN.minZ, boxS.minZ);
}

// ============================================================================
// 形状查找一致性测试（所有64种组合均不崩溃）
// ============================================================================

TEST_F(GlowLichenBlockTest, All64Combinations_NoCrash)
{
    // 确保所有64种形状组合均能正常访问，无越界或崩溃
    for (int down = 0; down <= 1; ++down) {
        for (int up = 0; up <= 1; ++up) {
            for (int north = 0; north <= 1; ++north) {
                for (int south = 0; south <= 1; ++south) {
                    for (int east = 0; east <= 1; ++east) {
                        for (int west = 0; west <= 1; ++west) {
                            auto state = block_->defaultState()
                                             .with(BlockStateProperties::DOWN(), down != 0)
                                             .with(BlockStateProperties::UP(), up != 0)
                                             .with(BlockStateProperties::NORTH(), north != 0)
                                             .with(BlockStateProperties::SOUTH(), south != 0)
                                             .with(BlockStateProperties::EAST(), east != 0)
                                             .with(BlockStateProperties::WEST(), west != 0);

                            // 不应崩溃
                            const CollisionShape& shape = block_->getShape(state);
                            EXPECT_FALSE(shape.isEmpty() && (down + up + north + south + east + west > 0))
                                << "Active faces should not produce empty shape";
                        }
                    }
                }
            }
        }
    }
}
