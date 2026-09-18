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
 * @file GuiSpriteTest.cpp
 * @brief GuiSprite 和 GuiNinePatch 单元测试（纯数据结构测试，不依赖Vulkan）
 */

#include "client/renderer/trident/gui/GuiSprite.hpp"
#include "common/core/Types.hpp"
#include <gtest/gtest.h>

using namespace mc::client::renderer::trident::gui;

/**
 * @brief 测试 GuiSprite 默认构造
 */
TEST(GuiSpriteTest, DefaultConstruction)
{
    GuiSprite sprite;
    EXPECT_TRUE(sprite.id.empty());
    EXPECT_FLOAT_EQ(sprite.u0, 0.0f);
    EXPECT_FLOAT_EQ(sprite.v0, 0.0f);
    EXPECT_FLOAT_EQ(sprite.u1, 1.0f);
    EXPECT_FLOAT_EQ(sprite.v1, 1.0f);
    EXPECT_EQ(sprite.width, 0);
    EXPECT_EQ(sprite.height, 0);
    EXPECT_FALSE(sprite.isValid());
}

/**
 * @brief 测试 GuiSprite 带参数构造
 */
TEST(GuiSpriteTest, ParameterizedConstruction)
{
    // widgets.png 中按钮精灵 (256x256 图集)
    GuiSprite sprite("button_normal", 0, 66, 200, 20, 256, 256);

    EXPECT_EQ(sprite.id, "button_normal");
    EXPECT_FLOAT_EQ(sprite.u0, 0.0f / 256.0f);
    EXPECT_FLOAT_EQ(sprite.v0, 66.0f / 256.0f);
    EXPECT_FLOAT_EQ(sprite.u1, 200.0f / 256.0f);
    EXPECT_FLOAT_EQ(sprite.v1, 86.0f / 256.0f);
    EXPECT_EQ(sprite.width, 200);
    EXPECT_EQ(sprite.height, 20);
    EXPECT_TRUE(sprite.isValid());
}

/**
 * @brief 测试 GuiSprite 九宫格设置
 */
TEST(GuiSpriteTest, SetNinePatch)
{
    GuiSprite sprite("button", 0, 66, 200, 20, 256, 256);

    sprite.setNinePatch(4, 4, 196, 16);

    EXPECT_TRUE(sprite.ninePatch.isValid());
    EXPECT_EQ(sprite.ninePatch.left, 4);
    EXPECT_EQ(sprite.ninePatch.top, 4);
    EXPECT_EQ(sprite.ninePatch.right, 196);
    EXPECT_EQ(sprite.ninePatch.bottom, 16);
}

/**
 * @brief 测试 GuiSprite 状态变体设置
 */
TEST(GuiSpriteTest, SetStateSprites)
{
    GuiSprite sprite("button", 0, 66, 200, 20, 256, 256);

    sprite.setStateSprites("button_hover", "button_disabled");

    EXPECT_EQ(sprite.hoverSprite, "button_hover");
    EXPECT_EQ(sprite.disabledSprite, "button_disabled");
}

/**
 * @brief 测试 GuiSprite 链式调用
 */
TEST(GuiSpriteTest, ChainedCalls)
{
    GuiSprite sprite("button", 0, 66, 200, 20, 256, 256);

    sprite.setNinePatch(4, 4, 196, 16).setStateSprites("button_hover", "button_disabled");

    EXPECT_TRUE(sprite.ninePatch.isValid());
    EXPECT_EQ(sprite.hoverSprite, "button_hover");
    EXPECT_EQ(sprite.disabledSprite, "button_disabled");
}

/**
 * @brief 测试 GuiNinePatch 默认构造
 */
TEST(GuiNinePatchTest, DefaultConstruction)
{
    GuiNinePatch ninePatch;
    EXPECT_EQ(ninePatch.left, 0);
    EXPECT_EQ(ninePatch.top, 0);
    EXPECT_EQ(ninePatch.right, 0);
    EXPECT_EQ(ninePatch.bottom, 0);
    EXPECT_FALSE(ninePatch.isValid());
}

