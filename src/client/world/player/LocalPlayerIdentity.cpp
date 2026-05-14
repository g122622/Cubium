#include "LocalPlayerIdentity.hpp"
#include "common/util/assert/AssertAll.hpp"

namespace mc::client {

void LocalPlayerIdentity::setIdentity(PlayerId playerId, EntityId entityId)
{
    MC_ASSERT_RELEASE(playerId != 0);
    MC_ASSERT_RELEASE(entityId != INVALID_ENTITY_ID);

    m_playerId = playerId;
    m_entityId = entityId;
    m_hasIdentity = true;
}

void LocalPlayerIdentity::clear()
{
    m_playerId = 0;
    m_entityId = INVALID_ENTITY_ID;
    m_hasIdentity = false;
}

bool LocalPlayerIdentity::hasIdentity() const
{
    return m_hasIdentity;
}

PlayerId LocalPlayerIdentity::playerId() const
{
    return m_playerId;
}

EntityId LocalPlayerIdentity::entityId() const
{
    return m_entityId;
}

bool LocalPlayerIdentity::isLocalPlayerEntity(EntityId entityId) const
{
    return m_hasIdentity && entityId == m_entityId;
}

bool LocalPlayerIdentity::isLocalPlayer(PlayerId playerId) const
{
    return m_hasIdentity && playerId == m_playerId;
}

} // namespace mc::client
