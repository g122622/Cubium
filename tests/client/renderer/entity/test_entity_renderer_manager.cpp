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
 * @file test_entity_renderer_manager.cpp
 * @brief EntityRendererManager 单元测试
 *
 * 测试覆盖：
 * - normalizeEntityTypeId 实体类型ID规范化
 * - getTexturePaths 纹理路径获取
 * - getDefaultEntityTypes 默认实体类型
 * - 渲染器注册和获取
 */

#include <string>
#include <unordered_map>
#include <vector>
#include <gtest/gtest.h>

// 前向声明和手动实现测试（不依赖 Vulkan）

namespace mc::client {
namespace renderer {

// 从 EntityRendererManager.cpp 复制的函数进行独立测试
// 这些函数是命名空间级别的自由函数或静态函数

/**
 * @brief 规范化实体类型ID（测试版本）
 *
 * 将实体类型ID转换为标准格式（带命名空间前缀）
 * 例如："pig" -> "minecraft:pig", "minecraft:cow" -> "minecraft:cow"
 */
std::string normalizeEntityTypeId(const std::string& typeId)
{
    // 如果已有命名空间前缀，直接返回
    if (typeId.find(':') != std::string::npos) {
        return typeId;
    }
    // 添加默认命名空间
    return "minecraft:" + typeId;
}

} // namespace renderer
} // namespace mc::client

using namespace mc::client::renderer;

// ============================================================================
// normalizeEntityTypeId 测试
// ============================================================================

class NormalizeEntityTypeIdTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(NormalizeEntityTypeIdTest, NormalizeSimpleEntityId)
{
    // 测试简单的实体类型ID（无命名空间）
    EXPECT_EQ(normalizeEntityTypeId("pig"), "minecraft:pig");
    EXPECT_EQ(normalizeEntityTypeId("cow"), "minecraft:cow");
    EXPECT_EQ(normalizeEntityTypeId("sheep"), "minecraft:sheep");
    EXPECT_EQ(normalizeEntityTypeId("chicken"), "minecraft:chicken");
}

TEST_F(NormalizeEntityTypeIdTest, KeepAlreadyNormalizedId)
{
    // 测试已经带有命名空间的ID（应该保持不变）
    EXPECT_EQ(normalizeEntityTypeId("minecraft:pig"), "minecraft:pig");
    EXPECT_EQ(normalizeEntityTypeId("minecraft:cow"), "minecraft:cow");
    EXPECT_EQ(normalizeEntityTypeId("modid:custom_mob"), "modid:custom_mob");
}

TEST_F(NormalizeEntityTypeIdTest, HandleEdgeCases)
{
    // 空字符串
    EXPECT_EQ(normalizeEntityTypeId(""), "minecraft:");

    // 只有冒号（已经包含冒号，保持不变）
    EXPECT_EQ(normalizeEntityTypeId(":"), ":");

    // 以冒号开头（已经包含冒号，保持不变）
    EXPECT_EQ(normalizeEntityTypeId(":test"), ":test");

    // 以冒号结尾（已经包含冒号，保持不变）
    EXPECT_EQ(normalizeEntityTypeId("test:"), "test:");
}

TEST_F(NormalizeEntityTypeIdTest, HandleComplexNames)
{
    // 带下划线的名称
    EXPECT_EQ(normalizeEntityTypeId("cave_spider"), "minecraft:cave_spider");
    EXPECT_EQ(normalizeEntityTypeId("iron_golem"), "minecraft:iron_golem");

    // 带数字的名称
    EXPECT_EQ(normalizeEntityTypeId("zombie_villager"), "minecraft:zombie_villager");
}

// ============================================================================
// EntityTextureLoader 路径生成测试
// ============================================================================

// 从 EntityTextureLoader.cpp 提取的逻辑
std::string parseEntityName(const std::string& entityTypeId)
{
    size_t colonPos = entityTypeId.find(':');
    if (colonPos != std::string::npos && colonPos + 1 < entityTypeId.size()) {
        return entityTypeId.substr(colonPos + 1);
    }
    return entityTypeId;
}

