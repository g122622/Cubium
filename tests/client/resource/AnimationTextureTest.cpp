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

#include "client/resource/TextureAtlasBuilder.hpp"
#include "common/resource/metadata/AnimationMetadata.hpp"

namespace mc {

namespace {

// 创建指定尺寸和颜色的纯色RGBA像素数据
std::vector<u8> makeSolidRgba(u32 width, u32 height, u8 r, u8 g, u8 b, u8 a)
{
    std::vector<u8> pixels(static_cast<size_t>(width) * static_cast<size_t>(height) * 4, 0);
    for (size_t i = 0; i < pixels.size(); i += 4) {
        pixels[i + 0] = r;
        pixels[i + 1] = g;
        pixels[i + 2] = b;
        pixels[i + 3] = a;
    }
    return pixels;
}

// 创建多帧动画纹理（垂直排列）
// 每帧使用不同颜色以便区分
std::vector<u8> makeAnimatedTexture(u32 frameWidth, u32 frameHeight, u32 frameCount)
{
    std::vector<u8> pixels(static_cast<size_t>(frameWidth) * static_cast<size_t>(frameHeight) * frameCount * 4, 0);
    for (u32 frame = 0; frame < frameCount; ++frame) {
        // 每帧使用不同的颜色
        const u8 r = static_cast<u8>((frame * 60) % 256);
        const u8 g = static_cast<u8>((frame * 100 + 50) % 256);
        const u8 b = static_cast<u8>((frame * 140 + 100) % 256);
        const u8 a = 255;

        const size_t frameOffset = static_cast<size_t>(frame) * frameWidth * frameHeight * 4;
        for (u32 y = 0; y < frameHeight; ++y) {
            for (u32 x = 0; x < frameWidth; ++x) {
                const size_t pixelIndex = frameOffset + (static_cast<size_t>(y) * frameWidth + x) * 4;
                pixels[pixelIndex + 0] = r;
                pixels[pixelIndex + 1] = g;
                pixels[pixelIndex + 2] = b;
                pixels[pixelIndex + 3] = a;
            }
        }
    }
    return pixels;
}

// 获取指定帧的像素颜色（用于验证帧数据）
void getFrameColor(const std::vector<u8>& framePixels, u8& r, u8& g, u8& b, u8& a)
{
    ASSERT_GE(framePixels.size(), 4u);
    r = framePixels[0];
    g = framePixels[1];
    b = framePixels[2];
    a = framePixels[3];
}

} // namespace

// ============================================================================
// TextureAtlasBuilder 动画帧提取测试
// ============================================================================

TEST(AnimationTextureTest, AnimatedTextureExtractsFirstFrameToAtlas)
{
    TextureAtlasBuilder builder;
    builder.setMaxSize(128, 128);

    const ResourceLocation loc("minecraft", "textures/block/test_animated");
    const u32 frameWidth = 16;
    const u32 frameHeight = 16;
    const u32 frameCount = 4;
    const u32 imageHeight = frameHeight * frameCount; // 64

    // 创建4帧动画纹理，每帧16x16，总共16x64
    const auto pixels = makeAnimatedTexture(frameWidth, frameHeight, frameCount);

    // 创建默认动画元数据
    const auto metadata = resource::metadata::AnimationMetadata();

    // 添加动画纹理
    builder.addTextureFrame(loc, pixels, frameWidth, imageHeight, frameWidth, frameHeight, metadata);
    auto result = builder.build();

    ASSERT_TRUE(result.success());
    const auto& atlas = result.value();

    // 验证纹理区域已注册（应该只包含首帧尺寸16x16）
    auto it = atlas.regions.find(loc);
    ASSERT_NE(it, atlas.regions.end());

    const TextureRegion& region = it->second;
    const f32 regionWidth = static_cast<f32>(region.u1 - region.u0);
    const f32 regionHeight = static_cast<f32>(region.v1 - region.v0);

    // 区域尺寸应该对应16x16的首帧
    EXPECT_NEAR(regionWidth, 16.0f / static_cast<f32>(atlas.width), 1e-6f);
    EXPECT_NEAR(regionHeight, 16.0f / static_cast<f32>(atlas.height), 1e-6f);
}

TEST(AnimationTextureTest, AnimatedTextureStoresAllFrames)
{
    TextureAtlasBuilder builder;
    builder.setMaxSize(128, 128);

    const ResourceLocation loc("minecraft", "textures/block/test_animated");
    const u32 frameWidth = 16;
    const u32 frameHeight = 16;
    const u32 frameCount = 4;
    const u32 imageHeight = frameHeight * frameCount;

    const auto pixels = makeAnimatedTexture(frameWidth, frameHeight, frameCount);
    const auto metadata = resource::metadata::AnimationMetadata();

    builder.addTextureFrame(loc, pixels, frameWidth, imageHeight, frameWidth, frameHeight, metadata);
    auto result = builder.build();

    ASSERT_TRUE(result.success());
    const auto& atlas = result.value();

    // 验证动画描述符已创建
    ASSERT_EQ(atlas.animations.size(), 1u);

    const auto& anim = atlas.animations[0];

    // 验证位置信息
    EXPECT_EQ(anim.location, loc);
    EXPECT_GT(anim.atlasX, 0u); // 应该有有效的图集位置
    EXPECT_GT(anim.atlasY, 0u);
    EXPECT_EQ(anim.frameWidth, frameWidth);
    EXPECT_EQ(anim.frameHeight, frameHeight);

    // 验证帧数据：应该有4帧
    ASSERT_EQ(anim.framePixels.size(), frameCount);

    // 验证每帧的像素数据大小正确
    const size_t expectedFrameSize = static_cast<size_t>(frameWidth) * frameHeight * 4;
    for (size_t i = 0; i < anim.framePixels.size(); ++i) {
        EXPECT_EQ(anim.framePixels[i].size(), expectedFrameSize) << "Frame " << i << " has incorrect size";
    }

    // 验证每帧的颜色不同（证明帧数据被正确提取）
    u8 r0, g0, b0, a0, r1, g1, b1, a1;
    getFrameColor(anim.framePixels[0], r0, g0, b0, a0);
    getFrameColor(anim.framePixels[1], r1, g1, b1, a1);

    // 帧的颜色应该不同
    EXPECT_FALSE(r0 == r1 && g0 == g1 && b0 == b1);
}

TEST(AnimationTextureTest, AnimationDescriptorPreservesMetadata)
{
    TextureAtlasBuilder builder;
    builder.setMaxSize(128, 128);

    const ResourceLocation loc("minecraft", "textures/block/test_animated");
    const u32 frameWidth = 16;
    const u32 frameHeight = 16;
    const u32 frameCount = 4;
    const u32 imageHeight = frameHeight * frameCount;

    const auto pixels = makeAnimatedTexture(frameWidth, frameHeight, frameCount);

    // 创建带有自定义设置的元数据
    resource::metadata::AnimationMetadata metadata;
    metadata.frametime = 5;
    metadata.width = static_cast<i32>(frameWidth);
    metadata.height = static_cast<i32>(frameHeight);
    metadata.interpolate = true;
    metadata.frames = {
        resource::metadata::AnimationFrame(0, 10),
        resource::metadata::AnimationFrame(1, 5),
        resource::metadata::AnimationFrame(2, 10),
        resource::metadata::AnimationFrame(3, 5),
    };

    builder.addTextureFrame(loc, pixels, frameWidth, imageHeight, frameWidth, frameHeight, metadata);
    auto result = builder.build();

    ASSERT_TRUE(result.success());
    const auto& atlas = result.value();
    ASSERT_EQ(atlas.animations.size(), 1u);

    const auto& anim = atlas.animations[0];

    // 验证元数据被正确保留
    EXPECT_EQ(anim.metadata.frametime, 5);
    EXPECT_EQ(anim.metadata.width, static_cast<i32>(frameWidth));
    EXPECT_EQ(anim.metadata.height, static_cast<i32>(frameHeight));
    EXPECT_TRUE(anim.metadata.interpolate);
    EXPECT_EQ(anim.metadata.frames.size(), 4u);
    EXPECT_EQ(anim.metadata.frames[0].index, 0);
    EXPECT_EQ(anim.metadata.frames[0].time, 10);
    EXPECT_EQ(anim.metadata.frames[1].index, 1);
    EXPECT_EQ(anim.metadata.frames[1].time, 5);
}

// ============================================================================
// 非动画纹理测试
// ============================================================================

TEST(AnimationTextureTest, NonAnimatedTextureProducesNoAnimations)
{
    TextureAtlasBuilder builder;
    builder.setMaxSize(128, 128);

    const ResourceLocation loc("minecraft", "textures/block/stone");
    const u32 width = 16;
    const u32 height = 16;

    // 创建普通非动画纹理：帧尺寸和图像尺寸相同
    const auto pixels = makeSolidRgba(width, height, 128, 128, 128, 255);

    // 使用普通addTexture方法
    builder.addTexture(loc, pixels, width, height);
    auto result = builder.build();

    ASSERT_TRUE(result.success());
    const auto& atlas = result.value();

    // 验证纹理区域存在
    auto it = atlas.regions.find(loc);
    ASSERT_NE(it, atlas.regions.end());

    // 验证没有动画条目
    EXPECT_EQ(atlas.animations.size(), 0u);
}

TEST(AnimationTextureTest, AddTextureFrameWithSameFrameImageSizeProducesNoAnimations)
{
    TextureAtlasBuilder builder;
    builder.setMaxSize(128, 128);

    const ResourceLocation loc("minecraft", "textures/block/stone");
    const u32 width = 16;
    const u32 height = 16;

    // 创建普通纹理
    const auto pixels = makeSolidRgba(width, height, 128, 128, 128, 255);

    // 使用addTextureFrame但帧尺寸等于图像尺寸（非动画）
    builder.addTextureFrame(loc, pixels, width, height, width, height);
    auto result = builder.build();

    ASSERT_TRUE(result.success());
    const auto& atlas = result.value();

    // 纹理区域存在
    auto it = atlas.regions.find(loc);
    ASSERT_NE(it, atlas.regions.end());

    // 没有动画条目
    EXPECT_EQ(atlas.animations.size(), 0u);
}

TEST(AnimationTextureTest, AddTextureFrameWithMetadataButSameSizeStillCreatesAnimation)
{
    TextureAtlasBuilder builder;
    builder.setMaxSize(128, 128);

    const ResourceLocation loc("minecraft", "textures/block/test");
    const u32 width = 16;
    const u32 height = 16;

    const auto pixels = makeSolidRgba(width, height, 128, 128, 128, 255);

    // 使用带元数据的addTextureFrame，即使帧尺寸等于图像尺寸
    // 如果显式传递了元数据，说明调用者知道这是动画（只是恰好只有1帧）
    resource::metadata::AnimationMetadata metadata;
    metadata.frametime = 2;

    builder.addTextureFrame(loc, pixels, width, height, width, height, metadata);
    auto result = builder.build();

    ASSERT_TRUE(result.success());
    const auto& atlas = result.value();

    // 纹理区域存在
    auto it = atlas.regions.find(loc);
    ASSERT_NE(it, atlas.regions.end());

    // 即使只有1帧，也应该创建动画条目（因为传递了元数据）
    ASSERT_EQ(atlas.animations.size(), 1u);
    EXPECT_EQ(atlas.animations[0].framePixels.size(), 1u);
}

// ============================================================================
// AnimationMetadata fromMcmeta 测试
// ============================================================================

TEST(AnimationMetadataTest, FromMcmeta_DefaultJson)
{
    // 空的动画配置，使用默认值
    const std::string jsonText = R"({"animation": {}})";
    const std::vector<u8> mcmetaData(jsonText.begin(), jsonText.end());

    const u32 imageWidth = 16;
    const u32 imageHeight = 64;

    auto metadata = resource::metadata::AnimationMetadata::fromMcmeta(mcmetaData, imageWidth, imageHeight);

    // 默认frametime为1
    EXPECT_EQ(metadata.frametime, 1);

    // 默认情况下width/height应该被自动检测为min(width, height)
    EXPECT_EQ(metadata.width, static_cast<i32>(imageWidth));
    EXPECT_EQ(metadata.height, static_cast<i32>(imageWidth));

    // 默认不插值
    EXPECT_FALSE(metadata.interpolate);

    // 默认无自定义帧序列
    EXPECT_EQ(metadata.frames.size(), 0u);
}

TEST(AnimationMetadataTest, FromMcmeta_WithFrametime)
{
    const std::string jsonText = R"({"animation": {"frametime": 10}})";
    const std::vector<u8> mcmetaData(jsonText.begin(), jsonText.end());

    auto metadata = resource::metadata::AnimationMetadata::fromMcmeta(mcmetaData, 16, 64);

    EXPECT_EQ(metadata.frametime, 10);
}

TEST(AnimationMetadataTest, FromMcmeta_WithInterpolate)
{
    const std::string jsonText = R"({"animation": {"interpolate": true}})";
    const std::vector<u8> mcmetaData(jsonText.begin(), jsonText.end());

    auto metadata = resource::metadata::AnimationMetadata::fromMcmeta(mcmetaData, 16, 64);

    EXPECT_TRUE(metadata.interpolate);
}

TEST(AnimationMetadataTest, FromMcmeta_WithCustomFrames)
{
    const std::string jsonText = R"({
        "animation": {
            "frametime": 5,
            "frames": [0, 1, 2, 1]
        }
    })";
    const std::vector<u8> mcmetaData(jsonText.begin(), jsonText.end());

    auto metadata = resource::metadata::AnimationMetadata::fromMcmeta(mcmetaData, 16, 64);

    EXPECT_EQ(metadata.frametime, 5);
    ASSERT_EQ(metadata.frames.size(), 4u);

    // 验证帧序列
    EXPECT_EQ(metadata.frames[0].index, 0);
    EXPECT_EQ(metadata.frames[0].time, -1); // 使用默认frametime

    EXPECT_EQ(metadata.frames[1].index, 1);
    EXPECT_EQ(metadata.frames[1].time, -1);

    EXPECT_EQ(metadata.frames[2].index, 2);
    EXPECT_EQ(metadata.frames[2].time, -1);

    EXPECT_EQ(metadata.frames[3].index, 1);
    EXPECT_EQ(metadata.frames[3].time, -1);
}

