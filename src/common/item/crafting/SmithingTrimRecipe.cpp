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

#include "SmithingTrimRecipe.hpp"
#include "common/entity/inventory/IInventory.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/crafting/IRecipe.hpp"
#include "common/item/crafting/Ingredient.hpp"
#include "common/resource/ResourceLocation.hpp"
#include <string>
#include <vector>

namespace mc {
namespace crafting {

const std::string SmithingTrimRecipe::EMPTY_GROUP = "";

SmithingTrimRecipe::SmithingTrimRecipe(const ResourceLocation& id,
    Ingredient templateIngredient,
    Ingredient base,
    Ingredient addition,
    ResourceLocation pattern)
    : m_id(id)
    , m_template(std::move(templateIngredient))
    , m_base(std::move(base))
    , m_addition(std::move(addition))
    , m_pattern(std::move(pattern))
{
    // 缓存原料列表：template + base + addition（顺序对应原版 3 槽）
    m_ingredients.push_back(m_template);
    m_ingredients.push_back(m_base);
    m_ingredients.push_back(m_addition);
}

bool SmithingTrimRecipe::matches(const IInventory& inventory) const
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

    return m_template.test(templateStack) && m_base.test(baseStack) && m_addition.test(additionStack);
}

ItemStack SmithingTrimRecipe::assemble(const IInventory& inventory) const
{
    // 对应 MC 1.21.11 SmithingTrimRecipe#assemble -> applyTrim(base, addition, pattern)
    // 原版逻辑：从 addition 解析 TrimMaterial，构造 ArmorTrim(material, pattern)，
    // 若 base 已有相同 trim 则返回 EMPTY（去重），否则返回 base 副本并写入 TRIM 组件；
    // addition 无法解析为 TrimMaterial 时返回 EMPTY。
    //
    // TODO: 纹饰系统（TrimMaterial/TrimPattern 注册表、ArmorTrim、TRIM 数据组件）未实现，
    // 当前暂返回 base 副本（不加纹饰），待纹饰系统接入后补全 TRIM 组件写入与去重逻辑。
    if (inventory.getContainerSize() > SLOT_BASE) {
        const ItemStack& baseStack = inventory.getItem(SLOT_BASE);
        if (!baseStack.isEmpty()) {
            ItemStack result = baseStack.copy();
            result.setCount(1);
            return result;
        }
    }
    return ItemStack::EMPTY;
}

std::vector<ItemStack> SmithingTrimRecipe::getRemainingItems(const IInventory& inventory) const
{
    return RecipeUtils::getDefaultRemainingItems(inventory);
}

} // namespace crafting
} // namespace mc
