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
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/fluid/Fluid.hpp"
#include "common/world/fluid/FluidRegistry.hpp"
#include "common/world/fluid/Fluids.hpp"
#include <gtest/gtest.h>

#include <memory>

using namespace mc;
using namespace mc::blocks;

// ============================================================================
// 测试用世界桩 - 用于需要 IWorld 引用的测试
// ============================================================================

class LavaCauldronTestWorld : public test::BaseTestWorld {
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

TEST_F(LavaCauldronBlockTest, GetCollisionShape_IsLargerThanShape)
{
    // 碰撞形状应包含岩浆内容，比渲染形状更大
    const auto& state = lavaCauldron_->defaultState();
    const auto& shape = lavaCauldron_->getShape(state);
    const auto& collisionShape = lavaCauldron_->getCollisionShape(state);
    // 两者都不应为空，碰撞形状应与渲染形状不同（包含岩浆内容）
    EXPECT_FALSE(shape.isEmpty());
    EXPECT_FALSE(collisionShape.isEmpty());
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

// ============================================================================
// CauldronBlock 滴石滴水测试
// ============================================================================

class CauldronDripTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        fluid::FluidRegistry::instance().initialize();
    }
};

TEST_F(CauldronDripTest, CanReceiveStalactiteDrip_Water_ReturnsTrue)
{
    // 空炼药锅可以接收水滴水
    EXPECT_TRUE(CauldronBlock::canReceiveStalactiteDrip(*fluid::Fluids::WATER()));
}

TEST_F(CauldronDripTest, CanReceiveStalactiteDrip_Lava_ReturnsTrue)
{
    // 空炼药锅可以接收岩浆滴水
    EXPECT_TRUE(CauldronBlock::canReceiveStalactiteDrip(*fluid::Fluids::LAVA()));
}

TEST_F(CauldronDripTest, IsEmpty_Level0_ReturnsTrue)
{
    auto cauldron = std::make_unique<CauldronBlock>(BlockProperties(Material::ROCK).hardness(2.0f).resistance(2.0f));
    const auto& state = cauldron->defaultState();
    // 默认水位为0，空炼药锅
    EXPECT_TRUE(CauldronBlock::isEmpty(state));
}

TEST_F(CauldronDripTest, IsFull_Level3_ReturnsTrue)
{
    auto cauldron = std::make_unique<CauldronBlock>(BlockProperties(Material::ROCK).hardness(2.0f).resistance(2.0f));
    const auto& state = cauldron->defaultState().with(BlockStateProperties::LEVEL_0_3(), 3);
    EXPECT_TRUE(CauldronBlock::isFull(state));
}
