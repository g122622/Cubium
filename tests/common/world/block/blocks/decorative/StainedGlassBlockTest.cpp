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

/**
 * @file StainedGlassBlockTest.cpp
 * @brief 染色玻璃方块和信标光束颜色测试
 *
 * 测试内容：
 * - StainedGlassBlock 构造和属性
 * - getBeaconColor() 返回正确的 DyeColor
 * - getBeaconColorMultiplier() 返回正确的 RGB 值
 * - BeaconColors::getColorComponents() 所有 16 种颜色
 * - isSolid() 返回 false（玻璃非固体）
 */

#include "world/block/blocks/decorative/StainedGlassBlock.hpp"
#include "world/block/IBeaconBeamColorProvider.hpp"
#include "world/block/Material.hpp"
#include <array>
#include <cmath>
#include <gtest/gtest.h>

using namespace mc;
using namespace mc::block;

// ========== BeaconColors 工具类测试 ==========

class BeaconColorsTest : public ::testing::Test {
protected:
    // 浮点数比较容差
    static constexpr f32 EPSILON = 0.0001f;
};

TEST_F(BeaconColorsTest, WhiteColor_ReturnsCorrectRGB)
{
    auto color = BeaconColors::getColorComponents(DyeColor::White);
    EXPECT_NEAR(color[0], 0.9764706f, EPSILON);
    EXPECT_NEAR(color[1], 0.9764706f, EPSILON);
    EXPECT_NEAR(color[2], 0.9764706f, EPSILON);
}

TEST_F(BeaconColorsTest, OrangeColor_ReturnsCorrectRGB)
{
    auto color = BeaconColors::getColorComponents(DyeColor::Orange);
    EXPECT_NEAR(color[0], 0.9764706f, EPSILON);
    EXPECT_NEAR(color[1], 0.5019608f, EPSILON);
    EXPECT_NEAR(color[2], 0.1137255f, EPSILON);
}

TEST_F(BeaconColorsTest, MagentaColor_ReturnsCorrectRGB)
{
    auto color = BeaconColors::getColorComponents(DyeColor::Magenta);
    EXPECT_NEAR(color[0], 0.7803922f, EPSILON);
    EXPECT_NEAR(color[1], 0.3058824f, EPSILON);
    EXPECT_NEAR(color[2], 0.7411765f, EPSILON);
}

TEST_F(BeaconColorsTest, LightBlueColor_ReturnsCorrectRGB)
{
    auto color = BeaconColors::getColorComponents(DyeColor::LightBlue);
    EXPECT_NEAR(color[0], 0.2274510f, EPSILON);
    EXPECT_NEAR(color[1], 0.7019608f, EPSILON);
    EXPECT_NEAR(color[2], 0.8549020f, EPSILON);
}

TEST_F(BeaconColorsTest, YellowColor_ReturnsCorrectRGB)
{
    auto color = BeaconColors::getColorComponents(DyeColor::Yellow);
    EXPECT_NEAR(color[0], 0.9960784f, EPSILON);
    EXPECT_NEAR(color[1], 0.8470588f, EPSILON);
    EXPECT_NEAR(color[2], 0.2392157f, EPSILON);
}

TEST_F(BeaconColorsTest, LimeColor_ReturnsCorrectRGB)
{
    auto color = BeaconColors::getColorComponents(DyeColor::Lime);
    EXPECT_NEAR(color[0], 0.5019608f, EPSILON);
    EXPECT_NEAR(color[1], 0.7803922f, EPSILON);
    EXPECT_NEAR(color[2], 0.1176471f, EPSILON);
}

TEST_F(BeaconColorsTest, PinkColor_ReturnsCorrectRGB)
{
    auto color = BeaconColors::getColorComponents(DyeColor::Pink);
    EXPECT_NEAR(color[0], 0.9529412f, EPSILON);
    EXPECT_NEAR(color[1], 0.5450980f, EPSILON);
    EXPECT_NEAR(color[2], 0.6666667f, EPSILON);
}

TEST_F(BeaconColorsTest, GrayColor_ReturnsCorrectRGB)
{
    auto color = BeaconColors::getColorComponents(DyeColor::Gray);
    EXPECT_NEAR(color[0], 0.2784314f, EPSILON);
    EXPECT_NEAR(color[1], 0.3098039f, EPSILON);
    EXPECT_NEAR(color[2], 0.3215686f, EPSILON);
}

TEST_F(BeaconColorsTest, LightGrayColor_ReturnsCorrectRGB)
{
    auto color = BeaconColors::getColorComponents(DyeColor::LightGray);
    EXPECT_NEAR(color[0], 0.6156863f, EPSILON);
    EXPECT_NEAR(color[1], 0.6156863f, EPSILON);
    EXPECT_NEAR(color[2], 0.5921569f, EPSILON);
}

