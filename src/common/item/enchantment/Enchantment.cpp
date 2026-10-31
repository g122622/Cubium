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
#include "common/core/Types.hpp"
#include "common/entity/core/EquipmentSlot.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/items/armor/ArmorItem.hpp"
#include "common/item/items/tool/ShearsItem.hpp"
#include "common/item/items/tool/SwordItem.hpp"
#include "common/item/items/tool/ToolItem.hpp"
#include "common/item/items/trial/MaceItem.hpp"
#include "common/item/items/weapon/BowItem.hpp"
#include "common/item/items/weapon/CrossbowItem.hpp"
#include "common/item/items/weapon/FishingRodItem.hpp"
#include "common/item/items/weapon/SpearItem.hpp"
#include "common/item/items/weapon/TridentItem.hpp"
#include <string>

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
    return canApplyAtEnchantingTable(stack);
}

bool Enchantment::canApplyAtEnchantingTable(const ItemStack& stack) const
{
    // 宝藏附魔不能在附魔台获得
    if (isTreasure()) {
        return false;
    }
    return canApplyToItem(stack);
}

namespace {

/**
 * @brief 是否为动物铠甲（马铠 / 狼铠 / 鹦鹉螺铠）
 *
 * 它们都是 `ArmorItem` 且带耐久，但原版**不在任何 enchantable 标签里**，
 * 因此不能附保护、耐久、经验修补、消失诅咒等附魔。
 * 目前按物品 ID 后缀近似判定；接入物品标签后可替换。
 *
 * TODO: 接入 `#minecraft:enchantable/` 系列与 `#minecraft:horse_armors` 等标签后
 * 删除这段按 ID 后缀的近似判定。
 */
bool _isAnimalArmor(const Item& item)
{
    const auto& id = item.itemLocation().path();
    return (id.size() > 11 && id.substr(id.size() - 11) == "_horse_armor") || id == "wolf_armor" ||
        (id.size() > 13 && id.substr(id.size() - 13) == "_nautilus_armor");
}

} // namespace

