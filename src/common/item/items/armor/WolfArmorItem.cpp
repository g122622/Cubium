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

#include "WolfArmorItem.hpp"

#include "common/entity/attribute/AttributeModifierUUIDs.hpp"
#include "common/entity/attribute/Attributes.hpp"
#include "common/item/attribute/ItemAttributeModifiers.hpp"

namespace mc {
namespace item::items {

namespace {

/**
 * @brief 获取盔甲槽位对应的装备槽位
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
        default:
            return entity::attribute::uuids::ARMOR_MODIFIER_UUID_HEAD;
    }
}

} // namespace

WolfArmorItem::WolfArmorItem(const armor::ArmorMaterial& material, armor::ArmorSlot slot, ItemProperties properties)
    : DyeableArmorItem(material, slot, std::move(properties))
{
    // ArmorItem 基类构造函数中 _buildAttributeModifiers() 使用 ArmorItem::getDefense()
    // 返回材质 Chest 槽位的防御值（6），但狼铠的 Body 槽位防御值应为 11。
    // 此处重建属性修饰符以使用正确的防御值。
    _rebuildAttributeModifiers();
}

void WolfArmorItem::_rebuildAttributeModifiers()
{
    // 清空基类构建的修饰符
    m_attributeModifiers = ItemAttributeModifiers();

    i32 equipmentSlot = armorSlotToEquipmentSlot(m_slot);
    std::string uuid = entity::attribute::uuids::fromString(getArmorModifierUUID(m_slot));

    // 1. 护甲值修饰符 (generic.armor) - 使用狼铠的 Body 槽位防御值 11
    auto armorModifier = entity::attribute::AttributeModifier(
        uuid, "Armor modifier", static_cast<f64>(getDefense()), entity::attribute::Operation::Addition);
    m_attributeModifiers.add(entity::attribute::Attributes::ARMOR, armorModifier, equipmentSlot);

    // 2. 护甲韧性修饰符 (generic.armor_toughness)
    auto toughnessModifier = entity::attribute::AttributeModifier(
        uuid, "Armor toughness", static_cast<f64>(getToughness()), entity::attribute::Operation::Addition);
    m_attributeModifiers.add(entity::attribute::Attributes::ARMOR_TOUGHNESS, toughnessModifier, equipmentSlot);

    // 3. 击退抗性修饰符 (generic.knockback_resistance) - 狼铠无击退抗性，跳过
}

} // namespace item::items
} // namespace mc