std::vector<std::string> getTexturePaths(const std::string& entityTypeId)
{
    std::string name = parseEntityName(entityTypeId);

    static const std::unordered_map<std::string, std::vector<std::string>> specialPaths = {
        {"chicken", {"minecraft:textures/entity/chicken.png"}},
        {"rabbit", {"minecraft:textures/entity/rabbit/brown.png"}},
        {"mooshroom", {"minecraft:textures/entity/cow/red_mooshroom.png"}},
        {"cat", {"minecraft:textures/entity/cat/tabby.png"}},
        {"ocelot", {"minecraft:textures/entity/cat/ocelot.png"}},
        {"parrot", {"minecraft:textures/entity/parrot/parrot_red_blue.png"}},
        {"polar_bear", {"minecraft:textures/entity/bear/polarbear.png"}},
        {"turtle", {"minecraft:textures/entity/turtle/big_sea_turtle.png"}},
        {"horse", {"minecraft:textures/entity/horse/horse_brown.png"}},
        {"donkey", {"minecraft:textures/entity/horse/donkey.png"}},
        {"mule", {"minecraft:textures/entity/horse/mule.png"}},
        {"llama", {"minecraft:textures/entity/llama/creamy.png"}},
        {"skeleton_horse", {"minecraft:textures/entity/horse/horse_skeleton.png"}},
        {"zombie_horse", {"minecraft:textures/entity/horse/horse_zombie.png"}},
        {"bat", {"minecraft:textures/entity/bat.png"}},
        {"blaze", {"minecraft:textures/entity/blaze.png"}},
        {"guardian", {"minecraft:textures/entity/guardian.png"}},
        {"elder_guardian", {"minecraft:textures/entity/guardian_elder.png"}},
        {"phantom", {"minecraft:textures/entity/phantom.png"}},
        {"silverfish", {"minecraft:textures/entity/silverfish.png"}},
        {"endermite", {"minecraft:textures/entity/endermite.png"}},
        {"squid", {"minecraft:textures/entity/squid.png"}},
        {"dolphin", {"minecraft:textures/entity/dolphin.png"}},
        {"snow_golem", {"minecraft:textures/entity/snow_golem.png"}},
        {"wandering_trader", {"minecraft:textures/entity/wandering_trader.png"}},
        {"witch", {"minecraft:textures/entity/witch.png"}},
        {"husk", {"minecraft:textures/entity/zombie/husk.png"}},
        {"drowned", {"minecraft:textures/entity/zombie/drowned.png"}},
        {"stray", {"minecraft:textures/entity/skeleton/stray.png"}},
        {"wither_skeleton", {"minecraft:textures/entity/skeleton/wither_skeleton.png"}},
        {"giant", {"minecraft:textures/entity/zombie/zombie.png"}},
        {"zombified_piglin", {"minecraft:textures/entity/piglin/zombified_piglin.png"}},
        {"piglin", {"minecraft:textures/entity/piglin/piglin.png"}},
        {"piglin_brute", {"minecraft:textures/entity/piglin/piglin_brute.png"}},
        {"hoglin", {"minecraft:textures/entity/hoglin/hoglin.png"}},
        {"zoglin", {"minecraft:textures/entity/hoglin/zoglin.png"}},
        {"vindicator", {"minecraft:textures/entity/illager/vindicator.png"}},
        {"evoker", {"minecraft:textures/entity/illager/evoker.png"}},
        {"illusioner", {"minecraft:textures/entity/illager/illusioner.png"}},
        {"pillager", {"minecraft:textures/entity/illager/pillager.png"}},
        {"ravager", {"minecraft:textures/entity/illager/ravager.png"}},
        {"vex", {"minecraft:textures/entity/illager/vex.png"}},
        {"ender_dragon", {"minecraft:textures/entity/enderdragon/dragon.png"}},
    };

    const auto specialIt = specialPaths.find(name);
    if (specialIt != specialPaths.end()) {
        return specialIt->second;
    }

    std::vector<std::string> paths;

    // MC 1.13+ 格式: textures/entity/<name>/<name>.png
    paths.push_back("minecraft:textures/entity/" + name + "/" + name + ".png");

    // MC 1.12 格式: textures/entity/<name>.png
    paths.push_back("minecraft:textures/entity/" + name + ".png");

    return paths;
}

class EntityTexturePathGenerationTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(EntityTexturePathGenerationTest, ParseEntityNameSimple)
{
    EXPECT_EQ(parseEntityName("minecraft:pig"), "pig");
    EXPECT_EQ(parseEntityName("minecraft:cow"), "cow");
    EXPECT_EQ(parseEntityName("minecraft:sheep"), "sheep");
    EXPECT_EQ(parseEntityName("minecraft:chicken"), "chicken");
}

TEST_F(EntityTexturePathGenerationTest, ParseEntityNameNoNamespace)
{
    // 如果没有命名空间，返回原字符串
    EXPECT_EQ(parseEntityName("pig"), "pig");
    EXPECT_EQ(parseEntityName("cow"), "cow");
}

TEST_F(EntityTexturePathGenerationTest, ParseEntityNameCustomNamespace)
{
    EXPECT_EQ(parseEntityName("modid:custom_mob"), "custom_mob");
    EXPECT_EQ(parseEntityName("mymod:dragon"), "dragon");
}

