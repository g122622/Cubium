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
 * @file TextureImageTest.cpp
 * @brief TextureImage 单元测试，重点覆盖 ImageFormat 构造参数
 */

#include "client/ui/kagero/paint/TextureImage.hpp"
#include <gtest/gtest.h>
#include <vulkan/vulkan.h>

using namespace mc::client::ui::kagero::paint;
using namespace mc;

// ==================== 默认参数测试 ====================

TEST(TextureImageTest, DefaultFormatIsRGBA8)
{
    // 不传 format 参数时，应默认为 RGBA8
    TextureImage image(VK_NULL_HANDLE, VK_NULL_HANDLE, 16, 16);
    EXPECT_EQ(ImageFormat::RGBA8, image.format());
}

TEST(TextureImageTest, DefaultFormatWithAllDefaults)
{
    // 仅传必需参数，所有默认参数（含 format）应生效
    TextureImage image(VK_NULL_HANDLE, VK_NULL_HANDLE, 32, 64);
    EXPECT_EQ(ImageFormat::RGBA8, image.format());
    EXPECT_EQ(32, image.width());
    EXPECT_EQ(64, image.height());
    EXPECT_EQ(0.0f, image.u0());
    EXPECT_EQ(0.0f, image.v0());
    EXPECT_EQ(1.0f, image.u1());
    EXPECT_EQ(1.0f, image.v1());
    EXPECT_EQ(1u, image.atlasSlot());
    EXPECT_TRUE(image.debugName().empty());
}

// ==================== 显式格式传入测试 ====================

TEST(TextureImageTest, ExplicitFormatR8)
{
    TextureImage image(VK_NULL_HANDLE, VK_NULL_HANDLE, 8, 8, 0.0f, 0.0f, 1.0f, 1.0f, 1, "r8_tex", ImageFormat::R8);
    EXPECT_EQ(ImageFormat::R8, image.format());
}

TEST(TextureImageTest, ExplicitFormatRG8)
{
    TextureImage image(VK_NULL_HANDLE, VK_NULL_HANDLE, 8, 8, 0.0f, 0.0f, 1.0f, 1.0f, 1, "rg8_tex", ImageFormat::RG8);
    EXPECT_EQ(ImageFormat::RG8, image.format());
}

TEST(TextureImageTest, ExplicitFormatRGB8)
{
    TextureImage image(VK_NULL_HANDLE, VK_NULL_HANDLE, 8, 8, 0.0f, 0.0f, 1.0f, 1.0f, 1, "rgb8_tex", ImageFormat::RGB8);
    EXPECT_EQ(ImageFormat::RGB8, image.format());
}

TEST(TextureImageTest, ExplicitFormatRGBA8)
{
    TextureImage image(
        VK_NULL_HANDLE, VK_NULL_HANDLE, 8, 8, 0.0f, 0.0f, 1.0f, 1.0f, 1, "rgba8_tex", ImageFormat::RGBA8);
    EXPECT_EQ(ImageFormat::RGBA8, image.format());
}

TEST(TextureImageTest, ExplicitFormatBGRA8)
{
    TextureImage image(
        VK_NULL_HANDLE, VK_NULL_HANDLE, 8, 8, 0.0f, 0.0f, 1.0f, 1.0f, 1, "bgra8_tex", ImageFormat::BGRA8);
    EXPECT_EQ(ImageFormat::BGRA8, image.format());
}

// ==================== 向后兼容性测试 ====================

TEST(TextureImageTest, BackwardCompatibleTenArgCall)
{
    // 旧式 10 参数调用（不含 format）应仍然编译并默认 RGBA8
    TextureImage image(VK_NULL_HANDLE, VK_NULL_HANDLE, 10, 20, 0.0f, 0.0f, 1.0f, 1.0f, 2, "legacy");
    EXPECT_EQ(ImageFormat::RGBA8, image.format());
    EXPECT_EQ(10, image.width());
    EXPECT_EQ(20, image.height());
    EXPECT_EQ(2u, image.atlasSlot());
    EXPECT_EQ("legacy", image.debugName());
}

// ==================== IImage 接口测试 ====================

TEST(TextureImageTest, ImplementsIImageInterface)
{
    const VkImageView dummyView = reinterpret_cast<VkImageView>(0x1234);
    const VkSampler dummySampler = reinterpret_cast<VkSampler>(0x5678);
    TextureImage image(dummyView, dummySampler, 128, 256, 0.25f, 0.5f, 0.75f, 1.0f, 3, "iface", ImageFormat::BGRA8);

    const IImage& asIImage = image;
    EXPECT_EQ(128, asIImage.width());
    EXPECT_EQ(256, asIImage.height());
    EXPECT_EQ(ImageFormat::BGRA8, asIImage.format());
    EXPECT_EQ("iface", asIImage.debugName());
}

// ==================== Vulkan 资源引用与坐标测试 ====================

