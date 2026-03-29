#include <gtest/gtest.h>

#include "client/resource/TextureAtlasBuilder.hpp"

namespace mc {

namespace {

std::vector<u8> makeSolidRgba(u32 width, u32 height, u8 r, u8 g, u8 b, u8 a) {
    std::vector<u8> pixels(static_cast<size_t>(width) * static_cast<size_t>(height) * 4, 0);
    for (size_t i = 0; i < pixels.size(); i += 4) {
        pixels[i + 0] = r;
        pixels[i + 1] = g;
        pixels[i + 2] = b;
        pixels[i + 3] = a;
    }
    return pixels;
}

} // namespace

TEST(TextureAtlasBuilderTest, AnimatedVerticalStripUsesFrameSizeForRegion) {
    TextureAtlasBuilder builder;
    builder.setMaxSize(128, 128);

    const ResourceLocation loc("minecraft", "textures/block/water_flow");
    const auto pixels = makeSolidRgba(16, 64, 64, 128, 255, 255);

    builder.addTextureFrame(loc, pixels, 16, 64, 16, 16);
    auto result = builder.build();

    ASSERT_TRUE(result.success());
    const auto& atlas = result.value();

    auto it = atlas.regions.find(loc);
    ASSERT_NE(it, atlas.regions.end());

    const TextureRegion& region = it->second;
    const f32 regionWidth = region.u1 - region.u0;
    const f32 regionHeight = region.v1 - region.v0;

    EXPECT_NEAR(regionWidth, 16.0f / static_cast<f32>(atlas.width), 1e-6f);
    EXPECT_NEAR(regionHeight, 16.0f / static_cast<f32>(atlas.height), 1e-6f);
}

TEST(TextureAtlasBuilderTest, InvalidFrameSizeFallsBackToFullImage) {
    TextureAtlasBuilder builder;
    builder.setMaxSize(128, 128);

    const ResourceLocation loc("minecraft", "textures/block/custom_tall_texture");
    const auto pixels = makeSolidRgba(16, 64, 255, 255, 255, 255);

    // 帧宽 17 非法（大于图片宽且无法整除），应回退为整图 16x64。
    builder.addTextureFrame(loc, pixels, 16, 64, 17, 16);
    auto result = builder.build();

    ASSERT_TRUE(result.success());
    const auto& atlas = result.value();

    auto it = atlas.regions.find(loc);
    ASSERT_NE(it, atlas.regions.end());

    const TextureRegion& region = it->second;
    const f32 regionWidth = region.u1 - region.u0;
    const f32 regionHeight = region.v1 - region.v0;

    EXPECT_NEAR(regionWidth, 16.0f / static_cast<f32>(atlas.width), 1e-6f);
    EXPECT_NEAR(regionHeight, 64.0f / static_cast<f32>(atlas.height), 1e-6f);
}

} // namespace mc
