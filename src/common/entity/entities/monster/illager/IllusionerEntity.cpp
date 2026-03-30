#include "IllusionerEntity.hpp"
#include "../../../attribute/Attributes.hpp"
#include "../../../core/EntityRegistry.hpp"
#include <memory>

namespace mc {

IllusionerEntity::IllusionerEntity(LegacyEntityType type, EntityId id)
    : AbstractIllagerEntity(type, id)
{
}

std::unique_ptr<Entity> IllusionerEntity::create(IWorld* /*world*/) {
    return std::make_unique<IllusionerEntity>(LegacyEntityType::Unknown, 0);
}

void IllusionerEntity::attackEntityWithRangedAttack(LivingEntity* target, f32 charge) {
    // TODO: 发射箭矢
    // auto arrow = std::make_unique<ArrowEntity>(LegacyEntityType::Unknown, 0);
    // arrow->setOwner(this);
    // arrow->setDamage(charge * 2.0f);
    // world().spawnEntity(std::move(arrow), position());
    (void)target;
    (void)charge;
}

void IllusionerEntity::castBlindnessSpell() {
    if (m_blindnessCooldown > 0) {
        return;
    }

    // TODO: 对目标施放失明效果
    // LivingEntity* target = getAttackTarget();
    // if (target) {
    //     target->addEffect(Effect(EffectType::BLINDNESS, 60, 0));
    // }
    m_blindnessCooldown = BLINDNESS_COOLDOWN;
}

void IllusionerEntity::castMirrorSpell() {
    if (m_mirrorCooldown > 0 || hasMirrors()) {
        return;
    }

    // TODO: 创建4个分身
    // for (int i = 0; i < 4; i++) {
    //     auto mirror = createMirrorEntity();
    //     m_mirrorEntities.push_back(mirror->id());
    //     world().spawnEntity(std::move(mirror), position());
    // }
    m_mirrorCooldown = MIRROR_COOLDOWN;
}

void IllusionerEntity::tick() {
    AbstractIllagerEntity::tick();

    // 更新冷却
    if (m_blindnessCooldown > 0) {
        m_blindnessCooldown--;
    }
    if (m_mirrorCooldown > 0) {
        m_mirrorCooldown--;
    }
}

void IllusionerEntity::registerGoals() {
    AbstractIllagerEntity::registerGoals();

    // TODO: 幻术师特有 AI 目标
    // - IllusionerAttackGoal (弓箭攻击)
    // - IllusionerBlindnessGoal (失明法术)
    // - IllusionerMirrorGoal (分身法术)
}

void IllusionerEntity::registerAttributes() {
    AbstractIllagerEntity::registerAttributes();

    // 幻术师属性
    m_attributes.setBaseValue(entity::attribute::Attributes::MAX_HEALTH, 32.0f);
    m_attributes.setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.5f);
    m_attributes.setBaseValue(entity::attribute::Attributes::FOLLOW_RANGE, 18.0f);
    m_attributes.setBaseValue(entity::attribute::Attributes::ATTACK_DAMAGE, 2.0f);
}

} // namespace mc
