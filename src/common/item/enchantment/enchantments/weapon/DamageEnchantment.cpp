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

#include "DamageEnchantment.hpp"
#include "../mace/BreachEnchantment.hpp"
#include "../mace/DensityEnchantment.hpp"
#include "../mace/WindBurstEnchantment.hpp"
#include "../trident/ImpalingEnchantment.hpp"

namespace mc {
namespace item {
namespace enchant {

DamageEnchantment::DamageEnchantment(Type damageType) noexcept
    : m_damageType(damageType)
{}

i32 DamageEnchantment::getMinCost(i32 level) const
{
    // 锋石：1 + (level - 1) * 11
    // 亡灵杀手/节肢杀手：5 + (level - 1) * 8
    if (m_damageType == Type::All) {
        return 1 + (level - 1) * 11;
    } else {
        return 5 + (level - 1) * 8;
    }
}

i32 DamageEnchantment::getMaxCost(i32 level) const
{
    return getMinCost(level) + 20;
}

f32 DamageEnchantment::getDamageBonus(i32 level, u32 entityType) const noexcept
{
    // 实体类型常量（与 EntityTypeKeys 对应）
    constexpr u32 EntityTypeUndead = 1;    // 亡灵：僵尸、骷髅、凋灵等
    constexpr u32 EntityTypeArthropod = 2; // 节肢：蜘蛛、蠹虫、末影螨等

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

bool DamageEnchantment::isCompatibleWith(const Enchantment& other) const
{
    // 伤害类附魔之间互斥
    if (dynamic_cast<const DamageEnchantment*>(&other) != nullptr) {
        return false;
    }
    // 伤害类附魔与重锤致密互斥
    if (dynamic_cast<const DensityEnchantment*>(&other) != nullptr) {
        return false;
    }
    // 伤害类附魔与重锤破甲互斥
    if (dynamic_cast<const BreachEnchantment*>(&other) != nullptr) {
        return false;
    }
    // 伤害类附魔与穿刺互斥（DAMAGE_EXCLUSIVE 组）
    if (dynamic_cast<const ImpalingEnchantment*>(&other) != nullptr) {
        return false;
    }
    // 伤害类附魔与风爆兼容（风爆不属于 DAMAGE_EXCLUSIVE 组）
    if (dynamic_cast<const WindBurstEnchantment*>(&other) != nullptr) {
        return true;
    }
    return Enchantment::isCompatibleWith(other);
}

} // namespace enchant
} // namespace item
} // namespace mc
