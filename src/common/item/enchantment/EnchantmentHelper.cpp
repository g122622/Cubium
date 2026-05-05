#include "EnchantmentHelper.hpp"
#include "EnchantmentRegistry.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/util/math/random/Random.hpp"
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

void EnchantmentHelper::setEnchantments(const std::vector<std::pair<const Enchantment*, i32>>& enchantments, ItemStack& stack) {
    if (stack.isEmpty()) {
        return;
    }

    // 清除现有附魔
    stack.getEnchantmentsMutable().clear();

    // 添加新附魔
    for (const auto& [enchantment, level] : enchantments) {
        if (enchantment != nullptr && level > 0) {
            stack.addEnchantment(enchantment->id(), level);
        }
    }
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

i32 EnchantmentHelper::getKnockbackLevel(const ItemStack& stack) {
    return getEnchantmentLevel(stack, "minecraft:knockback");
}

i32 EnchantmentHelper::getFireAspectLevel(const ItemStack& stack) {
    return getEnchantmentLevel(stack, "minecraft:fire_aspect");
}

i32 EnchantmentHelper::getLootingLevel(const ItemStack& stack) {
    return getEnchantmentLevel(stack, "minecraft:looting");
}

i32 EnchantmentHelper::getEfficiencyLevel(const ItemStack& stack) {
    return getEnchantmentLevel(stack, "minecraft:efficiency");
}

i32 EnchantmentHelper::getRespirationLevel(const ItemStack& stack) {
    return getEnchantmentLevel(stack, "minecraft:respiration");
}

i32 EnchantmentHelper::getDepthStriderLevel(const ItemStack& stack) {
    return getEnchantmentLevel(stack, "minecraft:depth_strider");
}

bool EnchantmentHelper::hasAquaAffinity(const ItemStack& stack) {
    return hasEnchantment(stack, "minecraft:aqua_affinity");
}

bool EnchantmentHelper::hasFrostWalker(const ItemStack& stack) {
    return hasEnchantment(stack, "minecraft:frost_walker");
}

bool EnchantmentHelper::hasSoulSpeed(const ItemStack& stack) {
    return hasEnchantment(stack, "minecraft:soul_speed");
}

bool EnchantmentHelper::hasBindingCurse(const ItemStack& stack) {
    return hasEnchantment(stack, "minecraft:binding_curse");
}

bool EnchantmentHelper::hasVanishingCurse(const ItemStack& stack) {
    return hasEnchantment(stack, "minecraft:vanishing_curse");
}

i32 EnchantmentHelper::getLoyaltyLevel(const ItemStack& stack) {
    return getEnchantmentLevel(stack, "minecraft:loyalty");
}

i32 EnchantmentHelper::getRiptideLevel(const ItemStack& stack) {
    return getEnchantmentLevel(stack, "minecraft:riptide");
}

bool EnchantmentHelper::hasChanneling(const ItemStack& stack) {
    return hasEnchantment(stack, "minecraft:channeling");
}

f32 EnchantmentHelper::getSweepingDamageRatio(const ItemStack& stack) {
    i32 level = getEnchantmentLevel(stack, "minecraft:sweeping");
    if (level <= 0) {
        return 0.0f;
    }
    // MC 1.16.5: I=1.0-1.0/(1+level)=0.5, II=0.667, III=0.75
    return 1.0f - 1.0f / static_cast<f32>(1 + level);
}

i32 EnchantmentHelper::getFishingLuckBonus(const ItemStack& stack) {
    return getEnchantmentLevel(stack, "minecraft:luck_of_the_sea");
}

i32 EnchantmentHelper::getFishingSpeedBonus(const ItemStack& stack) {
    return getEnchantmentLevel(stack, "minecraft:lure");
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

// ========== 耐久计算 ==========

bool EnchantmentHelper::shouldIgnoreDurabilityLoss(i32 level, bool isArmor, math::Random& random) {
    if (level <= 0) {
        return false;
    }

    // MC 1.16.5: 护甲有 60% 概率不触发耐久效果
    if (isArmor && random.nextFloat() < 0.6f) {
        return false;
    }

    // level/(level+1) 概率忽略损耗
    // I: 50%, II: 66.7%, III: 75%
    return random.nextInt(level + 1) > 0;
}

// ========== 附魔回调分发 ==========

void EnchantmentHelper::applyArthropodEnchantmentDamage(
    LivingEntity& user,
    Entity& target,
    const ItemStack& weapon) {

    if (weapon.isEmpty()) {
        return;
    }

    // 获取武器上的所有附魔
    auto enchantments = getEnchantments(weapon);
    for (const auto& [enchantment, level] : enchantments) {
        if (enchantment && level > 0) {
            // 调用附魔的 onEntityDamaged 回调
            enchantment->onEntityDamaged(user, target, level);
        }
    }
}

void EnchantmentHelper::applyThornsEnchantments(
    LivingEntity& user,
    Entity& attacker,
    const std::array<const ItemStack*, 4>& armorSlots) {

    // 遍历所有护甲槽位
    for (const ItemStack* slot : armorSlots) {
        if (slot == nullptr || slot->isEmpty()) {
            continue;
        }

        // 检查是否有荆棘附魔
        i32 thornsLevel = getEnchantmentLevel(*slot, "minecraft:thorns");
        if (thornsLevel <= 0) {
            continue;
        }

        // 获取荆棘附魔实例
        const Enchantment* thornsEnchant = EnchantmentRegistry::get("minecraft:thorns");
        if (thornsEnchant) {
            // 调用荆棘附魔的 onUserHurt 回调
            thornsEnchant->onUserHurt(user, attacker, thornsLevel);
        }
    }
}

} // namespace enchant
} // namespace item
} // namespace mc
