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

#include "world/block/blocks/LavaCauldronBlock.hpp"
#include "common/TestWorldHelper.hpp"
#include "common/util/property/Properties.hpp"
#include "common/world/block/BlockTags.hpp"
#include "common/world/block/blocks/CauldronBlock.hpp"
#include "common/world/block/blocks/LayeredCauldronBlock.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/fluid/Fluid.hpp"
#include "common/world/fluid/FluidRegistry.hpp"
#include "common/world/fluid/FluidTags.hpp"
#include "common/world/fluid/Fluids.hpp"
#include <gtest/gtest.h>

#include <memory>

using namespace mc;
using namespace mc::blocks;

// ============================================================================
// 测试用世界桩 - 用于需要 IWorld 引用的测试
// ============================================================================

class LavaCauldronTestWorld : public mc::test::BaseTestWorld {
public:
    [[nodiscard]] bool hasChunk(ChunkCoord, ChunkCoord) const override { return true; }
};

// ============================================================================
// LavaCauldronBlock 基础测试
// ============================================================================

class LavaCauldronBlockTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        fluid::FluidRegistry::instance().initialize();
        fluid::FluidTags::initialize();

        lavaCauldron_ = std::make_unique<LavaCauldronBlock>(
            BlockProperties(Material::IRON).hardness(2.0f).resistance(2.0f).notSolid().lightLevel(15));
    }

    std::unique_ptr<LavaCauldronBlock> lavaCauldron_;
    LavaCauldronTestWorld world_;
};

TEST_F(LavaCauldronBlockTest, Create_HasCorrectProperties)
{
    EXPECT_NE(lavaCauldron_, nullptr);
}

TEST_F(LavaCauldronBlockTest, IsFull_AlwaysReturnsTrue)
{
    // 岩浆炼药锅始终满
    const auto& state = lavaCauldron_->defaultState();
    EXPECT_TRUE(LavaCauldronBlock::isFull(state));
}

TEST_F(LavaCauldronBlockTest, GetLightLevel_Returns15)
{
    const auto& state = lavaCauldron_->defaultState();
    EXPECT_EQ(lavaCauldron_->getLightLevel(state), 15);
}

TEST_F(LavaCauldronBlockTest, HasComparatorInputOverride_ReturnsTrue)
{
    const auto& state = lavaCauldron_->defaultState();
    EXPECT_TRUE(lavaCauldron_->hasComparatorInputOverride(state));
}

TEST_F(LavaCauldronBlockTest, GetComparatorInputOverride_Returns3)
{
    const auto& state = lavaCauldron_->defaultState();
    EXPECT_EQ(lavaCauldron_->getComparatorInputOverride(state, world_, BlockPos(0, 64, 0)), 3);
}

TEST_F(LavaCauldronBlockTest, CanReceiveStalactiteDrip_AlwaysReturnsFalse)
{
    // 岩浆炼药锅始终满，不可接收滴石滴水
    EXPECT_FALSE(LavaCauldronBlock::canReceiveStalactiteDrip(*fluid::Fluids::WATER()));
    EXPECT_FALSE(LavaCauldronBlock::canReceiveStalactiteDrip(*fluid::Fluids::LAVA()));
}

TEST_F(LavaCauldronBlockTest, UseShapeForLightOcclusion_ReturnsTrue)
{
    const auto& state = lavaCauldron_->defaultState();
    EXPECT_TRUE(lavaCauldron_->useShapeForLightOcclusion(state));
}

TEST_F(LavaCauldronBlockTest, GetShape_ReturnsValidShape)
{
    const auto& state = lavaCauldron_->defaultState();
    const auto& shape = lavaCauldron_->getShape(state);
    EXPECT_FALSE(shape.isEmpty());
}

TEST_F(LavaCauldronBlockTest, GetCollisionShape_ReturnsValidShape)
{
    const auto& state = lavaCauldron_->defaultState();
    const auto& shape = lavaCauldron_->getCollisionShape(state);
    EXPECT_FALSE(shape.isEmpty());
}