TEST(AnimationMetadataTest, FromMcmeta_WithFrameObjects)
{
    const std::string jsonText = R"({
        "animation": {
            "frametime": 3,
            "frames": [
                {"index": 0, "time": 10},
                {"index": 1, "time": 5},
                {"index": 2, "time": 10},
                {"index": 3, "time": 5}
            ]
        }
    })";
    const std::vector<u8> mcmetaData(jsonText.begin(), jsonText.end());

    auto metadata = resource::metadata::AnimationMetadata::fromMcmeta(mcmetaData, 16, 64);

    EXPECT_EQ(metadata.frametime, 3);
    ASSERT_EQ(metadata.frames.size(), 4u);

    // 验证帧对象
    EXPECT_EQ(metadata.frames[0].index, 0);
    EXPECT_EQ(metadata.frames[0].time, 10);

    EXPECT_EQ(metadata.frames[1].index, 1);
    EXPECT_EQ(metadata.frames[1].time, 5);

    EXPECT_EQ(metadata.frames[2].index, 2);
    EXPECT_EQ(metadata.frames[2].time, 10);

    EXPECT_EQ(metadata.frames[3].index, 3);
    EXPECT_EQ(metadata.frames[3].time, 5);
}

TEST(AnimationMetadataTest, FromMcmeta_WithExplicitDimensions)
{
    const std::string jsonText = R"({
        "animation": {
            "width": 8,
            "height": 8
        }
    })";
    const std::vector<u8> mcmetaData(jsonText.begin(), jsonText.end());

    auto metadata = resource::metadata::AnimationMetadata::fromMcmeta(mcmetaData, 16, 32);

    EXPECT_EQ(metadata.width, 8);
    EXPECT_EQ(metadata.height, 8);
}

