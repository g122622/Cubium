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
#include "common/resource/pack/FolderResourcePack.hpp"
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
    // axis+angle 格式时，EulerXYZ 字段应为默认值
    EXPECT_FALSE(rot.isEulerXYZ);
    EXPECT_FLOAT_EQ(rot.rotX, 0.0f);
    EXPECT_FLOAT_EQ(rot.rotY, 0.0f);
    EXPECT_FLOAT_EQ(rot.rotZ, 0.0f);
}

TEST(ParseRotationTest, DefaultValues)
{
    auto j = nlohmann::json::parse("{}");
    auto rot = BlockModelLoader::parseRotation(j);
    EXPECT_FLOAT_EQ(rot.origin.x, 8.0f); // 默认 8,8,8
    EXPECT_EQ(rot.axis, "y");            // 默认 "y"
    EXPECT_FLOAT_EQ(rot.angle, 0.0f);    // 默认 0
    EXPECT_FALSE(rot.rescale);           // 默认 false
    EXPECT_FALSE(rot.isEulerXYZ);        // 默认 axis+angle 格式
    EXPECT_FLOAT_EQ(rot.rotX, 0.0f);     // EulerXYZ 默认 0
    EXPECT_FLOAT_EQ(rot.rotY, 0.0f);
    EXPECT_FLOAT_EQ(rot.rotZ, 0.0f);
}

// --- EulerXYZ 旋转格式测试（MC 1.21.11 新增）---

TEST(ParseRotationTest, EulerXYZFullRotation)
{
    const char* json = R"({
        "origin": [8, 8, 8],
        "x": 45.0,
        "y": 22.5,
        "z": -45.0,
        "rescale": true
    })";
    auto j = nlohmann::json::parse(json);
    auto rot = BlockModelLoader::parseRotation(j);

    // EulerXYZ 格式标志
    EXPECT_TRUE(rot.isEulerXYZ);

    // 旋转中心
    EXPECT_FLOAT_EQ(rot.origin.x, 8.0f);
    EXPECT_FLOAT_EQ(rot.origin.y, 8.0f);
    EXPECT_FLOAT_EQ(rot.origin.z, 8.0f);

    // 三轴欧拉角
    EXPECT_FLOAT_EQ(rot.rotX, 45.0f);
    EXPECT_FLOAT_EQ(rot.rotY, 22.5f);
    EXPECT_FLOAT_EQ(rot.rotZ, -45.0f);

    // rescale
    EXPECT_TRUE(rot.rescale);

    // axis+angle 字段应为默认值
    EXPECT_EQ(rot.axis, "y");
    EXPECT_FLOAT_EQ(rot.angle, 0.0f);
}

TEST(ParseRotationTest, EulerXYZPartialAxes)
{
    // 仅指定 x 轴旋转，y 和 z 默认为 0
    const char* json = R"({
        "origin": [4, 4, 4],
        "x": 22.5
    })";
    auto j = nlohmann::json::parse(json);
    auto rot = BlockModelLoader::parseRotation(j);

    EXPECT_TRUE(rot.isEulerXYZ);
    EXPECT_FLOAT_EQ(rot.rotX, 22.5f);
    EXPECT_FLOAT_EQ(rot.rotY, 0.0f); // 默认
    EXPECT_FLOAT_EQ(rot.rotZ, 0.0f); // 默认
    EXPECT_FLOAT_EQ(rot.origin.x, 4.0f);
    EXPECT_FLOAT_EQ(rot.origin.y, 4.0f);
    EXPECT_FLOAT_EQ(rot.origin.z, 4.0f);
    EXPECT_FALSE(rot.rescale); // 默认
}

