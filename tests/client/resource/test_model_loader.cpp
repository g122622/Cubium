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

#include "client/resource/BlockModelLoader.hpp"
#include "client/resource/BlockStateLoader.hpp"
#include "common/resource/FolderResourcePack.hpp"
#include <gtest/gtest.h>

using namespace mc;

// Direction测试
TEST(DirectionTest, ParseDirection)
{
    EXPECT_EQ(parseDirection("down"), Direction::Down);
    EXPECT_EQ(parseDirection("up"), Direction::Up);
    EXPECT_EQ(parseDirection("north"), Direction::North);
    EXPECT_EQ(parseDirection("south"), Direction::South);
    EXPECT_EQ(parseDirection("west"), Direction::West);
    EXPECT_EQ(parseDirection("east"), Direction::East);
    EXPECT_EQ(parseDirection("invalid"), Direction::None);
}

TEST(DirectionTest, DirectionToString)
{
    EXPECT_EQ(directionToString(Direction::Down), "down");
    EXPECT_EQ(directionToString(Direction::Up), "up");
    EXPECT_EQ(directionToString(Direction::North), "north");
    EXPECT_EQ(directionToString(Direction::South), "south");
    EXPECT_EQ(directionToString(Direction::West), "west");
    EXPECT_EQ(directionToString(Direction::East), "east");
    EXPECT_EQ(directionToString(Direction::None), "");
}

// BlockStateDefinition测试
TEST(BlockStateDefinitionTest, ParseSimpleVariant)
{
    const char* json = R"({
        "variants": {
            "normal": { "model": "stone" }
        }
    })";

    auto result = BlockStateDefinition::parse(json);
    ASSERT_TRUE(result.success());

    const auto* variants = result.value().getVariants("normal");
    ASSERT_NE(variants, nullptr);
    ASSERT_EQ(variants->variants.size(), 1u);
    EXPECT_EQ(variants->variants[0].model.toString(), "minecraft:stone");
    EXPECT_EQ(variants->variants[0].x, 0);
    EXPECT_EQ(variants->variants[0].y, 0);
}

TEST(BlockStateDefinitionTest, ParseVariantWithRotation)
{
    const char* json = R"({
        "variants": {
            "axis=x": { "model": "oak_log", "x": 90, "y": 90 }
        }
    })";

    auto result = BlockStateDefinition::parse(json);
    ASSERT_TRUE(result.success());

    const auto* variants = result.value().getVariants("axis=x");
    ASSERT_NE(variants, nullptr);
    ASSERT_EQ(variants->variants.size(), 1u);
    EXPECT_EQ(variants->variants[0].x, 90);
    EXPECT_EQ(variants->variants[0].y, 90);
}

TEST(BlockStateDefinitionTest, ParseVariantArray)
{
    const char* json = R"({
        "variants": {
            "normal": [
                { "model": "cobblestone", "weight": 1 },
                { "model": "cobblestone_1", "weight": 1 }
            ]
        }
    })";

    auto result = BlockStateDefinition::parse(json);
    ASSERT_TRUE(result.success());

    const auto* variants = result.value().getVariants("normal");
    ASSERT_NE(variants, nullptr);
    ASSERT_EQ(variants->variants.size(), 2u);
    EXPECT_EQ(variants->variants[0].weight, 1);
    EXPECT_EQ(variants->variants[1].weight, 1);
}

TEST(BlockStateDefinitionTest, VariantLookupIgnoresPropertyOrder)
{
    const char* json = R"({
        "variants": {
            "facing=north,lit=true": { "model": "block/redstone_lamp_on" }
        }
    })";

    auto result = BlockStateDefinition::parse(json);
    ASSERT_TRUE(result.success());

    const auto* variants = result.value().getVariants("lit=true,facing=north");
    ASSERT_NE(variants, nullptr);
    ASSERT_EQ(variants->variants.size(), 1u);
    EXPECT_EQ(variants->variants[0].model.toString(), "minecraft:block/redstone_lamp_on");
}

TEST(BlockStateDefinitionTest, EmptyAndNormalStateKeyAreEquivalent)
{
    const char* json = R"({
        "variants": {
            "": { "model": "block/stone" }
        }
    })";

    auto result = BlockStateDefinition::parse(json);
    ASSERT_TRUE(result.success());

    EXPECT_NE(result.value().getVariants(""), nullptr);
    EXPECT_NE(result.value().getVariants("normal"), nullptr);
}