TEST(AnimationMetadataTest, FromMcmeta_EmptyData_ReturnsDefault)
{
    const std::vector<u8> emptyData;
    auto metadata = resource::metadata::AnimationMetadata::fromMcmeta(emptyData, 16, 64);

    // 空数据应返回默认值
    EXPECT_EQ(metadata.frametime, 1);
    EXPECT_EQ(metadata.width, -1);
    EXPECT_EQ(metadata.height, -1);
    EXPECT_FALSE(metadata.interpolate);
    EXPECT_EQ(metadata.frames.size(), 0u);
}

TEST(AnimationMetadataTest, FromMcmeta_InvalidJson_ReturnsDefault)
{
    const std::string invalidJson = R"({invalid json})";
    const std::vector<u8> mcmetaData(invalidJson.begin(), invalidJson.end());

    auto metadata = resource::metadata::AnimationMetadata::fromMcmeta(mcmetaData, 16, 64);

    // 无效JSON应返回默认值
    EXPECT_EQ(metadata.frametime, 1);
    EXPECT_EQ(metadata.width, -1);
    EXPECT_EQ(metadata.height, -1);
}

TEST(AnimationMetadataTest, FromMcmeta_MissingAnimationKey_ReturnsDefault)
{
    const std::string jsonText = R"({"other": {}})";
    const std::vector<u8> mcmetaData(jsonText.begin(), jsonText.end());

    auto metadata = resource::metadata::AnimationMetadata::fromMcmeta(mcmetaData, 16, 64);

    // 没有"animation"键应返回默认值
    EXPECT_EQ(metadata.frametime, 1);
    EXPECT_EQ(metadata.width, -1);
    EXPECT_EQ(metadata.height, -1);
}

