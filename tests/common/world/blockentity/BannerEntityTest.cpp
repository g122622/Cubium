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

#include "world/blockentity/interactive/BannerEntity.hpp"
#include "util/color/DyeColor.hpp"
#include <gtest/gtest.h>

using namespace mc;
using namespace mc::blockentity;

// ========== BannerEntity 测试 ==========

class BannerEntityTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        pos_ = BlockPos(10, 64, 20);
        entity_ = std::make_unique<BannerEntity>(pos_);
    }

    BlockPos pos_;
    std::unique_ptr<BannerEntity> entity_;
};

TEST_F(BannerEntityTest, DefaultConstruction)
{
    EXPECT_EQ(entity_->getBaseColor(), DyeColor::Black);
    EXPECT_EQ(entity_->getPatternCount(), 0);
    EXPECT_FALSE(entity_->hasCustomDisplayName());
}

TEST_F(BannerEntityTest, SetBaseColor)
{
    entity_->setBaseColor(DyeColor::Red);
    EXPECT_EQ(entity_->getBaseColor(), DyeColor::Red);

    entity_->setBaseColor(DyeColor::White);
    EXPECT_EQ(entity_->getBaseColor(), DyeColor::White);
}

TEST_F(BannerEntityTest, AddPatterns)
{
    entity_->addPattern(BannerPatternType::StripeBottom, DyeColor::White);
    EXPECT_EQ(entity_->getPatternCount(), 1);

    entity_->addPattern(BannerPatternType::Cross, DyeColor::Red);
    EXPECT_EQ(entity_->getPatternCount(), 2);
}

TEST_F(BannerEntityTest, MaxPatternsLimit)
{
    // 最多6层图案
    for (i32 i = 0; i < 6; ++i) {
        entity_->addPattern(BannerPatternType::StripeBottom, DyeColor::White);
    }
    EXPECT_EQ(entity_->getPatternCount(), 6);

    // 超过6层应该失败
    bool added = entity_->addPattern(BannerPatternType::Cross, DyeColor::Red);
    EXPECT_FALSE(added);
    EXPECT_EQ(entity_->getPatternCount(), 6);
}

TEST_F(BannerEntityTest, CustomDisplayName)
{
    EXPECT_FALSE(entity_->hasCustomDisplayName());

    entity_->setCustomDisplayName("My Banner");
    EXPECT_TRUE(entity_->hasCustomDisplayName());
    EXPECT_EQ(entity_->getCustomDisplayName(), "My Banner");
}

TEST_F(BannerEntityTest, GetPatternsFromItemStack)
{
    // 空ItemStack应返回空图案列表
    ItemStack emptyStack;
    auto patterns = BannerEntity::getPatternsFromItemStack(emptyStack);
    EXPECT_TRUE(patterns.empty());
}

TEST_F(BannerEntityTest, GetPatternCountEmptyStack)
{
    ItemStack emptyStack;
    EXPECT_EQ(BannerEntity::getPatternCount(emptyStack), 0);
}

TEST_F(BannerEntityTest, RemoveBannerData)
{
    // 测试空物品不影响
    ItemStack emptyStack;
    BannerEntity::removeBannerData(emptyStack);
    // 无崩溃即通过
}
