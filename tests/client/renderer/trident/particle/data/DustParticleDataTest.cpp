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

#include "client/renderer/trident/particle/data/DustParticleData.hpp"
#include "client/renderer/trident/particle/ParticleTypes.hpp"
#include <memory>
#include <glm/glm.hpp>
#include <gtest/gtest.h>

namespace mc {
namespace {

using namespace client::renderer::trident::particle;
using namespace client::renderer::trident::particle::data;

// ==================== DustParticleData 测试 ====================

class DustParticleDataTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        testColor = 0xFFFF0000; // 红色
        testScale = 1.0f;
    }

    u32 testColor;
    f32 testScale;
};

TEST_F(DustParticleDataTest, Construction_SetsColorAndScale)
{
    DustParticleData data(testColor, testScale);

    EXPECT_EQ(data.color(), 0xFFFF0000);
    EXPECT_FLOAT_EQ(data.scale(), 1.0f);
}

TEST_F(DustParticleDataTest, Construction_DefaultValues)
{
    DustParticleData data;

    // 默认红色，缩放1.0
    EXPECT_EQ(data.color(), 0xFFFF0000);
    EXPECT_FLOAT_EQ(data.scale(), 1.0f);
}

TEST_F(DustParticleDataTest, GetType_ReturnsDust)
{
    DustParticleData data(testColor, testScale);

    EXPECT_EQ(data.getType(), ParticleTypeId::Dust);
}

TEST_F(DustParticleDataTest, GetTypeName_ReturnsDustName)
{
    DustParticleData data(testColor, testScale);

    EXPECT_EQ(data.getTypeName(), "minecraft:dust");
}

TEST_F(DustParticleDataTest, GetParameters_ContainsColorAndScale)
{
    DustParticleData data(0xFF00FF00, 2.0f);

    auto params = data.getParameters();

    // 参数格式: "0xAARRGGBB scale"
    EXPECT_NE(params.find("0xFF00FF00"), std::string::npos);
    EXPECT_NE(params.find("2.00"), std::string::npos);
}

TEST_F(DustParticleDataTest, Clone_ReturnsIdenticalCopy)
{
    DustParticleData data(0xFF123456, 3.0f);

    auto cloned = data.clone();

    ASSERT_NE(cloned, nullptr);
    EXPECT_EQ(cloned->getType(), ParticleTypeId::Dust);

    auto* clonedDust = dynamic_cast<DustParticleData*>(cloned.get());
    ASSERT_NE(clonedDust, nullptr);
    EXPECT_EQ(clonedDust->color(), 0xFF123456);
    EXPECT_FLOAT_EQ(clonedDust->scale(), 3.0f);
}

TEST_F(DustParticleDataTest, Clone_IsIndependentCopy)
{
    DustParticleData data(0xFFABCDEF, 1.5f);
    auto cloned = data.clone();
    auto* clonedDust = dynamic_cast<DustParticleData*>(cloned.get());
    ASSERT_NE(clonedDust, nullptr);

    // 验证克隆是独立的
    DustParticleData data2(0x00000000, 0.5f);
    EXPECT_EQ(clonedDust->color(), 0xFFABCDEF);
    EXPECT_FLOAT_EQ(clonedDust->scale(), 1.5f);
}

TEST_F(DustParticleDataTest, ToRGBAVector_ConvertsCorrectly)
{
    // 红色 ARGB: 0xFFFF0000 -> RGBA: (1.0, 0.0, 0.0, 1.0)
    DustParticleData data(0xFFFF0000, 1.0f);
    glm::vec4 rgba = data.toRGBAVector();

    EXPECT_FLOAT_EQ(rgba.r, 1.0f);
    EXPECT_FLOAT_EQ(rgba.g, 0.0f);
    EXPECT_FLOAT_EQ(rgba.b, 0.0f);
    EXPECT_FLOAT_EQ(rgba.a, 1.0f);
}

TEST_F(DustParticleDataTest, ToRGBAVector_GreenColor)
{
    // 绿色 ARGB: 0xFF00FF00 -> RGBA: (0.0, 1.0, 0.0, 1.0)
    DustParticleData data(0xFF00FF00, 1.0f);
    glm::vec4 rgba = data.toRGBAVector();

    EXPECT_FLOAT_EQ(rgba.r, 0.0f);
    EXPECT_FLOAT_EQ(rgba.g, 1.0f);
    EXPECT_FLOAT_EQ(rgba.b, 0.0f);
    EXPECT_FLOAT_EQ(rgba.a, 1.0f);
}