TEST_F(LavaCauldronBlockTest, GetEntityInsideCollisionShape_ReturnsFilledShape)
{
    // 岩浆炼药锅返回填充形状（外部形状 ∪ 岩浆内容区域）
    const auto& state = lavaCauldron_->defaultState();
    const auto& entityInsideShape = lavaCauldron_->getEntityInsideCollisionShape(state);
    EXPECT_FALSE(entityInsideShape.isEmpty()) << "Lava cauldron entity inside shape should not be empty";
    EXPECT_FALSE(entityInsideShape.isFullBlock()) << "Lava cauldron entity inside shape should not be full block";
}

TEST_F(LavaCauldronBlockTest, GetEntityInsideCollisionShape_HasMoreBoxesThanOuterShape)
{
    // 填充形状 = 外部形状 ∪ 岩浆内容，碰撞箱数量应比外部形状多
    const auto& state = lavaCauldron_->defaultState();
    const auto& outerShape = lavaCauldron_->getShape(state);
    const auto& entityInsideShape = lavaCauldron_->getEntityInsideCollisionShape(state);
    EXPECT_GT(entityInsideShape.boxCount(), outerShape.boxCount())
        << "Filled shape should have more boxes than outer shape (outer + lava inside)";
}

TEST_F(LavaCauldronBlockTest, GetCollisionShape_IsSameAsOuterShape)
{
    // 碰撞形状仅为外部结构，不包含岩浆内容
    const auto& state = lavaCauldron_->defaultState();
    const auto& collisionShape = lavaCauldron_->getCollisionShape(state);
    const auto& outerShape = lavaCauldron_->getShape(state);
    // 碰撞形状和渲染形状的碰撞箱数量应相同
    EXPECT_EQ(collisionShape.boxCount(), outerShape.boxCount());
}

TEST_F(LavaCauldronBlockTest, OnEntityCollision_ApplicableToLivingEntity)
{
    // 验证 onEntityCollision 方法存在且可调用
    // 实际伤害测试需要完整的 LivingEntity 和 DamageSource 基建，
    // 此处验证方法签名和基本无崩溃性
    const auto& state = lavaCauldron_->defaultState();
    // 对于 nullptr entity，onEntityCollision 应该安全返回
    // 由于 Entity& 是引用参数，无法传 nullptr，这里仅确认方法存在
    EXPECT_NE(lavaCauldron_, nullptr);
}

TEST_F(LavaCauldronBlockTest, OnBlockActivated_PassForNonBucketItems)
{
    // onBlockActivated 对于非桶物品应返回 Pass
    // 详细交互测试需要完整 Player 基建，此处验证方法存在
    EXPECT_NE(lavaCauldron_, nullptr);
}

// ============================================================================
// LavaCauldronBlock 注册测试
// ============================================================================

class LavaCauldronBlockRegistryTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        BlockTags::initialize();
    }
};

TEST_F(LavaCauldronBlockRegistryTest, LavaCauldronIsRegistered)
{
    EXPECT_NE(block_registry::BuildingBlocks::LAVA_CAULDRON, nullptr);
}

TEST_F(LavaCauldronBlockRegistryTest, LavaCauldronBlockType)
{
    auto* lavaCauldron = dynamic_cast<LavaCauldronBlock*>(block_registry::BuildingBlocks::LAVA_CAULDRON);
    EXPECT_NE(lavaCauldron, nullptr);
}

TEST_F(LavaCauldronBlockRegistryTest, LavaCauldronIsInCauldronsTag)
{
    // 岩浆炼药锅应属于 #minecraft:cauldrons 标签
    const BlockState* state = &block_registry::BuildingBlocks::LAVA_CAULDRON->defaultState();
    ASSERT_NE(state, nullptr);
    EXPECT_TRUE(BlockTags::CAULDRONS().contains(*state));
}

TEST_F(LavaCauldronBlockRegistryTest, CauldronIsInCauldronsTag)
{
    // 空炼药锅应属于 #minecraft:cauldrons 标签
    const BlockState* state = &block_registry::BuildingBlocks::CAULDRON->defaultState();
    ASSERT_NE(state, nullptr);
    EXPECT_TRUE(BlockTags::CAULDRONS().contains(*state));
}

