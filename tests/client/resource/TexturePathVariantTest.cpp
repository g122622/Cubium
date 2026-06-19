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

#include "client/resource/ResourceManager.hpp"
#include "client/resource/TextureAtlasBuilder.hpp"

namespace mc {

namespace {

// 辅助函数：生成纯色 RGBA 像素数据
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

} // namespace

// ============================================================================
// getAltTexturePath 静态方法测试
// 此方法为公共静态方法，可直接测试
// ============================================================================

TEST(GetAltTexturePathTest, ModernBlockPathReturnsLegacy)
{
    // textures/block/ -> textures/blocks/
    EXPECT_EQ(ResourceManager::getAltTexturePath("textures/block/stone"), "textures/blocks/stone");
    EXPECT_EQ(ResourceManager::getAltTexturePath("textures/block/grass_block_top"), "textures/blocks/grass_block_top");
    EXPECT_EQ(ResourceManager::getAltTexturePath("textures/block/dirt"), "textures/blocks/dirt");
}

TEST(GetAltTexturePathTest, LegacyBlockPathReturnsModern)
{
    // textures/blocks/ -> textures/block/
    EXPECT_EQ(ResourceManager::getAltTexturePath("textures/blocks/stone"), "textures/block/stone");
    EXPECT_EQ(ResourceManager::getAltTexturePath("textures/blocks/grass_block_top"), "textures/block/grass_block_top");
}

TEST(GetAltTexturePathTest, ModernItemPathReturnsLegacy)
{
    // textures/item/ -> textures/items/
    EXPECT_EQ(ResourceManager::getAltTexturePath("textures/item/diamond"), "textures/items/diamond");
    EXPECT_EQ(ResourceManager::getAltTexturePath("textures/item/iron_ingot"), "textures/items/iron_ingot");
}

TEST(GetAltTexturePathTest, LegacyItemPathReturnsModern)
{
    // textures/items/ -> textures/item/
    EXPECT_EQ(ResourceManager::getAltTexturePath("textures/items/diamond"), "textures/item/diamond");
    EXPECT_EQ(ResourceManager::getAltTexturePath("textures/items/iron_ingot"), "textures/item/iron_ingot");
}

TEST(GetAltTexturePathTest, NonTexturePathReturnsEmpty)
{
    // 不匹配任何已知前缀时返回空字符串
    EXPECT_TRUE(ResourceManager::getAltTexturePath("models/block/stone").empty());
    EXPECT_TRUE(ResourceManager::getAltTexturePath("blockstates/stone").empty());
    // 注意：textures/entity/steve 现在会返回 textures/entity/steve/steve（实体路径变体）
    // 而不是返回空字符串
    EXPECT_TRUE(ResourceManager::getAltTexturePath("textures/environment/clouds").empty());
    EXPECT_TRUE(ResourceManager::getAltTexturePath("textures/painting/kebab").empty());
    EXPECT_TRUE(ResourceManager::getAltTexturePath("").empty());
}

TEST(GetAltTexturePathTest, ExactPrefixWithoutSuffixReturnsEmpty)
{
    // 仅有前缀没有后续内容时返回空字符串（路径太短，无实际纹理名）
    EXPECT_TRUE(ResourceManager::getAltTexturePath("textures/block/").empty());
    EXPECT_TRUE(ResourceManager::getAltTexturePath("textures/blocks/").empty());
    EXPECT_TRUE(ResourceManager::getAltTexturePath("textures/item/").empty());
    EXPECT_TRUE(ResourceManager::getAltTexturePath("textures/items/").empty());
}

TEST(GetAltTexturePathTest, PathWithSubdirectories)
{
    // 包含子目录的路径也能正确转换
    EXPECT_EQ(ResourceManager::getAltTexturePath("textures/block/flower/rose"), "textures/blocks/flower/rose");
    EXPECT_EQ(ResourceManager::getAltTexturePath("textures/blocks/flower/rose"), "textures/block/flower/rose");
    EXPECT_EQ(ResourceManager::getAltTexturePath("textures/item/diamond_helmet"), "textures/items/diamond_helmet");
}

// ============================================================================
// 实体纹理路径变体测试
// getAltTexturePath 扩展支持 textures/entity/<name>/<name> <-> textures/entity/<name>
// ============================================================================

TEST(GetAltTexturePathTest, EntitySubdirectoryToFlat)
{
    // MC 1.13+ 子目录格式 -> MC 1.12 扁平格式
    // textures/entity/pig/pig -> textures/entity/pig
    EXPECT_EQ(ResourceManager::getAltTexturePath("textures/entity/pig/pig"), "textures/entity/pig");
    EXPECT_EQ(ResourceManager::getAltTexturePath("textures/entity/creeper/creeper"), "textures/entity/creeper");
    EXPECT_EQ(ResourceManager::getAltTexturePath("textures/entity/zombie/zombie"), "textures/entity/zombie");
}

TEST(GetAltTexturePathTest, EntityFlatToSubdirectory)
{
    // MC 1.12 扁平格式 -> MC 1.13+ 子目录格式
    // textures/entity/pig -> textures/entity/pig/pig
    EXPECT_EQ(ResourceManager::getAltTexturePath("textures/entity/pig"), "textures/entity/pig/pig");
    EXPECT_EQ(ResourceManager::getAltTexturePath("textures/entity/creeper"), "textures/entity/creeper/creeper");
    EXPECT_EQ(ResourceManager::getAltTexturePath("textures/entity/bat"), "textures/entity/bat/bat");
}

TEST(GetAltTexturePathTest, EntitySubdirectoryWithPngSuffix)
{
    // 带 .png 后缀的路径也能正确处理
    EXPECT_EQ(ResourceManager::getAltTexturePath("textures/entity/pig/pig.png"), "textures/entity/pig");
    EXPECT_EQ(ResourceManager::getAltTexturePath("textures/entity/zombie/zombie.png"), "textures/entity/zombie");
}

TEST(GetAltTexturePathTest, EntitySubdirectoryMismatchedNameReturnsEmpty)
{
    // 子目录名与文件名不匹配时不转换（如 textures/entity/horse/horse_brown）
    EXPECT_TRUE(ResourceManager::getAltTexturePath("textures/entity/horse/horse_brown").empty());
    EXPECT_TRUE(ResourceManager::getAltTexturePath("textures/entity/cow/red_mooshroom").empty());
}

TEST(GetAltTexturePathTest, EntityPrefixOnlyReturnsEmpty)
{
    // 仅有 textures/entity/ 前缀没有实体名时返回空
    EXPECT_TRUE(ResourceManager::getAltTexturePath("textures/entity/").empty());
}

// ============================================================================
// _findTextureRegion 路径变体回退测试
// 通过 getTextureRegion 间接测试（不依赖私有成员访问）
// ============================================================================

class FindTextureRegionFallbackTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        // 构建仅包含现代路径纹理的图集（不注册别名，仅使用原始路径）
        modernOnlyBuilder.setMaxSize(256, 256);
        modernOnlyBuilder.addTextureFrame(ResourceLocation("minecraft", "textures/block/stone"),
            makeSolidRgba(16, 16, 128, 128, 128, 255),
            16,
            16,
            16,
            16);
        modernOnlyBuilder.addTextureFrame(ResourceLocation("minecraft", "textures/item/diamond"),
            makeSolidRgba(16, 16, 0, 255, 255, 255),
            16,
            16,
            16,
            16);