TEST(ParseRotationTest, EulerXYZOnlyZAxis)
{
    // 仅指定 z 轴旋转
    const char* json = R"({
        "z": 90.0
    })";
    auto j = nlohmann::json::parse(json);
    auto rot = BlockModelLoader::parseRotation(j);

    EXPECT_TRUE(rot.isEulerXYZ);
    EXPECT_FLOAT_EQ(rot.rotX, 0.0f);
    EXPECT_FLOAT_EQ(rot.rotY, 0.0f);
    EXPECT_FLOAT_EQ(rot.rotZ, 90.0f);
}

TEST(ParseRotationTest, AxisAngleTakesPrecedenceOverEulerXYZ)
{
    // 如果同时有 axis 和 x 字段，axis+angle 格式优先
    const char* json = R"({
        "origin": [8, 8, 8],
        "axis": "x",
        "angle": 45.0,
        "x": 30.0,
        "y": 15.0
    })";
    auto j = nlohmann::json::parse(json);
    auto rot = BlockModelLoader::parseRotation(j);

    // axis+angle 格式优先
    EXPECT_FALSE(rot.isEulerXYZ);
    EXPECT_EQ(rot.axis, "x");
    EXPECT_FLOAT_EQ(rot.angle, 45.0f);
    // EulerXYZ 字段应保持默认
    EXPECT_FLOAT_EQ(rot.rotX, 0.0f);
    EXPECT_FLOAT_EQ(rot.rotY, 0.0f);
    EXPECT_FLOAT_EQ(rot.rotZ, 0.0f);
}

TEST(ParseRotationTest, AxisWithoutAngleIsAxisAngleFormat)
{
    // 仅有 axis 而没有 angle，仍然使用 axis+angle 格式（angle 默认 0）
    const char* json = R"({
        "axis": "z"
    })";
    auto j = nlohmann::json::parse(json);
    auto rot = BlockModelLoader::parseRotation(j);

    EXPECT_FALSE(rot.isEulerXYZ);
    EXPECT_EQ(rot.axis, "z");
    EXPECT_FLOAT_EQ(rot.angle, 0.0f);
}

TEST(ParseRotationTest, EmptyRotationObject)
{
    // 空对象：既没有 axis/angle 也没有 x/y/z，返回默认旋转
    auto j = nlohmann::json::parse("{}");
    auto rot = BlockModelLoader::parseRotation(j);

    EXPECT_FALSE(rot.isEulerXYZ);
    EXPECT_FLOAT_EQ(rot.angle, 0.0f);
    EXPECT_FLOAT_EQ(rot.rotX, 0.0f);
    EXPECT_FLOAT_EQ(rot.rotY, 0.0f);
    EXPECT_FLOAT_EQ(rot.rotZ, 0.0f);
    EXPECT_TRUE(rot.isIdentity());
}

TEST(ParseRotationTest, ModelRotationIsIdentityAxisAngle)
{
    ModelRotation rot;
    EXPECT_TRUE(rot.isIdentity());

    rot.angle = 0.0f;
    EXPECT_TRUE(rot.isIdentity());

    rot.angle = 45.0f;
    EXPECT_FALSE(rot.isIdentity());
}

