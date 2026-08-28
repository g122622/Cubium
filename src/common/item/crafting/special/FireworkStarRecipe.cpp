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

#include "FireworkStarRecipe.hpp"
#include "common/item/core/ItemStack.hpp"

namespace mc {
namespace crafting {

FireworkStarRecipe::FireworkStarRecipe(const ResourceLocation& id)
    : SpecialRecipe(id)
{}

bool FireworkStarRecipe::matches(const CraftingInventory& inventory) const
{
    // TODO: 烟花之星合成匹配逻辑未实现。原版规则：网格中含火药 + 染料（+ 可选附加效果物品）。
    // 待烟花物品组件（Fireworks/Explosion）系统接入后补全。
    (void)inventory;
    return false;
}

ItemStack FireworkStarRecipe::assemble(const CraftingInventory& inventory) const
{
    // TODO: 烟花之星合成逻辑未实现。待 Fireworks/Explosion 组件系统接入后补全。
    (void)inventory;
    return ItemStack::EMPTY;
}

std::vector<ItemStack> FireworkStarRecipe::getRemainingItems(const CraftingInventory& inventory) const
{
    return RecipeUtils::getDefaultRemainingItems(inventory);
}

} // namespace crafting
} // namespace mc
