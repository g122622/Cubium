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
#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/damage/DamageSource.hpp"
#include "common/util/math/MathConstants.hpp"
#include "common/util/math/MathUtils.hpp"
#include "common/util/math/Vector3.hpp"

namespace mc {
namespace entity {

bool IFlinging::hurtAndThrowTarget(LivingEntity& attacker, LivingEntity& target)
{
    // 伤害基础值取攻击力属性；成年生物的伤害随机化：f/2 + rand[0, f)，幼年取原值。
    f32 damage = static_cast<f32>(attacker.getAttributeValue(attribute::Attributes::ATTACK_DAMAGE, 1.0));
    if (!attacker.isChild() && static_cast<i32>(damage) > 0) {
        damage = damage / 2.0f + static_cast<f32>(attacker.getRandom().nextInt(static_cast<i32>(damage)));
    }

    EntityDamageSource damageSource(DamageType::MobAttack, &attacker);
    if (!target.hurt(damageSource, damage)) {
        return false;
    }

    // 后置附魔效果（节肢杀手的缓慢等）。必须在 hurt 成功之后，否则未命中也触发效果。
    attacker.onAttackEntity(target);

    // 幼年攻击者只造成伤害、不抛飞。
    if (!attacker.isChild()) {
        flingTarget(attacker, target);
    }

    return true;
}

void IFlinging::flingTarget(LivingEntity& attacker, LivingEntity& target)
{
    // 抛飞强度 = 击退属性 - 目标击退抗性；非正表示目标完全抗住，不做任何事。
    const f32 attackKnockback =
        static_cast<f32>(attacker.getAttributeValue(attribute::Attributes::ATTACK_KNOCKBACK, 0.0));
    const f32 knockbackResistance =
        static_cast<f32>(target.getAttributeValue(attribute::Attributes::KNOCKBACK_RESISTANCE, 0.0));
    const f32 strength = attackKnockback - knockbackResistance;
    if (strength <= 0.0f) {
        return;
    }

    Vector3 direction(target.x() - attacker.x(), 0.0f, target.z() - attacker.z());
    if (direction.lengthSquared() <= math::EPSILON) {
        // 两者水平位置重合：没有可用的抛飞方向，保持原速度。
        return;
    }

    math::IRandom& random = attacker.getRandom();

    // 水平分量：单位方向按 strength * (rand*0.5 + 0.2) 缩放，再随机绕 Y 轴偏转 rand[-10, 10] 度。
    const f32 horizontalScale = strength * (random.nextFloat() * 0.5f + 0.2f);
    const f32 rotationDegrees = static_cast<f32>(random.nextInt(21) - 10);
    const Vector3 horizontal = (direction.normalized() * horizontalScale).yRot(math::toRadians(rotationDegrees));

    // 垂直分量：strength * rand * 0.5。
    const f32 vertical = strength * random.nextFloat() * 0.5f;

    target.addVelocity(horizontal.x, vertical, horizontal.z);

    // 客户端需要收到速度同步才能看到抛飞效果。
    target.markHurt();
}

} // namespace entity
} // namespace mc