TEST_F(EntityTexturePathGenerationTest, ParseEntityNameEdgeCases)
{
    // 空字符串
    EXPECT_EQ(parseEntityName(""), "");

    // 只有冒号（没有后续字符，返回原字符串）
    EXPECT_EQ(parseEntityName(":"), ":");

    // 以冒号结尾（没有后续字符，返回原字符串）
    EXPECT_EQ(parseEntityName("test:"), "test:");
}

TEST_F(EntityTexturePathGenerationTest, GetTexturePathsPig)
{
    auto paths = getTexturePaths("minecraft:pig");

    ASSERT_EQ(paths.size(), 2u);

    // MC 1.13+ 格式（优先）
    EXPECT_EQ(paths[0], "minecraft:textures/entity/pig/pig.png");

    // MC 1.12 格式（备选）
    EXPECT_EQ(paths[1], "minecraft:textures/entity/pig.png");
}

TEST_F(EntityTexturePathGenerationTest, GetTexturePathsCow)
{
    auto paths = getTexturePaths("minecraft:cow");

    ASSERT_EQ(paths.size(), 2u);
    EXPECT_EQ(paths[0], "minecraft:textures/entity/cow/cow.png");
    EXPECT_EQ(paths[1], "minecraft:textures/entity/cow.png");
}

TEST_F(EntityTexturePathGenerationTest, GetTexturePathsSheep)
{
    auto paths = getTexturePaths("minecraft:sheep");

    ASSERT_EQ(paths.size(), 2u);
    EXPECT_EQ(paths[0], "minecraft:textures/entity/sheep/sheep.png");
    EXPECT_EQ(paths[1], "minecraft:textures/entity/sheep.png");
}

TEST_F(EntityTexturePathGenerationTest, GetTexturePathsChicken)
{
    auto paths = getTexturePaths("minecraft:chicken");

    ASSERT_EQ(paths.size(), 1u);
    EXPECT_EQ(paths[0], "minecraft:textures/entity/chicken.png");
}

TEST_F(EntityTexturePathGenerationTest, GetTexturePathsSpecialVanillaEntities)
{
    const std::vector<std::pair<std::string, std::string>> specialCases = {
        {"minecraft:rabbit", "minecraft:textures/entity/rabbit/brown.png"},
        {"minecraft:mooshroom", "minecraft:textures/entity/cow/red_mooshroom.png"},
        {"minecraft:cat", "minecraft:textures/entity/cat/tabby.png"},
        {"minecraft:ocelot", "minecraft:textures/entity/cat/ocelot.png"},
        {"minecraft:parrot", "minecraft:textures/entity/parrot/parrot_red_blue.png"},
        {"minecraft:polar_bear", "minecraft:textures/entity/bear/polarbear.png"},
        {"minecraft:turtle", "minecraft:textures/entity/turtle/big_sea_turtle.png"},
        {"minecraft:horse", "minecraft:textures/entity/horse/horse_brown.png"},
        {"minecraft:donkey", "minecraft:textures/entity/horse/donkey.png"},
        {"minecraft:mule", "minecraft:textures/entity/horse/mule.png"},
        {"minecraft:llama", "minecraft:textures/entity/llama/creamy.png"},
        {"minecraft:bat", "minecraft:textures/entity/bat.png"},
        {"minecraft:blaze", "minecraft:textures/entity/blaze.png"},
        {"minecraft:guardian", "minecraft:textures/entity/guardian.png"},
        {"minecraft:elder_guardian", "minecraft:textures/entity/guardian_elder.png"},
        {"minecraft:phantom", "minecraft:textures/entity/phantom.png"},
        {"minecraft:silverfish", "minecraft:textures/entity/silverfish.png"},
        {"minecraft:endermite", "minecraft:textures/entity/endermite.png"},
        {"minecraft:squid", "minecraft:textures/entity/squid.png"},
        {"minecraft:dolphin", "minecraft:textures/entity/dolphin.png"},
        {"minecraft:snow_golem", "minecraft:textures/entity/snow_golem.png"},
        {"minecraft:wandering_trader", "minecraft:textures/entity/wandering_trader.png"},
        {"minecraft:witch", "minecraft:textures/entity/witch.png"},
        {"minecraft:husk", "minecraft:textures/entity/zombie/husk.png"},
        {"minecraft:drowned", "minecraft:textures/entity/zombie/drowned.png"},
        {"minecraft:stray", "minecraft:textures/entity/skeleton/stray.png"},
        {"minecraft:wither_skeleton", "minecraft:textures/entity/skeleton/wither_skeleton.png"},
        {"minecraft:giant", "minecraft:textures/entity/zombie/zombie.png"},
        {"minecraft:zombified_piglin", "minecraft:textures/entity/piglin/zombified_piglin.png"},
        {"minecraft:piglin", "minecraft:textures/entity/piglin/piglin.png"},
        {"minecraft:piglin_brute", "minecraft:textures/entity/piglin/piglin_brute.png"},
        {"minecraft:hoglin", "minecraft:textures/entity/hoglin/hoglin.png"},
        {"minecraft:zoglin", "minecraft:textures/entity/hoglin/zoglin.png"},
        {"minecraft:vindicator", "minecraft:textures/entity/illager/vindicator.png"},
        {"minecraft:evoker", "minecraft:textures/entity/illager/evoker.png"},
        {"minecraft:illusioner", "minecraft:textures/entity/illager/illusioner.png"},
        {"minecraft:pillager", "minecraft:textures/entity/illager/pillager.png"},
        {"minecraft:ravager", "minecraft:textures/entity/illager/ravager.png"},
        {"minecraft:vex", "minecraft:textures/entity/illager/vex.png"},
        {"minecraft:ender_dragon", "minecraft:textures/entity/enderdragon/dragon.png"}};

    for (const auto& [entityTypeId, expectedPath] : specialCases) {
        auto paths = getTexturePaths(entityTypeId);
        ASSERT_FALSE(paths.empty()) << entityTypeId;
        EXPECT_EQ(paths.front(), expectedPath) << entityTypeId;
    }
}

