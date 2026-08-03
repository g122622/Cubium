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

#include "common/item/loot/conditions/TableBonusCondition.hpp"
#include "common/core/Types.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/enchantment/EnchantmentHelper.hpp"
#include "common/item/loot/context/LootContext.hpp"
#include "common/item/loot/context/LootParams.hpp"
#include "common/util/assert/AssertAll.hpp"

#include <algorithm>
#include <cstddef>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace mc {
namespace loot {

TableBonusCondition::TableBonusCondition(std::string enchantmentId, std::vector<f32> chances)
    : m_enchantmentId(std::move(enchantmentId))
    , m_chances(std::move(chances))
{
    MC_ASSERT_RELEASE(!m_enchantmentId.empty());
    MC_ASSERT_RELEASE(!m_chances.empty());
}

bool TableBonusCondition::test(LootContext& context) const
{
    // 从上下文中获取工具物品
    auto* tool = context.get<ItemStack>(LootParams::TOOL);

    // 获取工具上指定附魔的等级，无工具则等级为0
    i32 enchantmentLevel = 0;
    if (tool != nullptr && !tool->isEmpty()) {
        enchantmentLevel = item::enchant::EnchantmentHelper::getEnchantmentLevel(*tool, m_enchantmentId);
    }

    // 使用附魔等级作为索引查找概率表，超出范围则使用最后一个值
    i32 index = std::min(enchantmentLevel, static_cast<i32>(m_chances.size()) - 1);
    f32 chance = m_chances[static_cast<size_t>(index)];

    return context.getRandom().nextFloat() < chance;
}

std::unique_ptr<LootCondition> TableBonusCondition::clone() const noexcept
{
    return std::make_unique<TableBonusCondition>(m_enchantmentId, m_chances);
}

} // namespace loot
} // namespace mc
