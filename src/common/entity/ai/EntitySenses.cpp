#include "EntitySenses.hpp"

#include "../core/Entity.hpp"
#include "../core/MobEntity.hpp"

#include <algorithm>

namespace mc::entity::ai {

EntitySenses::EntitySenses(MobEntity* mob)
    : m_mob(mob)
{
}

void EntitySenses::tick()
{
    m_seenEntities.clear();
    m_unseenEntities.clear();
}

bool EntitySenses::canSee(const Entity& entity)
{
    if (std::find(m_seenEntities.begin(), m_seenEntities.end(), &entity) != m_seenEntities.end()) {
        return true;
    }

    if (std::find(m_unseenEntities.begin(), m_unseenEntities.end(), &entity) != m_unseenEntities.end()) {
        return false;
    }

    const bool visible = m_mob->canSee(entity);
    if (visible) {
        m_seenEntities.push_back(&entity);
    } else {
        m_unseenEntities.push_back(&entity);
    }

    return visible;
}

} // namespace mc::entity::ai
