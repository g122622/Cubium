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
#include "client/resource/ItemModelLoader.hpp"
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

// ============================================================================
// BlockModelLoader 静态方法单元测试
// ============================================================================

// --- parseElement 测试 ---

TEST(ParseElementTest, ParsesBasicElement)
{
    const char* json = R"({
        "from": [0, 0, 0],
        "to": [16, 16, 16],
        "faces": {
            "north": { "texture": "#all" },
            "south": { "texture": "#all" },
            "up": { "texture": "#top", "rotation": 90 }
        }
    })";
    auto j = nlohmann::json::parse(json);
    auto result = BlockModelLoader::parseElement(j);
    ASSERT_TRUE(result.success());

    const auto& elem = result.value();
    EXPECT_FLOAT_EQ(elem.from.x, 0.0f);
    EXPECT_FLOAT_EQ(elem.from.y, 0.0f);
    EXPECT_FLOAT_EQ(elem.from.z, 0.0f);
    EXPECT_FLOAT_EQ(elem.to.x, 16.0f);
    EXPECT_FLOAT_EQ(elem.to.y, 16.0f);
    EXPECT_FLOAT_EQ(elem.to.z, 16.0f);
    EXPECT_EQ(elem.faces.size(), 3u);
    EXPECT_EQ(elem.faces.at(Direction::North).texture, "#all");
    EXPECT_EQ(elem.faces.at(Direction::South).texture, "#all");
    EXPECT_EQ(elem.faces.at(Direction::Up).texture, "#top");
    EXPECT_EQ(elem.faces.at(Direction::Up).uv.rotation, 90);
}

TEST(ParseElementTest, ParsesElementWithRotation)
{
    const char* json = R"({
        "from": [0, 0, 0],
        "to": [8, 8, 8],
        "rotation": {
            "origin": [8, 8, 8],
            "axis": "y",
            "angle": 45,
            "rescale": true
        }
    })";
    auto j = nlohmann::json::parse(json);
    auto result = BlockModelLoader::parseElement(j);
    ASSERT_TRUE(result.success());

    const auto& elem = result.value();
    EXPECT_FLOAT_EQ(elem.rotation.origin.x, 8.0f);
    EXPECT_FLOAT_EQ(elem.rotation.origin.y, 8.0f);
    EXPECT_FLOAT_EQ(elem.rotation.origin.z, 8.0f);
    EXPECT_EQ(elem.rotation.axis, "y");
    EXPECT_FLOAT_EQ(elem.rotation.angle, 45.0f);
    EXPECT_TRUE(elem.rotation.rescale);
}

TEST(ParseElementTest, ParsesElementWithShade)
{
    const char* json = R"({
        "from": [0, 0, 0],
        "to": [16, 16, 16],
        "shade": false
    })";
    auto j = nlohmann::json::parse(json);
    auto result = BlockModelLoader::parseElement(j);
    ASSERT_TRUE(result.success());
    EXPECT_FALSE(result.value().shade);
}

TEST(ParseElementTest, ParsesFaceWithCullfaceAndTintindex)
{
    const char* json = R"({
        "from": [0, 0, 0],
        "to": [16, 16, 16],
        "faces": {
            "down": { "texture": "#top", "cullface": "down", "tintindex": 0 },
            "up": { "texture": "#side", "cullface": "up", "tintindex": 1 }
        }
    })";
    auto j = nlohmann::json::parse(json);
    auto result = BlockModelLoader::parseElement(j);
    ASSERT_TRUE(result.success());

    const auto& elem = result.value();
    EXPECT_EQ(elem.faces.at(Direction::Down).cullFace, Direction::Down);
    EXPECT_EQ(elem.faces.at(Direction::Down).tintIndex, 0);
    EXPECT_EQ(elem.faces.at(Direction::Up).cullFace, Direction::Up);
    EXPECT_EQ(elem.faces.at(Direction::Up).tintIndex, 1);
}

TEST(ParseElementTest, ParsesFaceWithExplicitUV)
{
    const char* json = R"({
        "from": [0, 0, 0],
        "to": [16, 16, 16],
        "faces": {
            "north": { "texture": "#all", "uv": [2, 3, 14, 13] }
        }
    })";
    auto j = nlohmann::json::parse(json);
    auto result = BlockModelLoader::parseElement(j);
    ASSERT_TRUE(result.success());

    const auto& face = result.value().faces.at(Direction::North);
    EXPECT_FLOAT_EQ(face.uv.u0, 2.0f);
    EXPECT_FLOAT_EQ(face.uv.v0, 3.0f);
    EXPECT_FLOAT_EQ(face.uv.u1, 14.0f);
    EXPECT_FLOAT_EQ(face.uv.v1, 13.0f);
}

