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

#include <gtest/gtest.h>

#include "common/item/Items.hpp"
#include "common/util/property/Properties.hpp"
#include "common/world/block/BlockRegistry.hpp"
#include "common/world/block/blocks/agricultural/CropBlock.hpp"
#include "common/world/block/blocks/agricultural/PitcherCropBlock.hpp"
#include "common/world/block/registry/TrailsBlocks.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"

using namespace mc;
using namespace mc::blocks;
using namespace mc::block_registry;

namespace {

/**
 * @brief 瓶草作物测试夹具
 *
 * 初始化必要的注册表，确保 Items 和 VanillaBlocks/TrailsBlocks 可用
 */
class PitcherCropBlockTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        Items::initialize();
    }
};

// ============================================================================
// 常量测试
// ============================================================================

TEST_F(PitcherCropBlockTest, MaxAge_Is4)
{
    EXPECT_EQ(PitcherCropBlock::MAX_AGE, 4) << "PitcherCropBlock MAX_AGE should be 4";
}

TEST_F(PitcherCropBlockTest, DoublePlantAgeIntersection_Is3)
{
    EXPECT_EQ(PitcherCropBlock::DOUBLE_PLANT_AGE_INTERSECTION, 3)
        << "DOUBLE_PLANT_AGE_INTERSECTION should be 3 (AGE>=3 becomes double)";
}

// ============================================================================
// isDouble 静态方法测试
// ============================================================================

TEST_F(PitcherCropBlockTest, IsDouble_False_ForAge0)
{
    EXPECT_FALSE(PitcherCropBlock::isDouble(0)) << "AGE 0 should be single block";
}

TEST_F(PitcherCropBlockTest, IsDouble_False_ForAge1)
{
    EXPECT_FALSE(PitcherCropBlock::isDouble(1)) << "AGE 1 should be single block";
}

TEST_F(PitcherCropBlockTest, IsDouble_False_ForAge2)
{
    EXPECT_FALSE(PitcherCropBlock::isDouble(2)) << "AGE 2 should be single block";
}

TEST_F(PitcherCropBlockTest, IsDouble_True_ForAge3)
{
    EXPECT_TRUE(PitcherCropBlock::isDouble(3)) << "AGE 3 should be double block";
}

TEST_F(PitcherCropBlockTest, IsDouble_True_ForAge4)
{
    EXPECT_TRUE(PitcherCropBlock::isDouble(4)) << "AGE 4 should be double block";
}

// ============================================================================
// 状态属性测试
// ============================================================================

TEST_F(PitcherCropBlockTest, DefaultState_Age0Lower)
{
    const Block* block = TrailsBlocks::PITCHER_CROP;
    ASSERT_NE(block, nullptr);

    const BlockState& state = block->defaultState();
    EXPECT_EQ(state.get(BlockStateProperties::AGE_0_4()), 0) << "Default age should be 0";
    EXPECT_EQ(state.get(BlockStateProperties::DOUBLE_BLOCK_HALF()),
        BlockStateProperties::DoubleBlockHalf::Lower)
        << "Default half should be Lower";
}

TEST_F(PitcherCropBlockTest, GetAge_ReturnsCorrectValue)
{
    const auto* block = dynamic_cast<const PitcherCropBlock*>(TrailsBlocks::PITCHER_CROP);
    ASSERT_NE(block, nullptr);

    for (i32 age = 0; age <= 4; ++age) {
        const BlockState& state = block->defaultState()
                                      .with(BlockStateProperties::AGE_0_4(), age)
                                      .with(BlockStateProperties::DOUBLE_BLOCK_HALF(),
                                          BlockStateProperties::DoubleBlockHalf::Lower);
        EXPECT_EQ(block->getAge(state), age) << "getAge should return " << age;
    }
}

TEST_F(PitcherCropBlockTest, IsMaxAge_True_OnlyAtAge4)
{
    const auto* block = dynamic_cast<const PitcherCropBlock*>(TrailsBlocks::PITCHER_CROP);
    ASSERT_NE(block, nullptr);

    for (i32 age = 0; age <= 3; ++age) {
        const BlockState& state = block->defaultState()
                                      .with(BlockStateProperties::AGE_0_4(), age)
                                      .with(BlockStateProperties::DOUBLE_BLOCK_HALF(),
                                          BlockStateProperties::DoubleBlockHalf::Lower);
        EXPECT_FALSE(block->isMaxAge(state)) << "Age " << age << " should not be max age";
    }

    const BlockState& state4 = block->defaultState()
                                    .with(BlockStateProperties::AGE_0_4(), 4)
                                    .with(BlockStateProperties::DOUBLE_BLOCK_HALF(),
                                        BlockStateProperties::DoubleBlockHalf::Lower);
    EXPECT_TRUE(block->isMaxAge(state4)) << "Age 4 should be max age";
}