        auto modernResult = modernOnlyBuilder.build();
        ASSERT_TRUE(modernResult.success());

        modernMgr.textureRegions() = modernResult.value().regions;
        // 注意：此处不注册别名，仅依赖 _findTextureRegion 的回退逻辑

        // 构建仅包含旧版路径纹理的图集
        legacyOnlyBuilder.setMaxSize(256, 256);
        legacyOnlyBuilder.addTextureFrame(ResourceLocation("minecraft", "textures/blocks/stone"),
            makeSolidRgba(16, 16, 64, 64, 64, 255),
            16,
            16,
            16,
            16);
        legacyOnlyBuilder.addTextureFrame(ResourceLocation("minecraft", "textures/items/diamond"),
            makeSolidRgba(16, 16, 0, 200, 200, 255),
            16,
            16,
            16,
            16);

        auto legacyResult = legacyOnlyBuilder.build();
        ASSERT_TRUE(legacyResult.success());

        legacyMgr.textureRegions() = legacyResult.value().regions;
    }

    TextureAtlasBuilder modernOnlyBuilder;
    TextureAtlasBuilder legacyOnlyBuilder;
    ResourceManager modernMgr;
    ResourceManager legacyMgr;
};

// 测试：直接匹配现代路径仍然有效
TEST_F(FindTextureRegionFallbackTest, DirectModernPathMatch)
{
    const TextureRegion* region = modernMgr.getTextureRegion(ResourceLocation("minecraft", "textures/block/stone"));
    ASSERT_NE(region, nullptr);
}