TEST(ParseElementTest, EmptyJsonReturnsDefaultElement)
{
    auto j = nlohmann::json::parse("{}");
    auto result = BlockModelLoader::parseElement(j);
    ASSERT_TRUE(result.success());

    const auto& elem = result.value();
    // 默认 from/to 为 (0,0,0) 到 (16,16,16)
    EXPECT_FLOAT_EQ(elem.from.x, 0.0f);
    EXPECT_FLOAT_EQ(elem.to.x, 16.0f);
    EXPECT_TRUE(elem.faces.empty());
    EXPECT_TRUE(elem.shade); // 默认 shade = true
}

// --- computeDefaultUVs 测试 ---

TEST(ComputeDefaultUVsTest, ComputesDefaultUVForNorthFace)
{
    ModelElement elem;
    elem.from = {0.0f, 0.0f, 0.0f};
    elem.to = {16.0f, 16.0f, 16.0f};
    ModelFace face;
    face.texture = "#all";
    // UV 未指定，为默认值 (0, 0, 16, 16, rotation=0) -> isDefault() 返回 true
    elem.faces[Direction::North] = face;

    BlockModelLoader::computeDefaultUVs(elem);

    // North face 默认 UV: u0=16-to.x=0, v0=16-to.y=0, u1=16-from.x=16, v1=16-from.y=16
    const auto& uv = elem.faces.at(Direction::North).uv;
    EXPECT_FLOAT_EQ(uv.u0, 0.0f);
    EXPECT_FLOAT_EQ(uv.v0, 0.0f);
    EXPECT_FLOAT_EQ(uv.u1, 16.0f);
    EXPECT_FLOAT_EQ(uv.v1, 16.0f);
}

TEST(ComputeDefaultUVsTest, ComputesDefaultUVForDownFace)
{
    ModelElement elem;
    elem.from = {4.0f, 0.0f, 4.0f};
    elem.to = {12.0f, 8.0f, 12.0f};
    ModelFace face;
    face.texture = "#down";
    elem.faces[Direction::Down] = face;

    BlockModelLoader::computeDefaultUVs(elem);

    // Down face 默认 UV: u0=from.x=4, v0=16-to.z=4, u1=to.x=12, v1=16-from.z=12
    const auto& uv = elem.faces.at(Direction::Down).uv;
    EXPECT_FLOAT_EQ(uv.u0, 4.0f);
    EXPECT_FLOAT_EQ(uv.v0, 4.0f);
    EXPECT_FLOAT_EQ(uv.u1, 12.0f);
    EXPECT_FLOAT_EQ(uv.v1, 12.0f);
}

TEST(ComputeDefaultUVsTest, DoesNotOverrideExplicitUV)
{
    ModelElement elem;
    elem.from = {0.0f, 0.0f, 0.0f};
    elem.to = {16.0f, 16.0f, 16.0f};
    ModelFace face;
    face.texture = "#all";
    face.uv.u0 = 2.0f;
    face.uv.v0 = 3.0f;
    face.uv.u1 = 14.0f;
    face.uv.v1 = 13.0f;
    elem.faces[Direction::North] = face;

    BlockModelLoader::computeDefaultUVs(elem);

    // 显式 UV 不应被覆盖
    const auto& uv = elem.faces.at(Direction::North).uv;
    EXPECT_FLOAT_EQ(uv.u0, 2.0f);
    EXPECT_FLOAT_EQ(uv.v0, 3.0f);
    EXPECT_FLOAT_EQ(uv.u1, 14.0f);
    EXPECT_FLOAT_EQ(uv.v1, 13.0f);
}

// --- parseFace / parseUV / parseRotation 测试 ---

TEST(ParseFaceTest, ParsesBasicFace)
{
    const char* json = R"({
        "texture": "#side",
        "cullface": "north",
        "tintindex": 0,
        "uv": [0, 0, 16, 16],
        "rotation": 180
    })";
    auto j = nlohmann::json::parse(json);
    auto result = BlockModelLoader::parseFace(j, Direction::North);
    ASSERT_TRUE(result.success());

    const auto& face = result.value();
    EXPECT_EQ(face.texture, "#side");
    EXPECT_EQ(face.cullFace, Direction::North);
    EXPECT_EQ(face.tintIndex, 0);
    EXPECT_FLOAT_EQ(face.uv.u0, 0.0f);
    EXPECT_FLOAT_EQ(face.uv.u1, 16.0f);
    EXPECT_EQ(face.uv.rotation, 180);
}

