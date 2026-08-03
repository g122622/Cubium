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

#include "SmithingRecipe.hpp"
#include "common/entity/inventory/IInventory.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/crafting/IRecipe.hpp"
#include "common/item/crafting/Ingredient.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/text/ITextComponent.hpp"
#include <string>
#include <vector>
#include <nlohmann/json_fwd.hpp>

namespace mc {
namespace crafting {

const std::string SmithingRecipe::EMPTY_GROUP = "";

SmithingRecipe::SmithingRecipe(
    const ResourceLocation& id, const Ingredient& base, const Ingredient& addition, const ItemStack& result)
    : m_id(id)
    , m_base(base)
    , m_addition(addition)
    , m_result(result)
{
    // 缓存原料列表
    m_ingredients.push_back(m_base);
    m_ingredients.push_back(m_addition);
}

bool SmithingRecipe::matches(const IInventory& inventory) const
{
    // 检查基础槽位和添加物槽位
    if (inventory.getContainerSize() < 2) {
        return false;
    }

    ItemStack baseStack = inventory.getItem(SLOT_BASE);
    ItemStack additionStack = inventory.getItem(SLOT_ADDITION);

    return m_base.test(baseStack) && m_addition.test(additionStack);
}

ItemStack SmithingRecipe::assemble(const IInventory& inventory) const
{
    // 锻造结果复制基础物品的 NBT 数据（附魔、耐久、自定义名称等）
    ItemStack resultStack = m_result.copy();

    // 获取基础物品（槽位0）
    if (inventory.getContainerSize() > SLOT_BASE) {
        const ItemStack& baseStack = inventory.getItem(SLOT_BASE);

        // 复制基础物品的NBT数据
        const nlohmann::json* baseTag = baseStack.getTag();
        if (baseTag != nullptr && !baseTag->is_null()) {
            // 使用 getOrCreateTag() 设置标签，然后复制数据
            nlohmann::json& resultTag = resultStack.getOrCreateTag();
            resultTag = *baseTag; // JSON拷贝赋值会执行深拷贝
        }

        // 复制附魔数据（如果NBT中没有附魔数据，这里作为备份）
        if (baseStack.hasEnchantments() && !resultStack.hasEnchantments()) {
            resultStack.getEnchantmentsMutable() = baseStack.getEnchantments();
        }

        // 复制耐久度
        if (baseStack.isDamageable() && baseStack.getDamage() > 0) {
            resultStack.setDamage(baseStack.getDamage());
        }

        // 复制修复成本
        if (baseStack.getRepairCost() > 0) {
            resultStack.setRepairCost(baseStack.getRepairCost());
        }

        // 复制自定义名称
        if (baseStack.hasCustomName()) {
            const text::ITextComponent* customName = baseStack.getCustomNameComponent();
            if (customName != nullptr) {
                resultStack.setCustomNameComponent(customName->deepCopy());
            }
        }
    }

    return resultStack;
}

const std::vector<Ingredient>& SmithingRecipe::getIngredients() const
{
    return m_ingredients;
}

std::vector<ItemStack> SmithingRecipe::getRemainingItems(const IInventory& inventory) const
{
    return RecipeUtils::getDefaultRemainingItems(inventory);
}

} // namespace crafting
} // namespace mc
