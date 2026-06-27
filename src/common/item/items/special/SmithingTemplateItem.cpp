/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to Use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies of substantial portions of the Software.
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

#include "item/items/special/SmithingTemplateItem.hpp"

namespace mc {
namespace item {

SmithingTemplateItem::SmithingTemplateItem(SmithingTemplateType type,
    const std::string& appliesTo,
    const std::string& ingredients,
    const std::string& baseSlotDescription,
    const std::string& additionsSlotDescription,
    ItemProperties properties)
    : Item(std::move(properties))
    , m_type(type)
    , m_appliesTo(appliesTo)
    , m_ingredients(ingredients)
    , m_baseSlotDescription(baseSlotDescription)
    , m_additionsSlotDescription(additionsSlotDescription)
{}

void SmithingTemplateItem::addInformation(
    const ItemStack& stack, IWorld& world, std::vector<std::string>& tooltip, bool advanced) const
{
    Item::addInformation(stack, world, tooltip, advanced);

    // 显示"适用于"和"材料"提示
    tooltip.push_back(m_appliesTo);
    tooltip.push_back(m_ingredients);
}

ItemProperties SmithingTemplateItem::armorTrimProperties()
{
    return ItemProperties().maxStackSize(64);
}

ItemProperties SmithingTemplateItem::netheriteUpgradeProperties()
{
    return ItemProperties().maxStackSize(64);
}

} // namespace item
} // namespace mc