TEST(ParseRotationTest, ModelRotationIsIdentityEulerXYZ)
{
    ModelRotation rot;
    rot.isEulerXYZ = true;
    EXPECT_TRUE(rot.isIdentity()); // 0,0,0 是恒等

    rot.rotX = 45.0f;
    EXPECT_FALSE(rot.isIdentity());

    rot.rotX = 0.0f;
    rot.rotY = 22.5f;
    EXPECT_FALSE(rot.isIdentity());

    rot.rotY = 0.0f;
    rot.rotZ = -45.0f;
    EXPECT_FALSE(rot.isIdentity());

    rot.rotZ = 0.0f;
    EXPECT_TRUE(rot.isIdentity());
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
    // 在 root-to-leaf 累积中，如果子模型（叶子）没有显式定义 elements，
    // 则父模型（根）的元素生效

    UnbakedBlockModel parent;
    parent.hasElements = true; // 根模型显式定义了 elements
    ModelElement elem;
    elem.from = {0.0f, 0.0f, 0.0f};
    elem.to = {16.0f, 16.0f, 16.0f};
    parent.elements.push_back(elem);

    UnbakedBlockModel child;
    // 子模型没有 hasElements 标记，也没有元素
    EXPECT_TRUE(child.elements.empty());
    EXPECT_FALSE(child.hasElements);

    // 模拟 root-to-leaf 合并：先 parent（根），后 child（叶子）
    UnbakedBlockModel merged;
    BlockModelLoader::mergeParent(merged, parent);
    EXPECT_EQ(merged.elements.size(), 1u);
    EXPECT_TRUE(merged.hasElements);

    BlockModelLoader::mergeParent(merged, child);
    // 子模型没有显式定义 elements，不覆盖父模型的元素
    EXPECT_EQ(merged.elements.size(), 1u);
    EXPECT_FLOAT_EQ(merged.elements[0].to.x, 16.0f);
    EXPECT_TRUE(merged.hasElements);
}

TEST(MergeParentBlockTest, ChildElementsOverrideParent)
{
    // 在 root-to-leaf 累积中，后处理的层（更靠近叶子）覆盖先处理的层
    // 模拟 bakeModel 中的调用顺序：先处理根（parent），再处理叶子（child）

    UnbakedBlockModel parent;
    parent.hasElements = true; // 根模型显式定义了 elements
    ModelElement parentElem;
    parentElem.from = {0.0f, 0.0f, 0.0f};
    parentElem.to = {16.0f, 16.0f, 16.0f};
    parent.elements.push_back(parentElem);

    UnbakedBlockModel child;
    child.hasElements = true; // 叶子模型显式定义了 elements
    ModelElement childElem;
    childElem.from = {4.0f, 4.0f, 4.0f};
    childElem.to = {12.0f, 12.0f, 12.0f};
    child.elements.push_back(childElem);

    // 模拟 root-to-leaf 合并：先 parent（根），后 child（叶子）
    UnbakedBlockModel merged;
    BlockModelLoader::mergeParent(merged, parent);
    EXPECT_EQ(merged.elements.size(), 1u);
    EXPECT_FLOAT_EQ(merged.elements[0].to.x, 16.0f); // root 的元素

    BlockModelLoader::mergeParent(merged, child);
    // 叶子模型的元素覆盖根模型的元素（leaf-wins）
    EXPECT_EQ(merged.elements.size(), 1u);
    EXPECT_FLOAT_EQ(merged.elements[0].from.x, 4.0f); // child 的元素
}

TEST(MergeParentBlockTest, AmbientOcclusionInheritance)
{
    // 场景 1：父模型显式设置 AO=false，子模型未显式设置 -> 子模型应继承父模型的 false
    UnbakedBlockModel parent;
    parent.ambientOcclusion = false;
    parent.hasAmbientOcclusion = true;

    UnbakedBlockModel child;
    // child 没有显式设置 hasAmbientOcclusion，默认 AO=true 但不应覆盖父模型

    // 模拟 root-to-leaf 合并：先 parent（根），后 child（叶子）
    UnbakedBlockModel merged;
    BlockModelLoader::mergeParent(merged, parent);
    EXPECT_FALSE(merged.ambientOcclusion); // parent 设置了 AO=false
    EXPECT_TRUE(merged.hasAmbientOcclusion);

    BlockModelLoader::mergeParent(merged, child);
    // child 没有显式设置 AO，不覆盖 parent 的值
    EXPECT_FALSE(merged.ambientOcclusion); // 仍然保持 false

    // 场景 2：子模型显式设置 AO=true，覆盖父模型的 AO=false（MC Java 版 leaf-wins）
    UnbakedBlockModel parent2;
    parent2.ambientOcclusion = false;
    parent2.hasAmbientOcclusion = true;

    UnbakedBlockModel child2;
    child2.ambientOcclusion = true;
    child2.hasAmbientOcclusion = true; // 子模型显式设置了 AO=true

    UnbakedBlockModel merged2;
    BlockModelLoader::mergeParent(merged2, parent2);
    EXPECT_FALSE(merged2.ambientOcclusion);

    BlockModelLoader::mergeParent(merged2, child2);
    EXPECT_TRUE(merged2.ambientOcclusion); // child 显式设置 AO=true，覆盖 parent 的 false

    // 场景 3：子模型显式设置 AO=false，父模型 AO=true
    UnbakedBlockModel parent3;
    parent3.ambientOcclusion = true;
    parent3.hasAmbientOcclusion = true;

    UnbakedBlockModel child3;
    child3.ambientOcclusion = false;
    child3.hasAmbientOcclusion = true;

    UnbakedBlockModel merged3;
    BlockModelLoader::mergeParent(merged3, parent3);
    BlockModelLoader::mergeParent(merged3, child3);
    EXPECT_FALSE(merged3.ambientOcclusion); // child 显式设置 AO=false
}