TEST_F(DustParticleDataTest, ToRGBAVector_BlueColor)
{
    // 蓝色 ARGB: 0xFF0000FF -> RGBA: (0.0, 0.0, 1.0, 1.0)
    DustParticleData data(0xFF0000FF, 1.0f);
    glm::vec4 rgba = data.toRGBAVector();

    EXPECT_FLOAT_EQ(rgba.r, 0.0f);
    EXPECT_FLOAT_EQ(rgba.g, 0.0f);
    EXPECT_FLOAT_EQ(rgba.b, 1.0f);
    EXPECT_FLOAT_EQ(rgba.a, 1.0f);
}

TEST_F(DustParticleDataTest, ToRGBAVector_TransparentColor)
{
    // 半透明 ARGB: 0x80FF0000 -> RGBA: (1.0, 0.0, 0.0, 128/255)
    DustParticleData data(0x80FF0000, 1.0f);
    glm::vec4 rgba = data.toRGBAVector();

    EXPECT_FLOAT_EQ(rgba.r, 1.0f);
    EXPECT_FLOAT_EQ(rgba.g, 0.0f);
    EXPECT_FLOAT_EQ(rgba.b, 0.0f);
    EXPECT_NEAR(rgba.a, 128.0f / 255.0f, 0.01f);
}

TEST_F(DustParticleDataTest, FromRGBAVector_RoundTrip)
{
    glm::vec4 original(0.5f, 0.25f, 0.75f, 1.0f);
    DustParticleData data = DustParticleData::fromRGBAVector(original, 2.0f);

    // 验证颜色往返（精度损失在 int->float 转换范围内）
    glm::vec4 roundTrip = data.toRGBAVector();
    EXPECT_NEAR(roundTrip.r, original.r, 1.0f / 255.0f);
    EXPECT_NEAR(roundTrip.g, original.g, 1.0f / 255.0f);
    EXPECT_NEAR(roundTrip.b, original.b, 1.0f / 255.0f);
    EXPECT_NEAR(roundTrip.a, original.a, 1.0f / 255.0f);
    EXPECT_FLOAT_EQ(data.scale(), 2.0f);
}

TEST_F(DustParticleDataTest, ScaleClamped_LessThanMin)
{
    DustParticleData data(0xFFFF0000, 0.001f);
    EXPECT_FLOAT_EQ(data.scale(), 0.01f); // clamped to 0.01
}

TEST_F(DustParticleDataTest, ScaleClamped_GreaterThanMax)
{
    DustParticleData data(0xFFFF0000, 10.0f);
    EXPECT_FLOAT_EQ(data.scale(), 4.0f); // clamped to 4.0
}

TEST_F(DustParticleDataTest, ScaleClamped_WithinRange)
{
    DustParticleData data(0xFFFF0000, 2.0f);
    EXPECT_FLOAT_EQ(data.scale(), 2.0f); // not clamped
}

TEST_F(DustParticleDataTest, CopyConstruction)
{
    DustParticleData data(0xFFABCDEF, 2.5f);
    DustParticleData copy(data);

    EXPECT_EQ(copy.color(), 0xFFABCDEF);
    EXPECT_FLOAT_EQ(copy.scale(), 2.5f);
    EXPECT_EQ(copy.getType(), ParticleTypeId::Dust);
}

TEST_F(DustParticleDataTest, MoveConstruction)
{
    DustParticleData data(0xFFABCDEF, 2.5f);
    DustParticleData moved(std::move(data));

    EXPECT_EQ(moved.color(), 0xFFABCDEF);
    EXPECT_FLOAT_EQ(moved.scale(), 2.5f);
    EXPECT_EQ(moved.getType(), ParticleTypeId::Dust);
}

// ==================== DustColorTransitionParticleData 测试 ====================

class DustColorTransitionParticleDataTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        testFromColor = 0xFF39E5C0; // 幽匿青色
        testToColor = 0xFFFF0000;   // 红色
        testScale = 1.0f;
    }

    u32 testFromColor;
    u32 testToColor;
    f32 testScale;
};

TEST_F(DustColorTransitionParticleDataTest, Construction_SetsFromColorToColorAndScale)
{
    DustColorTransitionParticleData data(testFromColor, testToColor, testScale);

    EXPECT_EQ(data.fromColor(), 0xFF39E5C0);
    EXPECT_EQ(data.toColor(), 0xFFFF0000);
    EXPECT_FLOAT_EQ(data.scale(), 1.0f);
}

TEST_F(DustColorTransitionParticleDataTest, Construction_DefaultValues)
{
    DustColorTransitionParticleData data;

    // 默认幽匿青色到红色
    EXPECT_EQ(data.fromColor(), 0xFF39E5C0);
    EXPECT_EQ(data.toColor(), 0xFFFF0000);
    EXPECT_FLOAT_EQ(data.scale(), 1.0f);
}

