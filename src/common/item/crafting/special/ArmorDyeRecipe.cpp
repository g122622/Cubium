#include "item/crafting/special/ArmorDyeRecipe.hpp"
#include "item/Items.hpp"
#include "item/items/armor/DyeableArmorItem.hpp"
#include "entity/entities/passive/basic/SheepEntity.hpp"
#include <algorithm>
#include <unordered_set>

namespace mc {
namespace crafting {

// 染料物品集合（MC 1.16.5 共16种染料 + 墨囊 + 可可豆）
static const std::unordered_set<const Item*>& getDyeItems() {
    static std::unordered_set<const Item*> dyeItems = {
        Items::INK_SAC,           // 墨囊（黑色染料）
        Items::RED_DYE,           // 红色染料
        Items::GREEN_DYE,         // 绿色染料
        Items::COCOA_BEANS,       // 可可豆（棕色染料）
        Items::LAPIS_LAZULI_DYE,  // 青金石（蓝色染料）
        Items::PURPLE_DYE,        // 紫色染料
        Items::CYAN_DYE,          // 青色染料
        Items::LIGHT_GRAY_DYE,    // 淡灰色染料
        Items::GRAY_DYE,          // 灰色染料
        Items::PINK_DYE,          // 粉红色染料
        Items::LIME_DYE,          // 黄绿色染料
        Items::YELLOW_DYE,        // 黄色染料
        Items::LIGHT_BLUE_DYE,    // 淡蓝色染料
        Items::MAGENTA_DYE,       // 品红色染料
        Items::ORANGE_DYE,        // 橙色染料
        Items::WHITE_DYE,         // 白色染料
    };
    return dyeItems;
}

ArmorDyeRecipe::ArmorDyeRecipe(const ResourceLocation& id)
    : SpecialRecipe(id) {
}

bool ArmorDyeRecipe::matches(const CraftingInventory& inventory) const {
    int armorCount = 0;
    int dyeCount = 0;

    for (i32 i = 0; i < inventory.getContainerSize(); ++i) {
        ItemStack stack = inventory.getItem(i);
        if (stack.isEmpty()) {
            continue;
        }

        if (isDyeableArmor(stack)) {
            ++armorCount;
        } else if (isDye(stack)) {
            ++dyeCount;
        } else {
            // 有其他物品，不匹配
            return false;
        }
    }

    // 必须恰好有一个可染色盔甲和至少一个染料
    return armorCount == 1 && dyeCount >= 1;
}

ItemStack ArmorDyeRecipe::assemble(const CraftingInventory& inventory) const {
    ItemStack armorStack;
    std::vector<u32> colors;

    // 收集盔甲和染料颜色
    for (i32 i = 0; i < inventory.getContainerSize(); ++i) {
        ItemStack stack = inventory.getItem(i);
        if (stack.isEmpty()) {
            continue;
        }

        if (isDyeableArmor(stack)) {
            armorStack = stack.copy();
        } else if (isDye(stack)) {
            // 从物品获取染料颜色
            // TODO: 需要实现从染料物品获取颜色的逻辑
            // 暂时使用默认颜色
            colors.push_back(0xFFFFFF);  // 白色
        }
    }

    if (armorStack.isEmpty() || colors.empty()) {
        return ItemStack::EMPTY;
    }

    // 获取当前颜色（如果有）
    u32 currentColor = 0xFFFFFF;  // 默认白色
    const Item* item = armorStack.getItem();
    if (item != nullptr) {
        const auto* dyeableItem = dynamic_cast<const item::items::DyeableArmorItem*>(item);
        if (dyeableItem != nullptr) {
            if (item::items::DyeableArmorItem::hasColor(armorStack)) {
                currentColor = dyeableItem->getColor(armorStack);
            } else {
                currentColor = dyeableItem->getDefaultColor();
            }
        }
    }

    // 混合颜色
    for (u32 dyeColor : colors) {
        currentColor = mixColors(currentColor, dyeColor);
    }

    // 设置颜色
    item::items::DyeableArmorItem::setColor(armorStack, currentColor);

    return armorStack;
}

std::vector<ItemStack> ArmorDyeRecipe::getRemainingItems(const CraftingInventory& inventory) const {
    // 染色配方消耗所有染料，不消耗盔甲（盔甲变成染色结果）
    std::vector<ItemStack> remaining(inventory.getContainerSize());
    // 所有染料都被消耗
    return remaining;
}

bool ArmorDyeRecipe::isDyeableArmor(const ItemStack& stack) {
    if (stack.isEmpty()) {
        return false;
    }
    const Item* item = stack.getItem();
    if (item == nullptr) {
        return false;
    }
    return dynamic_cast<const item::items::DyeableArmorItem*>(item) != nullptr;
}

bool ArmorDyeRecipe::isDye(const ItemStack& stack) {
    if (stack.isEmpty()) {
        return false;
    }
    const Item* item = stack.getItem();
    if (item == nullptr) {
        return false;
    }
    // MC 1.16.5: 检查物品是否为染料
    return getDyeItems().count(item) > 0;
}

u32 ArmorDyeRecipe::mixColors(u32 color1, u32 color2) {
    // MC 原版的颜色混合算法：将 RGB 分量分别取平均
    i32 r1 = static_cast<i32>((color1 >> 16) & 0xFF);
    i32 g1 = static_cast<i32>((color1 >> 8) & 0xFF);
    i32 b1 = static_cast<i32>(color1 & 0xFF);

    i32 r2 = static_cast<i32>((color2 >> 16) & 0xFF);
    i32 g2 = static_cast<i32>((color2 >> 8) & 0xFF);
    i32 b2 = static_cast<i32>(color2 & 0xFF);

    i32 r = (r1 + r2) / 2;
    i32 g = (g1 + g2) / 2;
    i32 b = (b1 + b2) / 2;

    return (0xFF << 24) | (static_cast<u32>(r) << 16) | (static_cast<u32>(g) << 8) | static_cast<u32>(b);
}

} // namespace crafting
} // namespace mc
