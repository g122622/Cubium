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
 * @file GuiSpriteParserTest.cpp
 * @brief GuiSpriteParser 单元测试
 */

#include "client/renderer/trident/gui/GuiSpriteParser.hpp"
#include <gtest/gtest.h>

using namespace mc::client::renderer::trident::gui;

/**
 * @brief 测试解析基本精灵定义
 */
TEST(GuiSpriteParserTest, ParseBasicSprites)
{
    const char* json = R"({
        "texture": "minecraft:textures/gui/widgets.png",
        "sprites": {
            "button_normal": { "x": 0, "y": 66, "width": 200, "height": 20 },
            "button_hover": { "x": 0, "y": 86, "width": 200, "height": 20 }
        }
    })";

    auto result = GuiSpriteParser::parse(json, 256, 256);
    ASSERT_TRUE(result.success());

    const auto& definition = result.value();
    EXPECT_EQ(definition.texture, "minecraft:textures/gui/widgets.png");
    EXPECT_EQ(definition.sprites.size(), 2);

    // 验证 button_normal
    auto it = definition.sprites.find("button_normal");
    ASSERT_NE(it, definition.sprites.end());
    EXPECT_EQ(it->second.id, "button_normal");
    EXPECT_EQ(it->second.width, 200);
    EXPECT_EQ(it->second.height, 20);
    EXPECT_FLOAT_EQ(it->second.u0, 0.0f);
    EXPECT_FLOAT_EQ(it->second.v0, 66.0f / 256.0f);
    EXPECT_FLOAT_EQ(it->second.u1, 200.0f / 256.0f);
    EXPECT_FLOAT_EQ(it->second.v1, 86.0f / 256.0f);
}

/**
 * @brief 测试解析九宫格定义
 */
TEST(GuiSpriteParserTest, ParseNinePatch)
{
    const char* json = R"({
        "texture": "minecraft:textures/gui/widgets.png",
        "sprites": {
            "button_normal": { "x": 0, "y": 66, "width": 200, "height": 20 }
        },
        "nine_patch": {
            "button_normal": { "left": 4, "top": 4, "right": 196, "bottom": 16 }
        }
    })";

    auto result = GuiSpriteParser::parse(json, 256, 256);
    ASSERT_TRUE(result.success());

    const auto& definition = result.value();

    // 验证九宫格数据
    auto npIt = definition.ninePatches.find("button_normal");
    ASSERT_NE(npIt, definition.ninePatches.end());
    EXPECT_EQ(npIt->second.left, 4);
    EXPECT_EQ(npIt->second.top, 4);
    EXPECT_EQ(npIt->second.right, 196);
    EXPECT_EQ(npIt->second.bottom, 16);

    // 验证精灵关联了九宫格
    auto spriteIt = definition.sprites.find("button_normal");
    ASSERT_NE(spriteIt, definition.sprites.end());
    EXPECT_TRUE(spriteIt->second.ninePatch.isValid());
    EXPECT_EQ(spriteIt->second.ninePatch.left, 4);
}

/**
 * @brief 测试解析状态变体
 */
TEST(GuiSpriteParserTest, ParseStateVariants)
{
    const char* json = R"({
        "texture": "minecraft:textures/gui/widgets.png",
        "sprites": {
            "button": { "x": 0, "y": 66, "width": 200, "height": 20 },
            "button_hover": { "x": 0, "y": 86, "width": 200, "height": 20 },
            "button_disabled": { "x": 0, "y": 46, "width": 200, "height": 20 }
        },
        "state_variants": {
            "button": {
                "hover": "button_hover",
                "disabled": "button_disabled"
            }
        }
    })";

    auto result = GuiSpriteParser::parse(json, 256, 256);
    ASSERT_TRUE(result.success());

    const auto& definition = result.value();
    EXPECT_EQ(definition.sprites.size(), 3);

    auto it = definition.sprites.find("button");
    ASSERT_NE(it, definition.sprites.end());
    EXPECT_EQ(it->second.hoverSprite, "button_hover");
    EXPECT_EQ(it->second.disabledSprite, "button_disabled");
}

/**
 * @brief 测试解析缺失纹理路径
 */