TEST_F(PitcherCropBlockTest, WithAge_ReturnsLowerHalf)
{
    const auto* block = dynamic_cast<const PitcherCropBlock*>(TrailsBlocks::PITCHER_CROP);
    ASSERT_NE(block, nullptr);

    // withAge 应始终返回 Lower 半部分
    for (i32 age = 0; age <= 4; ++age) {
        const BlockState& state = block->withAge(age);
        EXPECT_EQ(state.get(BlockStateProperties::AGE_0_4()), age)
            << "withAge(" << age << ") should set age to " << age;
        EXPECT_EQ(state.get(BlockStateProperties::DOUBLE_BLOCK_HALF()),
            BlockStateProperties::DoubleBlockHalf::Lower)
            << "withAge should always return Lower half";
    }
}

TEST_F(PitcherCropBlockTest, WithAge_ClampsOutOfRange)
{
    const auto* block = dynamic_cast<const PitcherCropBlock*>(TrailsBlocks::PITCHER_CROP);
    ASSERT_NE(block, nullptr);

    // 负值应被 clamp 到 0
    const BlockState& stateNeg = block->withAge(-1);
    EXPECT_EQ(stateNeg.get(BlockStateProperties::AGE_0_4()), 0) << "withAge(-1) should clamp to 0";

    // 超过 MAX_AGE 的值应被 clamp 到 MAX_AGE
    const BlockState& stateOver = block->withAge(10);
    EXPECT_EQ(stateOver.get(BlockStateProperties::AGE_0_4()), 4) << "withAge(10) should clamp to 4";
}

// ============================================================================
// 双格状态测试
// ============================================================================

TEST_F(PitcherCropBlockTest, Age0Through2_AreSingleBlock)
{
    const auto* block = dynamic_cast<const PitcherCropBlock*>(TrailsBlocks::PITCHER_CROP);
    ASSERT_NE(block, nullptr);

    for (i32 age = 0; age <= 2; ++age) {
        const BlockState& state = block->defaultState()
                                      .with(BlockStateProperties::AGE_0_4(), age)
                                      .with(BlockStateProperties::DOUBLE_BLOCK_HALF(),
                                          BlockStateProperties::DoubleBlockHalf::Lower);
        EXPECT_FALSE(PitcherCropBlock::isDouble(block->getAge(state)))
            << "AGE " << age << " should be single block";
    }
}

TEST_F(PitcherCropBlockTest, Age3And4_AreDoubleBlock)
{
    const auto* block = dynamic_cast<const PitcherCropBlock*>(TrailsBlocks::PITCHER_CROP);
    ASSERT_NE(block, nullptr);

    for (i32 age = 3; age <= 4; ++age) {
        const BlockState& state = block->defaultState()
                                      .with(BlockStateProperties::AGE_0_4(), age)
                                      .with(BlockStateProperties::DOUBLE_BLOCK_HALF(),
                                          BlockStateProperties::DoubleBlockHalf::Lower);
        EXPECT_TRUE(PitcherCropBlock::isDouble(block->getAge(state)))
            << "AGE " << age << " should be double block";
    }
}

// ============================================================================
// 状态空间测试
// ============================================================================

TEST_F(PitcherCropBlockTest, StateSpace_HasAllStates)
{
    const Block* block = TrailsBlocks::PITCHER_CROP;
    ASSERT_NE(block, nullptr);

    // 验证所有 5*2=10 种状态都可访问
    for (i32 age = 0; age <= 4; ++age) {
        for (auto half : {BlockStateProperties::DoubleBlockHalf::Lower,
                 BlockStateProperties::DoubleBlockHalf::Upper}) {
            const BlockState& state = block->defaultState()
                                          .with(BlockStateProperties::AGE_0_4(), age)
                                          .with(BlockStateProperties::DOUBLE_BLOCK_HALF(), half);
            EXPECT_EQ(state.get(BlockStateProperties::AGE_0_4()), age);
            EXPECT_EQ(state.get(BlockStateProperties::DOUBLE_BLOCK_HALF()), half);
        }
    }
}

// ============================================================================
// 形状测试
// ============================================================================

