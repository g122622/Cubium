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

#include "FurnaceSmeltFunction.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/crafting/IRecipe.hpp"
#include "common/item/crafting/RecipeManager.hpp"
#include "common/item/crafting/SmeltingRecipe.hpp"
#include "common/item/loot/context/LootContext.hpp"
#include "common/item/loot/functions/LootFunction.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include <memory>

namespace mc {
namespace loot {

ItemStack FurnaceSmeltFunction::apply(ItemStack stack, LootContext& context) const
{
    if (stack.isEmpty()) {
        return stack;
    }

    // 从 RecipeManager 查找熔炼配方
    const auto& recipeManager = crafting::RecipeManager::instance();
    const crafting::SmeltingRecipe* recipe = recipeManager.getSmeltingRecipe(stack, crafting::RecipeType::Smelting);

    if (recipe != nullptr) {
        // 获取熔炼结果物品
        ItemStack result = recipe->getResultItem();
        if (!result.isEmpty()) {
            // 复制结果物品
            ItemStack smelted = result.copy();
            // 计算输出数量：输入数量 * 配方输出数量
            // Forge 扩展：支持配方返回多个物品
            smelted.setCount(stack.getCount() * result.getCount());
            return smelted;
        }
    }

    // 没有找到熔炼配方，返回原始物品
    MC_UNUSED(context);
    return stack;
}

std::unique_ptr<LootFunction> FurnaceSmeltFunction::clone() const noexcept
{
    auto func = std::make_unique<FurnaceSmeltFunction>();
    for (const auto& cond : m_conditions) {
        func->addCondition(cond->clone());
    }
    return func;
}

} // namespace loot
} // namespace mc
