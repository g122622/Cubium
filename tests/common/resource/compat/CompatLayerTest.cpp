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

#include "resource/compat/PackFormat.hpp"
#include "resource/compat/ResourceMapper.hpp"
#include "resource/compat/TextureMapper.hpp"
#include <gtest/gtest.h>

using namespace mc;
using namespace mc::resource::compat;

/**
 * @brief 测试从 pack_format 值检测 PackFormat
 */
class PackFormatTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(PackFormatTest, DetectFormat_ValidValues)
{
    EXPECT_EQ(detectPackFormat(1), PackFormat::V1_6_to_1_8);
    EXPECT_EQ(detectPackFormat(2), PackFormat::V1_9_to_1_10);
    EXPECT_EQ(detectPackFormat(3), PackFormat::V1_11_to_1_12);
    EXPECT_EQ(detectPackFormat(4), PackFormat::V1_13_to_1_14);
    EXPECT_EQ(detectPackFormat(5), PackFormat::V1_15_to_1_16_1);
    EXPECT_EQ(detectPackFormat(6), PackFormat::V1_16_2_to_1_16_5);
    EXPECT_EQ(detectPackFormat(7), PackFormat::V1_17);
    EXPECT_EQ(detectPackFormat(8), PackFormat::V1_18);
    EXPECT_EQ(detectPackFormat(9), PackFormat::V1_19);
}

TEST_F(PackFormatTest, DetectFormat_UnknownValue)
{
    EXPECT_EQ(detectPackFormat(0), PackFormat::Unknown);
    EXPECT_EQ(detectPackFormat(-1), PackFormat::Unknown);
    EXPECT_EQ(detectPackFormat(100), PackFormat::Unknown);
}

TEST_F(PackFormatTest, UsesOldTexturePaths)
{
    // 旧版格式使用 textures/blocks/
    EXPECT_TRUE(usesOldTexturePaths(PackFormat::V1_6_to_1_8));
    EXPECT_TRUE(usesOldTexturePaths(PackFormat::V1_9_to_1_10));
    EXPECT_TRUE(usesOldTexturePaths(PackFormat::V1_11_to_1_12));

    // 新版格式使用 textures/block/
    EXPECT_FALSE(usesOldTexturePaths(PackFormat::V1_13_to_1_14));
    EXPECT_FALSE(usesOldTexturePaths(PackFormat::V1_16_2_to_1_16_5));
    EXPECT_FALSE(usesOldTexturePaths(PackFormat::V1_19));
}

TEST_F(PackFormatTest, UsesNewTexturePaths)
{
    // 旧版格式不使用新路径
    EXPECT_FALSE(usesNewTexturePaths(PackFormat::V1_6_to_1_8));
    EXPECT_FALSE(usesNewTexturePaths(PackFormat::V1_11_to_1_12));

    // 新版格式使用 textures/block/
    EXPECT_TRUE(usesNewTexturePaths(PackFormat::V1_13_to_1_14));
    EXPECT_TRUE(usesNewTexturePaths(PackFormat::V1_15_to_1_16_1));
    EXPECT_TRUE(usesNewTexturePaths(PackFormat::V1_16_2_to_1_16_5));
    EXPECT_TRUE(usesNewTexturePaths(PackFormat::V1_17));
    EXPECT_TRUE(usesNewTexturePaths(PackFormat::V1_18));
    EXPECT_TRUE(usesNewTexturePaths(PackFormat::V1_19));
}

TEST_F(PackFormatTest, RequiresTextureNameMapping)
{
    // 旧版格式需要名称映射
    EXPECT_TRUE(requiresTextureNameMapping(PackFormat::V1_6_to_1_8));
    EXPECT_TRUE(requiresTextureNameMapping(PackFormat::V1_11_to_1_12));

    // 新版格式不需要名称映射
    EXPECT_FALSE(requiresTextureNameMapping(PackFormat::V1_13_to_1_14));
    EXPECT_FALSE(requiresTextureNameMapping(PackFormat::V1_19));
}

