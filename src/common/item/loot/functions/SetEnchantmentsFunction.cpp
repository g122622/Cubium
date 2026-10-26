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

#include "SetEnchantmentsFunction.hpp"
#include "common/item/Items.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/enchantment/EnchantmentContainer.hpp"
#include "common/item/enchantment/EnchantmentRegistry.hpp"
#include "common/item/loot/context/LootContext.hpp"
#include "common/item/loot/functions/LootFunction.hpp"
#include <algorithm>
#include <memory>

namespace mc {
namespace loot {

SetEnchantmentsFunction::SetEnchantmentsFunction(std::vector<Entry> enchantments, bool add)
    : m_enchantments(std::move(enchantments))
    , m_add(add)
{}

ItemStack SetEnchantmentsFunction::run(ItemStack stack, LootContext& context) const
{
    if (stack.isEmpty()) {
        return stack;
    }

    // 书 -> 附魔书（vanilla 在设置附魔前先转物）
    if (stack.getItem() == Items::BOOK) {
        const Item* enchantedBook = ItemRegistry::instance().getItem(ResourceLocation("minecraft:enchanted_book"));
        if (enchantedBook != nullptr) {
            stack = stack.transmuteCopy(*enchantedBook);
        }
    }

    auto& container = stack.getEnchantmentsMutable();
    for (const auto& entry : m_enchantments) {
        const i32 rolled = entry.level.generateInt(context.getRandom());
        const i32 current = m_add ? container.getLevel(entry.enchantmentId.toString()) : 0;
        // vanilla 对等级做 clamp(0, 255)：0 表示移除该附魔
        const i32 level = std::clamp(current + rolled, 0, 255);
        if (level <= 0) {
            container.remove(entry.enchantmentId.toString());
        } else {
            container.set(entry.enchantmentId.toString(), level);
        }
    }
    return stack;
}

std::unique_ptr<LootFunction> SetEnchantmentsFunction::clone() const noexcept
{
    auto func = std::make_unique<SetEnchantmentsFunction>(m_enchantments, m_add);
    for (const auto& cond : m_conditions) {
        func->addCondition(cond->clone());
    }
    return func;
}

} // namespace loot
} // namespace mc
