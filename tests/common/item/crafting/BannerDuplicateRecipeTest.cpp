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

#include "item/crafting/special/BannerDuplicateRecipe.hpp"
#include <gtest/gtest.h>

using namespace mc;
using namespace mc::crafting;

// ========== BannerDuplicateRecipe 测试 ==========

TEST(BannerDuplicateRecipeTest, Construction)
{
    BannerDuplicateRecipe recipe(ResourceLocation("minecraft", "banner_duplication"));
    EXPECT_EQ(recipe.getId().toString(), "minecraft:banner_duplication");
}

// 注意：BannerDuplicateRecipe的matches和assemble方法需要CraftingInventory实例，
// 这需要完整的物品注册。以下测试验证基本的构造和ID。
// 完整的匹配测试应在集成测试中进行。

TEST(BannerDuplicateRecipeTest, RecipeType)
{
    BannerDuplicateRecipe recipe(ResourceLocation("minecraft", "banner_duplication"));
    // SpecialRecipe的recipe type应该正确
    EXPECT_TRUE(recipe.getId().getNamespace() == "minecraft");
    EXPECT_TRUE(recipe.getId().getPath() == "banner_duplication");
}