TEST_F(PackFormatTest, PackFormatToString)
{
    EXPECT_EQ(packFormatToString(PackFormat::Unknown), "Unknown");
    EXPECT_EQ(packFormatToString(PackFormat::V1_6_to_1_8), "1.6-1.8");
    EXPECT_EQ(packFormatToString(PackFormat::V1_11_to_1_12), "1.11-1.12");
    EXPECT_EQ(packFormatToString(PackFormat::V1_13_to_1_14), "1.13-1.14");
    EXPECT_EQ(packFormatToString(PackFormat::V1_16_2_to_1_16_5), "1.16.2-1.16.5");
    EXPECT_EQ(packFormatToString(PackFormat::V1_19), "1.19");
}

/**
 * @brief 测试 TextureMapper 双向映射
 */
class TextureMapperTest : public ::testing::Test {
protected:
    const TextureMapper& mapper = TextureMapper::instance();
};

// 原木纹理
TEST_F(TextureMapperTest, LogTextures)
{
    EXPECT_EQ(mapper.getLegacyName("oak_log"), "log_oak");
    EXPECT_EQ(mapper.getModernName("log_oak"), "oak_log");

    EXPECT_EQ(mapper.getLegacyName("jungle_log"), "log_jungle");
    EXPECT_EQ(mapper.getModernName("log_jungle"), "jungle_log");

    EXPECT_EQ(mapper.getLegacyName("dark_oak_log"), "log_big_oak");
    EXPECT_EQ(mapper.getModernName("log_big_oak"), "dark_oak_log");

    // 原木顶部
    EXPECT_EQ(mapper.getLegacyName("jungle_log_top"), "log_jungle_top");
    EXPECT_EQ(mapper.getModernName("log_jungle_top"), "jungle_log_top");
}

// 树叶纹理
TEST_F(TextureMapperTest, LeafTextures)
{
    EXPECT_EQ(mapper.getLegacyName("oak_leaves"), "leaves_oak");
    EXPECT_EQ(mapper.getModernName("leaves_oak"), "oak_leaves");

    EXPECT_EQ(mapper.getLegacyName("dark_oak_leaves"), "leaves_big_oak");
    EXPECT_EQ(mapper.getModernName("leaves_big_oak"), "dark_oak_leaves");
}

// 羊毛纹理
TEST_F(TextureMapperTest, WoolTextures)
{
    EXPECT_EQ(mapper.getLegacyName("white_wool"), "wool_colored_white");
    EXPECT_EQ(mapper.getModernName("wool_colored_white"), "white_wool");

    EXPECT_EQ(mapper.getLegacyName("light_gray_wool"), "wool_colored_silver");
    EXPECT_EQ(mapper.getModernName("wool_colored_silver"), "light_gray_wool");

    EXPECT_EQ(mapper.getLegacyName("red_wool"), "wool_colored_red");
    EXPECT_EQ(mapper.getModernName("wool_colored_red"), "red_wool");
}

// 石头变种
TEST_F(TextureMapperTest, StoneVariants)
{
    EXPECT_EQ(mapper.getLegacyName("granite"), "stone_granite");
    EXPECT_EQ(mapper.getModernName("stone_granite"), "granite");

    EXPECT_EQ(mapper.getLegacyName("polished_granite"), "stone_granite_smooth");
    EXPECT_EQ(mapper.getModernName("stone_granite_smooth"), "polished_granite");

    EXPECT_EQ(mapper.getLegacyName("andesite"), "stone_andesite");
    EXPECT_EQ(mapper.getModernName("stone_andesite"), "andesite");
}

// 草方块
TEST_F(TextureMapperTest, GrassBlock)
{
    EXPECT_EQ(mapper.getLegacyName("grass_block_top"), "grass_top");
    EXPECT_EQ(mapper.getModernName("grass_top"), "grass_block_top");

    EXPECT_EQ(mapper.getLegacyName("grass_block_side"), "grass_side");
    EXPECT_EQ(mapper.getModernName("grass_side"), "grass_block_side");
}

