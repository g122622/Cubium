#include "ProtectionEnchantment.hpp"
#include "../../../../entity/damage/DamageSource.hpp"

namespace mc {
namespace item {
namespace enchant {

ProtectionEnchantment::ProtectionEnchantment(Type protectionType)
    : m_protectionType(protectionType)
{}

i32 ProtectionEnchantment::getMinCost(i32 level) const
{
    // 参考 MC 1.16.5 ProtectionEnchantment.Type 枚举值
    // ALL("all", 1, 11)       - minEnchantability=1,  levelCost=11
    // FIRE("fire", 10, 8)     - minEnchantability=10, levelCost=8
    // FALL("fall", 5, 6)      - minEnchantability=5,  levelCost=6
    // EXPLOSION("explosion", 5, 8)   - minEnchantability=5,  levelCost=8
    // PROJECTILE("projectile", 3, 6) - minEnchantability=3,  levelCost=6
    switch (m_protectionType) {
        case Type::All:
            return 1 + (level - 1) * 11;
        case Type::Fire:
            return 10 + (level - 1) * 8;
        case Type::Fall:
            return 5 + (level - 1) * 6;
        case Type::Explosion:
            return 5 + (level - 1) * 8;
        case Type::Projectile:
            return 3 + (level - 1) * 6;
        default:
            return 1 + (level - 1) * 11;
    }
}

i32 ProtectionEnchantment::getMaxCost(i32 level) const
{
    // 参考 MC 1.16.5 ProtectionEnchantment.Type 枚举值
    // getMaxEnchantability = getMinEnchantability + levelCost
    switch (m_protectionType) {
        case Type::All:
            return getMinCost(level) + 11;
        case Type::Fire:
            return getMinCost(level) + 8;
        case Type::Fall:
            return getMinCost(level) + 6;
        case Type::Explosion:
            return getMinCost(level) + 8;
        case Type::Projectile:
            return getMinCost(level) + 6;
        default:
            return getMinCost(level) + 11;
    }
}

i32 ProtectionEnchantment::getDamageProtection(i32 level, u32 damageType) const
{
    // MC 1.16.5 calcModifierDamage 逻辑:
    // - 如果伤害源可以无视创造模式保护 (canHarmInCreative/bypassesInvulnerability)，返回 0
    // - 全保护对所有伤害有效，每级 EPF = level
    // - 火焰保护只对火焰伤害有效，每级 EPF = level * 2
    // - 摔落保护只对摔落伤害有效，每级 EPF = level * 3
    // - 爆炸保护只对爆炸伤害有效，每级 EPF = level * 2
    // - 弹射物保护只对弹射物伤害有效，每级 EPF = level * 2
    //
    // 注意：火焰保护、爆炸保护、弹射物保护对非匹配伤害类型返回 0，
    // 不提供基础保护。只有全保护对所有伤害有效。
    //
    // damageType 参数是一个位掩码，由 LivingEntity::applyPotionDamageCalculations 构建

    switch (m_protectionType) {
        case Type::All:
            // 全保护对所有伤害类型有效，每级 EPF = level
            return level;

        case Type::Fire:
            // 火焰保护只对火焰伤害有效，每级 EPF = level * 2
            if (damageType & DamageFlags::FIRE) {
                return level * 2;
            }
            return 0; // 对其他伤害无效

        case Type::Fall:
            // 摔落保护只对摔落伤害有效，每级 EPF = level * 3
            if (damageType & DamageFlags::FALL) {
                return level * 3;
            }
            return 0;

        case Type::Explosion:
            // 爆炸保护只对爆炸伤害有效，每级 EPF = level * 2
            if (damageType & DamageFlags::EXPLOSION) {
                return level * 2;
            }
            return 0; // 对其他伤害无效

        case Type::Projectile:
            // 弹射物保护只对弹射物伤害有效，每级 EPF = level * 2
            if (damageType & DamageFlags::PROJECTILE) {
                return level * 2;
            }
            return 0; // 对其他伤害无效

        default:
            return 0;
    }
}

bool ProtectionEnchantment::isCompatibleWith(const Enchantment& other) const
{
    // 保护类附魔之间互斥（不同类型的保护不能共存）
    // 但摔落保护可以与其他保护共存
    if (const auto* protection = dynamic_cast<const ProtectionEnchantment*>(&other)) {
        if (m_protectionType != Type::Fall && protection->m_protectionType != Type::Fall) {
            // 两个都不是摔落保护，互斥
            return false;
        }
        // 如果有一个是摔落保护，检查另一个是否也是摔落保护
        if (m_protectionType == Type::Fall && protection->m_protectionType == Type::Fall) {
            return false; // 相同类型的保护也互斥
        }
        return true; // 摔落保护可以与其他保护共存
    }
    return Enchantment::isCompatibleWith(other);
}

} // namespace enchant
} // namespace item
} // namespace mc
