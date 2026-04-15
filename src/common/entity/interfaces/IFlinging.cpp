#include "IFlinging.hpp"

#include "../attribute/Attributes.hpp"
#include "../core/LivingEntity.hpp"
#include "../damage/DamageSource.hpp"

namespace mc {
namespace entity {

bool IFlinging::attackWithFling(
    LivingEntity& attacker,
    LivingEntity& target,
    bool attackerIsBaby)
{
    const f32 attackDamage = static_cast<f32>(
        attacker.getAttributeValue(attribute::Attributes::ATTACK_DAMAGE, 1.0));

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
    const f32 attackKnockback = static_cast<f32>(
        attacker.getAttributeValue(attribute::Attributes::ATTACK_KNOCKBACK, 0.0));
    const f32 knockbackResistance = static_cast<f32>(
        target.getAttributeValue(attribute::Attributes::KNOCKBACK_RESISTANCE, 0.0));
    const f32 knockbackStrength = attackKnockback - knockbackResistance;
    if (knockbackStrength <= 0.0f) {
        return;
    }

    Vector3 direction(
        target.x() - attacker.x(),
        0.0f,
        target.z() - attacker.z());
    if (direction.lengthSquared() <= 1.0e-6f) {
        return;
    }

    direction = direction.normalized();
    target.addVelocity(Vector3(
        direction.x * knockbackStrength * 0.5f,
        knockbackStrength * 0.25f,
        direction.z * knockbackStrength * 0.5f));
}

} // namespace entity
} // namespace mc