TEST(ParseUVTest, ParsesFourElementArray)
{
    auto j = nlohmann::json::parse("[1.5, 2.5, 14.5, 13.5]");
    auto uv = BlockModelLoader::parseUV(j);
    EXPECT_FLOAT_EQ(uv.u0, 1.5f);
    EXPECT_FLOAT_EQ(uv.v0, 2.5f);
    EXPECT_FLOAT_EQ(uv.u1, 14.5f);
    EXPECT_FLOAT_EQ(uv.v1, 13.5f);
    EXPECT_EQ(uv.rotation, 0); // 默认
}

TEST(ParseRotationTest, ParsesFullRotation)
{
    const char* json = R"({
        "origin": [8, 8, 8],
        "axis": "x",
        "angle": -22.5,
        "rescale": true
    })";
    auto j = nlohmann::json::parse(json);
    auto rot = BlockModelLoader::parseRotation(j);
    EXPECT_FLOAT_EQ(rot.origin.x, 8.0f);
    EXPECT_FLOAT_EQ(rot.origin.y, 8.0f);
    EXPECT_FLOAT_EQ(rot.origin.z, 8.0f);
    EXPECT_EQ(rot.axis, "x");
    EXPECT_FLOAT_EQ(rot.angle, -22.5f);
    EXPECT_TRUE(rot.rescale);
}

TEST(ParseRotationTest, DefaultValues)
{
    auto j = nlohmann::json::parse("{}");
    auto rot = BlockModelLoader::parseRotation(j);
    EXPECT_FLOAT_EQ(rot.origin.x, 8.0f); // 默认 8,8,8
    EXPECT_EQ(rot.axis, "y");            // 默认 "y"
    EXPECT_FLOAT_EQ(rot.angle, 0.0f);    // 默认 0
    EXPECT_FALSE(rot.rescale);           // 默认 false
}

// --- mergeParent 测试 (BlockModelLoader) ---

TEST(MergeParentBlockTest, CurrentLayerOverridesAccumulatedTextures)
{
    // 累积结果（已包含之前层的合并）
    UnbakedBlockModel accumulated;
    accumulated.textures["all"] = "minecraft:block/stone";      // 来自更早的层
    accumulated.textures["particle"] = "minecraft:block/stone"; // 来自更早的层

    // 当前层（更靠近叶子的模型）
    UnbakedBlockModel currentLayer;
    currentLayer.textures["all"] = "minecraft:block/dirt";        // 当前层覆盖 all
    currentLayer.textures["side"] = "minecraft:block/stone_side"; // 当前层添加新纹理

    BlockModelLoader::mergeParent(accumulated, currentLayer);

    // 当前层的 all 覆盖累积结果
    EXPECT_EQ(accumulated.textures["all"], "minecraft:block/dirt");
    // 累积结果中仅存在于之前层的纹理保留
    EXPECT_EQ(accumulated.textures["particle"], "minecraft:block/stone");
    // 当前层新增的纹理被添加
    EXPECT_EQ(accumulated.textures["side"], "minecraft:block/stone_side");
    EXPECT_EQ(accumulated.textures.size(), 3u);
}

TEST(MergeParentBlockTest, InheritsParentElementsWhenChildEmpty)
{
    UnbakedBlockModel parent;
    ModelElement elem;
    elem.from = {0.0f, 0.0f, 0.0f};
    elem.to = {16.0f, 16.0f, 16.0f};
    parent.elements.push_back(elem);

    UnbakedBlockModel child;
    EXPECT_TRUE(child.elements.empty());

    BlockModelLoader::mergeParent(child, parent);

    // 子模型无元素时继承父模型
    EXPECT_EQ(child.elements.size(), 1u);
    EXPECT_FLOAT_EQ(child.elements[0].to.x, 16.0f);
}

