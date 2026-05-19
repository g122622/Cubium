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
 * @file GuiSpriteManagerTest.cpp
 * @brief GuiSpriteManager 单元测试（纯数据结构测试，不依赖Vulkan）
 */

#include "client/renderer/trident/gui/GuiSpriteManager.hpp"
#include "common/core/Types.hpp"
#include <gtest/gtest.h>

using namespace mc::client::renderer::trident::gui;

/**
 * @brief 测试 GuiSpriteManager 默认构造
 */
TEST(GuiSpriteManagerTest, DefaultConstruction)
{
    GuiSpriteManager manager;

    EXPECT_EQ(manager.spriteCount(), 0);
    EXPECT_EQ(manager.atlasWidth(), 256);  // 默认值
    EXPECT_EQ(manager.atlasHeight(), 256); // 默认值
}

/**
 * @brief 测试精灵注册和查询
 */
TEST(GuiSpriteManagerTest, RegisterAndFindSprite)
{
    GuiSpriteManager manager;

    // 注册精灵
    manager.registerSprite("button_normal", 0, 66, 200, 20, 256, 256);
    manager.registerSprite("button_hover", 0, 86, 200, 20, 256, 256);

    EXPECT_EQ(manager.spriteCount(), 2);

    // 查询精灵
    const GuiSprite* sprite = manager.getSprite("button_normal");
    ASSERT_NE(sprite, nullptr);
    EXPECT_EQ(sprite->id, "button_normal");
    EXPECT_EQ(sprite->width, 200);
    EXPECT_EQ(sprite->height, 20);
    EXPECT_FLOAT_EQ(sprite->u0, 0.0f);
    EXPECT_FLOAT_EQ(sprite->v0, 66.0f / 256.0f);

    // 查询不存在的精灵
    const GuiSprite* notFound = manager.getSprite("nonexistent");
    EXPECT_EQ(notFound, nullptr);
}

/**
 * @brief 测试精灵存在性检查
 */
TEST(GuiSpriteManagerTest, HasSprite)
{
    GuiSpriteManager manager;

    manager.registerSprite("test", 0, 0, 16, 16, 256, 256);

    EXPECT_TRUE(manager.hasSprite("test"));
    EXPECT_FALSE(manager.hasSprite("nonexistent"));
}

/**
 * @brief 测试使用 GuiSprite 对象注册
 */
TEST(GuiSpriteManagerTest, RegisterSpriteObject)
{
    GuiSpriteManager manager;

    GuiSprite sprite;
    sprite.id = "custom_sprite";
    sprite.u0 = 0.1f;
    sprite.v0 = 0.2f;
    sprite.u1 = 0.3f;
    sprite.v1 = 0.4f;
    sprite.width = 32;
    sprite.height = 32;
    sprite.setNinePatch(4, 4, 28, 28);

    manager.registerSprite(sprite);

    EXPECT_EQ(manager.spriteCount(), 1);

    const GuiSprite* found = manager.getSprite("custom_sprite");
    ASSERT_NE(found, nullptr);
    EXPECT_FLOAT_EQ(found->u0, 0.1f);
    EXPECT_FLOAT_EQ(found->v0, 0.2f);
    EXPECT_TRUE(found->ninePatch.isValid());
    EXPECT_EQ(found->ninePatch.left, 4);
}

/**
 * @brief 测试批量注册精灵
 */
TEST(GuiSpriteManagerTest, RegisterSprites)
{
    GuiSpriteManager manager;

    std::vector<GuiSprite> sprites;
    sprites.emplace_back("sprite1", 0, 0, 16, 16, 256, 256);
    sprites.emplace_back("sprite2", 16, 0, 16, 16, 256, 256);
    sprites.emplace_back("sprite3", 32, 0, 16, 16, 256, 256);

    manager.registerSprites(sprites);

    EXPECT_EQ(manager.spriteCount(), 3);
    EXPECT_TRUE(manager.hasSprite("sprite1"));
    EXPECT_TRUE(manager.hasSprite("sprite2"));
    EXPECT_TRUE(manager.hasSprite("sprite3"));
}

