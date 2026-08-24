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

#include "EnchantmentHelper.hpp"
#include "EnchantmentRegistry.hpp"
#include "common/core/Types.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/core/EquipmentSlot.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/item/Items.hpp"
#include "common/item/core/Item.hpp"
#include "common/item/enchantment/Enchantment.hpp"
#include "common/item/items/special/EnchantedBookItem.hpp"
#include "common/util/math/random/Random.hpp"
#include <algorithm>
#include <array>
#include <string>
#include <utility>
#include <vector>

namespace mc {
namespace item {
namespace enchant {

// ============================================================================
// EnchantmentHelper 实现
// ============================================================================

i32 EnchantmentHelper::getEnchantmentLevel(const ItemStack& stack, const std::string& enchantmentId)
{
    if (stack.isEmpty()) {
        return 0;
    }
    return stack.getEnchantmentLevel(enchantmentId);
}

i32 EnchantmentHelper::getEnchantmentLevel(const ItemStack& stack, const Enchantment* enchantment)
{
    if (!enchantment || stack.isEmpty()) {
        return 0;
    }
    return getEnchantmentLevel(stack, enchantment->id());
}

bool EnchantmentHelper::hasEnchantment(const ItemStack& stack, const std::string& enchantmentId)
{
    return getEnchantmentLevel(stack, enchantmentId) > 0;
}

bool EnchantmentHelper::hasEnchantmentType(const ItemStack& stack, EnchantmentType type)
{
    if (stack.isEmpty()) {
        return false;
    }
    return stack.getEnchantments().hasType(type);
}

bool EnchantmentHelper::hasEnchantments(const ItemStack& stack)
{
    if (stack.isEmpty()) {
        return false;
    }
    return stack.hasEnchantments();
}

std::vector<std::pair<const Enchantment*, i32>> EnchantmentHelper::getEnchantments(const ItemStack& stack)
{
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

void EnchantmentHelper::setEnchantments(
    const std::vector<std::pair<const Enchantment*, i32>>& enchantments, ItemStack& stack)
{
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

bool EnchantmentHelper::hasSilkTouch(const ItemStack& stack)
{
    return hasEnchantment(stack, "minecraft:silk_touch");
}

i32 EnchantmentHelper::getFortuneLevel(const ItemStack& stack)
{
    return getEnchantmentLevel(stack, "minecraft:fortune");
}

i32 EnchantmentHelper::getSharpnessLevel(const ItemStack& stack)
{
    return getEnchantmentLevel(stack, "minecraft:sharpness");
}

i32 EnchantmentHelper::getUnbreakingLevel(const ItemStack& stack)
{
    return getEnchantmentLevel(stack, "minecraft:unbreaking");
}

i32 EnchantmentHelper::getKnockbackLevel(const ItemStack& stack)
{
    return getEnchantmentLevel(stack, "minecraft:knockback");
}

i32 EnchantmentHelper::getFireAspectLevel(const ItemStack& stack)
{
    return getEnchantmentLevel(stack, "minecraft:fire_aspect");
}

i32 EnchantmentHelper::getLootingLevel(const ItemStack& stack)
{
    return getEnchantmentLevel(stack, "minecraft:looting");
}

i32 EnchantmentHelper::getEfficiencyLevel(const ItemStack& stack)
{
    return getEnchantmentLevel(stack, "minecraft:efficiency");
}

i32 EnchantmentHelper::getRespirationLevel(const ItemStack& stack)
{
    return getEnchantmentLevel(stack, "minecraft:respiration");
}

i32 EnchantmentHelper::getDepthStriderLevel(const ItemStack& stack)
{
    return getEnchantmentLevel(stack, "minecraft:depth_strider");
}

bool EnchantmentHelper::hasAquaAffinity(const ItemStack& stack)
{
    return hasEnchantment(stack, "minecraft:aqua_affinity");
}

bool EnchantmentHelper::hasFrostWalker(const ItemStack& stack)
{
    return hasEnchantment(stack, "minecraft:frost_walker");
}

bool EnchantmentHelper::hasSoulSpeed(const ItemStack& stack)
{
    return hasEnchantment(stack, "minecraft:soul_speed");
}

bool EnchantmentHelper::hasBindingCurse(const ItemStack& stack)
{
    return hasEnchantment(stack, "minecraft:binding_curse");
}

bool EnchantmentHelper::hasVanishingCurse(const ItemStack& stack)
{
    return hasEnchantment(stack, "minecraft:vanishing_curse");
}

i32 EnchantmentHelper::getDensityLevel(const ItemStack& stack)
{
    return getEnchantmentLevel(stack, "minecraft:density");
}

i32 EnchantmentHelper::getBreachLevel(const ItemStack& stack)
{
    return getEnchantmentLevel(stack, "minecraft:breach");
}

i32 EnchantmentHelper::getWindBurstLevel(const ItemStack& stack)
{
    return getEnchantmentLevel(stack, "minecraft:wind_burst");
}

i32 EnchantmentHelper::getLoyaltyLevel(const ItemStack& stack)
{
    return getEnchantmentLevel(stack, "minecraft:loyalty");
}

i32 EnchantmentHelper::getRiptideLevel(const ItemStack& stack)
{
    return getEnchantmentLevel(stack, "minecraft:riptide");
}

bool EnchantmentHelper::hasChanneling(const ItemStack& stack)
{
    return hasEnchantment(stack, "minecraft:channeling");
}

f32 EnchantmentHelper::getSweepingDamageRatio(const ItemStack& stack)
{
    i32 level = getEnchantmentLevel(stack, "minecraft:sweeping");
    if (level <= 0) {
        return 0.0f;
    }
    // MC 1.16.5: I=1.0-1.0/(1+level)=0.5, II=0.667, III=0.75
    return 1.0f - 1.0f / static_cast<f32>(1 + level);
}

i32 EnchantmentHelper::getFishingLuckBonus(const ItemStack& stack)
{
    return getEnchantmentLevel(stack, "minecraft:luck_of_the_sea");
}

i32 EnchantmentHelper::getFishingSpeedBonus(const ItemStack& stack)
{
    return getEnchantmentLevel(stack, "minecraft:lure");
}

// ========== 附魔计算 ==========

i32 EnchantmentHelper::getTotalProtection(const ItemStack& stack, u32 damageType)
{
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

f32 EnchantmentHelper::getTotalDamageBonus(const ItemStack& stack, const LivingEntity* target)
{
    if (stack.isEmpty()) {
        return 0.0f;
    }

    f32 total = 0.0f;
    auto enchantments = getEnchantments(stack);
    for (const auto& [enchantment, level] : enchantments) {
        if (enchantment) {
            total += enchantment->getDamageBonus(level, target);
        }
    }

    return total;
}

// ========== 护甲附魔保护计算 ==========

i32 EnchantmentHelper::getTotalArmorProtection(const std::array<const ItemStack*, 4>& armorSlots, u32 damageType)
{

    i32 totalEPF = 0;

    for (const ItemStack* slot : armorSlots) {
        if (slot && !slot->isEmpty()) {
            totalEPF += getProtectionFactor(*slot, damageType);
        }
    }

    // MC 1.16.5: EPF 上限为 20，对应 80% 减伤
    return std::min(totalEPF, 20);
}

i32 EnchantmentHelper::getProtectionFactor(const ItemStack& stack, u32 damageType)
{
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

bool EnchantmentHelper::shouldIgnoreDurabilityLoss(i32 level, bool isArmor, math::Random& random)
{
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

void EnchantmentHelper::applyArthropodEnchantmentDamage(LivingEntity& user, Entity& target, const ItemStack& weapon)
{

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

void EnchantmentHelper::applyThornsEnchantments(LivingEntity& user, Entity& attacker)
{
    // 对齐 vanilla 1.21.11 THORNS（Enchantments.java:337-346）的 POST_ATTACK(VICTIM→ATTACKER) 派发：
    // 遍历受害者护甲，每件带荆棘的护甲按概率（perLevel 0.15）触发反伤 + 耐久消耗。
    // 顺序 [Head, Chest, Legs, Feet] 与 getArmorSlots() 一致（vanilla EnchantmentHelper 遍历
    // ArmorSlot 枚举顺序）。耐久消耗需写装备槽原件，故用 getMutableEquipment 取可变引用。
    static constexpr std::array<EquipmentSlot, 4> armorSlotsOrder = {
        EquipmentSlot::Head,  // 头盔
        EquipmentSlot::Chest, // 胸甲
        EquipmentSlot::Legs,  // 护腿
        EquipmentSlot::Feet   // 靴子
    };

    for (EquipmentSlot slot : armorSlotsOrder) {
        ItemStack& armor = user.getMutableEquipment(slot);
        if (armor.isEmpty()) {
            continue;
        }

        // 检查是否有荆棘附魔
        i32 thornsLevel = getEnchantmentLevel(armor, "minecraft:thorns");
        if (thornsLevel <= 0) {
            continue;
        }

        // 获取荆棘附魔实例并调用 onUserHurt 回调（反伤 + 耐久消耗均在 onUserHurt 内处理）
        const Enchantment* thornsEnchant = EnchantmentRegistry::get("minecraft:thorns");
        if (thornsEnchant) {
            thornsEnchant->onUserHurt(user, attacker, armor, slot, thornsLevel);
        }
    }
}

void EnchantmentHelper::applyArthropodEnchantments(LivingEntity& user, Entity& target)
{
    // MC 1.16.5 EnchantmentHelper.applyArthropodEnchantments()
    // 遍历攻击者所有装备的附魔

    // 1. 遍历护甲槽位
    std::array<const ItemStack*, 4> armorSlots = user.getArmorSlots();
    for (const ItemStack* slot : armorSlots) {
        if (slot == nullptr || slot->isEmpty()) {
            continue;
        }

        auto enchantments = getEnchantments(*slot);
        for (const auto& [enchantment, level] : enchantments) {
            if (enchantment && level > 0) {
                enchantment->onEntityDamaged(user, target, level);
            }
        }
    }

    // 2. 遍历主手物品
    const ItemStack& mainHand = user.getMainHandItem();
    if (!mainHand.isEmpty()) {
        auto enchantments = getEnchantments(mainHand);
        for (const auto& [enchantment, level] : enchantments) {
            if (enchantment && level > 0) {
                enchantment->onEntityDamaged(user, target, level);
            }
        }
    }
}

// ========== 位置依赖附魔效果 ==========

void EnchantmentHelper::runLocationChangedEffects(LivingEntity& entity)
{
    // 遍历所有装备槽位
    for (i32 slotIndex = 0; slotIndex < static_cast<i32>(EquipmentSlot::Count); ++slotIndex) {
        auto slot = static_cast<EquipmentSlot>(slotIndex);
        const ItemStack& stack = entity.getEquipment(slot);
        if (stack.isEmpty()) {
            continue;
        }
        runLocationChangedEffects(entity, stack, slot);
    }
}

void EnchantmentHelper::runLocationChangedEffects(LivingEntity& entity, const ItemStack& stack, EquipmentSlot slot)
{
    if (stack.isEmpty()) {
        return;
    }

    auto enchantments = getEnchantments(stack);
    i32 slotIndex = static_cast<i32>(slot);

    for (const auto& [enchantment, level] : enchantments) {
        if (enchantment == nullptr || level <= 0) {
            continue;
        }

        bool wasActive = entity.locationEnchantmentTracker().isActive(slotIndex, enchantment->id());
        bool shouldActivate = enchantment->onLocationChanged(entity, stack, slotIndex, level, wasActive);

        if (shouldActivate && !wasActive) {
            // 附魔从非活跃变为活跃
            entity.locationEnchantmentTracker().setActive(slotIndex, enchantment->id());
        } else if (!shouldActivate && wasActive) {
            // 附魔从活跃变为非活跃，需要停用效果
            entity.locationEnchantmentTracker().setInactive(slotIndex, enchantment->id());
            enchantment->onLocationEffectDeactivated(entity, stack, slotIndex, level);
        }
    }
}

void EnchantmentHelper::stopLocationBasedEffects(LivingEntity& entity, const ItemStack& stack, EquipmentSlot slot)
{
    if (stack.isEmpty()) {
        return;
    }

    i32 slotIndex = static_cast<i32>(slot);
    auto activeEnchantments = entity.locationEnchantmentTracker().clearSlot(slotIndex);

    auto enchantments = getEnchantments(stack);
    for (const auto& [enchantment, level] : enchantments) {
        if (enchantment == nullptr || level <= 0) {
            continue;
        }
        if (activeEnchantments.count(enchantment->id()) > 0) {
            enchantment->onLocationEffectDeactivated(entity, stack, slotIndex, level);
        }
    }
}

void EnchantmentHelper::stopAllLocationBasedEffects(LivingEntity& entity)
{
    for (i32 slotIndex = 0; slotIndex < static_cast<i32>(EquipmentSlot::Count); ++slotIndex) {
        auto slot = static_cast<EquipmentSlot>(slotIndex);
        const ItemStack& stack = entity.getEquipment(slot);
        if (stack.isEmpty()) {
            continue;
        }

        auto activeEnchantments = entity.locationEnchantmentTracker().clearSlot(slotIndex);
        auto enchantments = getEnchantments(stack);
        for (const auto& [enchantment, level] : enchantments) {
            if (enchantment == nullptr || level <= 0) {
                continue;
            }
            if (activeEnchantments.count(enchantment->id()) > 0) {
                enchantment->onLocationEffectDeactivated(entity, stack, slotIndex, level);
            }
        }
    }
}

void EnchantmentHelper::applyEnchantmentAttributeModifiers(
    LivingEntity& entity, const ItemStack& stack, EquipmentSlot slot)
{
    if (stack.isEmpty()) {
        return;
    }

    const i32 slotIndex = static_cast<i32>(slot);
    auto enchantments = getEnchantments(stack);
    for (const auto& [enchantment, level] : enchantments) {
        if (enchantment == nullptr || level <= 0) {
            continue;
        }
        // 对齐 vanilla EnchantmentEffectComponents.ATTRIBUTES：取附魔该等级的属性修饰符，
        // 仅应用槽位匹配的条目。同 id 先移除后添加，保证等级变化时更新而非叠加
        // （对齐 LivingEntity 装备同步管线的 removeModifier+addModifier 范式）。
        item::ItemAttributeModifiers modifiers = enchantment->getAttributeModifiers(level);
        for (const auto& entry : modifiers.getEntries()) {
            if (entry.equipmentSlot == slotIndex) {
                entity.attributes().removeModifier(entry.attributeName, entry.modifier.id());
                entity.attributes().addModifier(entry.attributeName, entry.modifier);
            }
        }
    }
}

void EnchantmentHelper::removeEnchantmentAttributeModifiers(
    LivingEntity& entity, const ItemStack& stack, EquipmentSlot slot)
{
    if (stack.isEmpty()) {
        return;
    }

    const i32 slotIndex = static_cast<i32>(slot);
    auto enchantments = getEnchantments(stack);
    for (const auto& [enchantment, level] : enchantments) {
        if (enchantment == nullptr || level <= 0) {
            continue;
        }
        // 移除时按当前物品堆的附魔等级计算修饰符 id（与应用时一致）。
        item::ItemAttributeModifiers modifiers = enchantment->getAttributeModifiers(level);
        for (const auto& entry : modifiers.getEntries()) {
            if (entry.equipmentSlot == slotIndex) {
                entity.attributes().removeModifier(entry.attributeName, entry.modifier.id());
            }
        }
    }
}

// ========== 附魔生成（附魔台用） ==========

i32 EnchantmentHelper::calcItemStackEnchantability(
    math::Random& random, i32 slotIndex, i32 power, const ItemStack& stack)
{

    if (stack.isEmpty()) {
        return 0;
    }

    const Item* item = stack.getItem();
    if (item == nullptr) {
        return 0;
    }

    // 获取物品的可附魔度
    i32 enchantability = item->getItemEnchantability();
    if (enchantability <= 0) {
        return 0;
    }

    // 书架力量上限为15
    power = std::min(power, 15);

    // MC 1.16.5 公式：
    // j = rand(1-8) + 1 + floor(power/2) + rand(0-power)
    i32 j = random.nextInt(8) + 1 + (power >> 1) + random.nextInt(power + 1);

    // 根据槽位调整
    switch (slotIndex) {
        case 0:
            return std::max(j / 3, 1); // 槽位0：1/3
        case 1:
            return j * 2 / 3 + 1; // 槽位1：2/3 + 1
        case 2:
            return std::max(j, power * 2); // 槽位2：最大值
        default:
            return 0;
    }
}

std::vector<EnchantmentHelper::EnchantmentData> EnchantmentHelper::getEnchantmentDatas(
    i32 level, const ItemStack& stack, bool allowTreasure)
{

    std::vector<EnchantmentData> result;

    if (stack.isEmpty()) {
        return result;
    }

    const Item* item = stack.getItem();
    if (item == nullptr) {
        return result;
    }

    bool isBook = item == Items::BOOK;

    // 遍历所有注册的附魔
    for (const auto& [id, enchantment] : EnchantmentRegistry::all()) {
        if (enchantment == nullptr) {
            continue;
        }

        // 检查是否允许宝藏附魔
        if (enchantment->isTreasure() && !allowTreasure) {
            continue;
        }

        // 检查是否可以生成
        if (!enchantment->canGenerateInLoot()) {
            continue;
        }

        // 检查是否可以应用到物品
        if (!enchantment->canApplyAtEnchantingTable(stack)) {
            // 书可以有额外的附魔
            if (!isBook || !enchantment->isAllowedOnBooks()) {
                continue;
            }
        }

        // 找到最高可用的等级
        for (i32 lvl = enchantment->maxLevel(); lvl >= enchantment->minLevel(); --lvl) {
            i32 minCost = enchantment->getMinEnchantability(lvl);
            i32 maxCost = enchantment->getMaxEnchantability(lvl);

            if (level >= minCost && level <= maxCost) {
                result.emplace_back(enchantment.get(), lvl);
                break; // 只添加最高有效等级
            }
        }
    }

    return result;
}

std::vector<EnchantmentHelper::EnchantmentData> EnchantmentHelper::buildEnchantmentList(
    math::Random& random, const ItemStack& stack, i32 level, bool allowTreasure)
{

    std::vector<EnchantmentData> result;

    if (stack.isEmpty()) {
        return result;
    }

    const Item* item = stack.getItem();
    if (item == nullptr) {
        return result;
    }

    i32 enchantability = item->getItemEnchantability();
    if (enchantability <= 0) {
        return result;
    }

    // MC 1.16.5: 等级调整
    // level = level + 1 + rand(enchantability/4+1) + rand(enchantability/4+1)
    level = level + 1 + random.nextInt(enchantability / 4 + 1) + random.nextInt(enchantability / 4 + 1);

    // 随机波动 -15% 到 +15%
    float f = (random.nextFloat() + random.nextFloat() - 1.0f) * 0.15f;
    level = std::max(1, static_cast<i32>(level + level * f));

    // 获取可用附魔列表
    std::vector<EnchantmentData> available = getEnchantmentDatas(level, stack, allowTreasure);

    if (available.empty()) {
        return result;
    }

    // 第一个附魔：加权随机选择
    EnchantmentData firstEnchant = getRandomEnchantment(random, available);
    if (firstEnchant.enchantment == nullptr) {
        return result;
    }
    result.push_back(firstEnchant);

    // 后续附魔：概率 = level/50
    while (random.nextInt(50) < level) {
        // 移除与已选附魔不兼容的
        removeIncompatible(available, result.back().enchantment);

        if (available.empty()) {
            break;
        }

        EnchantmentData nextEnchant = getRandomEnchantment(random, available);
        if (nextEnchant.enchantment != nullptr) {
            result.push_back(nextEnchant);
        }

        // 等级减半用于后续附魔
        level /= 2;
    }

    return result;
}

void EnchantmentHelper::removeIncompatible(std::vector<EnchantmentData>& list, const Enchantment* enchantment)
{
    if (enchantment == nullptr) {
        return;
    }

    list.erase(std::remove_if(list.begin(),
                   list.end(),
                   [enchantment](const EnchantmentData& data) {
                       return data.enchantment != nullptr && !enchantment->isCompatibleWith(*data.enchantment);
                   }),
        list.end());
}

EnchantmentHelper::EnchantmentData EnchantmentHelper::getRandomEnchantment(
    math::Random& random, std::vector<EnchantmentData>& list)
{

    if (list.empty()) {
        return EnchantmentData(nullptr, 0);
    }

    // 计算总权重
    i32 totalWeight = 0;
    for (const auto& data : list) {
        totalWeight += data.weight;
    }

    if (totalWeight <= 0) {
        return EnchantmentData(nullptr, 0);
    }

    // 加权随机选择
    i32 randomValue = random.nextInt(totalWeight);
    i32 currentWeight = 0;

    for (auto& data : list) {
        currentWeight += data.weight;
        if (randomValue < currentWeight) {
            return data;
        }
    }

    // 不应该到达这里，但返回最后一个作为后备
    return list.back();
}

ItemStack EnchantmentHelper::addRandomEnchantment(math::Random& random, ItemStack stack, i32 level, bool allowTreasure)
{
    if (stack.isEmpty()) {
        return stack;
    }

    // MC 1.16.5: EnchantmentHelper.addRandomEnchantment
    // 生成附魔列表
    std::vector<EnchantmentData> enchantments = buildEnchantmentList(random, stack, level, allowTreasure);

    if (enchantments.empty()) {
        return stack;
    }

    // 检查是否是书
    const Item* item = stack.getItem();
    bool isBook = (item == Items::BOOK);

    // 如果是书，转换为附魔书
    if (isBook) {
        stack = ItemStack(Items::ENCHANTED_BOOK, stack.getCount());
    }

    // 应用所有附魔
    for (const auto& data : enchantments) {
        if (data.enchantment == nullptr || data.level <= 0) {
            continue;
        }

        if (isBook) {
            // 附魔书使用 StoredEnchantments
            item::items::EnchantedBookItem::addEnchantment(stack, *data.enchantment, data.level);
        } else {
            // 普通物品直接添加附魔
            stack.addEnchantment(data.enchantment->id(), data.level);
        }
    }

    return stack;
}

} // namespace enchant
} // namespace item
} // namespace mc
