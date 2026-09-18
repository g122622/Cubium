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
