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

#include "common/entity/entities/passive/basic/SheepEntity.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/block/VanillaBlocks.hpp"

using namespace mc;

// ============================================================================
// SheepEntity 颜色混合测试
// ============================================================================

class SheepColorMixingTest : public ::testing::Test {
protected:
    void SetUp() override { VanillaBlocks::initialize(); }
};

TEST_F(SheepColorMixingTest, SameColorReturnsSameColor)
{
    math::Random rng(42);

    // 相同颜色应该返回相同颜色
    EXPECT_EQ(SheepEntity::getDyeColorMixFromParents(DyeColor::White, DyeColor::White, rng), DyeColor::White);
    EXPECT_EQ(SheepEntity::getDyeColorMixFromParents(DyeColor::Red, DyeColor::Red, rng), DyeColor::Red);
    EXPECT_EQ(SheepEntity::getDyeColorMixFromParents(DyeColor::Black, DyeColor::Black, rng), DyeColor::Black);
}

TEST_F(SheepColorMixingTest, WhiteAndRedMakesPink)
{
    math::Random rng(42);

    // 白色 + 红色 = 粉红色
    EXPECT_EQ(SheepEntity::getDyeColorMixFromParents(DyeColor::White, DyeColor::Red, rng), DyeColor::Pink);
    EXPECT_EQ(SheepEntity::getDyeColorMixFromParents(DyeColor::Red, DyeColor::White, rng), DyeColor::Pink);
}

TEST_F(SheepColorMixingTest, RedAndYellowMakesOrange)
{
    math::Random rng(42);

    // 红色 + 黄色 = 橙色
    EXPECT_EQ(SheepEntity::getDyeColorMixFromParents(DyeColor::Red, DyeColor::Yellow, rng), DyeColor::Orange);
    EXPECT_EQ(SheepEntity::getDyeColorMixFromParents(DyeColor::Yellow, DyeColor::Red, rng), DyeColor::Orange);
}

TEST_F(SheepColorMixingTest, WhiteAndBlueMakesLightBlue)
{
    math::Random rng(42);

    // 白色 + 蓝色 = 淡蓝色
    EXPECT_EQ(SheepEntity::getDyeColorMixFromParents(DyeColor::White, DyeColor::Blue, rng), DyeColor::LightBlue);
    EXPECT_EQ(SheepEntity::getDyeColorMixFromParents(DyeColor::Blue, DyeColor::White, rng), DyeColor::LightBlue);
}

TEST_F(SheepColorMixingTest, BlueAndGreenMakesCyan)
{
    math::Random rng(42);

    // 蓝色 + 绿色 = 青色
    EXPECT_EQ(SheepEntity::getDyeColorMixFromParents(DyeColor::Blue, DyeColor::Green, rng), DyeColor::Cyan);
    EXPECT_EQ(SheepEntity::getDyeColorMixFromParents(DyeColor::Green, DyeColor::Blue, rng), DyeColor::Cyan);
}

TEST_F(SheepColorMixingTest, BlueAndRedMakesPurple)
{
    math::Random rng(42);

    // 蓝色 + 红色 = 紫色
    EXPECT_EQ(SheepEntity::getDyeColorMixFromParents(DyeColor::Blue, DyeColor::Red, rng), DyeColor::Purple);
    EXPECT_EQ(SheepEntity::getDyeColorMixFromParents(DyeColor::Red, DyeColor::Blue, rng), DyeColor::Purple);
}

TEST_F(SheepColorMixingTest, WhiteAndGreenMakesLime)
{
    math::Random rng(42);

    // 白色 + 绿色 = 黄绿色
    EXPECT_EQ(SheepEntity::getDyeColorMixFromParents(DyeColor::White, DyeColor::Green, rng), DyeColor::Lime);
    EXPECT_EQ(SheepEntity::getDyeColorMixFromParents(DyeColor::Green, DyeColor::White, rng), DyeColor::Lime);
}

TEST_F(SheepColorMixingTest, WhiteAndBlackMakesGray)
{
    math::Random rng(42);

    // 白色 + 黑色 = 灰色
    EXPECT_EQ(SheepEntity::getDyeColorMixFromParents(DyeColor::White, DyeColor::Black, rng), DyeColor::Gray);
    EXPECT_EQ(SheepEntity::getDyeColorMixFromParents(DyeColor::Black, DyeColor::White, rng), DyeColor::Gray);
}

TEST_F(SheepColorMixingTest, GrayAndWhiteMakesLightGray)
{
    math::Random rng(42);

    // 灰色 + 白色 = 淡灰色
    EXPECT_EQ(SheepEntity::getDyeColorMixFromParents(DyeColor::Gray, DyeColor::White, rng), DyeColor::LightGray);
    EXPECT_EQ(SheepEntity::getDyeColorMixFromParents(DyeColor::White, DyeColor::Gray, rng), DyeColor::LightGray);
}

