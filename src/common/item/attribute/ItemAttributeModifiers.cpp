#include "ItemAttributeModifiers.hpp"
#include <algorithm>

namespace mc {
namespace item {

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

u64 ItemAttributeModifiers::generateModifierUUID(u32 itemId, const String& attributeId) {
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
    // 使用预定义的属性名称创建修饰符
    // TODO: 需要属性注册表来获取属性指针
    (void)amount;
    (void)equipmentSlot;
    return *this;
}

ItemAttributeModifiersBuilder& ItemAttributeModifiersBuilder::attackSpeed(f64 amount, i32 equipmentSlot) {
    (void)amount;
    (void)equipmentSlot;
    return *this;
}

ItemAttributeModifiersBuilder& ItemAttributeModifiersBuilder::armor(f64 amount, i32 equipmentSlot) {
    (void)amount;
    (void)equipmentSlot;
    return *this;
}

ItemAttributeModifiersBuilder& ItemAttributeModifiersBuilder::armorToughness(f64 amount, i32 equipmentSlot) {
    (void)amount;
    (void)equipmentSlot;
    return *this;
}

ItemAttributeModifiersBuilder& ItemAttributeModifiersBuilder::knockbackResistance(f64 amount, i32 equipmentSlot) {
    (void)amount;
    (void)equipmentSlot;
    return *this;
}

ItemAttributeModifiersBuilder& ItemAttributeModifiersBuilder::movementSpeed(f64 amount, i32 equipmentSlot) {
    (void)amount;
    (void)equipmentSlot;
    return *this;
}

} // namespace item
} // namespace mc