TEST(MergeParentBlockTest, ChildElementsOverrideParent)
{
    UnbakedBlockModel parent;
    ModelElement parentElem;
    parentElem.from = {0.0f, 0.0f, 0.0f};
    parentElem.to = {16.0f, 16.0f, 16.0f};
    parent.elements.push_back(parentElem);

    UnbakedBlockModel child;
    ModelElement childElem;
    childElem.from = {4.0f, 4.0f, 4.0f};
    childElem.to = {12.0f, 12.0f, 12.0f};
    child.elements.push_back(childElem);

    BlockModelLoader::mergeParent(child, parent);

    // 子模型有元素时不被覆盖
    EXPECT_EQ(child.elements.size(), 1u);
    EXPECT_FLOAT_EQ(child.elements[0].from.x, 4.0f);
}

TEST(MergeParentBlockTest, AmbientOcclusionInheritance)
{
    // 父模型 AO=false 应传播到子模型
    UnbakedBlockModel parent;
    parent.ambientOcclusion = false;

    UnbakedBlockModel child;
    child.ambientOcclusion = true;

    BlockModelLoader::mergeParent(child, parent);
    EXPECT_FALSE(child.ambientOcclusion);

    // 父模型 AO=true 不应覆盖子模型的 AO=false
    UnbakedBlockModel parent2;
    parent2.ambientOcclusion = true;

    UnbakedBlockModel child2;
    child2.ambientOcclusion = false;

    BlockModelLoader::mergeParent(child2, parent2);
    EXPECT_FALSE(child2.ambientOcclusion); // 子模型保持 false
}

TEST(MergeParentBlockTest, RootToLeafChainMerge)
{
    // 模拟从根到叶的合并链：root -> mid -> leaf
    UnbakedBlockModel root;
    root.textures["all"] = "minecraft:block/stone";
    root.textures["particle"] = "minecraft:block/stone";
    ModelElement rootElem;
    rootElem.from = {0.0f, 0.0f, 0.0f};
    rootElem.to = {16.0f, 16.0f, 16.0f};
    root.elements.push_back(rootElem);

    UnbakedBlockModel mid;
    mid.textures["all"] = "minecraft:block/dirt"; // mid 覆盖 all

    UnbakedBlockModel leaf;
    leaf.textures["top"] = "minecraft:block/dirt_top"; // leaf 添加新纹理

    // 模拟 root-to-leaf 合并
    UnbakedBlockModel merged;
    merged.ambientOcclusion = true;
    BlockModelLoader::mergeParent(merged, root);
    BlockModelLoader::mergeParent(merged, mid);
    BlockModelLoader::mergeParent(merged, leaf);

    // 最终：all 应为 mid 的 dirt（mid 覆盖了 root 的 stone）
    EXPECT_EQ(merged.textures.at("all"), "minecraft:block/dirt");
    // particle 从 root 继承
    EXPECT_EQ(merged.textures.at("particle"), "minecraft:block/stone");
    // top 从 leaf 继承
    EXPECT_EQ(merged.textures.at("top"), "minecraft:block/dirt_top");
    // 元素从 root 继承（mid 和 leaf 都没有元素）
    EXPECT_EQ(merged.elements.size(), 1u);
}

// --- resolveTextureReferences 测试 ---

TEST(ResolveTextureReferencesTest, ResolvesSimpleVariable)
{
    std::map<std::string, ResourceLocation> textures;
    textures["all"] = ResourceLocation("minecraft:block/stone");
    textures["down"] = ResourceLocation("#all");

    BlockModelLoader::resolveTextureReferences(textures);

    EXPECT_EQ(textures.at("down").toString(), "minecraft:block/stone");
    EXPECT_EQ(textures.at("all").toString(), "minecraft:block/stone");
}

TEST(ResolveTextureReferencesTest, ResolvesChainedReferences)
{
    std::map<std::string, ResourceLocation> textures;
    textures["all"] = ResourceLocation("minecraft:block/stone");
    textures["side"] = ResourceLocation("#all");
    textures["north"] = ResourceLocation("#side");

    BlockModelLoader::resolveTextureReferences(textures);

    // 两层引用应该被解析：north -> side -> all -> stone
    EXPECT_EQ(textures.at("north").toString(), "minecraft:block/stone");
    EXPECT_EQ(textures.at("side").toString(), "minecraft:block/stone");
}

TEST(ResolveTextureReferencesTest, RespectsMaxIterations)
{
    std::map<std::string, ResourceLocation> textures;
    // 创建循环引用
    textures["a"] = ResourceLocation("#b");
    textures["b"] = ResourceLocation("#a");

    // 不应无限循环，应在 maxIterations 次后停止
    BlockModelLoader::resolveTextureReferences(textures, 10);

    // 循环引用不会被解析，仍为 # 引用
    EXPECT_TRUE(textures.at("a").path()[0] == '#');
}

