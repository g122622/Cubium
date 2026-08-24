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

#include "ThornsEnchantment.hpp"
#include "common/core/Types.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/damage/DamageSource.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/util/math/random/Random.hpp"

namespace mc {
namespace item {
namespace enchant {

bool ThornsEnchantment::shouldTrigger(i32 level, math::Random& random)
{
    if (level <= 0) {
        return false;
    }
    // 对齐 vanilla Enchantments.java:345 perLevel 0.15（每级 15% 触发概率）
    return random.nextFloat() < static_cast<f32>(level) * 0.15f;
}

f32 ThornsEnchantment::getThornsDamage(math::Random& random)
{
    // 对齐 vanilla 1.21.11 Enchantments.java:342 THORNS 的 DamageEntity(constant 1.0, constant 5.0)：
    //   Mth.randomBetween(random, 1.0F, 5.0F) = nextFloat() * (5.0 - 1.0) + 1.0 ∈ [1.0, 5.0)
    // 与等级无关（无老版本 level>10 分支）。Cubium IRandom::nextFloat(min, max) 语义等价。
    return random.nextFloat(1.0f, 5.0f);
}

void ThornsEnchantment::onUserHurt(
    LivingEntity& user, Entity& attacker, ItemStack& enchantedItem, EquipmentSlot slot, i32 level) const
{
    if (level <= 0 || &user == &attacker) {
        return;
    }

    // 使用用户实体的随机数生成器
    math::Random rng(static_cast<u64>(user.id()) ^ static_cast<u64>(user.ticksExisted()));

    // 检查是否触发荆棘效果（对齐 vanilla Enchantments.java:345 perLevel 0.15 概率门控）
    if (!shouldTrigger(level, rng)) {
        return;
    }

    // 1. DamageEntity(constant 1.0, constant 5.0, THORNS)：对攻击者造成 [1.0,5.0) 随机荆棘伤害
    //    （对齐 DamageEntity.java:28-29 p_345450_.hurtServer(...Mth.randomBetween(...))）。
    //    荆棘伤害源 DamageSources::thorns(&user) 的 causingEntity=user（对齐 vanilla damageSource
    //    owner = EnchantedItemInUse.owner() = 持有荆棘装备的受害者）。
    LivingEntity* livingAttacker = dynamic_cast<LivingEntity*>(&attacker);
    if (livingAttacker != nullptr) {
        auto damageSource = DamageSources::thorns(&user);
        f32 thornsDamage = getThornsDamage(rng);
        livingAttacker->hurt(damageSource, thornsDamage);
    }

    // 2. ChangeItemDamage(constant 2.0)：触发荆棘的护甲扣 2 耐久
    //    （对齐 ChangeItemDamage.java:25-26 itemstack.hurtAndBreak((int)amount.calculate(level), ...)，
    //    amount=constant 2.0 故 (int)2.0=2）。作用于 enchantedItem（触发荆棘的那件护甲，对齐 vanilla
    //    EnchantedItemInUse.itemStack），由本方法处理而非调用方（见 README.md:152）。
    //    hurtAndBreak 内部处理 Unbreaking 附魔减耗与耐久耗尽破坏回调（对齐 vanilla hurtAndBreak 语义）。
    LivingEntity::hurtAndBreak(enchantedItem, 2, &user, slot);
}

} // namespace enchant
} // namespace item
} // namespace mc