TEST(GuiSpriteParserTest, ParseMissingTexture)
{
    const char* json = R"({
        "sprites": {
            "test": { "x": 0, "y": 0, "width": 16, "height": 16 }
        }
    })";

    auto result = GuiSpriteParser::parse(json, 256, 256);
    ASSERT_TRUE(result.success());

    // 纹理路径可以为空
    EXPECT_TRUE(result.value().texture.empty());
}

/**
 * @brief 测试解析无效JSON
 */
TEST(GuiSpriteParserTest, ParseInvalidJson)
{
    const char* json = "{ invalid json }";

    auto result = GuiSpriteParser::parse(json, 256, 256);
    EXPECT_FALSE(result.success());
}

/**
 * @brief 测试解析缺失必需字段
 */
TEST(GuiSpriteParserTest, ParseMissingRequiredFields)
{
    const char* json = R"({
        "sprites": {
            "incomplete": { "x": 0, "y": 0 }
        }
    })";

    auto result = GuiSpriteParser::parse(json, 256, 256);
    ASSERT_TRUE(result.success());

    // 缺失字段的精灵应该被跳过
    EXPECT_EQ(result.value().sprites.size(), 0);
}

/**
 * @brief 测试解析空精灵列表
 */
TEST(GuiSpriteParserTest, ParseEmptySprites)
{
    const char* json = R"({
        "texture": "minecraft:textures/gui/test.png",
        "sprites": {}
    })";

    auto result = GuiSpriteParser::parse(json, 256, 256);
    ASSERT_TRUE(result.success());

    EXPECT_EQ(result.value().sprites.size(), 0);
}

/**
 * @brief 测试不同图集尺寸
 */
TEST(GuiSpriteParserTest, ParseDifferentAtlasSize)
{
    const char* json = R"({
        "sprites": {
            "test": { "x": 100, "y": 50, "width": 32, "height": 32 }
        }
    })";

    // 512x512 图集
    auto result = GuiSpriteParser::parse(json, 512, 512);
    ASSERT_TRUE(result.success());

    const auto& sprite = result.value().sprites.at("test");
    EXPECT_FLOAT_EQ(sprite.u0, 100.0f / 512.0f);
    EXPECT_FLOAT_EQ(sprite.v0, 50.0f / 512.0f);
    EXPECT_FLOAT_EQ(sprite.u1, 132.0f / 512.0f);
    EXPECT_FLOAT_EQ(sprite.v1, 82.0f / 512.0f);
}

/**
 * @brief 测试解析快捷栏精灵（真实MC数据）
 */
TEST(GuiSpriteParserTest, ParseHotbarSprites)
{
    const char* json = R"({
        "texture": "minecraft:textures/gui/widgets.png",
        "sprites": {
            "hotbar_bg": { "x": 0, "y": 0, "width": 182, "height": 22 },
            "hotbar_selection": { "x": 0, "y": 22, "width": 24, "height": 22 }
        }
    })";

    auto result = GuiSpriteParser::parse(json, 256, 256);
    ASSERT_TRUE(result.success());

    const auto& definition = result.value();

    auto bgIt = definition.sprites.find("hotbar_bg");
    ASSERT_NE(bgIt, definition.sprites.end());
    EXPECT_EQ(bgIt->second.width, 182);
    EXPECT_EQ(bgIt->second.height, 22);

    auto selIt = definition.sprites.find("hotbar_selection");
    ASSERT_NE(selIt, definition.sprites.end());
    EXPECT_EQ(selIt->second.width, 24);
    EXPECT_EQ(selIt->second.height, 22);
}

/**
 * @brief 测试解析心形图标（真实MC数据）
 */
TEST(GuiSpriteParserTest, ParseHeartSprites)
{
    const char* json = R"({
        "texture": "minecraft:textures/gui/icons.png",
        "sprites": {
            "heart_full": { "x": 52, "y": 0, "width": 9, "height": 9 },
            "heart_half": { "x": 61, "y": 0, "width": 9, "height": 9 },
            "heart_empty": { "x": 16, "y": 0, "width": 9, "height": 9 }
        }
    })";

    auto result = GuiSpriteParser::parse(json, 256, 256);
    ASSERT_TRUE(result.success());

    const auto& definition = result.value();
    EXPECT_EQ(definition.sprites.size(), 3);

    // 所有心形图标应该有相同的尺寸
    for (const auto& [id, sprite] : definition.sprites) {
        EXPECT_EQ(sprite.width, 9);
        EXPECT_EQ(sprite.height, 9);
    }
}
