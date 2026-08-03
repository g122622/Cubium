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

#include "item/crafting/special/RepairItemRecipe.hpp"
#include "common/core/Types.hpp"
#include "common/entity/inventory/CraftingInventory.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/crafting/SpecialRecipe.hpp"
#include "common/item/enchantment/Enchantment.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "item/enchantment/EnchantmentHelper.hpp"
#include <algorithm>
#include <utility>
#include <vector>

namespace mc {
namespace crafting {

RepairItemRecipe::RepairItemRecipe(const ResourceLocation& id)
    : SpecialRecipe(id)
{}

bool RepairItemRecipe::matches(const CraftingInventory& inventory) const
{
    // 收集所有非空的可修复物品
    std::vector<ItemStack> repairableItems;

    for (i32 i = 0; i < inventory.getContainerSize(); ++i) {
        ItemStack stack = inventory.getItem(i);
        if (!stack.isEmpty()) {
            // 检查物品是否可修复
            if (!stack.isDamageable() || stack.getCount() != 1) {
                return false; // 有不可修复的物品或堆叠数量不为1
            }
            repairableItems.push_back(stack);
        }
    }

    // 必须恰好有两个相同类型的可修复物品
    if (repairableItems.size() != 2) {
        return false;
    }

    // 检查两个物品类型是否相同
    return repairableItems[0].getItem() == repairableItems[1].getItem();
}

ItemStack RepairItemRecipe::assemble(const CraftingInventory& inventory) const
{
    // 收集所有非空的可修复物品
    std::vector<ItemStack> repairableItems;

    for (i32 i = 0; i < inventory.getContainerSize(); ++i) {
        ItemStack stack = inventory.getItem(i);
        if (!stack.isEmpty() && stack.isDamageable() && stack.getCount() == 1) {
            repairableItems.push_back(stack);
        }
    }

    if (repairableItems.size() != 2) {
        return ItemStack::EMPTY;
    }

    const ItemStack& stack1 = repairableItems[0];
    const ItemStack& stack2 = repairableItems[1];

    // 检查类型是否相同
    if (stack1.getItem() != stack2.getItem()) {
        return ItemStack::EMPTY;
    }

    // 创建修复后的物品
    ItemStack result(stack1.getItem(), 1);

    // 计算修复后的耐久度
    // 工作台修复公式: 结果耐久度 = min(剩余耐久1 + 剩久2 + 最大耐久 * 5%, 最大耐久)
    // 注意：铁砧修复使用12%奖励，与工作台不同
    i32 maxDamage = stack1.getMaxDamage();
    i32 remaining1 = maxDamage - stack1.getDamage();
    i32 remaining2 = maxDamage - stack2.getDamage();
    i32 bonus = maxDamage * 5 / 100; // 5% 额外修复
    i32 totalRemaining = remaining1 + remaining2 + bonus;
    i32 newDamage = std::max(0, maxDamage - totalRemaining);

    result.setDamage(newDamage);

    // 工作台修复：只保留诅咒附魔（绑定诅咒、消失诅咒）
    // 普通附魔会丢失！这与铁砧修复不同
    using EnchantEntry = std::pair<const item::enchant::Enchantment*, i32>;
    std::vector<EnchantEntry> combinedCurses;

    auto enchants1 = item::enchant::EnchantmentHelper::getEnchantments(stack1);
    auto enchants2 = item::enchant::EnchantmentHelper::getEnchantments(stack2);

    // 收集所有诅咒附魔
    for (const auto& [enchant, level] : enchants1) {
        if (enchant != nullptr && enchant->isCurse()) {
            // 检查是否已在列表中
            auto it = std::find_if(combinedCurses.begin(), combinedCurses.end(), [enchant](const EnchantEntry& e) {
                return e.first == enchant;
            });

            if (it != combinedCurses.end()) {
                it->second = std::max(it->second, level);
            } else {
                combinedCurses.emplace_back(enchant, level);
            }
        }
    }

    for (const auto& [enchant, level] : enchants2) {
        if (enchant != nullptr && enchant->isCurse()) {
            auto it = std::find_if(combinedCurses.begin(), combinedCurses.end(), [enchant](const EnchantEntry& e) {
                return e.first == enchant;
            });

            if (it != combinedCurses.end()) {
                it->second = std::max(it->second, level);
            } else {
                combinedCurses.emplace_back(enchant, level);
            }
        }
    }

    // 应用诅咒附魔
    for (const auto& [enchant, level] : combinedCurses) {
        if (enchant != nullptr && level > 0) {
            result.addEnchantment(enchant->id(), level);
        }
    }

    return result;
}

std::vector<ItemStack> RepairItemRecipe::getRemainingItems(const CraftingInventory& inventory) const
{
    // 修复配方消耗所有输入物品，没有剩余物品
    return std::vector<ItemStack>(inventory.getContainerSize());
}

} // namespace crafting
} // namespace mc