/**
 * @brief 测试 GuiNinePatch 有效性检查
 */
TEST(GuiNinePatchTest, IsValid)
{
    GuiNinePatch empty;
    EXPECT_FALSE(empty.isValid());

    GuiNinePatch valid{4, 4, 196, 16};
    EXPECT_TRUE(valid.isValid());

    // 只有左边距
    GuiNinePatch leftOnly{4, 0, 0, 0};
    EXPECT_TRUE(leftOnly.isValid());
}

/**
 * @brief 测试 UV 坐标计算精度
 */
TEST(GuiSpriteTest, UVCoordinatePrecision)
{
    // 测试各种图集尺寸
    GuiSprite small("small", 0, 0, 16, 16, 16, 16);
    EXPECT_FLOAT_EQ(small.u0, 0.0f);
    EXPECT_FLOAT_EQ(small.v0, 0.0f);
    EXPECT_FLOAT_EQ(small.u1, 1.0f);
    EXPECT_FLOAT_EQ(small.v1, 1.0f);

    // 256x256 图集（MC标准尺寸）
    GuiSprite standard("std", 16, 16, 32, 32, 256, 256);
    EXPECT_FLOAT_EQ(standard.u0, 16.0f / 256.0f);
    EXPECT_FLOAT_EQ(standard.v0, 16.0f / 256.0f);
    EXPECT_FLOAT_EQ(standard.u1, 48.0f / 256.0f);
    EXPECT_FLOAT_EQ(standard.v1, 48.0f / 256.0f);

    // 512x512 图集（高清材质）
    GuiSprite hd("hd", 32, 32, 64, 64, 512, 512);
    EXPECT_FLOAT_EQ(hd.u0, 32.0f / 512.0f);
    EXPECT_FLOAT_EQ(hd.v0, 32.0f / 512.0f);
    EXPECT_FLOAT_EQ(hd.u1, 96.0f / 512.0f);
    EXPECT_FLOAT_EQ(hd.v1, 96.0f / 512.0f);
}

/**
 * @brief 测试心形图标精灵定义
 */
TEST(GuiSpriteTest, HeartIconSprites)
{
    static const int ATLAS_SIZE = 256;

    // 心形图标 (9x9)
    GuiSprite heartFull("heart_full", 52, 0, 9, 9, ATLAS_SIZE, ATLAS_SIZE);
    GuiSprite heartHalf("heart_half", 61, 0, 9, 9, ATLAS_SIZE, ATLAS_SIZE);
    GuiSprite heartEmpty("heart_empty", 16, 0, 9, 9, ATLAS_SIZE, ATLAS_SIZE);

    EXPECT_TRUE(heartFull.isValid());
    EXPECT_TRUE(heartHalf.isValid());
    EXPECT_TRUE(heartEmpty.isValid());

    // 验证尺寸一致
    EXPECT_EQ(heartFull.width, heartHalf.width);
    EXPECT_EQ(heartFull.width, heartEmpty.width);
    EXPECT_EQ(heartFull.height, heartHalf.height);
    EXPECT_EQ(heartFull.height, heartEmpty.height);
}

/**
 * @brief 测试快捷栏精灵定义
 */
TEST(GuiSpriteTest, HotbarSprites)
{
    static const int ATLAS_SIZE = 256;

    // 快捷栏背景 (182x22)
    GuiSprite hotbarBg("hotbar_bg", 0, 0, 182, 22, ATLAS_SIZE, ATLAS_SIZE);
    EXPECT_EQ(hotbarBg.width, 182);
    EXPECT_EQ(hotbarBg.height, 22);

    // 选中高亮 (24x22)
    GuiSprite hotbarSel("hotbar_selection", 0, 22, 24, 22, ATLAS_SIZE, ATLAS_SIZE);
    EXPECT_EQ(hotbarSel.width, 24);
    EXPECT_EQ(hotbarSel.height, 22);
}
