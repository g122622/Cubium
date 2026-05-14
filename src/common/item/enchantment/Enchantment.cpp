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

#include "Enchantment.hpp"
#include "common/item/core/ItemStack.hpp"

namespace mc {
namespace item {
namespace enchant {

// ============================================================================
// Enchantment 实现
// ============================================================================

std::string Enchantment::getNameKey(i32 level) const
{
    (void)level; // 大多数附魔不使用等级前缀
    // 返回本地化键
    return "enchantment." + id();
}

bool Enchantment::canApplyTo(u32 itemType) const
{
    (void)itemType;
    // 默认实现：根据类型判断
    // 子类可以覆盖此方法
    return true;
}

bool Enchantment::canApply(const ItemStack& stack) const
{
    // 默认实现：调用 canApplyAtEnchantingTable
    // 参考: net.minecraft.enchantment.Enchantment.canApply
    return canApplyAtEnchantingTable(stack);
}

bool Enchantment::canApplyAtEnchantingTable(const ItemStack& stack) const
{
    (void)stack;
    // 默认实现：宝藏附魔不能在附魔台获得
    return !isTreasure();
}

bool Enchantment::isCompatibleWith(const Enchantment& other) const
{
    // 默认实现：同类型附魔互斥
    if (type() == other.type() && type() != EnchantmentType::All) {
        return false;
    }
    return true;
}

void Enchantment::onEntityDamaged(LivingEntity& user, Entity& target, i32 level) const
{
    // 默认实现：无操作
    // 子类可以覆盖此方法实现特定效果（如节肢杀手的缓慢效果）
    (void)user;
    (void)target;
    (void)level;
}

void Enchantment::onUserHurt(LivingEntity& user, Entity& attacker, i32 level) const
{
    // 默认实现：无操作
    // 子类可以覆盖此方法实现特定效果（如荆棘的反伤效果）
    (void)user;
    (void)attacker;
    (void)level;
}

i32 Enchantment::getMinCost(i32 level) const
{
    // 默认公式：1 + (level - 1) * 10
    // 参考 MC 1.16.5 Enchantment
    return 1 + (level - 1) * 10;
}

i32 Enchantment::getMaxCost(i32 level) const
{
    // 默认公式：getMinCost(level) + 5
    return getMinCost(level) + 5;
}

i32 Enchantment::getMinEnchantability(i32 level) const
{
    // 默认公式：getMinCost(level)
    return getMinCost(level);
}

i32 Enchantment::getMaxEnchantability(i32 level) const
{
    // 默认公式：getMinEnchantability(level) + 15
    return getMinEnchantability(level) + 15;
}

f32 Enchantment::getDamageBonus(i32 level, u32 entityType) const
{
    (void)level;
    (void)entityType;
    return 0.0f;
}

i32 Enchantment::getDamageProtection(i32 level, u32 damageType) const
{
    (void)level;
    (void)damageType;
    return 0;
}

i32 Enchantment::getRarityWeight(EnchantmentRarity rarity)
{
    switch (rarity) {
        case EnchantmentRarity::Common:
            return 10;
        case EnchantmentRarity::Uncommon:
            return 5;
        case EnchantmentRarity::Rare:
            return 2;
        case EnchantmentRarity::VeryRare:
            return 1;
        default:
            return 10;
    }
}

bool Enchantment::isTypeCompatibleWith(const Enchantment& other) const
{
    EnchantmentType thisType = type();
    EnchantmentType otherType = other.type();

    // 所有物品类型与任何类型都兼容
    if (thisType == EnchantmentType::All || otherType == EnchantmentType::All) {
        return true;
    }

    // 检查类型冲突
    // 参考 MC 1.16.5 EnchantmentType
    auto isArmor = [](EnchantmentType t) {
        return t == EnchantmentType::Armor || t == EnchantmentType::ArmorHead || t == EnchantmentType::ArmorChest ||
            t == EnchantmentType::ArmorFeet;
    };

    auto isWearable = [isArmor](EnchantmentType t) { return isArmor(t) || t == EnchantmentType::Wearable; };

    // 护甲附魔之间兼容
    if (isArmor(thisType) && isArmor(otherType)) {
        return true;
    }

    // 可穿戴附魔兼容护甲
    if ((thisType == EnchantmentType::Wearable && isArmor(otherType)) ||
        (otherType == EnchantmentType::Wearable && isArmor(thisType))) {
        return true;
    }

    // 同类型互斥
    if (thisType == otherType) {
        return false;
    }

    return true;
}

} // namespace enchant
} // namespace item
} // namespace mc
