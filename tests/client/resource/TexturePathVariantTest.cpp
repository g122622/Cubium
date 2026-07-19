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

#include "client/resource/atlas/TexturePathVariant.hpp"

namespace mc {

// ============================================================================
// TexturePathVariant::getAltTexturePath 静态方法测试
// 路径变体转换：textures/block <-> textures/blocks、textures/item <-> textures/items、
// textures/entity/<name>/<name> <-> textures/entity/<name>
// ============================================================================

TEST(GetAltTexturePathTest, ModernBlockPathReturnsLegacy)
{
    // textures/block/ -> textures/blocks/
    EXPECT_EQ(client::resource::atlas::TexturePathVariant::getAltTexturePath("textures/block/stone"),
        "textures/blocks/stone");
    EXPECT_EQ(client::resource::atlas::TexturePathVariant::getAltTexturePath("textures/block/grass_block_top"),
        "textures/blocks/grass_block_top");
    EXPECT_EQ(
        client::resource::atlas::TexturePathVariant::getAltTexturePath("textures/block/dirt"), "textures/blocks/dirt");
}

TEST(GetAltTexturePathTest, LegacyBlockPathReturnsModern)
{
    // textures/blocks/ -> textures/block/
    EXPECT_EQ(client::resource::atlas::TexturePathVariant::getAltTexturePath("textures/blocks/stone"),
        "textures/block/stone");
    EXPECT_EQ(client::resource::atlas::TexturePathVariant::getAltTexturePath("textures/blocks/grass_block_top"),
        "textures/block/grass_block_top");
}

TEST(GetAltTexturePathTest, ModernItemPathReturnsLegacy)
{
    // textures/item/ -> textures/items/
    EXPECT_EQ(client::resource::atlas::TexturePathVariant::getAltTexturePath("textures/item/diamond"),
        "textures/items/diamond");
    EXPECT_EQ(client::resource::atlas::TexturePathVariant::getAltTexturePath("textures/item/iron_ingot"),
        "textures/items/iron_ingot");
}

TEST(GetAltTexturePathTest, LegacyItemPathReturnsModern)
{
    // textures/items/ -> textures/item/
    EXPECT_EQ(client::resource::atlas::TexturePathVariant::getAltTexturePath("textures/items/diamond"),
        "textures/item/diamond");
    EXPECT_EQ(client::resource::atlas::TexturePathVariant::getAltTexturePath("textures/items/iron_ingot"),
        "textures/item/iron_ingot");
}

TEST(GetAltTexturePathTest, NonTexturePathReturnsEmpty)
{
    // 不匹配任何已知前缀时返回空字符串
    EXPECT_TRUE(client::resource::atlas::TexturePathVariant::getAltTexturePath("models/block/stone").empty());
    EXPECT_TRUE(client::resource::atlas::TexturePathVariant::getAltTexturePath("blockstates/stone").empty());
    // 注意：textures/entity/steve 现在会返回 textures/entity/steve/steve（实体路径变体）
    // 而不是返回空字符串
    EXPECT_TRUE(client::resource::atlas::TexturePathVariant::getAltTexturePath("textures/environment/clouds").empty());
    EXPECT_TRUE(client::resource::atlas::TexturePathVariant::getAltTexturePath("textures/painting/kebab").empty());
    EXPECT_TRUE(client::resource::atlas::TexturePathVariant::getAltTexturePath("").empty());
}

TEST(GetAltTexturePathTest, ExactPrefixWithoutSuffixReturnsEmpty)
{
    // 仅有前缀没有后续内容时返回空字符串（路径太短，无实际纹理名）
    EXPECT_TRUE(client::resource::atlas::TexturePathVariant::getAltTexturePath("textures/block/").empty());
    EXPECT_TRUE(client::resource::atlas::TexturePathVariant::getAltTexturePath("textures/blocks/").empty());
    EXPECT_TRUE(client::resource::atlas::TexturePathVariant::getAltTexturePath("textures/item/").empty());
    EXPECT_TRUE(client::resource::atlas::TexturePathVariant::getAltTexturePath("textures/items/").empty());
}

TEST(GetAltTexturePathTest, PathWithSubdirectories)
{
    // 包含子目录的路径也能正确转换
    EXPECT_EQ(client::resource::atlas::TexturePathVariant::getAltTexturePath("textures/block/flower/rose"),
        "textures/blocks/flower/rose");
    EXPECT_EQ(client::resource::atlas::TexturePathVariant::getAltTexturePath("textures/blocks/flower/rose"),
        "textures/block/flower/rose");
    EXPECT_EQ(client::resource::atlas::TexturePathVariant::getAltTexturePath("textures/item/diamond_helmet"),
        "textures/items/diamond_helmet");
}

// ============================================================================
// 实体纹理路径变体测试
// getAltTexturePath 扩展支持 textures/entity/<name>/<name> <-> textures/entity/<name>
// ============================================================================

TEST(GetAltTexturePathTest, EntitySubdirectoryToFlat)
{
    // MC 1.13+ 子目录格式 -> MC 1.12 扁平格式
    // 不含 .png 后缀
    EXPECT_EQ(client::resource::atlas::TexturePathVariant::getAltTexturePath("textures/entity/pig/pig"),
        "textures/entity/pig");
    EXPECT_EQ(client::resource::atlas::TexturePathVariant::getAltTexturePath("textures/entity/creeper/creeper"),
        "textures/entity/creeper");
    EXPECT_EQ(client::resource::atlas::TexturePathVariant::getAltTexturePath("textures/entity/zombie/zombie"),
        "textures/entity/zombie");
}

TEST(GetAltTexturePathTest, EntityFlatToSubdirectory)
{
    // MC 1.12 扁平格式 -> MC 1.13+ 子目录格式
    // 不含 .png 后缀
    EXPECT_EQ(client::resource::atlas::TexturePathVariant::getAltTexturePath("textures/entity/pig"),
        "textures/entity/pig/pig");
    EXPECT_EQ(client::resource::atlas::TexturePathVariant::getAltTexturePath("textures/entity/creeper"),
        "textures/entity/creeper/creeper");
    EXPECT_EQ(client::resource::atlas::TexturePathVariant::getAltTexturePath("textures/entity/bat"),
        "textures/entity/bat/bat");
}

TEST(GetAltTexturePathTest, EntitySubdirectoryWithPngSuffix)
{
    // 带 .png 后缀的路径：转换后保留 .png 后缀
    // 子目录 -> 扁平
    EXPECT_EQ(client::resource::atlas::TexturePathVariant::getAltTexturePath("textures/entity/pig/pig.png"),
        "textures/entity/pig.png");
    EXPECT_EQ(client::resource::atlas::TexturePathVariant::getAltTexturePath("textures/entity/zombie/zombie.png"),
        "textures/entity/zombie.png");
    // 扁平 -> 子目录
    EXPECT_EQ(client::resource::atlas::TexturePathVariant::getAltTexturePath("textures/entity/pig.png"),
        "textures/entity/pig/pig.png");
    EXPECT_EQ(client::resource::atlas::TexturePathVariant::getAltTexturePath("textures/entity/bat.png"),
        "textures/entity/bat/bat.png");
}

TEST(GetAltTexturePathTest, EntitySubdirectoryMismatchedNameReturnsEmpty)
{
    // 子目录名与文件名不匹配时不转换（如 textures/entity/horse/horse_brown）
    EXPECT_TRUE(
        client::resource::atlas::TexturePathVariant::getAltTexturePath("textures/entity/horse/horse_brown").empty());
    EXPECT_TRUE(
        client::resource::atlas::TexturePathVariant::getAltTexturePath("textures/entity/cow/red_mooshroom").empty());
    // 带 .png 后缀也不匹配
    EXPECT_TRUE(client::resource::atlas::TexturePathVariant::getAltTexturePath("textures/entity/horse/horse_brown.png")
            .empty());
    EXPECT_TRUE(client::resource::atlas::TexturePathVariant::getAltTexturePath("textures/entity/cow/red_mooshroom.png")
            .empty());
}

TEST(GetAltTexturePathTest, EntityPrefixOnlyReturnsEmpty)
{
    // 仅有 textures/entity/ 前缀没有实体名时返回空
    EXPECT_TRUE(client::resource::atlas::TexturePathVariant::getAltTexturePath("textures/entity/").empty());
}

} // namespace mc
