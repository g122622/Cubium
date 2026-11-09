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

#include "PlayerAttackHelper.hpp"
#include "common/core/Types.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/effect/EffectType.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/enchantment/EnchantmentHelper.hpp"

namespace mc::entity::combat {

// ========== 暴击判定 ==========

bool PlayerAttackHelper::isCriticalHit(const Player& player)
{
    // 暴击条件（全部满足）：
    // 1. 玩家正在下落（fallDistance > 0）
    // 2. 玩家不在地面
    // 3. 玩家不在水中
    // 4. 玩家不在梯子/藤蔓上
    // 5. 玩家没有失明效果
    // 6. 玩家没有骑乘
    // 7. 玩家不在疾跑（暴击与疾跑击退互斥）

    if (player.fallDistance() <= 0.0f) {
        return false;
    }

    if (player.isOnGround()) {
        return false;
    }

    if (player.isInWater()) {
        return false;
    }

    if (player.isOnLadder()) {
        return false;
    }

    if (player.hasEffect(effect::EffectType::Blindness)) {
        return false;
    }

    if (player.isRiding()) {
        return false;
    }

    if (player.isSprinting()) {
        return false;
    }

    return true;
}

// ========== 附魔伤害加成 ==========

f32 PlayerAttackHelper::getEnchantmentDamageBonus(const ItemStack& weapon, const LivingEntity* target)
{
    if (weapon.isEmpty()) {
        return 0.0f;
    }

    // 委托 EnchantmentHelper::getTotalDamageBonus 汇总武器上所有附魔的 getDamageBonus 虚函数。
    // 锋利（DamageEnchantment::Type::All）对所有生物 +0.5+level*0.5，与 target 无关；
    // 亡灵杀手（Type::Undead）/节肢杀手（Type::Arthropods）各自用 EntityTypeTags 标签
    // （SENSITIVE_TO_SMITE / SENSITIVE_TO_BANE_OF_ARTHROPODS）判定 target，命中则 +level*2.5。
    // 标签判定覆盖全部亡灵/节肢成员，包括 zombie_horse/zombie_nautilus 等枚举未覆盖的实体。
    // 穿刺（ImpalingEnchantment）属 Trident 类型附魔，与 Weapon 类型互斥，近战武器不会携带，无干扰。
    return item::enchant::EnchantmentHelper::getTotalDamageBonus(weapon, target);
}

} // namespace mc::entity::combat