// 花
TEST_F(TextureMapperTest, FlowerTextures)
{
    EXPECT_EQ(mapper.getLegacyName("dandelion"), "flower_dandelion");
    EXPECT_EQ(mapper.getModernName("flower_dandelion"), "dandelion");

    // 注意: rose -> poppy 重命名
    EXPECT_EQ(mapper.getLegacyName("poppy"), "flower_rose");
    EXPECT_EQ(mapper.getModernName("flower_rose"), "poppy");

    // 注意: houstonia -> azure_bluet 重命名
    EXPECT_EQ(mapper.getLegacyName("azure_bluet"), "flower_houstonia");
    EXPECT_EQ(mapper.getModernName("flower_houstonia"), "azure_bluet");
}

// 混凝土
TEST_F(TextureMapperTest, ConcreteTextures)
{
    EXPECT_EQ(mapper.getLegacyName("white_concrete"), "concrete_white");
    EXPECT_EQ(mapper.getModernName("concrete_white"), "white_concrete");

    EXPECT_EQ(mapper.getLegacyName("light_gray_concrete"), "concrete_silver");
    EXPECT_EQ(mapper.getModernName("concrete_silver"), "light_gray_concrete");
}

// 陶瓦
TEST_F(TextureMapperTest, TerracottaTextures)
{
    EXPECT_EQ(mapper.getLegacyName("white_terracotta"), "hardened_clay_stained_white");
    EXPECT_EQ(mapper.getModernName("hardened_clay_stained_white"), "white_terracotta");

    EXPECT_EQ(mapper.getLegacyName("cyan_terracotta"), "hardened_clay_stained_cyan");
    EXPECT_EQ(mapper.getModernName("hardened_clay_stained_cyan"), "cyan_terracotta");
}

// 砂岩
TEST_F(TextureMapperTest, SandstoneTextures)
{
    EXPECT_EQ(mapper.getLegacyName("cut_sandstone"), "sandstone_carved");
    EXPECT_EQ(mapper.getModernName("sandstone_carved"), "cut_sandstone");

    EXPECT_EQ(mapper.getLegacyName("chiseled_sandstone"), "sandstone_smooth");
    EXPECT_EQ(mapper.getModernName("sandstone_smooth"), "chiseled_sandstone");
}

// 高草
TEST_F(TextureMapperTest, TallGrassTextures)
{
    EXPECT_EQ(mapper.getLegacyName("short_grass"), "tallgrass");
    EXPECT_EQ(mapper.getModernName("tallgrass"), "short_grass");

    EXPECT_EQ(mapper.getLegacyName("tall_grass_top"), "double_plant_grass_top");
    EXPECT_EQ(mapper.getModernName("double_plant_grass_top"), "tall_grass_top");
}

// 存在映射
TEST_F(TextureMapperTest, HasMapping)
{
    EXPECT_TRUE(mapper.hasMapping("oak_log"));
    EXPECT_TRUE(mapper.hasMapping("log_oak"));
    EXPECT_TRUE(mapper.hasMapping("jungle_log"));
    EXPECT_TRUE(mapper.hasMapping("white_wool"));

    EXPECT_FALSE(mapper.hasMapping("nonexistent_texture"));
    EXPECT_FALSE(mapper.hasMapping("stone")); // stone 保持不变
}

// 名称变体
TEST_F(TextureMapperTest, GetNameVariants)
{
    auto variants = mapper.getNameVariants("jungle_log");
    EXPECT_GE(variants.size(), 2); // 至少有现代和旧版名称
    EXPECT_TRUE(std::find(variants.begin(), variants.end(), "jungle_log") != variants.end());
    EXPECT_TRUE(std::find(variants.begin(), variants.end(), "log_jungle") != variants.end());
}

// 路径转换
TEST_F(TextureMapperTest, PathTransformation)
{
    // 现代到旧版
    std::string legacy = mapper.toLegacyPath("textures/block/jungle_log.png");
    EXPECT_EQ(legacy, "textures/blocks/log_jungle.png");

    // 旧版到现代
    std::string modern = mapper.toModernPath("textures/blocks/log_jungle.png");
    EXPECT_EQ(modern, "textures/block/jungle_log.png");

    // 物品路径
    legacy = mapper.toLegacyPath("textures/item/diamond.png");
    EXPECT_EQ(legacy, "textures/items/diamond.png");

    modern = mapper.toModernPath("textures/items/diamond.png");
    EXPECT_EQ(modern, "textures/item/diamond.png");
}