/**
 * @brief 测试清空精灵
 */
TEST(GuiSpriteManagerTest, ClearSprites)
{
    GuiSpriteManager manager;

    manager.registerSprite("sprite1", 0, 0, 16, 16, 256, 256);
    manager.registerSprite("sprite2", 16, 0, 16, 16, 256, 256);

    EXPECT_EQ(manager.spriteCount(), 2);

    manager.clearSprites();

    EXPECT_EQ(manager.spriteCount(), 0);
    EXPECT_FALSE(manager.hasSprite("sprite1"));
}

/**
 * @brief 测试设置图集尺寸
 */
TEST(GuiSpriteManagerTest, SetAtlasSize)
{
    GuiSpriteManager manager;

    EXPECT_EQ(manager.atlasWidth(), 256);
    EXPECT_EQ(manager.atlasHeight(), 256);

    manager.setAtlasSize(512, 512);

    EXPECT_EQ(manager.atlasWidth(), 512);
    EXPECT_EQ(manager.atlasHeight(), 512);

    // 注册精灵使用新尺寸
    manager.registerSprite("test", 0, 0, 64, 64, 512, 512);

    const GuiSprite* sprite = manager.getSprite("test");
    ASSERT_NE(sprite, nullptr);
    EXPECT_FLOAT_EQ(sprite->u1, 64.0f / 512.0f);
    EXPECT_FLOAT_EQ(sprite->v1, 64.0f / 512.0f);
}

/**
 * @brief 测试精灵ID覆盖
 */
TEST(GuiSpriteManagerTest, SpriteOverride)
{
    GuiSpriteManager manager;

    manager.registerSprite("test", 0, 0, 16, 16, 256, 256);
    EXPECT_EQ(manager.spriteCount(), 1);

    const GuiSprite* sprite1 = manager.getSprite("test");
    ASSERT_NE(sprite1, nullptr);
    EXPECT_EQ(sprite1->width, 16);

    // 覆盖同名精灵
    manager.registerSprite("test", 0, 0, 32, 32, 256, 256);
    EXPECT_EQ(manager.spriteCount(), 1); // 数量不变

    const GuiSprite* sprite2 = manager.getSprite("test");
    ASSERT_NE(sprite2, nullptr);
    EXPECT_EQ(sprite2->width, 32); // 尺寸已更新
}

/**
 * @brief 测试空ID精灵注册
 */
TEST(GuiSpriteManagerTest, RegisterEmptyIdSprite)
{
    GuiSpriteManager manager;

    GuiSprite emptySprite;
    emptySprite.id = "";
    emptySprite.width = 16;
    emptySprite.height = 16;

    manager.registerSprite(emptySprite);

    // 空ID精灵应该被忽略
    EXPECT_EQ(manager.spriteCount(), 0);
}

/**
 * @brief 测试拷贝和移动
 */
TEST(GuiSpriteManagerTest, CopyAndMove)
{
    GuiSpriteManager manager1;
    manager1.registerSprite("test", 0, 0, 16, 16, 256, 256);
    manager1.setAtlasSize(512, 512);

    // 拷贝构造
    GuiSpriteManager manager2(manager1);
    EXPECT_EQ(manager2.spriteCount(), 1);
    EXPECT_TRUE(manager2.hasSprite("test"));
    EXPECT_EQ(manager2.atlasWidth(), 512);

    // 移动构造
    GuiSpriteManager manager3(std::move(manager2));
    EXPECT_EQ(manager3.spriteCount(), 1);
    EXPECT_TRUE(manager3.hasSprite("test"));

    // 拷贝赋值
    GuiSpriteManager manager4;
    manager4 = manager1;
    EXPECT_EQ(manager4.spriteCount(), 1);
    EXPECT_TRUE(manager4.hasSprite("test"));
}

/**
 * @brief 测试 UV 坐标计算精度
 */