// 测试：通过回退逻辑，旧版路径也能找到现代路径注册的方块纹理
TEST_F(FindTextureRegionFallbackTest, LegacyPathFindsModernBlockTexture)
{
    const TextureRegion* region = modernMgr.getTextureRegion(ResourceLocation("minecraft", "textures/blocks/stone"));
    ASSERT_NE(region, nullptr);
}

// 测试：通过回退逻辑，旧版路径也能找到现代路径注册的物品纹理
TEST_F(FindTextureRegionFallbackTest, LegacyPathFindsModernItemTexture)
{
    const TextureRegion* region = modernMgr.getTextureRegion(ResourceLocation("minecraft", "textures/items/diamond"));
    ASSERT_NE(region, nullptr);
}

// 测试：通过回退逻辑，现代路径也能找到旧版路径注册的方块纹理
TEST_F(FindTextureRegionFallbackTest, ModernPathFindsLegacyBlockTexture)
{
    const TextureRegion* region = legacyMgr.getTextureRegion(ResourceLocation("minecraft", "textures/block/stone"));
    ASSERT_NE(region, nullptr);
}

// 测试：通过回退逻辑，现代路径也能找到旧版路径注册的物品纹理
TEST_F(FindTextureRegionFallbackTest, ModernPathFindsLegacyItemTexture)
{
    const TextureRegion* region = legacyMgr.getTextureRegion(ResourceLocation("minecraft", "textures/item/diamond"));
    ASSERT_NE(region, nullptr);
}

// 测试：完全不存在的路径返回 nullptr
TEST_F(FindTextureRegionFallbackTest, NonexistentPathReturnsNull)
{
    const TextureRegion* region =
        modernMgr.getTextureRegion(ResourceLocation("minecraft", "textures/block/nonexistent"));
    EXPECT_EQ(region, nullptr);
}

// 测试：旧版路径回退到不存在的纹理也返回 nullptr
TEST_F(FindTextureRegionFallbackTest, LegacyPathFallbackToNonexistentReturnsNull)
{
    const TextureRegion* region =
        modernMgr.getTextureRegion(ResourceLocation("minecraft", "textures/blocks/nonexistent"));
    EXPECT_EQ(region, nullptr);
}

// 测试：非 textures/block/ 或 textures/item/ 前缀且非实体路径的路径不做回退
TEST_F(FindTextureRegionFallbackTest, NonTexturePathNoFallback)
{
    const TextureRegion* region = modernMgr.getTextureRegion(ResourceLocation("minecraft", "textures/painting/kebab"));
    EXPECT_EQ(region, nullptr);
}

