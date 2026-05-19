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

#include "world/block/blocks/CauldronBlock.hpp"
#include "util/property/Properties.hpp"
#include "world/block/BlockRegistry.hpp"
#include <gtest/gtest.h>

using namespace mc;
using namespace mc::blocks;

// ========== CauldronBlock 测试 ==========

class CauldronBlockTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        // 创建炼药锅方块
        cauldron_ = std::make_unique<CauldronBlock>(BlockProperties(Material::ROCK).hardness(2.0f).resistance(2.0f));
    }

    std::unique_ptr<CauldronBlock> cauldron_;
};

TEST_F(CauldronBlockTest, Create_HasCorrectProperties)
{
    EXPECT_NE(cauldron_, nullptr);
}

TEST_F(CauldronBlockTest, GetLevel_ReturnsZeroByDefault)
{
    const auto& state = cauldron_->defaultState();
    EXPECT_EQ(CauldronBlock::getLevel(state), 0);
}

TEST_F(CauldronBlockTest, IsEmpty_ReturnsTrueForDefault)
{
    const auto& state = cauldron_->defaultState();
    EXPECT_TRUE(CauldronBlock::isEmpty(state));
}

TEST_F(CauldronBlockTest, IsFull_ReturnsFalseForDefault)
{
    const auto& state = cauldron_->defaultState();
    EXPECT_FALSE(CauldronBlock::isFull(state));
}

TEST_F(CauldronBlockTest, HasComparatorInputOverride_ReturnsTrue)
{
    const auto& state = cauldron_->defaultState();
    EXPECT_TRUE(cauldron_->hasComparatorInputOverride(state));
}

TEST_F(CauldronBlockTest, TicksRandomly_ReturnsTrue)
{
    EXPECT_TRUE(cauldron_->ticksRandomly());
}

TEST_F(CauldronBlockTest, GetShape_ReturnsValidShape)
{
    const auto& state = cauldron_->defaultState();
    const auto& shape = cauldron_->getShape(state);
    EXPECT_FALSE(shape.isEmpty());
}

TEST_F(CauldronBlockTest, GetCollisionShape_ReturnsValidShape)
{
    const auto& state = cauldron_->defaultState();
    const auto& shape = cauldron_->getCollisionShape(state);
    EXPECT_FALSE(shape.isEmpty());
}

TEST_F(CauldronBlockTest, GetContentShape_ReturnsValidShapeForAllLevels)
{
    for (i32 level = 0; level <= 3; ++level) {
        const auto& shape = cauldron_->getContentShape(level);
        // 内容形状可以为空（水位0）
        if (level > 0) {
            EXPECT_FALSE(shape.isEmpty()) << "Level " << level << " should have content shape";
        }
    }
}
