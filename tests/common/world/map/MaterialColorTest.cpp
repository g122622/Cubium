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
 * IMPLIED, WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include <gtest/gtest.h>

#include "common/core/Types.hpp"
#include "common/world/map/MaterialColor.hpp"

using namespace mc;
using namespace mc::world::map;

/**
 * @brief MaterialColor 颜色系统测试
 */
class MaterialColorTest : public ::testing::Test {
protected:
    static void SetUpTestSuite() { MaterialColor::initialize(); }
};

TEST_F(MaterialColorTest, InitializeColors)
{
    // 初始化后应该能获取基本颜色
    const auto& grass = MaterialColor::getById(MaterialColorId::GRASS);
    EXPECT_EQ(grass.id(), MaterialColorId::GRASS);
    EXPECT_GT(grass.rgb(), 0u);
}

TEST_F(MaterialColorTest, ComputeMapColor)
{
    // AIR 颜色索引0应该返回0（透明）
    EXPECT_EQ(MaterialColor::computeMapColor(0, 0), 0u);
    EXPECT_EQ(MaterialColor::computeMapColor(0, 1), 0u);
    EXPECT_EQ(MaterialColor::computeMapColor(0, 2), 0u);
    EXPECT_EQ(MaterialColor::computeMapColor(0, 3), 0u);

    // 非AIR颜色应该返回非零值
    u32 grassColor0 = MaterialColor::computeMapColor(static_cast<u8>(MaterialColorId::GRASS), 0);
    u32 grassColor2 = MaterialColor::computeMapColor(static_cast<u8>(MaterialColorId::GRASS), 2);
    EXPECT_NE(grassColor0, 0u);
    EXPECT_NE(grassColor2, 0u);

    // 亮度级别2应该比级别0更亮
    // shade 2 = 255/255 (最亮), shade 0 = 180/255
    EXPECT_GT(grassColor2, grassColor0);
}

TEST_F(MaterialColorTest, PixelToArgb)
{
    // AIR 像素应该返回0（完全透明）
    EXPECT_EQ(MaterialColor::pixelToArgb(0), 0u);

    // 非AIR像素应该返回带alpha的颜色
    u8 pixel = static_cast<u8>(MaterialColorId::GRASS) * 4 + 2; // GRASS, shade 2
    u32 argb = MaterialColor::pixelToArgb(pixel);
    EXPECT_NE(argb, 0u);

    // Alpha应该是255
    u8 alpha = static_cast<u8>((argb >> 24) & 0xFF);
    EXPECT_EQ(alpha, 255u);
}

TEST_F(MaterialColorTest, ShadeMultipliers)
{
    // 验证阴影乘数定义
    EXPECT_EQ(MaterialColor::SHADE_MULTIPLIERS[0], 180u);
    EXPECT_EQ(MaterialColor::SHADE_MULTIPLIERS[1], 220u);
    EXPECT_EQ(MaterialColor::SHADE_MULTIPLIERS[2], 255u);
    EXPECT_EQ(MaterialColor::SHADE_MULTIPLIERS[3], 135u);
}

TEST_F(MaterialColorTest, GetById)
{
    // 获取所有已定义的颜色
    for (u8 i = 0; i < static_cast<u8>(MaterialColorId::COUNT); ++i) {
        const auto& color = MaterialColor::getByIndex(i);
        EXPECT_EQ(color.id(), static_cast<MaterialColorId>(i));
    }
}

TEST_F(MaterialColorTest, GetAllColorsHaveNonZeroRgb)
{
    // 所有非AIR颜色应该有非零RGB值
    for (u8 i = 1; i < static_cast<u8>(MaterialColorId::COUNT); ++i) {
        const auto& color = MaterialColor::getByIndex(i);
        EXPECT_GT(color.rgb(), 0u) << "Color index " << static_cast<int>(i) << " has zero RGB";
    }
}
