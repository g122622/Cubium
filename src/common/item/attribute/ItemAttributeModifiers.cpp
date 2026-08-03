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

#include "ItemAttributeModifiers.hpp"
#include "common/core/Types.hpp"
#include "common/entity/attribute/AttributeModifier.hpp"
#include "common/entity/attribute/AttributeModifierUUIDs.hpp"
#include "common/entity/attribute/Attributes.hpp"
#include <ios>
#include <sstream>
#include <string>
#include <vector>

namespace mc {
namespace item {

namespace {
// 将 u64 转换为 std::string UUID
std::string uuidToString(u64 uuid)
{
    std::stringstream ss;
    ss << std::hex << uuid;
    return ss.str();
}
} // namespace

void ItemAttributeModifiers::add(
    const std::string& attributeName, const entity::attribute::AttributeModifier& modifier, i32 equipmentSlot)
{
    m_entries.emplace_back(attributeName, modifier, equipmentSlot);
}

std::vector<ItemAttributeModifiers::Entry> ItemAttributeModifiers::getModifiersForSlot(i32 equipmentSlot) const
{
    std::vector<Entry> result;
    for (const auto& entry : m_entries) {
        if (entry.equipmentSlot == equipmentSlot) {
            result.emplace_back(entry.attributeName, entry.modifier, entry.equipmentSlot);
        }
    }
    return result;
}

u64 ItemAttributeModifiers::generateModifierUUID(u32 itemId, const std::string& attributeId)
{
    // 使用简单的哈希生成UUID
    u64 hash = static_cast<u64>(itemId) * 31;
    for (char c : attributeId) {
        hash = hash * 31 + static_cast<u64>(c);
    }
    return hash;
}

// ============================================================================
// ItemAttributeModifiersBuilder
// ============================================================================

ItemAttributeModifiersBuilder& ItemAttributeModifiersBuilder::attackDamage(f64 amount, i32 equipmentSlot)
{
    // 创建攻击伤害修饰符（加法操作）
    auto modifier = entity::attribute::AttributeModifier(
        entity::attribute::uuids::fromString(entity::attribute::uuids::ATTACK_DAMAGE_MODIFIER_UUID),
        "Attack damage modifier",
        amount,
        entity::attribute::Operation::Addition);
    m_modifiers.add(entity::attribute::Attributes::ATTACK_DAMAGE, modifier, equipmentSlot);
    return *this;
}

ItemAttributeModifiersBuilder& ItemAttributeModifiersBuilder::attackSpeed(f64 amount, i32 equipmentSlot)
{
    // 创建攻击速度修饰符（加法操作）
    auto modifier = entity::attribute::AttributeModifier(
        entity::attribute::uuids::fromString(entity::attribute::uuids::ATTACK_SPEED_MODIFIER_UUID),
        "Attack speed modifier",
        amount,
        entity::attribute::Operation::Addition);
    m_modifiers.add(entity::attribute::Attributes::ATTACK_SPEED, modifier, equipmentSlot);
    return *this;
}

ItemAttributeModifiersBuilder& ItemAttributeModifiersBuilder::armor(f64 amount, i32 equipmentSlot)
{
    u64 uuid = ItemAttributeModifiers::generateModifierUUID(0, entity::attribute::Attributes::ARMOR);
    auto modifier = entity::attribute::AttributeModifier(
        uuidToString(uuid), "Armor modifier", amount, entity::attribute::Operation::Addition);
    m_modifiers.add(entity::attribute::Attributes::ARMOR, modifier, equipmentSlot);
    return *this;
}

ItemAttributeModifiersBuilder& ItemAttributeModifiersBuilder::armorToughness(f64 amount, i32 equipmentSlot)
{
    u64 uuid = ItemAttributeModifiers::generateModifierUUID(0, entity::attribute::Attributes::ARMOR_TOUGHNESS);
    auto modifier = entity::attribute::AttributeModifier(
        uuidToString(uuid), "Armor toughness modifier", amount, entity::attribute::Operation::Addition);
    m_modifiers.add(entity::attribute::Attributes::ARMOR_TOUGHNESS, modifier, equipmentSlot);
    return *this;
}

ItemAttributeModifiersBuilder& ItemAttributeModifiersBuilder::knockbackResistance(f64 amount, i32 equipmentSlot)
{
    u64 uuid = ItemAttributeModifiers::generateModifierUUID(0, entity::attribute::Attributes::KNOCKBACK_RESISTANCE);
    auto modifier = entity::attribute::AttributeModifier(
        uuidToString(uuid), "Knockback resistance modifier", amount, entity::attribute::Operation::Addition);
    m_modifiers.add(entity::attribute::Attributes::KNOCKBACK_RESISTANCE, modifier, equipmentSlot);
    return *this;
}

ItemAttributeModifiersBuilder& ItemAttributeModifiersBuilder::movementSpeed(f64 amount, i32 equipmentSlot)
{
    u64 uuid = ItemAttributeModifiers::generateModifierUUID(0, entity::attribute::Attributes::MOVEMENT_SPEED);
    auto modifier = entity::attribute::AttributeModifier(
        uuidToString(uuid), "Movement speed modifier", amount, entity::attribute::Operation::MultiplyTotal);
    m_modifiers.add(entity::attribute::Attributes::MOVEMENT_SPEED, modifier, equipmentSlot);
    return *this;
}

} // namespace item
} // namespace mc
