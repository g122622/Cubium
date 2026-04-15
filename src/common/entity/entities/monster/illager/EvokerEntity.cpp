#include "EvokerEntity.hpp"

#include "../../../attribute/Attributes.hpp"
#include "../../../core/EntityRegistry.hpp"

namespace mc {

EvokerEntity::EvokerEntity(LegacyEntityType type, EntityId id)
    : SpellcastingIllagerEntity(type, id)
{
    registerAttributes();
}

std::unique_ptr<Entity> EvokerEntity::create(IWorld* /*world*/)
{
    return std::make_unique<EvokerEntity>(LegacyEntityType::Unknown, 0);
}

void EvokerEntity::startCasting(i32 spellType)
{
    setSpellType(SpellcastingIllagerEntity::spellTypeFromId(spellType));
    setSpellTicks(CASTING_DURATION);
}

void EvokerEntity::finishCasting()
{
    switch (spellType()) {
        case SpellType::Fangs:
            castFangsAttack();
            m_fangsCooldown = FANGS_COOLDOWN;
            break;
        case SpellType::SummonVex:
            summonVex();
            m_summonCooldown = SUMMON_COOLDOWN;
            break;
        case SpellType::None:
        case SpellType::Wololo:
        case SpellType::Disappear:
        case SpellType::Blindness:
        default:
            break;
    }

    clearSpellcasting();
}

void EvokerEntity::castFangsAttack()
{
    // TODO: 接入 EvokerFangsEntity 后补齐尖牙施法
}

void EvokerEntity::summonVex()
{
    // TODO: 接入 VexEntity 召唤链与所有者语义
}

void EvokerEntity::tick()
{
    const bool wasSpellcasting = isSpellcasting();
    SpellcastingIllagerEntity::tick();

    if (wasSpellcasting && !isSpellcasting()) {
        finishCasting();
    }

    if (m_fangsCooldown > 0) {
        --m_fangsCooldown;
    }
    if (m_summonCooldown > 0) {
        --m_summonCooldown;
    }
}

void EvokerEntity::registerGoals()
{
    SpellcastingIllagerEntity::registerGoals();
    // TODO: 接入 Evoker 专用施法 goal
}

void EvokerEntity::registerAttributes()
{
    SpellcastingIllagerEntity::registerAttributes();
    m_attributes.setBaseValue(entity::attribute::Attributes::MAX_HEALTH, 24.0f);
    m_attributes.setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.35f);
    m_attributes.setBaseValue(entity::attribute::Attributes::FOLLOW_RANGE, 12.0f);
}

} // namespace mc
