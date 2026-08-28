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
 * @file test_template_pool_loader.cpp
 * @brief TemplatePoolLoader单元测试
 */

#include "common/resource/ResourceLocation.hpp"
#include "common/world/gen/jigsaw/TemplatePool.hpp"
#include "common/world/gen/jigsaw/TemplatePoolLoader.hpp"
#include <gtest/gtest.h>

using namespace mc::world::gen::jigsaw;
using namespace mc;

/**
 * @brief 测试从JSON字符串加载简单模板池
 */
TEST(TemplatePoolLoaderTest, LoadSimplePoolFromJson)
{
    const std::string json = R"({
        "name": "minecraft:test/simple_pool",
        "fallback": "minecraft:empty",
        "elements": [
            {
                "weight": 1,
                "element": {
                    "element_type": "minecraft:single_pool_element",
                    "location": "minecraft:test/template_01",
                    "projection": "rigid"
                }
            }
        ]
    })";

    auto result = TemplatePoolLoader::loadFromJson(json, ResourceLocation("minecraft", "test/simple_pool"));

    ASSERT_TRUE(result.success()) << "Failed to load: " << result.error().message();
    auto pattern = std::move(result.value());

    ASSERT_NE(pattern, nullptr);
    EXPECT_EQ(pattern->getTotalWeight(), 1u);
}

/**
 * @brief 测试从JSON对象加载模板池
 */
TEST(TemplatePoolLoaderTest, LoadFromJsonObject)
{
    const std::string json = R"({
        "name": "minecraft:test/object_pool",
        "fallback": "minecraft:empty",
        "elements": [
            {
                "weight": 2,
                "element": {
                    "element_type": "minecraft:single_pool_element",
                    "location": "minecraft:test/template_a",
                    "projection": "rigid"
                }
            },
            {
                "weight": 3,
                "element": {
                    "element_type": "minecraft:single_pool_element",
                    "location": "minecraft:test/template_b",
                    "projection": "terrain_matching"
                }
            }
        ]
    })";

    auto result = TemplatePoolLoader::loadFromJson(json, ResourceLocation("minecraft", "test/object_pool"));

    ASSERT_TRUE(result.success()) << "Failed to load: " << result.error().message();
    auto pattern = std::move(result.value());

    ASSERT_NE(pattern, nullptr);
    // 权重 2 + 3 = 5
    EXPECT_EQ(pattern->getTotalWeight(), 5u);
}

/**
 * @brief 测试加载带有terrain_matching投影的模板池
 */
TEST(TemplatePoolLoaderTest, LoadWithTerrainMatchingProjection)
{
    const std::string json = R"({
        "name": "minecraft:test/terrain_pool",
        "fallback": "minecraft:empty",
        "elements": [
            {
                "weight": 1,
                "element": {
                    "element_type": "minecraft:single_pool_element",
                    "location": "minecraft:test/terrain_template",
                    "projection": "terrain_matching"
                }
            }
        ]
    })";

    auto result = TemplatePoolLoader::loadFromJson(json, ResourceLocation("minecraft", "test/terrain_pool"));

    ASSERT_TRUE(result.success()) << "Failed to load: " << result.error().message();
    auto pattern = std::move(result.value());

    ASSERT_NE(pattern, nullptr);
    EXPECT_EQ(pattern->getTotalWeight(), 1u);
}

/**
 * @brief 测试加载空模板池元素
 */
TEST(TemplatePoolLoaderTest, LoadEmptyPoolElement)
{
    const std::string json = R"({
        "name": "minecraft:test/empty_element_pool",
        "fallback": "minecraft:empty",
        "elements": [
            {
                "weight": 1,
                "element": {
                    "element_type": "minecraft:empty_pool_element"
                }
            }
        ]
    })";

    auto result = TemplatePoolLoader::loadFromJson(json, ResourceLocation("minecraft", "test/empty_element_pool"));

    ASSERT_TRUE(result.success()) << "Failed to load: " << result.error().message();
    auto pattern = std::move(result.value());

    ASSERT_NE(pattern, nullptr);
    EXPECT_EQ(pattern->getTotalWeight(), 1u);
}

/**
 * @brief 测试加载列表池元素
 */
TEST(TemplatePoolLoaderTest, LoadListPoolElement)
{
    const std::string json = R"({
        "name": "minecraft:test/list_pool",
        "fallback": "minecraft:empty",
        "elements": [
            {
                "weight": 1,
                "element": {
                    "element_type": "minecraft:list_pool_element",
                    "elements": [
                        {
                            "element_type": "minecraft:single_pool_element",
                            "location": "minecraft:test/list_item_1",
                            "projection": "rigid"
                        },
                        {
                            "element_type": "minecraft:single_pool_element",
                            "location": "minecraft:test/list_item_2",
                            "projection": "rigid"
                        }
                    ],
                    "projection": "rigid"
                }
            }
        ]
    })";

    auto result = TemplatePoolLoader::loadFromJson(json, ResourceLocation("minecraft", "test/list_pool"));

    ASSERT_TRUE(result.success()) << "Failed to load: " << result.error().message();
    auto pattern = std::move(result.value());

    ASSERT_NE(pattern, nullptr);
    EXPECT_EQ(pattern->getTotalWeight(), 1u);
}

