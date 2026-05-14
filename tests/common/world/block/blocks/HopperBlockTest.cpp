#include "world/block/blocks/HopperBlock.hpp"
#include "util/property/Properties.hpp"
#include "world/block/BlockRegistry.hpp"
#include "world/blockentity/BlockEntity.hpp"
#include <gtest/gtest.h>

using namespace mc;
using namespace mc::blocks;

// ========== HopperBlock 测试 ==========

class HopperBlockTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        // 创建漏斗方块
        hopper_ = std::make_unique<HopperBlock>(BlockProperties(Material::WOOD).hardness(3.0f).resistance(4.8f));
    }

    std::unique_ptr<HopperBlock> hopper_;
};

TEST_F(HopperBlockTest, Create_HasCorrectProperties)
{
    EXPECT_NE(hopper_, nullptr);
}

TEST_F(HopperBlockTest, HasBlockEntity_ReturnsTrue)
{
    EXPECT_TRUE(hopper_->hasBlockEntity());
}

TEST_F(HopperBlockTest, GetFacing_ReturnsDownByDefault)
{
    const auto& state = hopper_->defaultState();
    EXPECT_EQ(HopperBlock::getFacing(state), Direction::Down);
}

TEST_F(HopperBlockTest, IsEnabled_ReturnsTrueByDefault)
{
    const auto& state = hopper_->defaultState();
    EXPECT_TRUE(HopperBlock::isEnabled(state));
}

TEST_F(HopperBlockTest, GetShape_ReturnsValidShape)
{
    const auto& state = hopper_->defaultState();
    const auto& shape = hopper_->getShape(state);
    EXPECT_FALSE(shape.isEmpty());
}

TEST_F(HopperBlockTest, GetCollisionShape_ReturnsValidShape)
{
    const auto& state = hopper_->defaultState();
    const auto& shape = hopper_->getCollisionShape(state);
    EXPECT_FALSE(shape.isEmpty());
}

TEST_F(HopperBlockTest, FacingDown_IsValidOutputDirection)
{
    const auto& state = hopper_->defaultState();
    EXPECT_EQ(HopperBlock::getFacing(state), Direction::Down);
}

TEST_F(HopperBlockTest, HasComparatorInputOverride_ReturnsTrue)
{
    const auto& state = hopper_->defaultState();
    EXPECT_TRUE(hopper_->hasComparatorInputOverride(state));
}

TEST_F(HopperBlockTest, GetBlockEntityType_ReturnsHopper)
{
    EXPECT_EQ(hopper_->getBlockEntityType(), BlockEntityType::Hopper);
}