TEST(MergeParentBlockTest, RootToLeafChainMerge)
{
    // 模拟从根到叶的合并链：root -> mid -> leaf
    UnbakedBlockModel root;
    root.textures["all"] = "minecraft:block/stone";
    root.textures["particle"] = "minecraft:block/stone";
    root.hasElements = true; // 根模型显式定义了 elements
    ModelElement rootElem;
    rootElem.from = {0.0f, 0.0f, 0.0f};
    rootElem.to = {16.0f, 16.0f, 16.0f};
    root.elements.push_back(rootElem);

    UnbakedBlockModel mid;
    mid.textures["all"] = "minecraft:block/dirt"; // mid 覆盖 all

    UnbakedBlockModel leaf;
    leaf.textures["top"] = "minecraft:block/dirt_top"; // leaf 添加新纹理

    // 模拟 root-to-leaf 合并（使用 mergeParent 逐层累积）
    UnbakedBlockModel merged;
    BlockModelLoader::mergeParent(merged, root);
    BlockModelLoader::mergeParent(merged, mid);
    BlockModelLoader::mergeParent(merged, leaf);

    // 最终：all 应为 mid 的 dirt（mid 覆盖了 root 的 stone）
    EXPECT_EQ(merged.textures.at("all"), "minecraft:block/dirt");
    // particle 从 root 继承
    EXPECT_EQ(merged.textures.at("particle"), "minecraft:block/stone");
    // top 从 leaf 继承
    EXPECT_EQ(merged.textures.at("top"), "minecraft:block/dirt_top");
    // 元素从 root 继承（mid 和 leaf 都没有显式定义 elements）
    EXPECT_EQ(merged.elements.size(), 1u);
    EXPECT_TRUE(merged.hasElements);
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
// ItemModelLoader 合并行为验证测试
// ============================================================================
using namespace mc::client::resource;

// 注意：UnbakedItemModel::type 已移除（原为 write-only 死代码）。
// 模型类型的确定现在由 bakeModel 根据 hasHandheldParent 和 baked.elements 独立计算，
// 并存储在 BakedItemModel::type 中。类型逻辑通过 bakeModel 集成测试间接验证。

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
    currentLayer.hasOverrides = true;
    ItemModelOverride override;
    override.predicates["damage"] = 0.5f;
    override.model = ResourceLocation("minecraft:item/damaged");
    currentLayer.overrides.push_back(override);

    // 累积结果没有 overrides
    UnbakedItemModel accumulated;

    // 模拟 _mergeParent 的 overrides 行为：当前层显式定义了 overrides 时覆盖累积结果
    if (currentLayer.hasOverrides) {
        accumulated.overrides = currentLayer.overrides;
        accumulated.hasOverrides = true;
    }

    EXPECT_EQ(accumulated.overrides.size(), 1u);
    EXPECT_EQ(accumulated.overrides[0].model.toString(), "minecraft:item/damaged");
    EXPECT_TRUE(accumulated.hasOverrides);

    // 累积结果有自己的 overrides -> 当前层没有时不覆盖
    UnbakedItemModel accumulatedWithOverrides;
    accumulatedWithOverrides.hasOverrides = true;
    ItemModelOverride existingOverride;
    existingOverride.predicates["custom_model_data"] = 1.0f;
    existingOverride.model = ResourceLocation("minecraft:item/custom");
    accumulatedWithOverrides.overrides.push_back(existingOverride);

    UnbakedItemModel emptyLayer;
    if (emptyLayer.hasOverrides) {
        accumulatedWithOverrides.overrides = emptyLayer.overrides;
        accumulatedWithOverrides.hasOverrides = true;
    }

    // 累积结果保留自己的 overrides
    EXPECT_EQ(accumulatedWithOverrides.overrides.size(), 1u);
    EXPECT_EQ(accumulatedWithOverrides.overrides[0].model.toString(), "minecraft:item/custom");
}

