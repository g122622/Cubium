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

#include "common/resource/ResourceLocation.hpp"
#include <gtest/gtest.h>

using namespace mc;

/**
 * @brief 实体纹理路径测试
 *
 * 测试 ResourceLocation 对于实体纹理路径的正确性
 */
class EntityTexturePathTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

// 测试实体纹理路径转换
TEST_F(EntityTexturePathTest, PigTexturePath)
{
    // MC 1.13+ 格式
    ResourceLocation loc1("minecraft:textures/entity/pig/pig.png");
    EXPECT_EQ(loc1.namespace_(), "minecraft");
    EXPECT_EQ(loc1.path(), "textures/entity/pig/pig.png");
    EXPECT_EQ(loc1.toFilePath(), "assets/minecraft/textures/entity/pig/pig.png");

    // MC 1.12 格式
    ResourceLocation loc2("minecraft:textures/entity/pig.png");
    EXPECT_EQ(loc2.namespace_(), "minecraft");
    EXPECT_EQ(loc2.path(), "textures/entity/pig.png");
    EXPECT_EQ(loc2.toFilePath(), "assets/minecraft/textures/entity/pig.png");
}

TEST_F(EntityTexturePathTest, CowTexturePath)
{
    ResourceLocation loc("minecraft:textures/entity/cow/cow.png");
    EXPECT_EQ(loc.namespace_(), "minecraft");
    EXPECT_EQ(loc.path(), "textures/entity/cow/cow.png");
    EXPECT_EQ(loc.toFilePath(), "assets/minecraft/textures/entity/cow/cow.png");
}

TEST_F(EntityTexturePathTest, SheepTexturePath)
{
    // 羊纹理
    ResourceLocation loc1("minecraft:textures/entity/sheep/sheep.png");
    EXPECT_EQ(loc1.toFilePath(), "assets/minecraft/textures/entity/sheep/sheep.png");

    // 羊毛纹理
    ResourceLocation loc2("minecraft:textures/entity/sheep/sheep_fur.png");
    EXPECT_EQ(loc2.toFilePath(), "assets/minecraft/textures/entity/sheep/sheep_fur.png");
}

TEST_F(EntityTexturePathTest, ChickenTexturePath)
{
    // 鸡纹理（MC 1.12 格式）
    ResourceLocation loc("minecraft:textures/entity/chicken.png");
    EXPECT_EQ(loc.namespace_(), "minecraft");
    EXPECT_EQ(loc.path(), "textures/entity/chicken.png");
    EXPECT_EQ(loc.toFilePath(), "assets/minecraft/textures/entity/chicken.png");
}

// 测试自定义命名空间
TEST_F(EntityTexturePathTest, CustomNamespace)
{
    ResourceLocation loc("mymod:textures/entity/custom_mob.png");
    EXPECT_EQ(loc.namespace_(), "mymod");
    EXPECT_EQ(loc.path(), "textures/entity/custom_mob.png");
    EXPECT_EQ(loc.toFilePath(), "assets/mymod/textures/entity/custom_mob.png");
}

// 测试路径解析
TEST_F(EntityTexturePathTest, PathParsing)
{
    // 测试 toString 和 toFilePath 的一致性
    ResourceLocation loc("minecraft:textures/entity/pig/pig.png");

    // toString 应该返回 namespace:path 格式
    EXPECT_EQ(loc.toString(), "minecraft:textures/entity/pig/pig.png");

    // toFilePath 应该返回 assets/namespace/path 格式
    EXPECT_EQ(loc.toFilePath(), "assets/minecraft/textures/entity/pig/pig.png");
}

// 测试带扩展名的路径
TEST_F(EntityTexturePathTest, WithExtension)
{
    ResourceLocation loc("minecraft:textures/entity/pig/pig");
    EXPECT_EQ(loc.toFilePath("png"), "assets/minecraft/textures/entity/pig/pig.png");
}