TEST_F(SheepColorMixingTest, NoMixingResultReturnsParentColor)
{
    // 当没有混合配方时，应该随机返回父母颜色之一
    // 使用固定种子的随机数生成器，nextBoolean() 返回确定性结果
    math::Random rng(12345);

    // 选择一对没有混合配方的颜色（例如棕色和粉色）
    DyeColor result = SheepEntity::getDyeColorMixFromParents(DyeColor::Brown, DyeColor::Pink, rng);

    // 结果应该是父母颜色之一
    EXPECT_TRUE(result == DyeColor::Brown || result == DyeColor::Pink);
}

// ============================================================================
// SheepEntity 基础测试
// ============================================================================

class SheepEntityTest : public ::testing::Test {
protected:
    void SetUp() override { VanillaBlocks::initialize(); }
};

TEST_F(SheepEntityTest, InitialState)
{
    SheepEntity sheep(LegacyEntityType::Unknown, 1);

    // 初始状态
    EXPECT_FALSE(sheep.isSheared());
    EXPECT_EQ(sheep.getFleeceColor(), DyeColor::White);
    EXPECT_FALSE(sheep.isChild());
}

TEST_F(SheepEntityTest, SetFleeceColor)
{
    SheepEntity sheep(LegacyEntityType::Unknown, 1);

    sheep.setFleeceColor(DyeColor::Red);
    EXPECT_EQ(sheep.getFleeceColor(), DyeColor::Red);

    sheep.setFleeceColor(DyeColor::Black);
    EXPECT_EQ(sheep.getFleeceColor(), DyeColor::Black);
}

TEST_F(SheepEntityTest, SetSheared)
{
    SheepEntity sheep(LegacyEntityType::Unknown, 1);

    EXPECT_FALSE(sheep.isSheared());
    EXPECT_TRUE(sheep.isShearable());

    sheep.setSheared(true);
    EXPECT_TRUE(sheep.isSheared());
    EXPECT_FALSE(sheep.isShearable());

    sheep.setSheared(false);
    EXPECT_FALSE(sheep.isSheared());
    EXPECT_TRUE(sheep.isShearable());
}

TEST_F(SheepEntityTest, ChildCannotBeSheared)
{
    SheepEntity sheep(LegacyEntityType::Unknown, 1);
    sheep.setChild(true);

    EXPECT_FALSE(sheep.isShearable());
}

TEST_F(SheepEntityTest, EatGrassBonusRegrowsWool)
{
    SheepEntity sheep(LegacyEntityType::Unknown, 1);
    sheep.setSheared(true);

    EXPECT_TRUE(sheep.isSheared());

    sheep.eatGrassBonus();

    EXPECT_FALSE(sheep.isSheared());
}

TEST_F(SheepEntityTest, EatGrassBonusAcceleratesChildGrowth)
{
    SheepEntity sheep(LegacyEntityType::Unknown, 1);
    sheep.setChild(true);
    sheep.setGrowingAge(-24000); // 幼羊，-24000 ticks

    EXPECT_TRUE(sheep.isChild());

    i32 ageBefore = sheep.getGrowingAge();
    sheep.eatGrassBonus();
    i32 ageAfter = sheep.getGrowingAge();

    // 应该加速成长 60 ticks
    EXPECT_EQ(ageAfter - ageBefore, 60);
}

TEST_F(SheepEntityTest, GetRandomSheepColor)
{
    // 测试随机颜色生成的分布
    math::Random rng(42);
    int whiteCount = 0;
    int blackCount = 0;
    int grayCount = 0;
    int lightGrayCount = 0;
    int brownCount = 0;
    int pinkCount = 0;

    const int iterations = 10000;
    for (int i = 0; i < iterations; ++i) {
        DyeColor color = SheepEntity::getRandomSheepColor(rng);
        switch (color) {
            case DyeColor::White:
                ++whiteCount;
                break;
            case DyeColor::Black:
                ++blackCount;
                break;
            case DyeColor::Gray:
                ++grayCount;
                break;
            case DyeColor::LightGray:
                ++lightGrayCount;
                break;
            case DyeColor::Brown:
                ++brownCount;
                break;
            case DyeColor::Pink:
                ++pinkCount;
                break;
            default:
                break;
        }
    }

    // 验证概率分布大致正确
    // 白色应该占 ~81.8%
    EXPECT_GT(whiteCount, iterations * 0.75);
    EXPECT_LT(whiteCount, iterations * 0.90);

    // 黑色、灰色、淡灰色各占 ~5%
    EXPECT_GT(blackCount, iterations * 0.03);
    EXPECT_LT(blackCount, iterations * 0.08);
    EXPECT_GT(grayCount, iterations * 0.03);
    EXPECT_LT(grayCount, iterations * 0.08);
    EXPECT_GT(lightGrayCount, iterations * 0.03);
    EXPECT_LT(lightGrayCount, iterations * 0.08);

    // 棕色占 ~3%
    EXPECT_GT(brownCount, iterations * 0.01);
    EXPECT_LT(brownCount, iterations * 0.05);

    // 粉色占 ~0.2%
    EXPECT_GT(pinkCount, 0);
    EXPECT_LT(pinkCount, iterations * 0.01);
}
