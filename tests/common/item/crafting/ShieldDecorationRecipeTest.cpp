/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the other conditions:
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

#include "item/crafting/special/ShieldDecorationRecipe.hpp"
#include <gtest/gtest.h>

using namespace mc;
using namespace mc::crafting;

// ========== ShieldDecorationRecipe 测试 ==========

TEST(ShieldDecorationRecipeTest, Construction)
{
    ShieldDecorationRecipe recipe(ResourceLocation("minecraft", "shield_decoration"));
    EXPECT_EQ(recipe.getId().toString(), "minecraft:shield_decoration");
}

TEST(ShieldDecorationRecipeTest, RecipeType)
{
    ShieldDecorationRecipe recipe(ResourceLocation("minecraft", "shield_decoration"));
    EXPECT_TRUE(recipe.getId().namespace_() == "minecraft");
    EXPECT_TRUE(recipe.getId().path() == "shield_decoration");
}
