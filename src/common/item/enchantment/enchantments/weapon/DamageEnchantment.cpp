#include "DamageEnchantment.hpp"

namespace mc {
namespace item {
namespace enchant {

DamageEnchantment::DamageEnchantment(Type damageType)
    : m_damageType(damageType) {
}

i32 DamageEnchantment::getMinCost(i32 level) const {
    // 参考 MC 1.16.5 伤害附魔成本
    // 锋利: 1 + (level - 1) * 11
    // 亡灵杀手/节肢杀手: 5 + (level - 1) * 8
    if (m_damageType == Type::All) {
        return 1 + (level - 1) * 11;
    } else {
        return 5 + (level - 1) * 8;
    }
}

i32 DamageEnchantment::getMaxCost(i32 level) const {
    return getMinCost(level) + 20;
}

f32 DamageEnchantment::getDamageBonus(i32 level, u32 entityType) const {
    // 实体类型常量（与 EntityTypes 对应）
    constexpr u32 EntityTypeUndead = 1;      // 亡灵：僵尸、骷髅、凋灵等
    constexpr u32 EntityTypeArthropod = 2;   // 节肢：蜘蛛、蠹虫、末影螨等

    switch (m_damageType) {
        case Type::All:
            // 锋利：对所有生物造成额外伤害
            // 每级增加 0.5 * level + 0.5
            return static_cast<f32>(level) * 0.5f + 0.5f;

        case Type::Undead:
            // 亡灵杀手：对亡灵生物额外伤害
            if (entityType == EntityTypeUndead) {
                return static_cast<f32>(level) * 2.5f;
            }
            return 0.0f;

        case Type::Arthropods:
            // 节肢杀手：对节肢生物额外伤害
            if (entityType == EntityTypeArthropod) {
                return static_cast<f32>(level) * 2.5f;
            }
            return 0.0f;

        default:
            return 0.0f;
    }
}

bool DamageEnchantment::isCompatibleWith(const Enchantment& other) const {
    // 伤害类附魔之间互斥
    if (dynamic_cast<const DamageEnchantment*>(&other) != nullptr) {
        return false;
    }
    return Enchantment::isCompatibleWith(other);
}

} // namespace enchant
} // namespace item
} // namespace mc
