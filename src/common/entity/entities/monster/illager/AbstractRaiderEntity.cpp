#include "AbstractRaiderEntity.hpp"

namespace mc {

AbstractRaiderEntity::AbstractRaiderEntity(LegacyEntityType type, EntityId id)
    : AbstractIllagerEntity(type, id)
{
}

void AbstractRaiderEntity::startCelebrating() {
    m_celebrationTime = CELEBRATION_DURATION;
    setState(IllagerState::Celebrating);
}

void AbstractRaiderEntity::tick() {
    AbstractIllagerEntity::tick();

    // 更新庆祝时间
    if (m_celebrationTime > 0) {
        m_celebrationTime--;
        if (m_celebrationTime <= 0) {
            setState(IllagerState::Neutral);
        }
    }
}

} // namespace mc
