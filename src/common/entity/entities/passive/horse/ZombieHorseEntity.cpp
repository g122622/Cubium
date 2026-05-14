#include "ZombieHorseEntity.hpp"
#include "../../../attribute/Attributes.hpp"
#include "../../../core/EntityRegistry.hpp"
#include <memory>

namespace mc {

ZombieHorseEntity::ZombieHorseEntity(LegacyEntityType type, EntityId id)
    : AbstractHorseEntity(type, id)
{
    // 僵尸马默认已驯服
    setTame(true);
    // 设置跳跃强度
    setJumpStrength(0.96f);
}

std::unique_ptr<Entity> ZombieHorseEntity::create(IWorld* /*world*/)
{
    return std::make_unique<ZombieHorseEntity>(LegacyEntityType::Unknown, 0);
}

bool ZombieHorseEntity::canBeRiddenBy(Player* player) const
{
    // 僵尸马不需要驯服即可骑乘
    if (m_rider != nullptr && m_rider != player) {
        return false;
    }
    return true;
}

void ZombieHorseEntity::tick()
{
    AbstractHorseEntity::tick();
    // 僵尸马不需要额外的 tick 逻辑
}

void ZombieHorseEntity::registerGoals()
{
    AbstractHorseEntity::registerGoals();
    // 僵尸马没有额外 AI
}

void ZombieHorseEntity::registerAttributes()
{
    AbstractHorseEntity::registerAttributes();

    // 僵尸马的属性
    m_attributes.setBaseValue(entity::attribute::Attributes::MAX_HEALTH, 15.0f);
    m_attributes.setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.2f);
}

} // namespace mc
