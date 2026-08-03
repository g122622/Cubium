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

#include "IFlinging.hpp"

#include "common/entity/attribute/Attributes.hpp"
#include "common/entity/core/DataParameter.hpp"
#include "common/entity/core/EntitySize.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/damage/DamageSource.hpp"
#include "common/util/math/MathConstants.hpp"

namespace mc {
namespace entity {

bool IFlinging::attackWithFling(LivingEntity& attacker, LivingEntity& target, bool attackerIsBaby)
{
    const f32 attackDamage = static_cast<f32>(attacker.getAttributeValue(attribute::Attributes::ATTACK_DAMAGE, 1.0));

    EntityDamageSource damageSource(DamageType::MobAttack, &attacker);
    if (!target.hurt(damageSource, attackDamage)) {
        return false;
    }

    if (!attackerIsBaby) {
        flingTarget(attacker, target);
    }

    return true;
}

void IFlinging::flingTarget(LivingEntity& attacker, LivingEntity& target)
{
    const f32 attackKnockback =
        static_cast<f32>(attacker.getAttributeValue(attribute::Attributes::ATTACK_KNOCKBACK, 0.0));
    const f32 knockbackResistance =
        static_cast<f32>(target.getAttributeValue(attribute::Attributes::KNOCKBACK_RESISTANCE, 0.0));
    const f32 knockbackStrength = attackKnockback - knockbackResistance;
    if (knockbackStrength <= 0.0f) {
        return;
    }

    Vector3 direction(target.x() - attacker.x(), 0.0f, target.z() - attacker.z());
    if (direction.lengthSquared() <= math::EPSILON) {
        return;
    }

    direction = direction.normalized();
    target.addVelocity(Vector3(
        direction.x * knockbackStrength * 0.5f, knockbackStrength * 0.25f, direction.z * knockbackStrength * 0.5f));
}

} // namespace entity
} // namespace mc