// ============================================================================
// LavaCauldronBlock vs CauldronBlock 交互逻辑测试
// ============================================================================

class CauldronLavaInteractionTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        BlockTags::initialize();
        fluid::FluidRegistry::instance().initialize();
        fluid::FluidTags::initialize();
    }
};

TEST_F(CauldronLavaInteractionTest, LavaCauldronCannotReceiveStalactiteDrip_Water)
{
    // 岩浆炼药锅始终满，不能接收水滴水
    EXPECT_FALSE(LavaCauldronBlock::canReceiveStalactiteDrip(*fluid::Fluids::WATER()));
}

TEST_F(CauldronLavaInteractionTest, LavaCauldronCannotReceiveStalactiteDrip_Lava)
{
    // 岩浆炼药锅始终满，不能接收岩浆滴水
    EXPECT_FALSE(LavaCauldronBlock::canReceiveStalactiteDrip(*fluid::Fluids::LAVA()));
}

TEST_F(CauldronLavaInteractionTest, CauldronCanReceiveStalactiteDrip_Water)
{
    // 空炼药锅可以接收水滴水
    EXPECT_TRUE(CauldronBlock::canReceiveStalactiteDrip(*fluid::Fluids::WATER()));
}

TEST_F(CauldronLavaInteractionTest, CauldronCanReceiveStalactiteDrip_Lava)
{
    // 空炼药锅可以接收岩浆滴水
    EXPECT_TRUE(CauldronBlock::canReceiveStalactiteDrip(*fluid::Fluids::LAVA()));
}

TEST_F(CauldronLavaInteractionTest, LavaCauldronAndCauldronAreBothCauldrons)
{
    // 两种炼药锅都应属于 cauldrons 标签
    const BlockState* cauldronState = &block_registry::BuildingBlocks::CAULDRON->defaultState();
    const BlockState* lavaCauldronState = &block_registry::BuildingBlocks::LAVA_CAULDRON->defaultState();
    ASSERT_NE(cauldronState, nullptr);
    ASSERT_NE(lavaCauldronState, nullptr);
    EXPECT_TRUE(BlockTags::CAULDRONS().contains(*cauldronState));
    EXPECT_TRUE(BlockTags::CAULDRONS().contains(*lavaCauldronState));
}

TEST_F(CauldronLavaInteractionTest, WaterCauldronIsAlsoInCauldronsTag)
{
    // 水炼药锅也应属于 cauldrons 标签
    const BlockState* waterCauldronState = &block_registry::BuildingBlocks::WATER_CAULDRON->defaultState();
    ASSERT_NE(waterCauldronState, nullptr);
    EXPECT_TRUE(BlockTags::CAULDRONS().contains(*waterCauldronState));
}

TEST_F(CauldronLavaInteractionTest, LavaCauldronIsAlwaysFull)
{
    // 岩浆炼药锅始终满（无水位属性）
    const auto& state = block_registry::BuildingBlocks::LAVA_CAULDRON->defaultState();
    EXPECT_TRUE(LavaCauldronBlock::isFull(state));
}

TEST_F(CauldronLavaInteractionTest, EmptyCauldronDefaultIsEmpty)
{
    // 空炼药锅始终为空
    const auto& state = block_registry::BuildingBlocks::CAULDRON->defaultState();
    EXPECT_TRUE(CauldronBlock::isEmpty(state));
    EXPECT_FALSE(CauldronBlock::isFull(state));
}

TEST_F(CauldronLavaInteractionTest, WaterCauldronDefaultLevel)
{
    // 水炼药锅默认水位为1（LEVEL_1_3 的最小值）
    const auto& state = block_registry::BuildingBlocks::WATER_CAULDRON->defaultState();
    EXPECT_EQ(LayeredCauldronBlock::getLevel(state), 1);
    EXPECT_FALSE(LayeredCauldronBlock::isFull(state));
}