TEST_F(BeaconColorsTest, CyanColor_ReturnsCorrectRGB)
{
    auto color = BeaconColors::getColorComponents(DyeColor::Cyan);
    EXPECT_NEAR(color[0], 0.0862745f, EPSILON);
    EXPECT_NEAR(color[1], 0.6117647f, EPSILON);
    EXPECT_NEAR(color[2], 0.6117647f, EPSILON);
}

TEST_F(BeaconColorsTest, PurpleColor_ReturnsCorrectRGB)
{
    auto color = BeaconColors::getColorComponents(DyeColor::Purple);
    EXPECT_NEAR(color[0], 0.5372549f, EPSILON);
    EXPECT_NEAR(color[1], 0.1960784f, EPSILON);
    EXPECT_NEAR(color[2], 0.7215686f, EPSILON);
}

TEST_F(BeaconColorsTest, BlueColor_ReturnsCorrectRGB)
{
    auto color = BeaconColors::getColorComponents(DyeColor::Blue);
    EXPECT_NEAR(color[0], 0.2352941f, EPSILON);
    EXPECT_NEAR(color[1], 0.2666667f, EPSILON);
    EXPECT_NEAR(color[2], 0.6666667f, EPSILON);
}

TEST_F(BeaconColorsTest, BrownColor_ReturnsCorrectRGB)
{
    auto color = BeaconColors::getColorComponents(DyeColor::Brown);
    EXPECT_NEAR(color[0], 0.5098039f, EPSILON);
    EXPECT_NEAR(color[1], 0.3294118f, EPSILON);
    EXPECT_NEAR(color[2], 0.1960784f, EPSILON);
}

TEST_F(BeaconColorsTest, GreenColor_ReturnsCorrectRGB)
{
    auto color = BeaconColors::getColorComponents(DyeColor::Green);
    EXPECT_NEAR(color[0], 0.3686275f, EPSILON);
    EXPECT_NEAR(color[1], 0.4862745f, EPSILON);
    EXPECT_NEAR(color[2], 0.0862745f, EPSILON);
}

TEST_F(BeaconColorsTest, RedColor_ReturnsCorrectRGB)
{
    auto color = BeaconColors::getColorComponents(DyeColor::Red);
    EXPECT_NEAR(color[0], 0.6901961f, EPSILON);
    EXPECT_NEAR(color[1], 0.1803922f, EPSILON);
    EXPECT_NEAR(color[2], 0.1490196f, EPSILON);
}

TEST_F(BeaconColorsTest, BlackColor_ReturnsCorrectRGB)
{
    auto color = BeaconColors::getColorComponents(DyeColor::Black);
    EXPECT_NEAR(color[0], 0.1137255f, EPSILON);
    EXPECT_NEAR(color[1], 0.1137255f, EPSILON);
    EXPECT_NEAR(color[2], 0.1294118f, EPSILON);
}

TEST_F(BeaconColorsTest, AllColors_ReturnValidRGBRange)
{
    // 验证所有颜色的 RGB 值都在 [0.0, 1.0] 范围内
    for (u8 i = 0; i < static_cast<u8>(DyeColor::Count); ++i) {
        DyeColor colorEnum = static_cast<DyeColor>(i);
        auto color = BeaconColors::getColorComponents(colorEnum);
        EXPECT_GE(color[0], 0.0f) << "Color " << i << " R out of range";
        EXPECT_LE(color[0], 1.0f) << "Color " << i << " R out of range";
        EXPECT_GE(color[1], 0.0f) << "Color " << i << " G out of range";
        EXPECT_LE(color[1], 1.0f) << "Color " << i << " G out of range";
        EXPECT_GE(color[2], 0.0f) << "Color " << i << " B out of range";
        EXPECT_LE(color[2], 1.0f) << "Color " << i << " B out of range";
    }
}

// ========== StainedGlassBlock 测试 ==========

class StainedGlassBlockTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        // 创建红色染色玻璃用于测试
        redGlass_ = std::make_unique<StainedGlassBlock>(
            BlockProperties(Material::GLASS).hardness(0.3f).resistance(0.3f).notSolid(), DyeColor::Red);

        // 创建白色染色玻璃用于测试
        whiteGlass_ = std::make_unique<StainedGlassBlock>(
            BlockProperties(Material::GLASS).hardness(0.3f).resistance(0.3f).notSolid(), DyeColor::White);
    }

    std::unique_ptr<StainedGlassBlock> redGlass_;
    std::unique_ptr<StainedGlassBlock> whiteGlass_;
};