TEST(AnimationMetadataTest, FromMcmeta_FrameSizeLargerThanImage_ReturnsDefault)
{
    const std::string jsonText = R"({
        "animation": {
            "width": 32,
            "height": 32
        }
    })";
    const std::vector<u8> mcmetaData(jsonText.begin(), jsonText.end());

    // 图像只有16x64，帧尺寸32x32超出宽度
    auto metadata = resource::metadata::AnimationMetadata::fromMcmeta(mcmetaData, 16, 64);

    // 帧尺寸超过图像尺寸应返回默认（无效）值
    EXPECT_EQ(metadata.frametime, 1);
    EXPECT_EQ(metadata.width, -1);
    EXPECT_EQ(metadata.height, -1);
}

TEST(AnimationMetadataTest, FromMcmeta_FrameSizeNotDivisible_ReturnsDefault)
{
    const std::string jsonText = R"({
        "animation": {
            "width": 10,
            "height": 10
        }
    })";
    const std::vector<u8> mcmetaData(jsonText.begin(), jsonText.end());

    // 图像是16x64，帧尺寸10x10不能整除
    auto metadata = resource::metadata::AnimationMetadata::fromMcmeta(mcmetaData, 16, 64);

    // 帧尺寸不能整除图像尺寸应返回默认（无效）值
    EXPECT_EQ(metadata.frametime, 1);
    EXPECT_EQ(metadata.width, -1);
    EXPECT_EQ(metadata.height, -1);
}

