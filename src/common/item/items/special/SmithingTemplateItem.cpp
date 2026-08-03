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
#include "common/item/core/Item.hpp"
#include "resource/LanguageManager.hpp"
#include <string>
#include <utility>
#include <vector>

namespace mc {
namespace item {

// ============================================================================
// 翻译键常量
// ============================================================================

namespace {

// 标题行翻译键（灰色）
constexpr const char* SMITHING_TEMPLATE_KEY = "item.minecraft.smithing_template";
constexpr const char* APPLIES_TO_TITLE_KEY = "item.minecraft.smithing_template.applies_to";
constexpr const char* INGREDIENTS_TITLE_KEY = "item.minecraft.smithing_template.ingredients";

} // namespace

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
    const ItemStack& stack, IWorld* world, std::vector<std::string>& tooltip, bool advanced) const
{
    Item::addInformation(stack, world, tooltip, advanced);

    // 不依赖 world，world 为 null 时仍可通过 LanguageManager 翻译
    auto& lang = LanguageManager::instance();

    // 1. "Smithing Template" 后缀标题（灰色）
    tooltip.push_back(lang.get(SMITHING_TEMPLATE_KEY));

    // 2. 空行分隔
    tooltip.emplace_back("");

    // 3. "Applies to:" 标题（灰色）
    tooltip.push_back(lang.get(APPLIES_TO_TITLE_KEY));

    // 4. 具体适用描述（蓝色），如 "Armor" 或 "Diamond Equipment"
    //    MC Java 中标题与描述之间有空格，此处将翻译后的标题与描述拼接为单行
    std::string appliesToText = lang.get(m_appliesTo);
    tooltip.push_back(" " + appliesToText);

    // 5. "Ingredients:" 标题（灰色）
    tooltip.push_back(lang.get(INGREDIENTS_TITLE_KEY));

    // 6. 具体材料描述（蓝色），如 "Ingots & Crystals" 或 "Netherite Ingot"
    std::string ingredientsText = lang.get(m_ingredients);
    tooltip.push_back(" " + ingredientsText);
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
