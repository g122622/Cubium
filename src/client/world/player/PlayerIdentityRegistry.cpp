/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without including limitation the rights to use, copy, modify,
 * merge, publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
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

#include "PlayerIdentityRegistry.hpp"
#include <spdlog/spdlog.h>

namespace mc::client {

void PlayerIdentityRegistry::_indexEntry(const Entry& entry)
{
    m_entityByPlayer[entry.playerId] = entry.entityId;
    m_entityByUsername[entry.username] = entry.entityId;
    if (entry.hasUuid) {
        m_entityByUuid[entry.uuid] = entry.entityId;
    }
}

void PlayerIdentityRegistry::_unindexEntry(const Entry& entry)
{
    m_entityByPlayer.erase(entry.playerId);
    m_entityByUsername.erase(entry.username);
    if (entry.hasUuid) {
        m_entityByUuid.erase(entry.uuid);
    }
}

void PlayerIdentityRegistry::registerLocalPlayer(
    EntityInstanceId entityId, PlayerId playerId, const Uuid& uuid, const std::string& username)
{
    // 若已存在同名网络玩家条目（PlayerSpawnPacket 先到），先移除旧条目再以本地身份重建。
    auto byNameIt = m_entityByUsername.find(username);
    if (byNameIt != m_entityByUsername.end() && byNameIt->second != entityId) {
        spdlog::warn("PlayerIdentityRegistry: replacing existing entry for username '{}' "
                     "(old entityId={}, new local entityId={})",
            username,
            byNameIt->second,
            entityId);
        removeByEntityId(byNameIt->second);
    }

    if (m_byEntity.count(entityId) > 0) {
        _unindexEntry(m_byEntity.at(entityId));
    }

    Entry entry;
    entry.entityId = entityId;
    entry.playerId = playerId;
    entry.uuid = uuid;
    entry.username = username;
    entry.isLocal = true;
    entry.hasUuid = true;

    m_byEntity[entityId] = entry;
    _indexEntry(m_byEntity.at(entityId));
    m_uuidByUsername[username] = uuid; // 本地玩家 UUID 已知，同步暂存表
    m_localEntityId = entityId;
}

void PlayerIdentityRegistry::registerNetworkPlayer(
    EntityInstanceId entityId, PlayerId playerId, const std::string& username)
{
    if (m_byEntity.count(entityId) > 0) {
        _unindexEntry(m_byEntity.at(entityId));
    }

    Entry entry;
    entry.entityId = entityId;
    entry.playerId = playerId;
    entry.username = username;
    entry.isLocal = false;
    entry.hasUuid = false;

    // 若 PlayerListEntry 已先到，从暂存表取用 UUID 补全。
    auto uuidIt = m_uuidByUsername.find(username);
    if (uuidIt != m_uuidByUsername.end()) {
        entry.uuid = uuidIt->second;
        entry.hasUuid = true;
    }

    m_byEntity[entityId] = entry;
    _indexEntry(m_byEntity.at(entityId));
}

void PlayerIdentityRegistry::registerPlayerListUuid(const Uuid& uuid, const std::string& username)
{
    // 暂存 username→uuid，供后续 registerNetworkPlayer 取用
    m_uuidByUsername[username] = uuid;

    // 若该 username 已有实体条目，直接补全其 UUID
    auto byNameIt = m_entityByUsername.find(username);
    if (byNameIt != m_entityByUsername.end()) {
        assignUuidToEntity(byNameIt->second, uuid);
    }
}

bool PlayerIdentityRegistry::assignUuidToEntity(EntityInstanceId entityId, const Uuid& uuid)
{
    auto it = m_byEntity.find(entityId);
    if (it == m_byEntity.end()) {
        return false;
    }

    Entry& entry = it->second;
    if (entry.hasUuid) {
        if (entry.uuid == uuid) {
            return true; // 幂等
        }
        m_entityByUuid.erase(entry.uuid);
    }
    entry.uuid = uuid;
    entry.hasUuid = true;
    m_entityByUuid[uuid] = entityId;
    m_uuidByUsername[entry.username] = uuid;
    return true;
}

void PlayerIdentityRegistry::removeByEntityId(EntityInstanceId entityId)
{
    auto it = m_byEntity.find(entityId);
    if (it == m_byEntity.end()) {
        return;
    }
    const std::string username = it->second.username;
    _unindexEntry(it->second);
    // 仅当暂存表的 username 仍指向自己时才清除，避免误删同名的后来条目
    auto uuidIt = m_uuidByUsername.find(username);
    if (uuidIt != m_uuidByUsername.end()) {
        // 暂存表只记 username→uuid，无 entityId 指针，username 唯一故可安全删
        m_uuidByUsername.erase(uuidIt);
    }
    if (m_localEntityId == entityId) {
        m_localEntityId = INVALID_ENTITY_ID;
    }
    m_byEntity.erase(it);
}

void PlayerIdentityRegistry::removeByPlayerId(PlayerId playerId)
{
    auto it = m_entityByPlayer.find(playerId);
    if (it == m_entityByPlayer.end()) {
        return;
    }
    removeByEntityId(it->second);
}

void PlayerIdentityRegistry::removeByUuid(const Uuid& uuid)
{
    auto it = m_entityByUuid.find(uuid);
    if (it == m_entityByUuid.end()) {
        return;
    }
    removeByEntityId(it->second);
}

void PlayerIdentityRegistry::clear()
{
    m_byEntity.clear();
    m_entityByPlayer.clear();
    m_entityByUuid.clear();
    m_entityByUsername.clear();
    m_uuidByUsername.clear();
    m_localEntityId = INVALID_ENTITY_ID;
}

const Uuid* PlayerIdentityRegistry::uuidOf(EntityInstanceId entityId) const
{
    auto it = m_byEntity.find(entityId);
    if (it == m_byEntity.end() || !it->second.hasUuid) {
        return nullptr;
    }
    return &it->second.uuid;
}

EntityInstanceId PlayerIdentityRegistry::entityIdOf(const Uuid& uuid) const
{
    auto it = m_entityByUuid.find(uuid);
    if (it == m_entityByUuid.end()) {
        return INVALID_ENTITY_ID;
    }
    return it->second;
}

PlayerId PlayerIdentityRegistry::playerIdOf(EntityInstanceId entityId) const
{
    auto it = m_byEntity.find(entityId);
    if (it == m_byEntity.end()) {
        return 0;
    }
    return it->second.playerId;
}

const Uuid* PlayerIdentityRegistry::uuidByUsername(const std::string& username) const
{
    // 优先查暂存表（PlayerListEntry 可能早于实体）
    auto uuidIt = m_uuidByUsername.find(username);
    if (uuidIt != m_uuidByUsername.end()) {
        return &uuidIt->second;
    }
    return nullptr;
}

EntityInstanceId PlayerIdentityRegistry::entityIdByUsername(const std::string& username) const
{
    auto it = m_entityByUsername.find(username);
    if (it == m_entityByUsername.end()) {
        return INVALID_ENTITY_ID;
    }
    return it->second;
}

bool PlayerIdentityRegistry::isLocal(EntityInstanceId entityId) const
{
    return entityId != INVALID_ENTITY_ID && entityId == m_localEntityId;
}

} // namespace mc::client