TEST(AnimationMetadataTest, FromMcmeta_ZeroImageSize_ReturnsDefault)
{
    const std::string jsonText = R"({"animation": {}})";
    const std::vector<u8> mcmetaData(jsonText.begin(), jsonText.end());

    auto metadata = resource::metadata::AnimationMetadata::fromMcmeta(mcmetaData, 0, 64);

    // 零尺寸图像应返回默认值
    EXPECT_EQ(metadata.width, -1);
    EXPECT_EQ(metadata.height, -1);
}

// ============================================================================
// AnimationMetadata 辅助方法测试
// ============================================================================

TEST(AnimationMetadataTest, IsValidAnimation)
{
    resource::metadata::AnimationMetadata valid;
    valid.frametime = 5;
    valid.width = 16;
    valid.height = 16;

    EXPECT_TRUE(valid.isValidAnimation());

    resource::metadata::AnimationMetadata invalidFrametime;
    invalidFrametime.frametime = 0;
    invalidFrametime.width = 16;
    invalidFrametime.height = 16;

    EXPECT_FALSE(invalidFrametime.isValidAnimation());

    resource::metadata::AnimationMetadata invalidWidth;
    invalidWidth.frametime = 5;
    invalidWidth.width = -1;
    invalidWidth.height = 16;

    EXPECT_FALSE(invalidWidth.isValidAnimation());
}

TEST(AnimationMetadataTest, GetFrameIndex)
{
    resource::metadata::AnimationMetadata metadata;
    metadata.frames = {
        resource::metadata::AnimationFrame(2, -1),
        resource::metadata::AnimationFrame(0, -1),
        resource::metadata::AnimationFrame(1, -1),
    };

    // 有自定义帧序列时，按序列返回帧索引
    EXPECT_EQ(metadata.getFrameIndex(0), 2);
    EXPECT_EQ(metadata.getFrameIndex(1), 0);
    EXPECT_EQ(metadata.getFrameIndex(2), 1);
    EXPECT_EQ(metadata.getFrameIndex(3), 2); // 循环
    EXPECT_EQ(metadata.getFrameIndex(4), 0); // 循环
}

TEST(AnimationMetadataTest, GetFrameIndex_EmptyFrames_ReturnsPosition)
{
    resource::metadata::AnimationMetadata metadata;
    // frames为空

    // 无自定义帧序列时，返回位置本身作为帧索引
    EXPECT_EQ(metadata.getFrameIndex(0), 0);
    EXPECT_EQ(metadata.getFrameIndex(1), 1);
    EXPECT_EQ(metadata.getFrameIndex(5), 5);
}

TEST(AnimationMetadataTest, GetFrameTime)
{
    resource::metadata::AnimationMetadata metadata;
    metadata.frametime = 3;
    metadata.frames = {
        resource::metadata::AnimationFrame(0, 10),
        resource::metadata::AnimationFrame(1, -1), // 使用默认frametime
        resource::metadata::AnimationFrame(2, 5),
    };

    EXPECT_EQ(metadata.getFrameTime(0), 10); // 显式指定10
    EXPECT_EQ(metadata.getFrameTime(1), 3);  // 使用默认frametime
    EXPECT_EQ(metadata.getFrameTime(2), 5);  // 显式指定5
}

TEST(AnimationMetadataTest, GetFrameTime_EmptyFrames_ReturnsDefault)
{
    resource::metadata::AnimationMetadata metadata;
    metadata.frametime = 7;

    EXPECT_EQ(metadata.getFrameTime(0), 7);
    EXPECT_EQ(metadata.getFrameTime(10), 7);
}

} // namespace mc