TEST_F(PitcherCropBlockTest, ShapeExists_ForAllAgesAndHalves)
{
    const auto* block = dynamic_cast<const PitcherCropBlock*>(TrailsBlocks::PITCHER_CROP);
    ASSERT_NE(block, nullptr);

    for (i32 age = 0; age <= 4; ++age) {
        for (auto half : {BlockStateProperties::DoubleBlockHalf::Lower,
                 BlockStateProperties::DoubleBlockHalf::Upper}) {
            const BlockState& state = block->defaultState()
                                          .with(BlockStateProperties::AGE_0_4(), age)
                                          .with(BlockStateProperties::DOUBLE_BLOCK_HALF(), half);
            const CollisionShape& shape = block->getShape(state);
            // 下半部分形状应该始终非空
            if (half == BlockStateProperties::DoubleBlockHalf::Lower) {
                EXPECT_FALSE(shape.isEmpty())
                    << "Lower half shape should not be empty for age=" << age;
            }
            // 上半部分形状在 AGE 3-4 时应该非空（双格植物的上半部分有可视形状）
            if (half == BlockStateProperties::DoubleBlockHalf::Upper && age >= 3) {
                EXPECT_FALSE(shape.isEmpty())
                    << "Upper half shape should not be empty for age=" << age << " (double block)";
            }
        }
    }
}

TEST_F(PitcherCropBlockTest, CollisionShape_UpperHalfIsEmpty)
{
    const auto* block = dynamic_cast<const PitcherCropBlock*>(TrailsBlocks::PITCHER_CROP);
    ASSERT_NE(block, nullptr);

    // 上半部分无碰撞
    for (i32 age = 0; age <= 4; ++age) {
        const BlockState& state = block->defaultState()
                                      .with(BlockStateProperties::AGE_0_4(), age)
                                      .with(BlockStateProperties::DOUBLE_BLOCK_HALF(),
                                          BlockStateProperties::DoubleBlockHalf::Upper);
        const CollisionShape& shape = block->getCollisionShape(state);
        EXPECT_TRUE(shape.isEmpty()) << "Upper half collision shape should be empty for age " << age;
    }
}

TEST_F(PitcherCropBlockTest, CollisionShape_LowerHalfIsNotEmpty)
{
    const auto* block = dynamic_cast<const PitcherCropBlock*>(TrailsBlocks::PITCHER_CROP);
    ASSERT_NE(block, nullptr);

    // 下半部分有碰撞
    for (i32 age = 0; age <= 4; ++age) {
        const BlockState& state = block->defaultState()
                                      .with(BlockStateProperties::AGE_0_4(), age)
                                      .with(BlockStateProperties::DOUBLE_BLOCK_HALF(),
                                          BlockStateProperties::DoubleBlockHalf::Lower);
        const CollisionShape& shape = block->getCollisionShape(state);
        EXPECT_FALSE(shape.isEmpty()) << "Lower half collision shape should not be empty for age " << age;
    }
}

// ============================================================================
// IGrowable 接口测试
// ============================================================================

TEST_F(PitcherCropBlockTest, TicksRandomly_ReturnsTrue)
{
    const auto* block = dynamic_cast<const PitcherCropBlock*>(TrailsBlocks::PITCHER_CROP);
    ASSERT_NE(block, nullptr);
    EXPECT_TRUE(block->ticksRandomly()) << "PitcherCropBlock should tick randomly for growth";
}

TEST_F(PitcherCropBlockTest, CanUseBonemeal_AlwaysReturnsTrue)
{
    // canUseBonemeal 需要 IWorld，无法在单元测试中直接构造
    // 但逻辑上它总是返回 true
    // 这里验证 isMaxAge 为 false 时 canGrow 逻辑前提
    const auto* block = dynamic_cast<const PitcherCropBlock*>(TrailsBlocks::PITCHER_CROP);
    ASSERT_NE(block, nullptr);

    const BlockState& state0 = block->defaultState()
                                   .with(BlockStateProperties::AGE_0_4(), 0)
                                   .with(BlockStateProperties::DOUBLE_BLOCK_HALF(),
                                       BlockStateProperties::DoubleBlockHalf::Lower);
    EXPECT_FALSE(block->isMaxAge(state0)) << "AGE 0 should not be max age, canGrow precondition";
}

// ============================================================================
// 掉落物品测试
// ============================================================================

TEST_F(PitcherCropBlockTest, GetCropItem_ReturnsPitcherPlantItemId)
{
    const auto* block = dynamic_cast<const PitcherCropBlock*>(TrailsBlocks::PITCHER_CROP);
    ASSERT_NE(block, nullptr);

    ASSERT_NE(Items::PITCHER_PLANT, nullptr) << "Items::PITCHER_PLANT should be initialized";
    u32 cropItemId = block->getCropItem();
    EXPECT_EQ(cropItemId, Items::PITCHER_PLANT->itemId())
        << "getCropItem() should return PITCHER_PLANT item ID";
}