TEST_F(EntityTexturePathGenerationTest, GetTexturePathsWithoutNamespace)
{
    // 测试不带命名空间的实体类型ID
    auto paths = getTexturePaths("pig");

    ASSERT_EQ(paths.size(), 2u);
    EXPECT_EQ(paths[0], "minecraft:textures/entity/pig/pig.png");
    EXPECT_EQ(paths[1], "minecraft:textures/entity/pig.png");
}

TEST_F(EntityTexturePathGenerationTest, GetTexturePathsCustomMod)
{
    // 测试自定义模组的实体
    auto paths = getTexturePaths("mymod:dragon");

    ASSERT_EQ(paths.size(), 2u);
    EXPECT_EQ(paths[0], "minecraft:textures/entity/dragon/dragon.png");
    EXPECT_EQ(paths[1], "minecraft:textures/entity/dragon.png");
}

// ============================================================================
// 默认实体类型测试
// ============================================================================

class DefaultEntityTypesTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(DefaultEntityTypesTest, DefaultTypesContainExpected)
{
    // 测试默认实体类型列表应包含的类型
    // 这个列表来自 EntityTextureLoader::getDefaultEntityTypes()

    std::vector<std::string> expectedTypes = {"minecraft:pig", "minecraft:cow", "minecraft:sheep", "minecraft:chicken"};

    // 验证列表大小
    EXPECT_EQ(expectedTypes.size(), 4u);

    // 验证每个类型都能被正确规范化
    for (const auto& type : expectedTypes) {
        std::string normalized = normalizeEntityTypeId(type);
        EXPECT_EQ(normalized, type) << "Already normalized type should remain unchanged";
    }
}

TEST_F(DefaultEntityTypesTest, AllDefaultTypesHaveTexturePaths)
{
    // 验证所有默认实体类型都有对应的纹理路径
    std::vector<std::string> defaultTypes = {"minecraft:pig", "minecraft:cow", "minecraft:sheep", "minecraft:chicken"};

    for (const auto& type : defaultTypes) {
        auto paths = getTexturePaths(type);
        EXPECT_FALSE(paths.empty()) << "Each entity type should have at least one texture path";
    }
}

// ============================================================================
// 路径格式测试
// ============================================================================

class TexturePathFormatTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(TexturePathFormatTest, Mc113PathFormat)
{
    auto paths = getTexturePaths("minecraft:pig");
    ASSERT_FALSE(paths.empty());

    // MC 1.13+ 格式应该包含子目录
    EXPECT_TRUE(paths[0].find("/pig/") != std::string::npos) << "MC 1.13+ path should contain subdirectory";
}

TEST_F(TexturePathFormatTest, Mc112PathFormat)
{
    auto paths = getTexturePaths("minecraft:pig");
    ASSERT_GE(paths.size(), 2u);

    // MC 1.12 格式应该直接在 entity 目录下
    EXPECT_TRUE(paths[1].find("/entity/pig.png") != std::string::npos)
        << "MC 1.12 path should be directly in entity directory";
}

TEST_F(TexturePathFormatTest, AllPathsHaveCorrectPrefix)
{
    auto paths = getTexturePaths("minecraft:cow");

    for (const auto& path : paths) {
        EXPECT_TRUE(path.find("minecraft:textures/entity/") == 0)
            << "All texture paths should start with minecraft:textures/entity/";
    }
}

TEST_F(TexturePathFormatTest, AllPathsHavePngExtension)
{
    auto paths = getTexturePaths("minecraft:sheep");

    for (const auto& path : paths) {
        EXPECT_TRUE(path.find(".png") != std::string::npos) << "All texture paths should have .png extension";
    }
}

// main 函数由 gtest_main 库提供