// ============================================================================
// 别名注册测试
// 验证 buildTextureAtlas 后的别名注册逻辑
// 通过模拟别名注册验证
// ============================================================================

TEST(AtlasAliasRegistrationTest, AliasRegistrationCreatesBothPaths)
{
    // 构建图集（仅现代路径）
    TextureAtlasBuilder builder;
    builder.setMaxSize(256, 256);
    builder.addTextureFrame(ResourceLocation("minecraft", "textures/block/stone"),
        makeSolidRgba(16, 16, 128, 128, 128, 255),
        16,
        16,
        16,
        16);

    auto result = builder.build();
    ASSERT_TRUE(result.success());

    ResourceManager mgr;
    mgr.textureRegions() = result.value().regions;

    // 模拟 buildTextureAtlas 中的别名注册逻辑
    {
        std::vector<std::pair<ResourceLocation, TextureRegion>> aliases;
        for (const auto& [loc, region] : mgr.textureRegions()) {
            std::string altPath = ResourceManager::getAltTexturePath(loc.path());
            if (!altPath.empty()) {
                ResourceLocation altLoc(loc.namespace_(), std::move(altPath));
                if (mgr.textureRegions().find(altLoc) == mgr.textureRegions().end()) {
                    aliases.emplace_back(std::move(altLoc), region);
                }
            }
        }
        for (auto& [altLoc, region] : aliases) {
            mgr.textureRegions().emplace(std::move(altLoc), std::move(region));
        }
    }

    // 别名注册后，旧版路径应能通过直接查找找到（无需 _findTextureRegion 回退）
    auto modernIt = mgr.textureRegions().find(ResourceLocation("minecraft", "textures/block/stone"));
    auto legacyIt = mgr.textureRegions().find(ResourceLocation("minecraft", "textures/blocks/stone"));
    ASSERT_NE(modernIt, mgr.textureRegions().end());
    ASSERT_NE(legacyIt, mgr.textureRegions().end());
}

TEST(AtlasAliasRegistrationTest, AliasDoesNotOverrideExisting)
{
    // 构建图集，同时包含现代和旧版路径的同一纹理
    TextureAtlasBuilder builder;
    builder.setMaxSize(256, 256);
    builder.addTextureFrame(ResourceLocation("minecraft", "textures/block/stone"),
        makeSolidRgba(16, 16, 128, 128, 128, 255),
        16,
        16,
        16,
        16);
    builder.addTextureFrame(
        ResourceLocation("minecraft", "textures/blocks/stone"), makeSolidRgba(16, 16, 64, 64, 64, 255), 16, 16, 16, 16);

    auto result = builder.build();
    ASSERT_TRUE(result.success());

    ResourceManager mgr;
    mgr.textureRegions() = result.value().regions;

    // 注册别名时，旧版路径已存在，别名不应覆盖
    {
        std::vector<std::pair<ResourceLocation, TextureRegion>> aliases;
        for (const auto& [loc, region] : mgr.textureRegions()) {
            std::string altPath = ResourceManager::getAltTexturePath(loc.path());
            if (!altPath.empty()) {
                ResourceLocation altLoc(loc.namespace_(), std::move(altPath));
                if (mgr.textureRegions().find(altLoc) == mgr.textureRegions().end()) {
                    aliases.emplace_back(std::move(altLoc), region);
                }
            }
        }
        for (auto& [altLoc, region] : aliases) {
            mgr.textureRegions().emplace(std::move(altLoc), std::move(region));
        }
    }

    // 两个路径都应有各自的纹理（不互相覆盖）
    auto modernIt = mgr.textureRegions().find(ResourceLocation("minecraft", "textures/block/stone"));
    auto legacyIt = mgr.textureRegions().find(ResourceLocation("minecraft", "textures/blocks/stone"));
    ASSERT_NE(modernIt, mgr.textureRegions().end());
    ASSERT_NE(legacyIt, mgr.textureRegions().end());
}

} // namespace mc