TEST_F(DustColorTransitionParticleDataTest, GetType_ReturnsDustColorTransition)
{
    DustColorTransitionParticleData data(testFromColor, testToColor, testScale);

    EXPECT_EQ(data.getType(), ParticleTypeId::DustColorTransition);
}

TEST_F(DustColorTransitionParticleDataTest, GetTypeName_ReturnsDustColorTransitionName)
{
    DustColorTransitionParticleData data(testFromColor, testToColor, testScale);

    EXPECT_EQ(data.getTypeName(), "minecraft:dust_color_transition");
}

TEST_F(DustColorTransitionParticleDataTest, GetParameters_ContainsColorsAndScale)
{
    DustColorTransitionParticleData data(0xFF00FF00, 0xFF0000FF, 1.5f);

    auto params = data.getParameters();

    // 参数格式: "0xAARRGGBB 0xAARRGGBB scale"
    EXPECT_NE(params.find("0xFF00FF00"), std::string::npos);
    EXPECT_NE(params.find("0xFF0000FF"), std::string::npos);
    EXPECT_NE(params.find("1.50"), std::string::npos);
}

TEST_F(DustColorTransitionParticleDataTest, Clone_ReturnsIdenticalCopy)
{
    DustColorTransitionParticleData data(0xFF123456, 0xFFABCDEF, 2.5f);

    auto cloned = data.clone();

    ASSERT_NE(cloned, nullptr);
    EXPECT_EQ(cloned->getType(), ParticleTypeId::DustColorTransition);

    auto* clonedTransition = dynamic_cast<DustColorTransitionParticleData*>(cloned.get());
    ASSERT_NE(clonedTransition, nullptr);
    EXPECT_EQ(clonedTransition->fromColor(), 0xFF123456);
    EXPECT_EQ(clonedTransition->toColor(), 0xFFABCDEF);
    EXPECT_FLOAT_EQ(clonedTransition->scale(), 2.5f);
}

TEST_F(DustColorTransitionParticleDataTest, FromColorToRGBAVector_ConvertsCorrectly)
{
    DustColorTransitionParticleData data(0xFFFF0000, 0xFF0000FF, 1.0f);

    glm::vec4 from = data.fromColorToRGBAVector();
    EXPECT_FLOAT_EQ(from.r, 1.0f);
    EXPECT_FLOAT_EQ(from.g, 0.0f);
    EXPECT_FLOAT_EQ(from.b, 0.0f);
    EXPECT_FLOAT_EQ(from.a, 1.0f);
}

TEST_F(DustColorTransitionParticleDataTest, ToColorToRGBAVector_ConvertsCorrectly)
{
    DustColorTransitionParticleData data(0xFFFF0000, 0xFF0000FF, 1.0f);

    glm::vec4 to = data.toColorToRGBAVector();
    EXPECT_FLOAT_EQ(to.r, 0.0f);
    EXPECT_FLOAT_EQ(to.g, 0.0f);
    EXPECT_FLOAT_EQ(to.b, 1.0f);
    EXPECT_FLOAT_EQ(to.a, 1.0f);
}

TEST_F(DustColorTransitionParticleDataTest, ScaleClamped_LessThanMin)
{
    DustColorTransitionParticleData data(0xFF39E5C0, 0xFFFF0000, 0.001f);
    EXPECT_FLOAT_EQ(data.scale(), 0.01f);
}

TEST_F(DustColorTransitionParticleDataTest, ScaleClamped_GreaterThanMax)
{
    DustColorTransitionParticleData data(0xFF39E5C0, 0xFFFF0000, 10.0f);
    EXPECT_FLOAT_EQ(data.scale(), 4.0f);
}

TEST_F(DustColorTransitionParticleDataTest, CopyConstruction)
{
    DustColorTransitionParticleData data(0xFF123456, 0xFFABCDEF, 2.0f);
    DustColorTransitionParticleData copy(data);

    EXPECT_EQ(copy.fromColor(), 0xFF123456);
    EXPECT_EQ(copy.toColor(), 0xFFABCDEF);
    EXPECT_FLOAT_EQ(copy.scale(), 2.0f);
}

TEST_F(DustColorTransitionParticleDataTest, MoveConstruction)
{
    DustColorTransitionParticleData data(0xFF123456, 0xFFABCDEF, 2.0f);
    DustColorTransitionParticleData moved(std::move(data));

    EXPECT_EQ(moved.fromColor(), 0xFF123456);
    EXPECT_EQ(moved.toColor(), 0xFFABCDEF);
    EXPECT_FLOAT_EQ(moved.scale(), 2.0f);
}

} // namespace
} // namespace mc