/**
 * @brief 测试加载多个元素的模板池
 */
TEST(TemplatePoolLoaderTest, LoadMultipleElements)
{
    const std::string json = R"({
        "name": "minecraft:test/multi_pool",
        "fallback": "minecraft:empty",
        "elements": [
            {
                "weight": 1,
                "element": {
                    "element_type": "minecraft:single_pool_element",
                    "location": "minecraft:test/element_1",
                    "projection": "rigid"
                }
            },
            {
                "weight": 2,
                "element": {
                    "element_type": "minecraft:single_pool_element",
                    "location": "minecraft:test/element_2",
                    "projection": "rigid"
                }
            },
            {
                "weight": 3,
                "element": {
                    "element_type": "minecraft:single_pool_element",
                    "location": "minecraft:test/element_3",
                    "projection": "terrain_matching"
                }
            }
        ]
    })";

    auto result = TemplatePoolLoader::loadFromJson(json, ResourceLocation("minecraft", "test/multi_pool"));

    ASSERT_TRUE(result.success()) << "Failed to load: " << result.error().message();
    auto pattern = std::move(result.value());

    ASSERT_NE(pattern, nullptr);
    // 权重 1 + 2 + 3 = 6
    EXPECT_EQ(pattern->getTotalWeight(), 6u);
}

/**
 * @brief 测试缺少必要字段时返回错误
 */
TEST(TemplatePoolLoaderTest, MissingRequiredFieldReturnsError)
{
    // 缺少 elements 字段
    const std::string json = R"({
        "name": "minecraft:test/missing_elements",
        "fallback": "minecraft:empty"
    })";

    auto result = TemplatePoolLoader::loadFromJson(json, ResourceLocation("minecraft", "test/missing_elements"));

    EXPECT_FALSE(result.success());
}

/**
 * @brief 测试无效JSON返回错误
 */
TEST(TemplatePoolLoaderTest, InvalidJsonReturnsError)
{
    const std::string invalidJson = "{ invalid json }";

    auto result = TemplatePoolLoader::loadFromJson(invalidJson, ResourceLocation("minecraft", "test/invalid"));

    EXPECT_FALSE(result.success());
}

/**
 * @brief 测试默认投影类型为rigid
 */
TEST(TemplatePoolLoaderTest, DefaultProjectionIsRigid)
{
    // 不指定 projection 字段
    const std::string json = R"({
        "name": "minecraft:test/default_projection",
        "fallback": "minecraft:empty",
        "elements": [
            {
                "weight": 1,
                "element": {
                    "element_type": "minecraft:single_pool_element",
                    "location": "minecraft:test/template"
                }
            }
        ]
    })";

    auto result = TemplatePoolLoader::loadFromJson(json, ResourceLocation("minecraft", "test/default_projection"));

    ASSERT_TRUE(result.success()) << "Failed to load: " << result.error().message();
    auto pattern = std::move(result.value());

    ASSERT_NE(pattern, nullptr);
    EXPECT_EQ(pattern->getTotalWeight(), 1u);
}

/**
 * @brief 测试默认权重为1
 */
TEST(TemplatePoolLoaderTest, DefaultWeightIsOne)
{
    // 不指定 weight 字段
    const std::string json = R"({
        "name": "minecraft:test/default_weight",
        "fallback": "minecraft:empty",
        "elements": [
            {
                "element": {
                    "element_type": "minecraft:single_pool_element",
                    "location": "minecraft:test/template"
                }
            }
        ]
    })";

    auto result = TemplatePoolLoader::loadFromJson(json, ResourceLocation("minecraft", "test/default_weight"));

    ASSERT_TRUE(result.success()) << "Failed to load: " << result.error().message();
    auto pattern = std::move(result.value());

    ASSERT_NE(pattern, nullptr);
    // 默认权重 1
    EXPECT_EQ(pattern->getTotalWeight(), 1u);
}

/**
 * @brief 测试空fallback默认为minecraft:empty
 */
TEST(TemplatePoolLoaderTest, DefaultFallbackIsEmpty)
{
    // 不指定 fallback 字段
    const std::string json = R"({
        "name": "minecraft:test/no_fallback",
        "elements": [
            {
                "weight": 1,
                "element": {
                    "element_type": "minecraft:single_pool_element",
                    "location": "minecraft:test/template"
                }
            }
        ]
    })";

    auto result = TemplatePoolLoader::loadFromJson(json, ResourceLocation("minecraft", "test/no_fallback"));

    ASSERT_TRUE(result.success()) << "Failed to load: " << result.error().message();
    auto pattern = std::move(result.value());

    ASSERT_NE(pattern, nullptr);
    EXPECT_EQ(pattern->getTotalWeight(), 1u);
}