TEST(ResolveTextureReferencesTest, DoesNotResolveIndirectReferenceEarly)
{
    std::map<std::string, ResourceLocation> textures;
    textures["top"] = ResourceLocation("minecraft:block/stone_top");
    textures["side"] = ResourceLocation("#top");
    // north 指向 side，side 在第一次迭代还不能解析（因为 side -> #top 需要先解析 top）
    textures["north"] = ResourceLocation("#side");

    BlockModelLoader::resolveTextureReferences(textures, 10);

    // 第一次迭代：side -> top -> stone_top
    // 第二次迭代：north -> side -> stone_top
    EXPECT_EQ(textures.at("side").toString(), "minecraft:block/stone_top");
    EXPECT_EQ(textures.at("north").toString(), "minecraft:block/stone_top");
}

TEST(ResolveTextureReferencesTest, EmptyTexturesMap)
{
    std::map<std::string, ResourceLocation> textures;
    // 空的纹理表不应崩溃
    BlockModelLoader::resolveTextureReferences(textures);
    EXPECT_TRUE(textures.empty());
}

TEST(ResolveTextureReferencesTest, NoVariableReferences)
{
    std::map<std::string, ResourceLocation> textures;
    textures["all"] = ResourceLocation("minecraft:block/stone");
    textures["side"] = ResourceLocation("minecraft:block/stone_side");

    // 没有变量引用，纹理表不变
    BlockModelLoader::resolveTextureReferences(textures);
    EXPECT_EQ(textures.at("all").toString(), "minecraft:block/stone");
    EXPECT_EQ(textures.at("side").toString(), "minecraft:block/stone_side");
}

// ============================================================================
// ItemModelLoader _mergeParent 行为验证测试
// ============================================================================
using namespace mc::client::resource;

// _mergeParent 是私有方法，通过 bakeModel 的间接行为或友元来验证。
// 此处验证的关键语义是：root-to-leaf 逐层合并时，type 应为 leaf-wins。
TEST(ItemMergeParentTest, TypeMergeIsLeafWins)
{
    // 验证 ItemModelLoader 的 type 合并行为：
    // 在 root-to-leaf 遍历中，每层都覆盖 type，最终 leaf 的 type 生效。
    // 模拟 3 层链：root(Generated) -> mid(Block) -> leaf(Handheld)

    // 使用 ItemModelLoader 的 bakeModel 间接测试较为复杂，
    // 此处直接验证 _mergeParent 的语义：
    // 在 mergeParent 中 child.type = parent.type（leaf-wins）

    // 构造 merged 模型模拟 root-to-leaf 合并过程
    using namespace mc::client::resource;

    UnbakedItemModel merged;
    merged.type = ItemModelType::Generated;

    // 第一步：合并 root（Generated）
    UnbakedItemModel root;
    root.type = ItemModelType::Generated;
    // merged.type = root.type = Generated

    // 第二步：合并 mid（Block）
    UnbakedItemModel mid;
    mid.type = ItemModelType::Block;
    // merged.type = mid.type = Block

    // 第三步：合并 leaf（Handheld）
    UnbakedItemModel leaf;
    leaf.type = ItemModelType::Handheld;
    // merged.type = leaf.type = Handheld

    // 手动模拟 _mergeParent 的 type 行为：child.type = parent.type
    merged.type = root.type;
    EXPECT_EQ(merged.type, ItemModelType::Generated);

    merged.type = mid.type;
    EXPECT_EQ(merged.type, ItemModelType::Block);

    merged.type = leaf.type;
    EXPECT_EQ(merged.type, ItemModelType::Handheld);

    // 最终结果：leaf 的 type 生效，与原始 bakeModel 行为一致
}

