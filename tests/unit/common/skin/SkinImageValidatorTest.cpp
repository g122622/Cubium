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
 * @file SkinImageValidatorTest.cpp
 * @brief 皮肤 PNG 校验/规范化测试
 *
 * 该模块是 FileSkinLoader 与 HttpSkinLoader 共用的唯一校验实现：两个来源必须产出
 * 完全一致的字节，否则同一张皮肤的缓存哈希会分裂。
 */

#include <gtest/gtest.h>

#include "common/skin/loader/SkinImageValidator.hpp"

#include <memory>
#include <vector>
#include <stb_image.h>
#include <stb_image_write.h>

using namespace mc;

namespace {

/**
 * @brief 生成一张指定尺寸的纯色 RGBA PNG 字节流
 */
std::vector<u8> _makePng(i32 width, i32 height, u8 r, u8 g, u8 b, u8 a)
{
    std::vector<u8> rgba(static_cast<size_t>(width) * height * 4);
    for (size_t i = 0; i < rgba.size(); i += 4) {
        rgba[i + 0] = r;
        rgba[i + 1] = g;
        rgba[i + 2] = b;
        rgba[i + 3] = a;
    }

    struct PngWriteContext {
        std::vector<u8> buffer;
    };

    PngWriteContext ctx;
    stbi_write_png_to_func(
        [](void* userdata, void* data, i32 size) {
            auto* context = static_cast<PngWriteContext*>(userdata);
            auto* bytes = static_cast<u8*>(data);
            context->buffer.insert(context->buffer.end(), bytes, bytes + size);
        },
        &ctx,
        width,
        height,
        4,
        rgba.data(),
        width * 4);
    return ctx.buffer;
}

/**
 * @brief 解码 PNG 并返回尺寸
 */
bool _decodeSize(const std::vector<u8>& png, i32& width, i32& height)
{
    i32 channels = 0;
    u8* pixels = stbi_load_from_memory(png.data(), static_cast<i32>(png.size()), &width, &height, &channels, 4);
    if (pixels == nullptr) {
        return false;
    }
    stbi_image_free(pixels);
    return true;
}

} // namespace

TEST(SkinImageValidatorTest, AcceptsModernSkinAndNormalizesTo64x64)
{
    const auto png = _makePng(64, 64, 10, 20, 30, 255);
    ASSERT_FALSE(png.empty());

    auto result = skin::SkinImageValidator::validateAndNormalize(png);
    ASSERT_TRUE(result.success()) << result.error().message();

    i32 width = 0;
    i32 height = 0;
    ASSERT_TRUE(_decodeSize(result.value(), width, height));
    EXPECT_EQ(width, 64);
    EXPECT_EQ(height, 64);
}

TEST(SkinImageValidatorTest, ConvertsLegacy64x32SkinTo64x64)
{
    // 旧版皮肤只有上半部分：规范化后必须是 64x64，未覆盖区域保持透明。
    const auto png = _makePng(64, 32, 200, 100, 50, 255);
    ASSERT_FALSE(png.empty());

    auto result = skin::SkinImageValidator::validateAndNormalize(png);
    ASSERT_TRUE(result.success()) << result.error().message();

    i32 width = 0;
    i32 height = 0;
    ASSERT_TRUE(_decodeSize(result.value(), width, height));
    EXPECT_EQ(width, 64);
    EXPECT_EQ(height, 64);
}

TEST(SkinImageValidatorTest, RejectsWrongWidth)
{
    const auto png = _makePng(32, 64, 1, 2, 3, 255);
    auto result = skin::SkinImageValidator::validateAndNormalize(png);

    EXPECT_FALSE(result.success());
    EXPECT_EQ(result.error().code(), ErrorCode::InvalidData);
}

TEST(SkinImageValidatorTest, RejectsWrongHeight)
{
    // 64x48 既不是现代皮肤也不是旧版皮肤。
    const auto png = _makePng(64, 48, 1, 2, 3, 255);
    auto result = skin::SkinImageValidator::validateAndNormalize(png);

    EXPECT_FALSE(result.success());
    EXPECT_EQ(result.error().code(), ErrorCode::InvalidData);
}

TEST(SkinImageValidatorTest, RejectsEmptyData)
{
    const std::vector<u8> empty;
    auto result = skin::SkinImageValidator::validateAndNormalize(empty);

    EXPECT_FALSE(result.success());
    EXPECT_EQ(result.error().code(), ErrorCode::InvalidData);
}

TEST(SkinImageValidatorTest, RejectsNonPngData)
{
    const std::vector<u8> garbage(256, 0xAB);
    auto result = skin::SkinImageValidator::validateAndNormalize(garbage);

    EXPECT_FALSE(result.success());
    EXPECT_EQ(result.error().code(), ErrorCode::InvalidData);
}

TEST(SkinImageValidatorTest, IsDeterministicForSameInput)
{
    // 同一张皮肤两次规范化必须得到完全相同的字节，否则缓存哈希会不稳定。
    const auto png = _makePng(64, 64, 7, 8, 9, 255);

    auto first = skin::SkinImageValidator::validateAndNormalize(png);
    auto second = skin::SkinImageValidator::validateAndNormalize(png);
    ASSERT_TRUE(first.success());
    ASSERT_TRUE(second.success());

    EXPECT_EQ(first.value(), second.value());
}