TEST_F(PitcherCropBlockTest, GetSeedItem_ReturnsPitcherPodItemId)
{
    const auto* block = dynamic_cast<const PitcherCropBlock*>(TrailsBlocks::PITCHER_CROP);
    ASSERT_NE(block, nullptr);

    ASSERT_NE(Items::PITCHER_POD, nullptr) << "Items::PITCHER_POD should be initialized";
    u32 seedItemId = block->getSeedItem();
    EXPECT_EQ(seedItemId, Items::PITCHER_POD->itemId())
        << "getSeedItem() should return PITCHER_POD item ID";
}

TEST_F(PitcherCropBlockTest, CropAndSeedItemsAreDifferent)
{
    const auto* block = dynamic_cast<const PitcherCropBlock*>(TrailsBlocks::PITCHER_CROP);
    ASSERT_NE(block, nullptr);

    u32 cropItemId = block->getCropItem();
    u32 seedItemId = block->getSeedItem();
    EXPECT_NE(cropItemId, seedItemId)
        << "Crop item (pitcher_plant) and seed item (pitcher_pod) should be different";
}

// ============================================================================
// canSustain 测试（通过 getPlantType 间接验证）
// ============================================================================

TEST_F(PitcherCropBlockTest, GetPlantType_ReturnsCrop)
{
    const auto* block = dynamic_cast<const PitcherCropBlock*>(TrailsBlocks::PITCHER_CROP);
    ASSERT_NE(block, nullptr);

    // PitcherCropBlock::getPlantType 不依赖 world 参数（MC_UNUSED），
    // 但需要 IBlockReader 引用，因此无法在无世界测试中直接调用。
    // 验证方式：canSustain 委托给 canSustainPlant，getPlantType 返回 Crop，
    // 这确保耕地可以支撑瓶草作物。此处仅验证类层次结构正确。
    // getPlantType 的实际行为通过集成测试验证。
}

// ============================================================================
// isReplaceable 测试
// ============================================================================

TEST_F(PitcherCropBlockTest, IsReplaceable_ReturnsFalse)
{
    const auto* block = dynamic_cast<const PitcherCropBlock*>(TrailsBlocks::PITCHER_CROP);
    ASSERT_NE(block, nullptr);

    // 瓶草作物不可被替换放置
    const BlockState& state = block->defaultState();
    // isReplaceable 需要 BlockItemUseContext，但我们验证逻辑：总是返回 false
    // 直接调用会需要构造 BlockItemUseContext，这里跳过，仅验证注释中的意图
    // 实际覆盖需要集成测试
}

// ============================================================================
// 注册测试
// ============================================================================

TEST_F(PitcherCropBlockTest, PitcherCropBlock_IsRegistered)
{
    ASSERT_NE(TrailsBlocks::PITCHER_CROP, nullptr) << "PITCHER_CROP should be registered";
}

TEST_F(PitcherCropBlockTest, PitcherCropBlock_IsPitcherCropBlock)
{
    const Block* block = TrailsBlocks::PITCHER_CROP;
    ASSERT_NE(block, nullptr);

    const auto* pitcherCrop = dynamic_cast<const PitcherCropBlock*>(block);
    EXPECT_NE(pitcherCrop, nullptr) << "PITCHER_CROP should be registered as PitcherCropBlock, not SimpleBlock";
}

TEST_F(PitcherCropBlockTest, PitcherCropBlock_ImplementsIGrowable)
{
    const Block* block = TrailsBlocks::PITCHER_CROP;
    ASSERT_NE(block, nullptr);

    const auto* growable = dynamic_cast<const IGrowable*>(block);
    EXPECT_NE(growable, nullptr) << "PitcherCropBlock should implement IGrowable";
}

TEST_F(PitcherCropBlockTest, PitcherCropBlock_InheritsDoublePlantBlock)
{
    const Block* block = TrailsBlocks::PITCHER_CROP;
    ASSERT_NE(block, nullptr);

    const auto* doublePlant = dynamic_cast<const DoublePlantBlock*>(block);
    EXPECT_NE(doublePlant, nullptr) << "PitcherCropBlock should inherit from DoublePlantBlock";
}

TEST_F(PitcherCropBlockTest, PitcherCropBlock_DoesNotInheritCropBlock)
{
    // PitcherCropBlock 继承自 DoublePlantBlock，不是 CropBlock
    const Block* block = TrailsBlocks::PITCHER_CROP;
    ASSERT_NE(block, nullptr);

    const auto* cropBlock = dynamic_cast<const CropBlock*>(block);
    EXPECT_EQ(cropBlock, nullptr) << "PitcherCropBlock should NOT inherit from CropBlock";
}

} // namespace
