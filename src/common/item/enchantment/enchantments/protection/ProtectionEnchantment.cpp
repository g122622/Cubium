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

#include "ProtectionEnchantment.hpp"
#include "common/core/Types.hpp"
#include "common/entity/damage/DamageSource.hpp"
#include "common/item/enchantment/Enchantment.hpp"

namespace mc {
namespace item {
namespace enchant {

ProtectionEnchantment::ProtectionEnchantment(Type protectionType)
    : m_protectionType(protectionType)
{}

i32 ProtectionEnchantment::getMinCost(i32 level) const
{
    // 各保护类型的附魔花费系数
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
    // 最大附魔花费 = 最小花费 + 级差系数
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

i32 ProtectionEnchantment::getDamageProtection(i32 level, u32 damageType) const noexcept
{
    // 计算保护附魔对特定伤害类型的EPF（Enchantment Protection Factor）值
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