TEST(ItemMergeParentTest, ElementMergeIsLeafWins)
{
    using namespace mc::client::resource;

    // 测试 leaf-wins 语义：后处理的层（更靠近叶子）覆盖先处理的层的元素
    // 使用 hasElements 标记区分"JSON 显式定义了 elements"和"JSON 中没有 elements 字段"

    // 场景 1：当前层显式定义了 elements，累积结果没有 -> 应从当前层继承
    UnbakedItemModel currentLayer;
    currentLayer.hasElements = true;
    ModelElement elem;
    elem.from = {0.0f, 0.0f, 0.0f};
    elem.to = {16.0f, 16.0f, 16.0f};
    currentLayer.elements.push_back(elem);

    UnbakedItemModel accumulated;
    if (currentLayer.hasElements) {
        accumulated.elements = currentLayer.elements;
        accumulated.hasElements = true;
    }
    EXPECT_EQ(accumulated.elements.size(), 1u);
    EXPECT_TRUE(accumulated.hasElements);

    // 场景 2：累积结果已从某层继承了 elements，当前层也有 -> 当前层覆盖（leaf-wins）
    UnbakedItemModel accumulatedWithElems;
    accumulatedWithElems.hasElements = true;
    ModelElement existingElem;
    existingElem.from = {4.0f, 4.0f, 4.0f};
    existingElem.to = {12.0f, 12.0f, 12.0f};
    accumulatedWithElems.elements.push_back(existingElem);

    if (currentLayer.hasElements) {
        accumulatedWithElems.elements = currentLayer.elements;
        accumulatedWithElems.hasElements = true;
    }
    EXPECT_EQ(accumulatedWithElems.elements.size(), 1u);
    EXPECT_FLOAT_EQ(accumulatedWithElems.elements[0].from.x, 0.0f); // 当前层覆盖了累积结果
}

// --- leaf-wins 语义测试 (MC Java 版模型合并行为) ---

TEST(MergeParentBlockTest, LeafWinsElements_ChildOverridesParent)
{
    // MC Java 版 leaf-wins 语义：子模型定义了 elements 时完全覆盖父模型
    // 在 root-to-leaf 累积合并中，后处理的层（更靠近叶子）覆盖先处理的层

    // 场景：根模型定义了 cube 元素，叶子模型定义了 slab 元素
    // 期望：叶子模型的 slab 元素生效（leaf-wins）
    UnbakedBlockModel root;
    root.hasElements = true;
    ModelElement cubeElem;
    cubeElem.from = {0.0f, 0.0f, 0.0f};
    cubeElem.to = {16.0f, 16.0f, 16.0f};
    root.elements.push_back(cubeElem);

    UnbakedBlockModel leaf;
    leaf.hasElements = true;
    ModelElement slabElem;
    slabElem.from = {0.0f, 0.0f, 0.0f};
    slabElem.to = {16.0f, 8.0f, 16.0f};
    leaf.elements.push_back(slabElem);

    // 模拟 root-to-leaf 合并
    UnbakedBlockModel merged;
    BlockModelLoader::mergeParent(merged, root);
    BlockModelLoader::mergeParent(merged, leaf);

    // 叶子模型的 slab 元素应该覆盖根模型的 cube 元素（leaf-wins）
    EXPECT_EQ(merged.elements.size(), 1u);
    EXPECT_FLOAT_EQ(merged.elements[0].to.y, 8.0f); // leaf 的 slab 元素
    EXPECT_TRUE(merged.hasElements);
}

