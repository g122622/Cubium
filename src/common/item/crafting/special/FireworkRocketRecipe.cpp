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

#include "FireworkRocketRecipe.hpp"
#include "common/item/core/ItemStack.hpp"

namespace mc {
namespace crafting {

FireworkRocketRecipe::FireworkRocketRecipe(const ResourceLocation& id)
    : SpecialRecipe(id)
{}

bool FireworkRocketRecipe::matches(const CraftingInventory& inventory) const
{
    // TODO: 烟花火箭合成匹配逻辑未实现。原版规则：网格中含火药 + 纸 + 0~N 个烟花之星，
    // 火药数量决定飞行时长。待烟花物品组件（Fireworks）系统接入后补全。
    (void)inventory;
    return false;
}

ItemStack FireworkRocketRecipe::assemble(const CraftingInventory& inventory) const
{
    // TODO: 烟花火箭合成逻辑未实现。待 Fireworks 组件系统接入后补全。
    (void)inventory;
    return ItemStack::EMPTY;
}

std::vector<ItemStack> FireworkRocketRecipe::getRemainingItems(const CraftingInventory& inventory) const
{
    return RecipeUtils::getDefaultRemainingItems(inventory);
}

} // namespace crafting
} // namespace mc
