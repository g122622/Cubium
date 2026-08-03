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

#include "EnchantRandomlyFunction.hpp"
#include "common/core/Types.hpp"
#include "common/item/Items.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/enchantment/Enchantment.hpp"
#include "common/item/enchantment/EnchantmentRegistry.hpp"
#include "common/item/items/special/EnchantedBookItem.hpp"
#include "common/item/loot/context/LootContext.hpp"
#include "common/item/loot/functions/LootFunction.hpp"
#include "common/util/math/random/Random.hpp"
#include <memory>
#include <string>
#include <vector>

namespace mc {
namespace loot {

EnchantRandomlyFunction::EnchantRandomlyFunction(const std::vector<std::string>& enchantments, bool treasure)
    : m_enchantments(enchantments)
    , m_treasure(treasure)
{}

ItemStack EnchantRandomlyFunction::apply(ItemStack stack, LootContext& context) const
{
    if (stack.isEmpty()) {
        return stack;
    }

    math::Random& random = context.getRandom();
    const item::enchant::Enchantment* selectedEnchantment = nullptr;

    if (m_enchantments.empty()) {
        // 没有指定附魔列表，从所有可用附魔中随机选择
        bool isBook = stack.getItem() == Items::BOOK;

        // 收集所有可用的附魔
        std::vector<const item::enchant::Enchantment*> availableEnchants;
        for (const auto& [id, enchantment] : item::enchant::EnchantmentRegistry::all()) {
            if (enchantment == nullptr) {
                continue;
            }

            // 检查是否可以生成
            if (!enchantment->canGenerateInLoot()) {
                continue;
            }

            // 检查是否可以应用到物品
            if (isBook) {
                if (!enchantment->isAllowedOnBooks()) {
                    continue;
                }
            } else {
                if (!enchantment->canApply(stack)) {
                    continue;
                }
            }

            availableEnchants.push_back(enchantment.get());
        }

        if (availableEnchants.empty()) {
            // 没有可用的附魔
            return stack;
        }

        // 随机选择一个
        selectedEnchantment = availableEnchants[random.nextInt(static_cast<i32>(availableEnchants.size()))];
    } else {
        // 从指定的附魔列表中随机选择
        if (m_enchantments.empty()) {
            return stack;
        }

        const std::string& enchId = m_enchantments[random.nextInt(static_cast<i32>(m_enchantments.size()))];
        selectedEnchantment = item::enchant::EnchantmentRegistry::get(enchId);
    }

    if (selectedEnchantment == nullptr) {
        return stack;
    }

    // 随机选择等级（从最小到最大）
    i32 level = random.nextInt(selectedEnchantment->maxLevel() - selectedEnchantment->minLevel() + 1) +
        selectedEnchantment->minLevel();

    // 检查是否是书
    bool isBook = stack.getItem() == Items::BOOK;

    // 如果是书，转换为附魔书
    if (isBook) {
        stack = ItemStack(Items::ENCHANTED_BOOK, stack.getCount());
    }

    // 应用附魔
    if (isBook) {
        // 附魔书使用 StoredEnchantments
        item::items::EnchantedBookItem::addEnchantment(stack, *selectedEnchantment, level);
    } else {
        // 普通物品直接添加附魔
        stack.addEnchantment(selectedEnchantment->id(), level);
    }

    return stack;
}

std::unique_ptr<LootFunction> EnchantRandomlyFunction::clone() const noexcept
{
    auto func = std::make_unique<EnchantRandomlyFunction>(m_enchantments, m_treasure);
    for (const auto& cond : m_conditions) {
        func->addCondition(cond->clone());
    }
    return func;
}

} // namespace loot
} // namespace mc
