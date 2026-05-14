#include "item/crafting/special/RepairItemRecipe.hpp"
#include "item/enchantment/EnchantmentContainer.hpp"
#include "item/enchantment/EnchantmentHelper.hpp"
#include <algorithm>

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
    // MC 原版公式: newDamage = maxDamage - min((remaining1 + remaining2 + maxDamage * 5%), maxDamage)
    i32 maxDamage = stack1.getMaxDamage();
    i32 remaining1 = maxDamage - stack1.getDamage();
    i32 remaining2 = maxDamage - stack2.getDamage();
    i32 bonus = maxDamage * 5 / 100; // 5% 额外修复
    i32 totalRemaining = remaining1 + remaining2 + bonus;
    i32 newDamage = std::max(0, maxDamage - totalRemaining);

    result.setDamage(newDamage);

    // 合并诅咒附魔（取最高等级）
    // 注意：MC 原版只合并诅咒附魔，普通附魔不合并
    using EnchantEntry = std::pair<const item::enchant::Enchantment*, i32>;
    std::vector<EnchantEntry> combinedEnchants;

    auto enchants1 = item::enchant::EnchantmentHelper::getEnchantments(stack1);
    auto enchants2 = item::enchant::EnchantmentHelper::getEnchantments(stack2);

    // 收集所有诅咒附魔
    for (const auto& [enchant, level] : enchants1) {
        if (enchant != nullptr && enchant->isCurse()) {
            // 检查是否已在列表中
            auto it = std::find_if(combinedEnchants.begin(), combinedEnchants.end(), [enchant](const EnchantEntry& e) {
                return e.first == enchant;
            });

            if (it != combinedEnchants.end()) {
                it->second = std::max(it->second, level);
            } else {
                combinedEnchants.emplace_back(enchant, level);
            }
        }
    }

    for (const auto& [enchant, level] : enchants2) {
        if (enchant != nullptr && enchant->isCurse()) {
            auto it = std::find_if(combinedEnchants.begin(), combinedEnchants.end(), [enchant](const EnchantEntry& e) {
                return e.first == enchant;
            });

            if (it != combinedEnchants.end()) {
                it->second = std::max(it->second, level);
            } else {
                combinedEnchants.emplace_back(enchant, level);
            }
        }
    }

    // 应用诅咒附魔
    for (const auto& [enchant, level] : combinedEnchants) {
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