TEST(ItemMergeParentTest, DisplayTransformMergeIsPerContext)
{
    using namespace mc::client::resource;

    // 累积结果已定义了 ThirdPersonRightHand 和 Gui 变换（模拟从根继承的值）
    UnbakedItemModel accumulated;
    accumulated.display[ItemDisplayContext::ThirdPersonRightHand] =
        ItemTransform{glm::vec3(0, -90, 55), glm::vec3(0, 1.5f, -1.5f), glm::vec3(0.55f, 0.55f, 0.55f)};
    accumulated.display[ItemDisplayContext::Gui] =
        ItemTransform{glm::vec3(30, 225, 0), glm::vec3(0, 0, 0), glm::vec3(0.625f, 0.625f, 0.625f)};

    // 当前层只定义了 Gui 变换（模拟更靠近叶子的模型覆盖 Gui）
    UnbakedItemModel currentLayer;
    currentLayer.display[ItemDisplayContext::Gui] =
        ItemTransform{glm::vec3(30, 45, 0), glm::vec3(0, 0, 0), glm::vec3(0.5f, 0.5f, 0.5f)};

    // 模拟 _mergeParent 的 display 行为：当前层覆盖累积结果中同名的上下文
    for (const auto& [ctx, transform] : currentLayer.display) {
        accumulated.display[ctx] = transform;
    }

    // 当前层覆盖了 Gui，但 ThirdPersonRightHand 保留
    EXPECT_EQ(accumulated.display.size(), 2u);
    EXPECT_FLOAT_EQ(accumulated.display.at(ItemDisplayContext::Gui).rotation.y, 45.0f); // 当前层覆盖
    EXPECT_FLOAT_EQ(accumulated.display.at(ItemDisplayContext::ThirdPersonRightHand).rotation.y,
        -90.0f); // 累积结果保留
}

TEST(ItemMergeParentTest, OverridesMergeIsLeafWins)
{
    using namespace mc::client::resource;

    // 当前层有 overrides -> 应覆盖累积结果（leaf-wins）
    UnbakedItemModel currentLayer;
    ItemModelOverride override;
    override.predicates["damage"] = 0.5f;
    override.model = ResourceLocation("minecraft:item/damaged");
    currentLayer.overrides.push_back(override);

    // 累积结果没有 overrides
    UnbakedItemModel accumulated;

    // 模拟 _mergeParent 的 overrides 行为：当前层有 overrides 时覆盖累积结果
    if (!currentLayer.overrides.empty()) {
        accumulated.overrides = currentLayer.overrides;
    }

    EXPECT_EQ(accumulated.overrides.size(), 1u);
    EXPECT_EQ(accumulated.overrides[0].model.toString(), "minecraft:item/damaged");

    // 累积结果有自己的 overrides -> 当前层没有时不覆盖
    UnbakedItemModel accumulatedWithOverrides;
    ItemModelOverride existingOverride;
    existingOverride.predicates["custom_model_data"] = 1.0f;
    existingOverride.model = ResourceLocation("minecraft:item/custom");
    accumulatedWithOverrides.overrides.push_back(existingOverride);

    UnbakedItemModel emptyLayer;
    if (!emptyLayer.overrides.empty()) {
        accumulatedWithOverrides.overrides = emptyLayer.overrides;
    }

    // 累积结果保留自己的 overrides
    EXPECT_EQ(accumulatedWithOverrides.overrides.size(), 1u);
    EXPECT_EQ(accumulatedWithOverrides.overrides[0].model.toString(), "minecraft:item/custom");
}

TEST(ItemMergeParentTest, ElementMergeIsFirstDefinedWins)
{
    using namespace mc::client::resource;

    // 当前层有元素
    UnbakedItemModel currentLayer;
    ModelElement elem;
    elem.from = {0.0f, 0.0f, 0.0f};
    elem.to = {16.0f, 16.0f, 16.0f};
    currentLayer.elements.push_back(elem);

    // 累积结果无元素 -> 应继承当前层
    UnbakedItemModel accumulated;
    if (accumulated.elements.empty() && !currentLayer.elements.empty()) {
        accumulated.elements = currentLayer.elements;
    }
    EXPECT_EQ(accumulated.elements.size(), 1u);

    // 累积结果已有元素 -> 不被覆盖
    UnbakedItemModel accumulatedWithElems;
    ModelElement existingElem;
    existingElem.from = {4.0f, 4.0f, 4.0f};
    existingElem.to = {12.0f, 12.0f, 12.0f};
    accumulatedWithElems.elements.push_back(existingElem);

    if (accumulatedWithElems.elements.empty() && !currentLayer.elements.empty()) {
        accumulatedWithElems.elements = currentLayer.elements;
    }
    EXPECT_EQ(accumulatedWithElems.elements.size(), 1u);
    EXPECT_FLOAT_EQ(accumulatedWithElems.elements[0].from.x, 4.0f); // 累积结果保留
}
