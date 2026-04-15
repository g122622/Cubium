#include "IllusionerEntity.hpp"

#include "../../../attribute/Attributes.hpp"
#include "../../../core/EntityRegistry.hpp"

namespace mc {

IllusionerEntity::IllusionerEntity(LegacyEntityType type, EntityId id)
    : SpellcastingIllagerEntity(type, id)
{
    registerAttributes();
}

std::unique_ptr<Entity> IllusionerEntity::create(IWorld* /*world*/)
{
    return std::make_unique<IllusionerEntity>(LegacyEntityType::Unknown, 0);
}

void IllusionerEntity::attackEntityWithRangedAttack(LivingEntity* target, f32 charge)
{
    // TODO: 接入箭矢实体后补齐幻术师远程攻击
    (void)target;
    (void)charge;
}

void IllusionerEntity::castBlindnessSpell()
{
    if (m_blindnessCooldown > 0 || isSpellcasting()) {
        return;
    }

    setSpellType(SpellType::Blindness);
    setSpellTicks(SPELLCASTING_DURATION);
    m_blindnessCooldown = BLINDNESS_COOLDOWN;
    // TODO: 接入目标与药水效果后补齐失明法术结算
}

void IllusionerEntity::castMirrorSpell()
{
    if (m_mirrorCooldown > 0 || hasMirrors() || isSpellcasting()) {
        return;
    }

    setSpellType(SpellType::Disappear);
    setSpellTicks(SPELLCASTING_DURATION);
    m_mirrorCooldown = MIRROR_COOLDOWN;
    // TODO: 接入分身实体后补齐镜像法术
}

void IllusionerEntity::tick()
{
    SpellcastingIllagerEntity::tick();

    if (m_blindnessCooldown > 0) {
        --m_blindnessCooldown;
    }
    if (m_mirrorCooldown > 0) {
        --m_mirrorCooldown;
    }
}

void IllusionerEntity::registerGoals()
{
    SpellcastingIllagerEntity::registerGoals();
    // TODO: 接入 Illusioner 专用远程/施法 goal
}

void IllusionerEntity::registerAttributes()
{
    SpellcastingIllagerEntity::registerAttributes();
    m_attributes.setBaseValue(entity::attribute::Attributes::MAX_HEALTH, 32.0f);
    m_attributes.setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.5f);
    m_attributes.setBaseValue(entity::attribute::Attributes::FOLLOW_RANGE, 18.0f);
    m_attributes.setBaseValue(entity::attribute::Attributes::ATTACK_DAMAGE, 2.0f);
}

} // namespace mc
