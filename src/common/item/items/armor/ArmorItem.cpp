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

#include "ArmorItem.hpp"

#include "common/core/Types.hpp"
#include "common/entity/attribute/AttributeModifier.hpp"
#include "common/entity/attribute/AttributeModifierUUIDs.hpp"
#include "common/entity/attribute/Attributes.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/inventory/PlayerInventory.hpp"
#include "common/item/armor/ArmorMaterial.hpp"
#include "common/item/attribute/ItemAttributeModifiers.hpp"
#include "common/item/core/ActionResult.hpp"
#include "common/item/core/Item.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"
#include <algorithm>
#include <string>
#include <utility>

namespace mc {
namespace item::items {

namespace {

/**
 * @brief 获取盔甲槽位对应的装备槽位
 *
 * ArmorSlot (Head/Chest/Legs/Feet/Body) 映射到 EquipmentSlot (Head/Chest/Legs/Feet/Body)
 * Body 槽位对应 EquipmentSlot::Body，用于非玩家实体护甲（狼铠、鹦鹉螺铠甲、马铠）。
 */
[[nodiscard]] i32 armorSlotToEquipmentSlot(armor::ArmorSlot slot) noexcept
{
    switch (slot) {
        case armor::ArmorSlot::Feet:
            return static_cast<i32>(EquipmentSlot::Feet);
        case armor::ArmorSlot::Legs:
            return static_cast<i32>(EquipmentSlot::Legs);
        case armor::ArmorSlot::Chest:
            return static_cast<i32>(EquipmentSlot::Chest);
        case armor::ArmorSlot::Head:
            return static_cast<i32>(EquipmentSlot::Head);
        case armor::ArmorSlot::Body:
            return static_cast<i32>(EquipmentSlot::Body);
        default:
            return static_cast<i32>(EquipmentSlot::Head);
    }
}

/**
 * @brief 获取盔甲槽位对应的UUID
 */
[[nodiscard]] const char* getArmorModifierUUID(armor::ArmorSlot slot) noexcept
{
    switch (slot) {
        case armor::ArmorSlot::Feet:
            return entity::attribute::uuids::ARMOR_MODIFIER_UUID_FEET;
        case armor::ArmorSlot::Legs:
            return entity::attribute::uuids::ARMOR_MODIFIER_UUID_LEGS;
        case armor::ArmorSlot::Chest:
            return entity::attribute::uuids::ARMOR_MODIFIER_UUID_CHEST;
        case armor::ArmorSlot::Head:
            return entity::attribute::uuids::ARMOR_MODIFIER_UUID_HEAD;
        case armor::ArmorSlot::Body:
            return entity::attribute::uuids::ARMOR_MODIFIER_UUID_BODY;
        default:
            return entity::attribute::uuids::ARMOR_MODIFIER_UUID_HEAD;
    }
}

[[nodiscard]] const ItemStack& getArmorEquipment(const LivingEntity& entity, armor::ArmorSlot slot) noexcept
{
    switch (slot) {
        case armor::ArmorSlot::Feet:
            return entity.getEquipment(EquipmentSlot::Feet);
        case armor::ArmorSlot::Legs:
            return entity.getEquipment(EquipmentSlot::Legs);
        case armor::ArmorSlot::Chest:
            return entity.getEquipment(EquipmentSlot::Chest);
        case armor::ArmorSlot::Head:
            return entity.getEquipment(EquipmentSlot::Head);
        case armor::ArmorSlot::Body:
            return entity.getEquipment(EquipmentSlot::Body);
        default:
            return entity.getEquipment(EquipmentSlot::Head);
    }
}

} // namespace

ArmorItem::ArmorItem(const armor::ArmorMaterial& material, armor::ArmorSlot slot, ItemProperties properties)
    : Item(std::move(properties))
    , m_material(material)
    , m_slot(slot)
{
    _buildAttributeModifiers(getDefense());
}

void ArmorItem::_buildAttributeModifiers(i32 defense)
{
    // 清空已有修饰符（供子类重建时使用）
    m_attributeModifiers = ItemAttributeModifiers();

    i32 equipmentSlot = armorSlotToEquipmentSlot(m_slot);
    std::string uuid = entity::attribute::uuids::fromString(getArmorModifierUUID(m_slot));

    // 1. 护甲值修饰符 (generic.armor)
    auto armorModifier = entity::attribute::AttributeModifier(
        uuid, "Armor modifier", static_cast<f64>(defense), entity::attribute::Operation::Addition);
    m_attributeModifiers.add(entity::attribute::Attributes::ARMOR, armorModifier, equipmentSlot);

    // 2. 护甲韧性修饰符 (generic.armor_toughness)
    auto toughnessModifier = entity::attribute::AttributeModifier(
        uuid, "Armor toughness", static_cast<f64>(getToughness()), entity::attribute::Operation::Addition);
    m_attributeModifiers.add(entity::attribute::Attributes::ARMOR_TOUGHNESS, toughnessModifier, equipmentSlot);

    // 3. 击退抗性修饰符 (generic.knockback_resistance) - 仅当有击退抗性时
    f32 knockbackRes = getKnockbackResistance();
    if (knockbackRes > 0.0f) {
        auto knockbackModifier = entity::attribute::AttributeModifier(
            uuid, "Armor knockback resistance", static_cast<f64>(knockbackRes), entity::attribute::Operation::Addition);
        m_attributeModifiers.add(entity::attribute::Attributes::KNOCKBACK_RESISTANCE, knockbackModifier, equipmentSlot);
    }
}

item::ItemAttributeModifiers ArmorItem::getAttributeModifiers(i32 equipmentSlot) const
{
    // 只有当槽位匹配盔甲的槽位时才返回修饰符
    // 对应 MC 原版 ArmorItem.getAttributeModifiers(EquipmentSlot)
    if (equipmentSlot == armorSlotToEquipmentSlot(m_slot)) {
        return m_attributeModifiers;
    }
    return item::ItemAttributeModifiers();
}

f32 ArmorItem::getDestroySpeed(const ItemStack& /*stack*/, const BlockState& /*state*/) const noexcept
{
    // 盔甲不是工具，返回默认速度
    return 1.0f;
}

ItemActionResult ArmorItem::onItemRightClick(IWorld& world, Player& player, Hand hand)
{
    (void)world;

    ItemStack& heldStack = player.getHeldItem(hand);
    if (heldStack.isEmpty()) {
        return ItemActionResult::pass(heldStack);
    }

    PlayerInventory& inventory = player.inventory();
    switch (m_slot) {
        case armor::ArmorSlot::Head:
            if (!inventory.getHelmet().isEmpty()) {
                return ItemActionResult::pass(heldStack);
            }
            inventory.setHelmet(heldStack);
            break;
        case armor::ArmorSlot::Chest:
            if (!inventory.getChestplate().isEmpty()) {
                return ItemActionResult::pass(heldStack);
            }
            inventory.setChestplate(heldStack);
            break;
        case armor::ArmorSlot::Legs:
            if (!inventory.getLeggings().isEmpty()) {
                return ItemActionResult::pass(heldStack);
            }
            inventory.setLeggings(heldStack);
            break;
        case armor::ArmorSlot::Feet:
            if (!inventory.getBoots().isEmpty()) {
                return ItemActionResult::pass(heldStack);
            }
            inventory.setBoots(heldStack);
            break;
        case armor::ArmorSlot::Body:
            // Body 槽位用于非玩家实体护甲（狼铠、鹦鹉螺铠甲、马铠），
            // 玩家无法直接装备，装备逻辑由实体侧（WolfEntity/NautilusEntity/AbstractHorseEntity）处理
            return ItemActionResult::pass(heldStack);
        default:
            return ItemActionResult::pass(heldStack);
    }

    heldStack = ItemStack();
    return ItemActionResult::consume(ItemStack());
}

i32 ArmorItem::getTotalArmorValue(const LivingEntity& entity)
{
    i32 total = 0;

    for (armor::ArmorSlot slot :
        {armor::ArmorSlot::Feet, armor::ArmorSlot::Legs, armor::ArmorSlot::Chest, armor::ArmorSlot::Head}) {
        const ItemStack& stack = getArmorEquipment(entity, slot);
        const auto* armor = dynamic_cast<const ArmorItem*>(stack.getItem());
        if (armor != nullptr) {
            total += armor->getDefense();
        }
    }

    return total;
}

f32 ArmorItem::getTotalToughness(const LivingEntity& entity)
{
    f32 total = 0.0f;

    for (armor::ArmorSlot slot :
        {armor::ArmorSlot::Feet, armor::ArmorSlot::Legs, armor::ArmorSlot::Chest, armor::ArmorSlot::Head}) {
        const ItemStack& stack = getArmorEquipment(entity, slot);
        const auto* armor = dynamic_cast<const ArmorItem*>(stack.getItem());
        if (armor != nullptr) {
            total += armor->getToughness();
        }
    }

    return total;
}

f32 ArmorItem::getTotalKnockbackResistance(const LivingEntity& entity)
{
    f32 total = 0.0f;

    for (armor::ArmorSlot slot :
        {armor::ArmorSlot::Feet, armor::ArmorSlot::Legs, armor::ArmorSlot::Chest, armor::ArmorSlot::Head}) {
        const ItemStack& stack = getArmorEquipment(entity, slot);
        const auto* armor = dynamic_cast<const ArmorItem*>(stack.getItem());
        if (armor != nullptr) {
            total += armor->getKnockbackResistance();
        }
    }

    return std::min(total, 1.0f); // 上限为1.0
}

bool ArmorItem::getIsRepairable(const ItemStack& /*toRepair*/, const ItemStack& repair) const
{
    // 盔甲修复不依赖于待修复物品的状态
    return m_material.getRepairMaterial().test(repair);
}

} // namespace item::items
} // namespace mc
