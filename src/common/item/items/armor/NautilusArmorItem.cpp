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
 * LIABILITY, CONTRACT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include "NautilusArmorItem.hpp"
#include "common/entity/attribute/AttributeModifier.hpp"
#include "common/entity/attribute/AttributeModifierIds.hpp"
#include "common/entity/attribute/Attributes.hpp"
#include "common/entity/core/EquipmentSlot.hpp"
#include "common/item/armor/ArmorMaterial.hpp"
#include "common/item/core/Item.hpp"

namespace mc {
namespace item::items {

namespace {

/// 身体护甲槽位。
constexpr i32 kBodySlot = static_cast<i32>(EquipmentSlot::Body);

} // namespace

NautilusArmorItem::NautilusArmorItem(const ItemProperties& properties, const armor::ArmorMaterial& material)
    : Item(properties)
    // 护甲值由材质的 Body 槽位防御值提供（与 MC 1.21.11 Item.Properties.nautilusArmor
    // 通过 ArmorMaterial.createAttributes(ArmorType.BODY) 取护甲值的语义一致）
    , m_armorValue(material.getDefense(armor::ArmorSlot::Body))
    , m_equipSound(material.getEquipSound())
    , m_material(material)
{}

item::ItemAttributeModifiers NautilusArmorItem::getAttributeModifiers(i32 equipmentSlot) const
{
    if (equipmentSlot != kBodySlot) {
        return Item::getAttributeModifiers(equipmentSlot);
    }
    // 对齐 vanilla `ArmorMaterial.createAttributes(ArmorType.BODY)`：身体护甲同样给
    // 护甲值、韧性两条修饰符（韧性为 0 也照原版登记），三条共用同一个修饰符 id。
    item::ItemAttributeModifiers modifiers;
    const char* modifierId = entity::attribute::ids::ARMOR_MODIFIER_BODY;
    modifiers.add(entity::attribute::Attributes::ARMOR,
        entity::attribute::AttributeModifier(
            modifierId, "Armor modifier", static_cast<f64>(m_armorValue), entity::attribute::Operation::Addition),
        equipmentSlot);
    modifiers.add(entity::attribute::Attributes::ARMOR_TOUGHNESS,
        entity::attribute::AttributeModifier(modifierId,
            "Armor toughness",
            static_cast<f64>(m_material.getToughness()),
            entity::attribute::Operation::Addition),
        equipmentSlot);
    const f32 knockbackResistance = m_material.getKnockbackResistance();
    if (knockbackResistance > 0.0f) {
        modifiers.add(entity::attribute::Attributes::KNOCKBACK_RESISTANCE,
            entity::attribute::AttributeModifier(modifierId,
                "Armor knockback resistance",
                static_cast<f64>(knockbackResistance),
                entity::attribute::Operation::Addition),
            equipmentSlot);
    }
    return modifiers;
}

} // namespace item::items
} // namespace mc
