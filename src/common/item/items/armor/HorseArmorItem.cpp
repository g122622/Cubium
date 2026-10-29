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

#include "HorseArmorItem.hpp"
#include "common/core/Types.hpp"
#include "common/entity/attribute/AttributeModifier.hpp"
#include "common/entity/attribute/AttributeModifierIds.hpp"
#include "common/entity/attribute/Attributes.hpp"
#include "common/entity/core/EquipmentSlot.hpp"
#include "common/item/core/Item.hpp"
#include "common/resource/ResourceLocation.hpp"

namespace mc {
namespace item::items {

HorseArmorItem::HorseArmorItem(const ItemProperties& properties, i32 armorValue, const ResourceLocation& texturePath)
    : Item(properties)
    , m_armorValue(armorValue)
    , m_texturePath(texturePath)
{}

item::ItemAttributeModifiers HorseArmorItem::getAttributeModifiers(i32 equipmentSlot) const
{
    if (equipmentSlot != static_cast<i32>(EquipmentSlot::Body)) {
        return Item::getAttributeModifiers(equipmentSlot);
    }
    // 对齐原版护甲材质的 `createAttributes(BODY)`：护甲值与韧性共用同一个修饰符 id，
    // 韧性为 0 也照原版登记；击退抗性只在材质给出正值时登记。
    item::ItemAttributeModifiers modifiers;
    const char* modifierId = entity::attribute::ids::ARMOR_MODIFIER_BODY;
    modifiers.add(entity::attribute::Attributes::ARMOR,
        entity::attribute::AttributeModifier(
            modifierId, "Armor modifier", static_cast<f64>(m_armorValue), entity::attribute::Operation::Addition),
        equipmentSlot);
    modifiers.add(entity::attribute::Attributes::ARMOR_TOUGHNESS,
        entity::attribute::AttributeModifier(
            modifierId, "Armor toughness", static_cast<f64>(m_toughness), entity::attribute::Operation::Addition),
        equipmentSlot);
    if (m_knockbackResistance > 0.0f) {
        modifiers.add(entity::attribute::Attributes::KNOCKBACK_RESISTANCE,
            entity::attribute::AttributeModifier(modifierId,
                "Armor knockback resistance",
                static_cast<f64>(m_knockbackResistance),
                entity::attribute::Operation::Addition),
            equipmentSlot);
    }
    return modifiers;
}

} // namespace item::items
} // namespace mc
