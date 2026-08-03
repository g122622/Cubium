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

#include "LocalPlayerIdentity.hpp"
#include "common/core/Types.hpp"
#include "common/util/assert/AssertAll.hpp"

namespace mc::client {

void LocalPlayerIdentity::setIdentity(PlayerId playerId, EntityInstanceId entityId)
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

EntityInstanceId LocalPlayerIdentity::entityId() const
{
    return m_entityId;
}

bool LocalPlayerIdentity::isLocalPlayerEntity(EntityInstanceId entityId) const
{
    return m_hasIdentity && entityId == m_entityId;
}

bool LocalPlayerIdentity::isLocalPlayer(PlayerId playerId) const
{
    return m_hasIdentity && playerId == m_playerId;
}

} // namespace mc::client
