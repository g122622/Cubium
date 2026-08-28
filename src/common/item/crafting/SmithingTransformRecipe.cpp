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

#include "SmithingTransformRecipe.hpp"
#include "common/entity/inventory/IInventory.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/crafting/IRecipe.hpp"
#include "common/item/crafting/Ingredient.hpp"
#include "common/resource/ResourceLocation.hpp"
#include <string>
#include <vector>

namespace mc {
namespace crafting {

const std::string SmithingTransformRecipe::EMPTY_GROUP = "";

SmithingTransformRecipe::SmithingTransformRecipe(
    const ResourceLocation& id, Ingredient templateIngredient, Ingredient base, Ingredient addition, ItemStack result)
    : m_id(id)
    , m_template(std::move(templateIngredient))
    , m_base(std::move(base))
    , m_addition(std::move(addition))
    , m_result(std::move(result))
{
    // 缓存原料列表：template + base + addition（顺序对应原版 3 槽）
    m_ingredients.push_back(m_template);
    m_ingredients.push_back(m_base);
    m_ingredients.push_back(m_addition);
}

bool SmithingTransformRecipe::matches(const IInventory& inventory) const
{
    // TODO: 锻造台容器槽位语义待对齐。项目当前无 SmithingMenu 实现，
    // 此处按原版 1.21+ 锻造台 3 槽（template=0/base=1/addition=2）匹配。
    // 待锻造台容器接入后，需根据实际槽位布局调整 SLOT_* 常量与此处读取。
    if (inventory.getContainerSize() < 3) {
        return false;
    }

    ItemStack templateStack = inventory.getItem(SLOT_TEMPLATE);
    ItemStack baseStack = inventory.getItem(SLOT_BASE);
    ItemStack additionStack = inventory.getItem(SLOT_ADDITION);

    // template/addition 为空 Ingredient 时 test() 仅对空堆返回 true，
    // 天然复刻原版 Ingredient.testOptionalIngredient 语义。
    return m_template.test(templateStack) && m_base.test(baseStack) && m_addition.test(additionStack);
}

ItemStack SmithingTransformRecipe::assemble(const IInventory& inventory) const
{
    // 对应 MC 1.21.11 SmithingTransformRecipe#assemble -> TransmuteResult#apply(base)
    // 基于 base 物品做 transmuteCopy：保留 base 的附魔/自定义名/修复成本等数据组件，
    // 仅替换物品类型为 result 物品。与 TransmuteRecipe::assemble 同款语义。
    const Item* resultItem = m_result.getItem();
    if (resultItem == nullptr) {
        return ItemStack::EMPTY;
    }

    if (inventory.getContainerSize() > SLOT_BASE) {
        const ItemStack& baseStack = inventory.getItem(SLOT_BASE);
        if (!baseStack.isEmpty()) {
            return baseStack.transmuteCopy(*resultItem, m_result.getCount());
        }
    }

    // base 槽为空时退化为返回结果物品本身（不应在正常 matches 通过后发生）
    return m_result.copy();
}

std::vector<ItemStack> SmithingTransformRecipe::getRemainingItems(const IInventory& inventory) const
{
    return RecipeUtils::getDefaultRemainingItems(inventory);
}

} // namespace crafting
} // namespace mc
