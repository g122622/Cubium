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
#include "common/core/Types.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/item/enchantment/Enchantment.hpp"

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

f32 DamageEnchantment::getDamageBonus(i32 level, const LivingEntity* target) const noexcept
{
    // 目标生物属性：从 target 取 getCreatureAttribute()（虚函数，子类按亡灵/节肢/水生覆写）。
    // target 为 nullptr 时无目标，返回 0（无伤害加成）。
    //
    // TODO: 对齐 MC Java 1.21.11，亡灵杀手/节肢杀手应改用 EntityTypeTags::SENSITIVE_TO_SMITE /
    //   SENSITIVE_TO_BANE_OF_ARTHROPODS 标签判定目标（同穿刺 ImpalingEnchantment 已改用
    //   SENSITIVE_TO_IMPALING 标签），而非 getCreatureAttribute 枚举。当前保留枚举判定因：
    //   (1) DamageEnchantment（锋利/亡灵杀手/节肢杀手）是 Weapon 类型附魔，与 Trident 类型互斥，
    //       三叉戟不会携带这些附魔，故 EnchantmentHelper::getTotalDamageBonus（当前唯一调用方，
    //       仅 TridentEntity 调）遍历三叉戟物品附魔时不会命中 DamageEnchantment，本方法事实未被调用；
    //   (2) 近战攻击链路（MobEntity::attackEntityAsMob / Player::attack）尚未接入
    //       getTotalDamageBonus，锋利/亡灵杀手/节肢杀手在近战中本就不生效（独立偏差，待修复）。
    //   待近战附魔伤害接入时，一并迁移到标签判定彻底对齐 vanilla。
    const CreatureAttribute creatureType =
        (target != nullptr) ? target->getCreatureAttribute() : CreatureAttribute::Undefined;

    switch (m_damageType) {
        case Type::All:
            // 锋利：对所有生物造成额外伤害
            // 每级增加 0.5 * level + 0.5
            return static_cast<f32>(level) * 0.5f + 0.5f;

        case Type::Undead:
            // 亡灵杀手：对亡灵生物额外伤害
            if (creatureType == CreatureAttribute::Undead) {
                return static_cast<f32>(level) * 2.5f;
            }
            return 0.0f;

        case Type::Arthropods:
            // 节肢杀手：对节肢生物额外伤害
            if (creatureType == CreatureAttribute::Arthropod) {
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