bool Enchantment::canApplyToItem(const ItemStack& stack) const
{
    // 对应原版 1.21 `Enchantment.isSupportedItem`（`supported_items` 标签）。
    // 项目里附魔定义是硬编码的，没有 `supported_items` 字段，改用等价的
    // `EnchantmentType` 分类判定：这个枚举本身就是从原版各附魔的 supported_items 归纳出来的。
    //
    // 关键点：这层判定**必须先于**"是否在附魔池标签里"。原版
    // `getAvailableEnchantmentResults` 是 `filter(isPrimaryItem || isBook)`，先按物品过滤，
    // 再在剩下的里面找等级区间；缺了这一步，"铁镐附力量"这类不支持的组合会混进交易。
    const Item* item = stack.getItem();
    if (item == nullptr) {
        return false;
    }

    // 武器类附魔先统一判定，避免在 switch 里跨 case 定义局部变量。
    // 三种武器物品的继承链各不相同，无法靠 `ToolItem` 统一识别：
    //   - 剑 `SwordItem : public TieredItem`（不继承 ToolItem）
    //   - 重锤 `MaceItem : public Item`（不继承 ToolItem）
    //   - 矛 `SpearItem : public tool::TieredItem`（独立类）
    // 斧走正常的 `ToolItem`。此前代码假设「剑走 ToolItem」，导致锋利等附魔
    // 根本识别不到剑（由差分能力 enchantment_applicability 查出）。
    {
        const bool isSword = dynamic_cast<const item::tool::SwordItem*>(item) != nullptr;
        const bool isSpear = dynamic_cast<const item::SpearItem*>(item) != nullptr;
        const bool isMace = dynamic_cast<const item::MaceItem*>(item) != nullptr;
        const auto* weaponTool = dynamic_cast<const item::tool::ToolItem*>(item);
        const bool isAxe = weaponTool != nullptr && weaponTool->getToolType() == item::tool::ToolType::Axe;

        switch (type()) {
            // 原版 `#enchantable/weapon` = 剑 ∪ 矛 ∪ 斧 ∪ 重锤
            case EnchantmentType::Weapon:
                return isSword || isSpear || isAxe || isMace;
            // 原版 `#enchantable/sharp_weapon` = 剑 ∪ 矛 ∪ 斧（**不含重锤**）
            case EnchantmentType::SharpWeapon:
                return isSword || isSpear || isAxe;
            // 原版 `#enchantable/melee_weapon` = 剑 ∪ 矛
            case EnchantmentType::MeleeWeapon:
                return isSword || isSpear;
            // 原版 `#enchantable/fire_aspect` = 剑 ∪ 矛 ∪ 重锤（**不含斧**）
            case EnchantmentType::FireAspectWeapon:
                return isSword || isSpear || isMace;
            // 原版 `#enchantable/sweeping` = 仅剑
            case EnchantmentType::SweepingWeapon:
                return isSword;
            // 原版 `#enchantable/lunge` = 仅矛
            case EnchantmentType::LungeWeapon:
                return isSpear;
            default:
                break;
        }
    }

    switch (type()) {
        case EnchantmentType::All:
            return true;
        case EnchantmentType::Armor: {
            // 原版 `#enchantable/armor` = 头盔 ∪ 胸甲 ∪ 护腿 ∪ 靴子，**只含玩家护甲四部件**。
            // 马铠、狼铠、鹦鹉螺铠虽然也是 `ArmorItem`，但它们不在该标签里，
            // 不能附保护类附魔（由差分能力 enchantment_applicability 查出：
            // 此前 `wolf_armor` 被误判为可附 protection / mending / unbreaking）。
            const auto* armorItem = dynamic_cast<const item::items::ArmorItem*>(item);
            if (armorItem == nullptr) {
                return false;
            }
            return armorItem->isHelmet() || armorItem->isChestplate() || armorItem->isLeggings() ||
                armorItem->isBoots();
        }
        case EnchantmentType::ArmorHead:
        case EnchantmentType::ArmorChest:
        case EnchantmentType::ArmorLegs:
        case EnchantmentType::ArmorFeet: {
            // 原版按 `#enchantable/head_armor` 等物品标签细分部位。这里必须真正区分，
            // 不能统一回落到 `isArmor()`：此前四个部位分支全部 `return item->isArmor()`，
            // 导致「水下速掘」「水下呼吸」能附在靴子上、「深海漫游」「冰霜行者」能附在
            // 头盔上（由差分能力 enchantment_applicability 查出，各差 22-23 个物品）。
            const auto* armorItem = dynamic_cast<const item::items::ArmorItem*>(item);
            if (armorItem == nullptr) {
                return false;
            }
            switch (type()) {
                case EnchantmentType::ArmorHead:
                    return armorItem->isHelmet();
                case EnchantmentType::ArmorChest:
                    return armorItem->isChestplate();
                case EnchantmentType::ArmorLegs:
                    return armorItem->isLeggings();
                case EnchantmentType::ArmorFeet:
                    return armorItem->isBoots();
                default:
                    return false;
            }
        }
        case EnchantmentType::Digger: {
            // 原版 `#minecraft:enchantable/mining` = 镐 / 斧 / 铲 / 锄 / 剪刀
            //
            // 剪刀 `ShearsItem` 直接继承 `Item`，不是 `ToolItem`，必须单独判类型——
            // 否则「效率附不到剪刀上」。这是本项目里同一模式的第三例
            // （前两例是 `SwordItem` 与 `MaceItem`）。
            if (dynamic_cast<const item::tool::ShearsItem*>(item) != nullptr) {
                return true;
            }
            const auto* toolItem = dynamic_cast<const item::tool::ToolItem*>(item);
            if (toolItem == nullptr) {
                return false;
            }
            switch (toolItem->getToolType()) {
                case item::tool::ToolType::Pickaxe:
                case item::tool::ToolType::Axe:
                case item::tool::ToolType::Shovel:
                case item::tool::ToolType::Hoe:
                case item::tool::ToolType::Shears:
                    return true;
                case item::tool::ToolType::None:
                case item::tool::ToolType::Sword:
                    return false;
            }
            return false;
        }
        case EnchantmentType::DiggerLoot: {
            // 原版 `#enchantable/mining_loot` = 镐 / 斧 / 铲 / 锄，**不含剪刀**。
            // 时运与精准采集用它；效率用 `Digger`（`#enchantable/mining`，含剪刀）。
            // 此前三者共用一个类型，导致剪刀被误判为可附时运/精准采集。
            const auto* toolItem = dynamic_cast<const item::tool::ToolItem*>(item);
            if (toolItem == nullptr) {
                return false;
            }
            const auto toolType = toolItem->getToolType();
            return toolType == item::tool::ToolType::Pickaxe || toolType == item::tool::ToolType::Axe ||
                toolType == item::tool::ToolType::Shovel || toolType == item::tool::ToolType::Hoe;
        }
        case EnchantmentType::FishingRod:
            return dynamic_cast<const item::FishingRodItem*>(item) != nullptr;
        case EnchantmentType::Bow:
            return dynamic_cast<const item::BowItem*>(item) != nullptr;
        case EnchantmentType::Crossbow:
            return dynamic_cast<const item::CrossbowItem*>(item) != nullptr;
        case EnchantmentType::Trident:
            return dynamic_cast<const item::TridentItem*>(item) != nullptr;
        case EnchantmentType::Mace:
            // 原版 `#enchantable/mace` 只含重锤。破甲、密度、风爆三个附魔属于这里，
            // 此前它们被标成 `Weapon`，于是斧、剑、矛全都能附（各多判 21 个物品）。
            return dynamic_cast<const item::MaceItem*>(item) != nullptr;
        case EnchantmentType::Breakable:
            // 原版 `#enchantable/durability`：有耐久的物品。
            // 此前这里直接 `return true`，导致经验修补、耐久能附到泥土、空气、
            // 刷怪蛋等任意物品上（每种差 1124 个物品）。
            //
            // 但**动物铠甲**（马铠、狼铠、鹦鹉螺铠）虽然也有耐久，却不在该标签里
            // （由差分能力 enchantment_applicability 查出：`wolf_armor` 被误判为
            // 可附 mending / unbreaking / vanishing_curse）。它们不在任何
            // `#enchantable/*` 标签中，因此这里统一排除。
            return item->isDamageable() && !_isAnimalArmor(*item);
        case EnchantmentType::Wearable: {
            // 原版 `#enchantable/equippable` = 护甲四部件 ∪ 鞘翅 ∪ `#minecraft:skulls` ∪ 雕刻南瓜。
            // 这里覆盖护甲四部件与鞘翅；头颅与雕刻南瓜需要物品标签支持，
            // 目前只按物品 ID 前缀近似判定（比全放行或全拒绝更接近原版）。
            const auto* armorItem = dynamic_cast<const item::items::ArmorItem*>(item);
            if (armorItem != nullptr) {
                return armorItem->isHelmet() || armorItem->isChestplate() || armorItem->isLeggings() ||
                    armorItem->isBoots();
            }
            const auto itemId = item->itemLocation().path();
            // TODO: 接入 `#minecraft:skulls` 物品标签后删除这段按 ID 后缀的近似判定。
            return itemId == "elytra" || itemId == "carved_pumpkin" ||
                (itemId.size() > 5 && itemId.substr(itemId.size() - 5) == "_head") ||
                (itemId.size() > 6 && itemId.substr(itemId.size() - 6) == "_skull");
        }
        case EnchantmentType::Vanishable: {
            // 原版 `#enchantable/vanishing` = `#enchantable/durability` ∪ 指南针 ∪
            // 雕刻南瓜 ∪ `#minecraft:skulls`。
            // 动物铠甲不算 `durability`，与上面 Breakable 保持同一口径。
            if (item->isDamageable() && !_isAnimalArmor(*item)) {
                return true;
            }
            const auto itemId = item->itemLocation().path();
            // TODO: 接入 `#minecraft:skulls` 物品标签后删除这段按 ID 后缀的近似判定。
            return itemId == "compass" || itemId == "carved_pumpkin" ||
                (itemId.size() > 5 && itemId.substr(itemId.size() - 5) == "_head") ||
                (itemId.size() > 6 && itemId.substr(itemId.size() - 6) == "_skull");
        }
        // 武器类六种类型已在函数开头统一处理，这里不会走到。
        case EnchantmentType::Weapon:
        case EnchantmentType::SharpWeapon:
        case EnchantmentType::MeleeWeapon:
        case EnchantmentType::FireAspectWeapon:
        case EnchantmentType::SweepingWeapon:
        case EnchantmentType::LungeWeapon:
            return false;
    }
    return true;
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

void Enchantment::onUserHurt(
    LivingEntity& user, Entity& attacker, ItemStack& enchantedItem, EquipmentSlot slot, i32 level) const
{
    // 默认实现：无操作
    // 子类可以覆盖此方法实现特定效果（如荆棘的反伤效果）
    (void)user;
    (void)attacker;
    (void)enchantedItem;
    (void)slot;
    (void)level;
}

bool Enchantment::onLocationChanged(
    LivingEntity& entity, const ItemStack& stack, i32 slot, i32 level, bool isActive) const
{
    // 默认实现：不激活位置依赖效果
    // 子类（如 FrostWalkerEnchantment、SoulSpeedEnchantment）覆盖此方法
    (void)entity;
    (void)stack;
    (void)slot;
    (void)level;
    (void)isActive;
    return false;
}

void Enchantment::onLocationEffectDeactivated(LivingEntity& entity, const ItemStack& stack, i32 slot, i32 level) const
{
    // 默认实现：无操作
    // 子类可以覆盖此方法来清理位置依赖效果（如移除属性修饰符）
    (void)entity;
    (void)stack;
    (void)slot;
    (void)level;
}

void Enchantment::onTick(LivingEntity& entity, const ItemStack& stack, EquipmentSlot slot, i32 level) const
{
    // 默认实现：无每 tick 效果
    // 子类（如 SoulSpeedEnchantment）覆盖此方法实现持续型效果
    (void)entity;
    (void)stack;
    (void)slot;
    (void)level;
}

i32 Enchantment::getMinCost(i32 level) const
{
    // 默认公式：1 + (level - 1) * 10
    return 1 + (level - 1) * 10;
}

i32 Enchantment::getMaxCost(i32 level) const
{
    // 默认公式：getMinCost(level) + 5
    return getMinCost(level) + 5;
}

i32 Enchantment::getMinEnchantability(i32 level) const noexcept
{
    // 默认公式：getMinCost(level)
    return getMinCost(level);
}

i32 Enchantment::getMaxEnchantability(i32 level) const noexcept
{
    // 默认公式：getMinEnchantability(level) + 15
    return getMinEnchantability(level) + 15;
}

f32 Enchantment::getDamageBonus(i32 level, const LivingEntity* target) const noexcept
{
    (void)level;
    (void)target;
    return 0.0f;
}

f32 Enchantment::getDamageBonusPerLevel(i32 level) const noexcept
{
    (void)level;
    return 0.0f;
}

i32 Enchantment::getDamageProtection(i32 level, u32 damageType) const noexcept
{
    (void)level;
    (void)damageType;
    return 0;
}

item::ItemAttributeModifiers Enchantment::getAttributeModifiers(i32 level) const
{
    (void)level;
    // 默认附魔不提供属性修饰符。对齐 vanilla：只有 JSON 里写了 `minecraft:attributes` 的附魔
    // 才有这一项（水下速掘、效率、深海探索者、横扫之刃、迅捷潜行、爆炸/火焰保护、水下呼吸等）。
    return {};
}

i32 Enchantment::getRarityWeight(EnchantmentRarity rarity) noexcept
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
    auto isArmor = [](EnchantmentType t) {
        return t == EnchantmentType::Armor || t == EnchantmentType::ArmorHead || t == EnchantmentType::ArmorChest ||
            t == EnchantmentType::ArmorFeet;
    };

    auto isWearable = [isArmor](EnchantmentType t) {
        return isArmor(t) || t == EnchantmentType::Wearable;
    };

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