TEST(GuiSpriteManagerTest, UVCoordinatePrecision)
{
    GuiSpriteManager manager;

    // 测试各种图集尺寸
    manager.registerSprite("small", 0, 0, 16, 16, 16, 16);
    const GuiSprite* small = manager.getSprite("small");
    ASSERT_NE(small, nullptr);
    EXPECT_FLOAT_EQ(small->u0, 0.0f);
    EXPECT_FLOAT_EQ(small->v0, 0.0f);
    EXPECT_FLOAT_EQ(small->u1, 1.0f);
    EXPECT_FLOAT_EQ(small->v1, 1.0f);

    // 256x256 图集（MC标准尺寸）
    manager.registerSprite("std", 16, 16, 32, 32, 256, 256);
    const GuiSprite* std = manager.getSprite("std");
    ASSERT_NE(std, nullptr);
    EXPECT_FLOAT_EQ(std->u0, 16.0f / 256.0f);
    EXPECT_FLOAT_EQ(std->v0, 16.0f / 256.0f);
    EXPECT_FLOAT_EQ(std->u1, 48.0f / 256.0f);
    EXPECT_FLOAT_EQ(std->v1, 48.0f / 256.0f);

    // 512x512 图集（高清材质）
    manager.registerSprite("hd", 32, 32, 64, 64, 512, 512);
    const GuiSprite* hd = manager.getSprite("hd");
    ASSERT_NE(hd, nullptr);
    EXPECT_FLOAT_EQ(hd->u0, 32.0f / 512.0f);
    EXPECT_FLOAT_EQ(hd->v0, 32.0f / 512.0f);
    EXPECT_FLOAT_EQ(hd->u1, 96.0f / 512.0f);
    EXPECT_FLOAT_EQ(hd->v1, 96.0f / 512.0f);
}

/**
 * @brief 测试心形图标精灵定义
 */
TEST(GuiSpriteManagerTest, HeartIconSprites)
{
    GuiSpriteManager manager;
    static const int ATLAS_SIZE = 256;

    // 心形图标 (9x9)
    manager.registerSprite("heart_full", 52, 0, 9, 9, ATLAS_SIZE, ATLAS_SIZE);
    manager.registerSprite("heart_half", 61, 0, 9, 9, ATLAS_SIZE, ATLAS_SIZE);
    manager.registerSprite("heart_empty", 16, 0, 9, 9, ATLAS_SIZE, ATLAS_SIZE);

    const GuiSprite* heartFull = manager.getSprite("heart_full");
    const GuiSprite* heartHalf = manager.getSprite("heart_half");
    const GuiSprite* heartEmpty = manager.getSprite("heart_empty");

    ASSERT_NE(heartFull, nullptr);
    ASSERT_NE(heartHalf, nullptr);
    ASSERT_NE(heartEmpty, nullptr);

    EXPECT_TRUE(heartFull->isValid());
    EXPECT_TRUE(heartHalf->isValid());
    EXPECT_TRUE(heartEmpty->isValid());

    // 验证尺寸一致
    EXPECT_EQ(heartFull->width, heartHalf->width);
    EXPECT_EQ(heartFull->width, heartEmpty->width);
    EXPECT_EQ(heartFull->height, heartHalf->height);
    EXPECT_EQ(heartFull->height, heartEmpty->height);
}

/**
 * @brief 测试快捷栏精灵定义
 */
TEST(GuiSpriteManagerTest, HotbarSprites)
{
    GuiSpriteManager manager;
    static const int ATLAS_SIZE = 256;

    // 快捷栏背景 (182x22)
    manager.registerSprite("hotbar_bg", 0, 0, 182, 22, ATLAS_SIZE, ATLAS_SIZE);
    const GuiSprite* hotbarBg = manager.getSprite("hotbar_bg");
    ASSERT_NE(hotbarBg, nullptr);
    EXPECT_EQ(hotbarBg->width, 182);
    EXPECT_EQ(hotbarBg->height, 22);

    // 选中高亮 (24x22)
    manager.registerSprite("hotbar_selection", 0, 22, 24, 22, ATLAS_SIZE, ATLAS_SIZE);
    const GuiSprite* hotbarSel = manager.getSprite("hotbar_selection");
    ASSERT_NE(hotbarSel, nullptr);
    EXPECT_EQ(hotbarSel->width, 24);
    EXPECT_EQ(hotbarSel->height, 22);
}