TEST(BlockStateDefinitionTest, MultipartProvidesNormalFallbackVariant)
{
    const char* json = R"({
        "multipart": [
            { "apply": { "model": "block/redstone_dust_dot" } },
            { "apply": { "model": "block/redstone_dust_side", "y": 90 } }
        ]
    })";

    auto result = BlockStateDefinition::parse(json);
    ASSERT_TRUE(result.success());
    EXPECT_TRUE(result.value().hasMultipart());

    const auto* variants = result.value().getVariants("normal");
    ASSERT_NE(variants, nullptr);
    ASSERT_FALSE(variants->variants.empty());
    EXPECT_EQ(variants->variants[0].model.toString(), "minecraft:block/redstone_dust_dot");
}

// BlockModelLoader测试 - 使用实际资源包
class BlockModelLoaderTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        pack = std::make_unique<FolderResourcePack>("z:/方块概念材质");
        auto result = pack->initialize();
        packInitialized = result.success();
    }

    std::unique_ptr<FolderResourcePack> pack;
    bool packInitialized = false;
};

TEST_F(BlockModelLoaderTest, LoadSimpleModel)
{
    if (!packInitialized) {
        GTEST_SKIP() << "Resource pack not available";
    }

    BlockModelLoader loader;
    auto loadResult = loader.loadFromResourcePack(*pack);
    ASSERT_TRUE(loadResult.success());

    // 加载cobblestone模型
    ResourceLocation modelLoc("minecraft:block/cobblestone");
    auto result = loader.loadModel(modelLoc);

    if (result.success()) {
        const auto& model = result.value();
        EXPECT_EQ(model.parentLocation.toString(), "minecraft:block/cube_all");
        EXPECT_FALSE(model.textures.empty());
        EXPECT_TRUE(model.textures.count("all") > 0);
    }
}

TEST_F(BlockModelLoaderTest, BakeModel)
{
    if (!packInitialized) {
        GTEST_SKIP() << "Resource pack not available";
    }

    BlockModelLoader loader;
    auto loadResult = loader.loadFromResourcePack(*pack);
    ASSERT_TRUE(loadResult.success());

    // 烘焙cobblestone模型
    ResourceLocation modelLoc("minecraft:block/cobblestone");
    auto result = loader.bakeModel(modelLoc);

    if (result.success()) {
        const auto& baked = result.value();
        EXPECT_FALSE(baked.textures.empty());
        // cube_all父模型应该有elements
        EXPECT_FALSE(baked.elements.empty());
    }
}

TEST_F(BlockModelLoaderTest, LoadOakLogModel)
{
    if (!packInitialized) {
        GTEST_SKIP() << "Resource pack not available";
    }

    BlockModelLoader loader;
    auto loadResult = loader.loadFromResourcePack(*pack);
    ASSERT_TRUE(loadResult.success());

    ResourceLocation modelLoc("minecraft:block/oak_log");
    auto result = loader.bakeModel(modelLoc);

    if (result.success()) {
        const auto& baked = result.value();
        EXPECT_TRUE(baked.textures.count("end") > 0);
        EXPECT_TRUE(baked.textures.count("side") > 0);
    }
}

// BlockStateLoader测试
class BlockStateLoaderTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        pack = std::make_unique<FolderResourcePack>("z:/方块概念材质");
        auto result = pack->initialize();
        packInitialized = result.success();
    }

    std::unique_ptr<FolderResourcePack> pack;
    bool packInitialized = false;
};

TEST_F(BlockStateLoaderTest, LoadBlockStates)
{
    if (!packInitialized) {
        GTEST_SKIP() << "Resource pack not available";
    }

    BlockStateLoader loader;
    auto result = loader.loadFromResourcePack(*pack);
    ASSERT_TRUE(result.success());

    auto loadedStates = loader.getLoadedBlockStates();
    EXPECT_FALSE(loadedStates.empty());
}

TEST_F(BlockStateLoaderTest, GetOakLogVariant)
{
    if (!packInitialized) {
        GTEST_SKIP() << "Resource pack not available";
    }

    BlockStateLoader loader;
    auto result = loader.loadFromResourcePack(*pack);
    ASSERT_TRUE(result.success());

    ResourceLocation oakLog("minecraft:oak_log");
    const auto* variant = loader.getVariant(oakLog, "axis=y");
    if (variant) {
        EXPECT_EQ(variant->model.toString(), "minecraft:block/oak_log");
    }
}

TEST_F(BlockStateLoaderTest, GetCobblestoneVariant)
{
    if (!packInitialized) {
        GTEST_SKIP() << "Resource pack not available";
    }

    BlockStateLoader loader;
    auto result = loader.loadFromResourcePack(*pack);
    ASSERT_TRUE(result.success());

    ResourceLocation cobblestone("minecraft:cobblestone");
    const auto* variant = loader.getVariant(cobblestone, "normal");
    if (variant) {
        // cobblestone有多个变体，选择第一个
        EXPECT_TRUE(variant->model.path().find("cobblestone") != std::string::npos);
    }
}