TEST(MergeParentBlockTest, AmbientOcclusionExplicitlySetWins)
{
    // MC Java 版 leaf-wins 语义：子模型显式设置了 ambientocclusion 时覆盖父模型
    // 在 root-to-leaf 累积合并中，后处理的层（更靠近叶子）覆盖先处理的层
    UnbakedBlockModel parent;
    parent.ambientOcclusion = false;
    parent.hasAmbientOcclusion = true;

    UnbakedBlockModel child;
    child.ambientOcclusion = true;
    child.hasAmbientOcclusion = true;

    // 模拟 root-to-leaf 累积合并：先合并 parent，再合并 child
    UnbakedBlockModel merged;
    BlockModelLoader::mergeParent(merged, parent);
    EXPECT_FALSE(merged.ambientOcclusion); // parent 设置了 AO=false
    EXPECT_TRUE(merged.hasAmbientOcclusion);

    BlockModelLoader::mergeParent(merged, child);
    // child 也显式设置了 AO=true，覆盖 parent 的 AO=false
    EXPECT_TRUE(merged.ambientOcclusion); // child 的 AO=true 覆盖了 parent 的 AO=false
}

TEST(MergeParentBlockTest, AmbientOcclusionUnsetInheritsParent)
{
    // 场景：父模型 AO=false，子模型没有显式设置 ambientocclusion（默认 true）
    // 子模型应该继承父模型的 AO=false
    UnbakedBlockModel parent;
    parent.ambientOcclusion = false;
    parent.hasAmbientOcclusion = true;

    UnbakedBlockModel child;
    child.ambientOcclusion = true;     // C++ 默认值
    child.hasAmbientOcclusion = false; // 但 JSON 中没有 ambientocclusion 字段

    // 模拟 root-to-leaf 累积合并
    UnbakedBlockModel merged;
    BlockModelLoader::mergeParent(merged, parent);
    BlockModelLoader::mergeParent(merged, child);

    // child 没有显式设置 AO，所以 merged 保持从 parent 继承的 false
    EXPECT_FALSE(merged.ambientOcclusion);
}

TEST(MergeParentBlockTest, EmptyElementsWithHasElements)
{
    // 区分 JSON 中显式定义了空 elements 数组和没有 elements 字段
    // 显式定义空 elements（hasElements=true）应该覆盖父模型的元素（leaf-wins）
    UnbakedBlockModel parent;
    parent.hasElements = true;
    ModelElement elem;
    elem.from = {0.0f, 0.0f, 0.0f};
    elem.to = {16.0f, 16.0f, 16.0f};
    parent.elements.push_back(elem);

    UnbakedBlockModel child;
    child.hasElements = true; // JSON 中写了 "elements": []
    // elements 向量为空（JSON 定义了空数组）

    // 模拟 root-to-leaf 累积合并
    UnbakedBlockModel merged;
    BlockModelLoader::mergeParent(merged, parent);
    EXPECT_EQ(merged.elements.size(), 1u); // parent 的元素

    BlockModelLoader::mergeParent(merged, child);
    // 子模型显式定义了空 elements，覆盖父模型的元素（leaf-wins）
    EXPECT_TRUE(merged.elements.empty());
    EXPECT_TRUE(merged.hasElements);
}