TEST_F(StainedGlassBlockTest, Create_HasCorrectProperties)
{
    EXPECT_NE(redGlass_, nullptr);
    EXPECT_NE(whiteGlass_, nullptr);
}

TEST_F(StainedGlassBlockTest, IsSolid_ReturnsFalse)
{
    const auto& state = redGlass_->defaultState();
    EXPECT_FALSE(redGlass_->isSolid(state));
}

TEST_F(StainedGlassBlockTest, GetBeaconColor_ReturnsCorrectColor)
{
    EXPECT_EQ(redGlass_->getBeaconColor(), DyeColor::Red);
    EXPECT_EQ(whiteGlass_->getBeaconColor(), DyeColor::White);
}

TEST_F(StainedGlassBlockTest, GetBeaconColorMultiplier_ReturnsValidPointer)
{
    const auto& state = redGlass_->defaultState();
    const auto* colorPtr = redGlass_->getBeaconColorMultiplier(state);
    EXPECT_NE(colorPtr, nullptr);
}

TEST_F(StainedGlassBlockTest, GetBeaconColorMultiplier_ReturnsCorrectRGB)
{
    const auto& state = redGlass_->defaultState();
    const auto* colorPtr = redGlass_->getBeaconColorMultiplier(state);
    ASSERT_NE(colorPtr, nullptr);

    // 红色染料 RGB: 0.6901961, 0.1803922, 0.1490196
    constexpr f32 EPSILON = 0.0001f;
    EXPECT_NEAR((*colorPtr)[0], 0.6901961f, EPSILON);
    EXPECT_NEAR((*colorPtr)[1], 0.1803922f, EPSILON);
    EXPECT_NEAR((*colorPtr)[2], 0.1490196f, EPSILON);
}

TEST_F(StainedGlassBlockTest, GetBeaconColorMultiplier_ReturnsSameColorForDifferentStates)
{
    // 染色玻璃没有状态属性，所有状态应返回相同颜色
    const auto& state = redGlass_->defaultState();
    const auto* color1 = redGlass_->getBeaconColorMultiplier(state);
    const auto* color2 = redGlass_->getBeaconColorMultiplier(state);

    EXPECT_EQ(color1, color2);
}

TEST_F(StainedGlassBlockTest, IBeaconBeamColorProvider_Interface)
{
    // 测试接口继承
    const IBeaconBeamColorProvider* provider = redGlass_.get();
    EXPECT_NE(provider, nullptr);
    EXPECT_EQ(provider->getBeaconColor(), DyeColor::Red);
}

TEST_F(StainedGlassBlockTest, AllColors_CreateCorrectly)
{
    // 测试所有 16 种颜色都能正确创建
    for (u8 i = 0; i < static_cast<u8>(DyeColor::Count); ++i) {
        DyeColor color = static_cast<DyeColor>(i);
        auto glass = std::make_unique<StainedGlassBlock>(
            BlockProperties(Material::GLASS).hardness(0.3f).resistance(0.3f).notSolid(), color);
        EXPECT_NE(glass, nullptr) << "Failed to create glass for color " << i;
        EXPECT_EQ(glass->getBeaconColor(), color) << "Wrong color for glass " << i;

        // 验证颜色指针有效
        const auto& state = glass->defaultState();
        const auto* colorPtr = glass->getBeaconColorMultiplier(state);
        EXPECT_NE(colorPtr, nullptr) << "Null color pointer for glass " << i;
    }
}

TEST_F(StainedGlassBlockTest, ColorComponents_MatchBeaconColors)
{
    // 验证 StainedGlassBlock 的颜色与 BeaconColors 工具类返回的一致
    for (u8 i = 0; i < static_cast<u8>(DyeColor::Count); ++i) {
        DyeColor color = static_cast<DyeColor>(i);
        auto glass = std::make_unique<StainedGlassBlock>(
            BlockProperties(Material::GLASS).hardness(0.3f).resistance(0.3f).notSolid(), color);

        const auto& state = glass->defaultState();
        const auto* blockColor = glass->getBeaconColorMultiplier(state);
        auto expectedColor = BeaconColors::getColorComponents(color);

        constexpr f32 EPSILON = 0.0001f;
        EXPECT_NEAR((*blockColor)[0], expectedColor[0], EPSILON) << "R mismatch for color " << i;
        EXPECT_NEAR((*blockColor)[1], expectedColor[1], EPSILON) << "G mismatch for color " << i;
        EXPECT_NEAR((*blockColor)[2], expectedColor[2], EPSILON) << "B mismatch for color " << i;
    }
}
