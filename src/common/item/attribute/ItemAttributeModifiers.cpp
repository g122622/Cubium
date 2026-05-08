#include "ItemAttributeModifiers.hpp"
#include "../../entity/attribute/Attributes.hpp"
#include "../../entity/attribute/AttributeModifierUUIDs.hpp"
#include <algorithm>
#include <sstream>

namespace mc {
namespace item {

namespace {
    // 将 u64 转换为 std::string UUID
    std::string uuidToString(u64 uuid) {
        std::stringstream ss;
        ss << std::hex << uuid;
        return ss.str();
    }
}

void ItemAttributeModifiers::add(const entity::attribute::Attribute* attribute,
                                   const entity::attribute::AttributeModifier& modifier,
                                   i32 equipmentSlot) {
    m_entries.emplace_back(attribute, modifier, equipmentSlot);
}

std::vector<ItemAttributeModifiers::Entry> ItemAttributeModifiers::getModifiersForSlot(i32 equipmentSlot) const {
    std::vector<Entry> result;
    for (const auto& entry : m_entries) {
        if (entry.equipmentSlot == equipmentSlot) {
            result.push_back(entry);
        }
    }
    return result;
}

u64 ItemAttributeModifiers::generateModifierUUID(u32 itemId, const std::string& attributeId) {
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

ItemAttributeModifiersBuilder& ItemAttributeModifiersBuilder::attackDamage(f64 amount, i32 equipmentSlot) {
    // 创建攻击伤害修饰符（加法操作）
    // 使用 MC 1.16.5 标准UUID (Item.java:45)
    auto attr = entity::attribute::Attributes::attackDamage();
    auto modifier = entity::attribute::AttributeModifier(
        entity::attribute::uuids::fromString(entity::attribute::uuids::ATTACK_DAMAGE_MODIFIER_UUID),
        "Attack damage modifier",
        amount,
        entity::attribute::Operation::Addition
    );
    m_modifiers.add(attr.get(), modifier, equipmentSlot);
    return *this;
}

ItemAttributeModifiersBuilder& ItemAttributeModifiersBuilder::attackSpeed(f64 amount, i32 equipmentSlot) {
    // 创建攻击速度修饰符（加法操作）
    // 使用 MC 1.16.5 标准UUID (Item.java:46)
    auto attr = entity::attribute::Attributes::attackSpeed();
    auto modifier = entity::attribute::AttributeModifier(
        entity::attribute::uuids::fromString(entity::attribute::uuids::ATTACK_SPEED_MODIFIER_UUID),
        "Attack speed modifier",
        amount,
        entity::attribute::Operation::Addition
    );
    m_modifiers.add(attr.get(), modifier, equipmentSlot);
    return *this;
}

ItemAttributeModifiersBuilder& ItemAttributeModifiersBuilder::armor(f64 amount, i32 equipmentSlot) {
    auto attr = entity::attribute::Attributes::armor();
    u64 uuid = ItemAttributeModifiers::generateModifierUUID(0, entity::attribute::Attributes::ARMOR);
    auto modifier = entity::attribute::AttributeModifier(
        uuidToString(uuid),
        "Armor modifier",
        amount,
        entity::attribute::Operation::Addition
    );
    m_modifiers.add(attr.get(), modifier, equipmentSlot);
    return *this;
}

ItemAttributeModifiersBuilder& ItemAttributeModifiersBuilder::armorToughness(f64 amount, i32 equipmentSlot) {
    auto attr = entity::attribute::Attributes::armorToughness();
    u64 uuid = ItemAttributeModifiers::generateModifierUUID(0, entity::attribute::Attributes::ARMOR_TOUGHNESS);
    auto modifier = entity::attribute::AttributeModifier(
        uuidToString(uuid),
        "Armor toughness modifier",
        amount,
        entity::attribute::Operation::Addition
    );
    m_modifiers.add(attr.get(), modifier, equipmentSlot);
    return *this;
}

ItemAttributeModifiersBuilder& ItemAttributeModifiersBuilder::knockbackResistance(f64 amount, i32 equipmentSlot) {
    auto attr = entity::attribute::Attributes::knockbackResistance();
    u64 uuid = ItemAttributeModifiers::generateModifierUUID(0, entity::attribute::Attributes::KNOCKBACK_RESISTANCE);
    auto modifier = entity::attribute::AttributeModifier(
        uuidToString(uuid),
        "Knockback resistance modifier",
        amount,
        entity::attribute::Operation::Addition
    );
    m_modifiers.add(attr.get(), modifier, equipmentSlot);
    return *this;
}

ItemAttributeModifiersBuilder& ItemAttributeModifiersBuilder::movementSpeed(f64 amount, i32 equipmentSlot) {
    auto attr = entity::attribute::Attributes::movementSpeed();
    u64 uuid = ItemAttributeModifiers::generateModifierUUID(0, entity::attribute::Attributes::MOVEMENT_SPEED);
    auto modifier = entity::attribute::AttributeModifier(
        uuidToString(uuid),
        "Movement speed modifier",
        amount,
        entity::attribute::Operation::MultiplyTotal
    );
    m_modifiers.add(attr.get(), modifier, equipmentSlot);
    return *this;
}

} // namespace item
} // namespace mc
