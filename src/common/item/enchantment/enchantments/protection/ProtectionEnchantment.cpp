#include "ProtectionEnchantment.hpp"

namespace mc {
namespace item {
namespace enchant {

ProtectionEnchantment::ProtectionEnchantment(Type protectionType)
    : m_protectionType(protectionType) {
}

i32 ProtectionEnchantment::getMinCost(i32 level) const {
    // 参考 MC 1.16.5 保护类附魔成本
    // 全保护: 1 + (level - 1) * 11
    // 其他保护: 5 + (level - 1) * 8
    if (m_protectionType == Type::All) {
        return 1 + (level - 1) * 11;
    } else {
        return 5 + (level - 1) * 8;
    }
}

i32 ProtectionEnchantment::getMaxCost(i32 level) const {
    return getMinCost(level) + m_protectionType == Type::All ? 11 : 8;
}

i32 ProtectionEnchantment::getDamageProtection(i32 level, u32 damageType) const {
    // 伤害类型常量
    constexpr u32 DamageTypeInFire = 1;
    constexpr u32 DamageTypeLava = 2;
    constexpr u32 DamageTypeOnFire = 3;
    constexpr u32 DamageTypeFall = 4;
    constexpr u32 DamageTypeExplosion = 5;
    constexpr u32 DamageTypeProjectile = 6;
    constexpr u32 DamageTypeMagic = 7;
    constexpr u32 DamageTypeDrown = 8;
    constexpr u32 DamageTypeThorns = 9;

    // 保护效果计算
    switch (m_protectionType) {
        case Type::All:
            // 全保护对所有伤害类型有效，每级减少 4% (EPF = level)
            return level;

        case Type::Fire:
            // 火焰保护对火焰伤害额外有效
            if (damageType == DamageTypeInFire || damageType == DamageTypeLava ||
                damageType == DamageTypeOnFire) {
                return level * 2;  // 双倍效果
            }
            return level;  // 对其他伤害也有基础保护

        case Type::Fall:
            // 摔落保护只对摔落伤害有效
            if (damageType == DamageTypeFall) {
                return level * 3;  // 三倍效果
            }
            return 0;

        case Type::Explosion:
            // 爆炸保护对爆炸伤害额外有效
            if (damageType == DamageTypeExplosion) {
                return level * 2;
            }
            return level;

        case Type::Projectile:
            // 弹射物保护对弹射物伤害额外有效
            if (damageType == DamageTypeProjectile) {
                return level * 2;
            }
            return level;

        default:
            return 0;
    }
}

bool ProtectionEnchantment::isCompatibleWith(const Enchantment& other) const {
    // 保护类附魔之间互斥（不同类型的保护不能共存）
    // 但摔落保护可以与其他保护共存
    if (const auto* protection = dynamic_cast<const ProtectionEnchantment*>(&other)) {
        if (m_protectionType != Type::Fall && protection->m_protectionType != Type::Fall) {
            // 两个都不是摔落保护，互斥
            return false;
        }
        // 如果有一个是摔落保护，检查另一个是否也是摔落保护
        if (m_protectionType == Type::Fall && protection->m_protectionType == Type::Fall) {
            return false;  // 相同类型的保护也互斥
        }
        return true;  // 摔落保护可以与其他保护共存
    }
    return Enchantment::isCompatibleWith(other);
}

} // namespace enchant
} // namespace item
} // namespace mc