TEST(TextureImageTest, HoldsVulkanReferencesAndCoords)
{
    const VkImageView view = reinterpret_cast<VkImageView>(0xDEAD);
    const VkSampler sampler = reinterpret_cast<VkSampler>(0xBEEF);
    TextureImage image(view, sampler, 64, 64, 0.1f, 0.2f, 0.3f, 0.4f, 5, "coords", ImageFormat::RG8);

    EXPECT_EQ(view, image.imageView());
    EXPECT_EQ(sampler, image.sampler());
    EXPECT_FLOAT_EQ(0.1f, image.u0());
    EXPECT_FLOAT_EQ(0.2f, image.v0());
    EXPECT_FLOAT_EQ(0.3f, image.u1());
    EXPECT_FLOAT_EQ(0.4f, image.v1());
    EXPECT_EQ(5u, image.atlasSlot());
    EXPECT_TRUE(image.isValid());
}

TEST(TextureImageTest, NullHandleIsInvalid)
{
    TextureImage image(VK_NULL_HANDLE, VK_NULL_HANDLE, 0, 0, 0.0f, 0.0f, 0.0f, 0.0f, 0, "empty", ImageFormat::R8);
    EXPECT_FALSE(image.isValid());
}

// ==================== atlasSlot 修改测试 ====================

TEST(TextureImageTest, SetAtlasSlot)
{
    TextureImage image(VK_NULL_HANDLE, VK_NULL_HANDLE, 16, 16, 0.0f, 0.0f, 1.0f, 1.0f, 1, "slot", ImageFormat::RGBA8);
    EXPECT_EQ(1u, image.atlasSlot());
    image.setAtlasSlot(7);
    EXPECT_EQ(7u, image.atlasSlot());
    // format 不应受 setAtlasSlot 影响
    EXPECT_EQ(ImageFormat::RGBA8, image.format());
}

// ==================== 移动语义测试 ====================

TEST(TextureImageTest, MovePreservesFormat)
{
    const VkImageView view = reinterpret_cast<VkImageView>(0xABCD);
    TextureImage src(view, VK_NULL_HANDLE, 48, 48, 0.0f, 0.0f, 1.0f, 1.0f, 2, "movable", ImageFormat::R8);
    TextureImage dst = std::move(src);

    // 移动后 format 应保持为 R8
    EXPECT_EQ(ImageFormat::R8, dst.format());
    EXPECT_EQ(view, dst.imageView());
    EXPECT_EQ(48, dst.width());
    EXPECT_EQ(48, dst.height());
    EXPECT_EQ(2u, dst.atlasSlot());
    EXPECT_EQ("movable", dst.debugName());
}

// ==================== DEFAULT_TINT 常量测试 ====================

TEST(TextureImageTest, DefaultTintConstant)
{
    // 白色全不透明：ARGB = 0xFFFFFFFF
    EXPECT_EQ(0xFFFFFFFFu, TextureImage::DEFAULT_TINT);
}

// ==================== tint 成员访问测试 ====================

TEST(TextureImageTest, DefaultTintIsOpaqueWhite)
{
    // 构造时不显式指定 tint，应默认为 DEFAULT_TINT
    TextureImage image(VK_NULL_HANDLE, VK_NULL_HANDLE, 16, 16);
    EXPECT_EQ(TextureImage::DEFAULT_TINT, image.tint());
    EXPECT_EQ(0xFFFFFFFFu, image.tint());
}

TEST(TextureImageTest, SetTintUpdatesValue)
{
    TextureImage image(VK_NULL_HANDLE, VK_NULL_HANDLE, 16, 16);
    image.setTint(0x80FF0000u);
    EXPECT_EQ(0x80FF0000u, image.tint());
}

TEST(TextureImageTest, SetTintDoesNotAffectOtherState)
{
    const VkImageView view = reinterpret_cast<VkImageView>(0xCAFE);
    TextureImage image(view, VK_NULL_HANDLE, 32, 48, 0.1f, 0.2f, 0.3f, 0.4f, 4, "tint_test", ImageFormat::BGRA8);
    image.setTint(0xABCDEF12u);

    // tint 修改不应影响其它纹理状态
    EXPECT_EQ(view, image.imageView());
    EXPECT_EQ(32, image.width());
    EXPECT_EQ(48, image.height());
    EXPECT_FLOAT_EQ(0.1f, image.u0());
    EXPECT_EQ(4u, image.atlasSlot());
    EXPECT_EQ(ImageFormat::BGRA8, image.format());
    EXPECT_EQ("tint_test", image.debugName());
    EXPECT_EQ(0xABCDEF12u, image.tint());
}

TEST(TextureImageTest, SetTintCanBeResetToDefault)
{
    TextureImage image(VK_NULL_HANDLE, VK_NULL_HANDLE, 16, 16);
    image.setTint(0x00000000u);
    ASSERT_EQ(0x00000000u, image.tint());

    // 可恢复为默认 tint
    image.setTint(TextureImage::DEFAULT_TINT);
    EXPECT_EQ(TextureImage::DEFAULT_TINT, image.tint());
}
