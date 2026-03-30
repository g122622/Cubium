#include "EvokerEntity.hpp"
#include "../../../attribute/Attributes.hpp"
#include "../../../core/EntityRegistry.hpp"
#include <memory>

namespace mc {

EvokerEntity::EvokerEntity(LegacyEntityType type, EntityId id)
    : AbstractIllagerEntity(type, id)
{
}

std::unique_ptr<Entity> EvokerEntity::create(IWorld* /*world*/) {
    return std::make_unique<EvokerEntity>(LegacyEntityType::Unknown, 0);
}

void EvokerEntity::startCasting(i32 spellType) {
    m_casting = true;
    m_spellType = spellType;
    m_castingTime = CASTING_DURATION;
}

void EvokerEntity::finishCasting() {
    if (!m_casting) {
        return;
    }

    switch (m_spellType) {
        case 1:
            castFangsAttack();
            m_fangsCooldown = FANGS_COOLDOWN;
            break;
        case 2:
            summonVex();
            m_summonCooldown = SUMMON_COOLDOWN;
            break;
        default:
            break;
    }

    m_casting = false;
    m_spellType = 0;
}

void EvokerEntity::castFangsAttack() {
    // TODO: 生成尖牙实体
    // EvokerFangsEntity 需要单独实现
}

void EvokerEntity::summonVex() {
    // TODO: 生成恼鬼
    // auto vex = std::make_unique<VexEntity>(LegacyEntityType::Unknown, 0);
    // vex->setOwner(this);
    // world().spawnEntity(std::move(vex), position());
}

void EvokerEntity::tick() {
    AbstractIllagerEntity::tick();

    // 更新施法时间
    if (m_casting && m_castingTime > 0) {
        m_castingTime--;
        if (m_castingTime <= 0) {
            finishCasting();
        }
    }

    // 更新冷却
    if (m_fangsCooldown > 0) {
        m_fangsCooldown--;
    }
    if (m_summonCooldown > 0) {
        m_summonCooldown--;
    }
}

void EvokerEntity::registerGoals() {
    AbstractIllagerEntity::registerGoals();

    // TODO: 唤魔者特有 AI 目标
    // - EvokerAttackGoal (尖牙攻击)
    // - EvokerSummonGoal (召唤恼鬼)
}

void EvokerEntity::registerAttributes() {
    AbstractIllagerEntity::registerAttributes();

    // 唤魔者属性
    m_attributes.setBaseValue(entity::attribute::Attributes::MAX_HEALTH, 24.0f);
    m_attributes.setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.35f);
    m_attributes.setBaseValue(entity::attribute::Attributes::FOLLOW_RANGE, 12.0f);
}

} // namespace mc