// 路径变体
TEST_F(TextureMapperTest, GetPathVariants)
{
    auto variants = mapper.getPathVariants("textures/block/jungle_log.png");

    // 应该有多个变体
    EXPECT_GE(variants.size(), 2);

    // 检查现代和旧版路径都存在
    bool hasModern = false, hasLegacy = false;
    for (const auto& v : variants) {
        if (v.find("textures/block/jungle_log") != std::string::npos) hasModern = true;
        if (v.find("textures/blocks/log_jungle") != std::string::npos) hasLegacy = true;
    }
    EXPECT_TRUE(hasModern);
    EXPECT_TRUE(hasLegacy);
}

/**
 * @brief 测试 ResourceMapper 工厂
 */
class ResourceMapperFactoryTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(ResourceMapperFactoryTest, CreateV112Mapper)
{
    auto mapper = ResourceMapper::create(PackFormat::V1_11_to_1_12);
    ASSERT_NE(mapper, nullptr);
    EXPECT_EQ(mapper->getTargetFormat(), PackFormat::V1_11_to_1_12);
}

TEST_F(ResourceMapperFactoryTest, CreateV113Mapper)
{
    auto mapper = ResourceMapper::create(PackFormat::V1_13_to_1_14);
    ASSERT_NE(mapper, nullptr);
    EXPECT_EQ(mapper->getTargetFormat(), PackFormat::V1_13_to_1_14);
}

TEST_F(ResourceMapperFactoryTest, CreateV116Mapper)
{
    auto mapper = ResourceMapper::create(PackFormat::V1_16_2_to_1_16_5);
    ASSERT_NE(mapper, nullptr);
    // 现代版本应该使用 v1_13 映射器
    EXPECT_EQ(mapper->getTargetFormat(), PackFormat::V1_13_to_1_14);
}

TEST_F(ResourceMapperFactoryTest, CreateUnknownMapper)
{
    auto mapper = ResourceMapper::create(PackFormat::Unknown);
    ASSERT_NE(mapper, nullptr);
    // 未知格式默认使用现代格式
    EXPECT_EQ(mapper->getTargetFormat(), PackFormat::V1_13_to_1_14);
}

/**
 * @brief 测试 v1_12 映射器
 */
class ResourceMapperV112Test : public ::testing::Test {
protected:
    std::unique_ptr<ResourceMapper> mapper;

    void SetUp() override { mapper = ResourceMapper::create(PackFormat::V1_11_to_1_12); }
};

TEST_F(ResourceMapperV112Test, ToUnifiedTexturePath)
{
    // 将旧版路径转换为现代路径
    std::string unified = mapper->toUnifiedTexturePath("textures/blocks/log_jungle.png");
    EXPECT_EQ(unified, "textures/block/jungle_log.png");

    unified = mapper->toUnifiedTexturePath("textures/blocks/stone.png");
    EXPECT_EQ(unified, "textures/block/stone.png");

    unified = mapper->toUnifiedTexturePath("textures/items/diamond.png");
    EXPECT_EQ(unified, "textures/item/diamond.png");
}

TEST_F(ResourceMapperV112Test, GetTexturePathVariants)
{
    auto variants = mapper->getTexturePathVariants("textures/block/jungle_log.png");

    // 应该包含现代路径
    bool hasModernPath = false;
    for (const auto& v : variants) {
        if (v == "textures/block/jungle_log.png") {
            hasModernPath = true;
            break;
        }
    }
    EXPECT_TRUE(hasModernPath);

    // 应该包含旧版路径配旧版名称
    bool hasLegacyPath = false;
    for (const auto& v : variants) {
        if (v.find("textures/blocks/log_jungle") != std::string::npos) {
            hasLegacyPath = true;
            break;
        }
    }
    EXPECT_TRUE(hasLegacyPath);
}
