#include "EnchantmentHelper.hpp"
#include "EnchantmentRegistry.hpp"
#include <algorithm>

namespace mc {
namespace item {
namespace enchant {

// ============================================================================
// EnchantmentHelper 实现
// ============================================================================

i32 EnchantmentHelper::getEnchantmentLevel(const ItemStack& stack, const String& enchantmentId) {
    if (stack.isEmpty()) {
        return 0;
    }
    return stack.getEnchantmentLevel(enchantmentId);
}

i32 EnchantmentHelper::getEnchantmentLevel(const ItemStack& stack, const Enchantment* enchantment) {
    if (!enchantment || stack.isEmpty()) {
        return 0;
    }
    return getEnchantmentLevel(stack, enchantment->id());
}

bool EnchantmentHelper::hasEnchantment(const ItemStack& stack, const String& enchantmentId) {
    return getEnchantmentLevel(stack, enchantmentId) > 0;
}

bool EnchantmentHelper::hasEnchantmentType(const ItemStack& stack, EnchantmentType type) {
    if (stack.isEmpty()) {
        return false;
    }
    return stack.getEnchantments().hasType(type);
}

bool EnchantmentHelper::hasEnchantments(const ItemStack& stack) {
    if (stack.isEmpty()) {
        return false;
    }
    return stack.hasEnchantments();
}

std::vector<std::pair<const Enchantment*, i32>> EnchantmentHelper::getEnchantments(const ItemStack& stack) {
    std::vector<std::pair<const Enchantment*, i32>> result;

    if (stack.isEmpty()) {
        return result;
    }

    const auto& instances = stack.getEnchantments().getAll();
    result.reserve(instances.size());

    for (const auto& instance : instances) {
        const Enchantment* enchantment = EnchantmentRegistry::get(instance.enchantmentId);
        if (enchantment) {
            result.emplace_back(enchantment, instance.level);
        }
    }

    return result;
}

// ========== 特定附魔便捷方法 ==========

bool EnchantmentHelper::hasSilkTouch(const ItemStack& stack) {
    return hasEnchantment(stack, "minecraft:silk_touch");
}

i32 EnchantmentHelper::getFortuneLevel(const ItemStack& stack) {
    return getEnchantmentLevel(stack, "minecraft:fortune");
}

i32 EnchantmentHelper::getSharpnessLevel(const ItemStack& stack) {
    return getEnchantmentLevel(stack, "minecraft:sharpness");
}

i32 EnchantmentHelper::getUnbreakingLevel(const ItemStack& stack) {
    return getEnchantmentLevel(stack, "minecraft:unbreaking");
}

// ========== 附魔计算 ==========

i32 EnchantmentHelper::getTotalProtection(const ItemStack& stack, u32 damageType) {
    if (stack.isEmpty()) {
        return 0;
    }

    i32 total = 0;
    auto enchantments = getEnchantments(stack);
    for (const auto& [enchantment, level] : enchantments) {
        if (enchantment) {
            total += enchantment->getDamageProtection(level, damageType);
        }
    }

    return total;
}

f32 EnchantmentHelper::getTotalDamageBonus(const ItemStack& stack, u32 entityType) {
    if (stack.isEmpty()) {
        return 0.0f;
    }

    f32 total = 0.0f;
    auto enchantments = getEnchantments(stack);
    for (const auto& [enchantment, level] : enchantments) {
        if (enchantment) {
            total += enchantment->getDamageBonus(level, entityType);
        }
    }

    return total;
}

// ========== 护甲附魔保护计算 ==========

i32 EnchantmentHelper::getTotalArmorProtection(
    const std::array<const ItemStack*, 4>& armorSlots,
    u32 damageType) {

    i32 totalEPF = 0;

    for (const ItemStack* slot : armorSlots) {
        if (slot && !slot->isEmpty()) {
            totalEPF += getProtectionFactor(*slot, damageType);
        }
    }

    // MC 1.16.5: EPF 上限为 20，对应 80% 减伤
    return std::min(totalEPF, 20);
}

i32 EnchantmentHelper::getProtectionFactor(const ItemStack& stack, u32 damageType) {
    if (stack.isEmpty()) {
        return 0;
    }

    i32 total = 0;
    auto enchantments = getEnchantments(stack);
    for (const auto& [enchantment, level] : enchantments) {
        if (enchantment) {
            total += enchantment->getDamageProtection(level, damageType);
        }
    }

    return total;
}

} // namespace enchant
} // namespace item
} // namespace mc
