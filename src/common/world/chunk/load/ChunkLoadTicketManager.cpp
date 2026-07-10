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

#include "common/world/chunk/load/ChunkLoadTicketManager.hpp"
#include "common/profiler/TraceEvents.hpp"
#include "common/world/chunk/load/ChunkLoadTicket.hpp"
#include <algorithm>
#include <mutex>
#include <optional>
#include <spdlog/spdlog.h>

using namespace mc::trace;

namespace {

struct TrackingChangeEvent {
    mc::PlayerId playerId;
    mc::ChunkCoord x;
    mc::ChunkCoord z;
    bool isTracking;
};

} // namespace

namespace mc::world::chunk {

// ============================================================================
// 预定义票据类型
// ============================================================================

namespace TicketTypes {
// 强制加载票据
const ChunkLoadTicketType<ChunkPos> FORCED = ChunkLoadTicketType<ChunkPos>::create("forced");

// 传送门票据（300 tick 生命周期）
const ChunkLoadTicketType<ChunkPos> PORTAL = ChunkLoadTicketType<ChunkPos>::create("portal", 300);

// 传送后票据（5 tick 生命周期）
const ChunkLoadTicketType<u32> POST_TELEPORT = ChunkLoadTicketType<u32>::create("post_teleport", 5);

// 未知票据
const ChunkLoadTicketType<ChunkPos> UNKNOWN = ChunkLoadTicketType<ChunkPos>::create("unknown");

// 世界启动票据
const ChunkLoadTicketType<Unit> START = ChunkLoadTicketType<Unit>::create("start");

// 末影龙战斗票据
const ChunkLoadTicketType<Unit> DRAGON = ChunkLoadTicketType<Unit>::create("dragon");

// 光照计算票据
const ChunkLoadTicketType<ChunkPos> LIGHT = ChunkLoadTicketType<ChunkPos>::create("light");

} // namespace TicketTypes

// ============================================================================
// ChunkTicketSet 实现
// ============================================================================

void ChunkTicketSet::addTicket(ChunkLoadTicket ticket)
{
    // 去重
    for (const auto& existingTicket : m_tickets) {
        if (existingTicket == ticket) {
            return;
        }
    }

    m_tickets.push_back(std::move(ticket));
}

bool ChunkTicketSet::removeTicket(const ChunkLoadTicket& ticket)
{
    for (auto it = m_tickets.begin(); it != m_tickets.end(); ++it) {
        if (*it == ticket) {
            m_tickets.erase(it);
            return true;
        }
    }
    return false;
}

i32 ChunkTicketSet::getMinLevel() const noexcept
{
    if (m_tickets.empty()) {
        return static_cast<i32>(ChunkLoadLevel::MaxLevel);
    }

    i32 minLevel = static_cast<i32>(ChunkLoadLevel::MaxLevel);
    for (const auto& ticket : m_tickets) {
        if (ticket.level() < minLevel) {
            minLevel = ticket.level();
        }
    }
    return minLevel;
}

void ChunkTicketSet::removeExpired(u64 currentTime)
{
    auto it = m_tickets.begin();
    while (it != m_tickets.end()) {
        if (it->isExpired(currentTime)) {
            it = m_tickets.erase(it);
        } else {
            ++it;
        }
    }
}

// ============================================================================
// ChunkLoadTicketManager 实现
// ============================================================================

ChunkLoadTicketManager::ChunkLoadTicketManager()
    : m_viewDistance(10)
{
    // 设置距离图回调（统一用于所有来源）
    m_distanceGraph.setLevelChangeCallback([this](ChunkCoord x, ChunkCoord z, i32 oldLevel, i32 newLevel) {
        if (m_levelChangeCallback) {
            m_levelChangeCallback(x, z, oldLevel, newLevel);
        }
    });
}

void ChunkLoadTicketManager::_addTicket(ChunkPos pos, ChunkLoadTicket ticket)
{
    u64 key = _posToKey(pos.x, pos.z);

    auto& ticketSet = m_chunkTickets[key];
    ticketSet.addTicket(std::move(ticket));

    m_dirtyChunks.insert(key);
}

void ChunkLoadTicketManager::_removeTicket(ChunkPos pos, const ChunkLoadTicket& ticket)
{
    u64 key = _posToKey(pos.x, pos.z);

    auto it = m_chunkTickets.find(key);
    if (it != m_chunkTickets.end()) {
        it->second.removeTicket(ticket);

        // 如果票据集合为空，移除整个条目
        if (it->second.empty()) {
            m_chunkTickets.erase(it);
        }

        // 标记区块为脏
        m_dirtyChunks.insert(key);
    }
}

void ChunkLoadTicketManager::updatePlayerPosition(PlayerId playerId, ChunkCoord x, ChunkCoord z)
{
    ChunkPos newPos(x, z);
    auto& state = m_playerStates[playerId];
    if (state.hasPosition && state.center.x == x && state.center.z == z) {
        return;
    }

    const std::optional<ChunkPos> oldPos = state.hasPosition ? std::optional<ChunkPos>(state.center) : std::nullopt;
    const std::unordered_set<u64> oldTrackedChunks = state.trackedChunks;

    if (oldPos.has_value()) {
        _updatePlayerSourceCenter(&oldPos.value(), &newPos);
    } else {
        _updatePlayerSourceCenter(nullptr, &newPos);
    }

    state.center = newPos;
    state.hasPosition = true;
    state.trackedChunks = _buildTrackedChunkSet(x, z);
    m_playerPositions[playerId] = newPos;

    _applyTrackingDelta(playerId, oldTrackedChunks, state.trackedChunks);
    processUpdates();
}

void ChunkLoadTicketManager::removePlayer(PlayerId playerId)
{
    auto stateIt = m_playerStates.find(playerId);
    if (stateIt == m_playerStates.end()) {
        m_playerPositions.erase(playerId);
        return;
    }

    const PlayerSourceState state = stateIt->second;
    if (state.hasPosition) {
        _updatePlayerSourceCenter(&state.center, nullptr);
    }

    _applyTrackingDelta(playerId, state.trackedChunks, {});
    m_playerStates.erase(stateIt);
    m_playerPositions.erase(playerId);
    processUpdates();
}

i32 ChunkLoadTicketManager::getChunkLevel(ChunkCoord x, ChunkCoord z) const
{
    return m_distanceGraph.getLevel(x, z);
}

void ChunkLoadTicketManager::tick()
{
    ++m_currentTime;

    // 清理过期票据
    for (auto& [key, ticketSet] : m_chunkTickets) {
        const size_t oldSize = ticketSet.size();
        ticketSet.removeExpired(m_currentTime);
        if (ticketSet.size() != oldSize) {
            m_dirtyChunks.insert(key);
        }
    }

    // 移除空的票据集合
    for (auto it = m_chunkTickets.begin(); it != m_chunkTickets.end();) {
        if (it->second.empty()) {
            it = m_chunkTickets.erase(it);
        } else {
            ++it;
        }
    }

    // 处理距离图更新
    processUpdates();
}

void ChunkLoadTicketManager::setViewDistance(i32 distance)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Initialization, "ChunkLoadTicketManager::setViewDistance");

    const i32 clampedDistance = std::clamp(distance, 2, 32);

    if (m_viewDistance == clampedDistance) {
        return;
    }

    m_viewDistance = clampedDistance;

    for (const auto& [playerId, state] : m_playerStates) {
        if (state.hasPosition) {
            m_dirtyChunks.insert(_posToKey(state.center.x, state.center.z));
        }
    }
    _rebuildAllPlayerSources();
    processUpdates();
}

size_t ChunkLoadTicketManager::totalTicketCount() const
{
    size_t count = 0;
    for (const auto& [key, ticketSet] : m_chunkTickets) {
        count += ticketSet.size();
    }
    return count;
}

void ChunkLoadTicketManager::forceChunk(ChunkCoord x, ChunkCoord z, bool force)
{
    ChunkPos pos(x, z);
    ChunkLoadTicket ticket(TicketTypes::FORCED, static_cast<i32>(ChunkLoadLevel::Full), pos);

    if (force) {
        _addTicket(pos, ticket);
    } else {
        _removeTicket(pos, ticket);
    }

    processUpdates();
}

void ChunkLoadTicketManager::processUpdates()
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Chunk,
        "ChunkLoadTicketManager::processUpdates",
        "dirtyChunkCount",
        m_dirtyChunks.size(),
        "playerCount",
        m_playerStates.size());

    for (u64 key : m_dirtyChunks) {
        ChunkCoord x = static_cast<ChunkCoord>(key >> 32);
        ChunkCoord z = static_cast<ChunkCoord>(key & 0xFFFFFFFF);
        _refreshChunkSourceLevel(x, z);
    }
    m_dirtyChunks.clear();

    m_distanceGraph.processUpdates(1000);
}

void ChunkLoadTicketManager::_refreshChunkSourceLevel(ChunkCoord x, ChunkCoord z)
{
    const u64 key = _posToKey(x, z);

    i32 sourceLevel = ChunkDistanceGraph::MAX_LEVEL;
    auto ticketIt = m_chunkTickets.find(key);
    if (ticketIt != m_chunkTickets.end()) {
        sourceLevel = std::min(sourceLevel, ticketIt->second.getMinLevel());
    }

    auto playerIt = m_playerSourceCounts.find(key);
    if (playerIt != m_playerSourceCounts.end() && playerIt->second > 0) {
        sourceLevel = std::min(sourceLevel, viewDistanceToLevel(m_viewDistance));
    }

    const bool isDecreasing = sourceLevel < m_distanceGraph.getLevel(x, z);
    m_distanceGraph.updateSourceLevel(x, z, sourceLevel, isDecreasing);
}

void ChunkLoadTicketManager::_updatePlayerSourceCenter(const ChunkPos* oldPos, const ChunkPos* newPos)
{
    if (oldPos != nullptr) {
        const u64 oldKey = _posToKey(oldPos->x, oldPos->z);
        auto it = m_playerSourceCounts.find(oldKey);
        if (it != m_playerSourceCounts.end()) {
            --it->second;
            if (it->second <= 0) {
                m_playerSourceCounts.erase(it);
            }
        }
        m_dirtyChunks.insert(oldKey);
    }

    if (newPos != nullptr) {
        const u64 newKey = _posToKey(newPos->x, newPos->z);
        ++m_playerSourceCounts[newKey];
        m_dirtyChunks.insert(newKey);
    }
}

std::unordered_set<u64> ChunkLoadTicketManager::_buildTrackedChunkSet(ChunkCoord centerX, ChunkCoord centerZ) const
{
    std::unordered_set<u64> chunks;
    for (i32 dx = -m_viewDistance; dx <= m_viewDistance; ++dx) {
        for (i32 dz = -m_viewDistance; dz <= m_viewDistance; ++dz) {
            const i32 distance = std::max(std::abs(dx), std::abs(dz));
            if (distance <= m_viewDistance) {
                chunks.insert(_posToKey(centerX + dx, centerZ + dz));
            }
        }
    }
    return chunks;
}

void ChunkLoadTicketManager::_applyTrackingDelta(
    PlayerId playerId, const std::unordered_set<u64>& oldChunks, const std::unordered_set<u64>& newChunks)
{
    std::vector<TrackingChangeEvent> trackingEvents;
    {
        std::lock_guard<std::mutex> lock(m_trackingPlayersMutex);

        for (u64 key : newChunks) {
            if (oldChunks.find(key) != oldChunks.end()) {
                continue;
            }
            m_chunkTrackingPlayers[key].insert(playerId);
            if (m_trackingChangeCallback) {
                ChunkCoord cx = static_cast<ChunkCoord>(key >> 32);
                ChunkCoord cz = static_cast<ChunkCoord>(key & 0xFFFFFFFF);
                trackingEvents.push_back({playerId, cx, cz, true});
            }
        }

        for (u64 key : oldChunks) {
            if (newChunks.find(key) != newChunks.end()) {
                continue;
            }
            auto it = m_chunkTrackingPlayers.find(key);
            if (it != m_chunkTrackingPlayers.end()) {
                it->second.erase(playerId);
                if (it->second.empty()) {
                    m_chunkTrackingPlayers.erase(it);
                }
            }

            if (m_trackingChangeCallback) {
                ChunkCoord cx = static_cast<ChunkCoord>(key >> 32);
                ChunkCoord cz = static_cast<ChunkCoord>(key & 0xFFFFFFFF);
                trackingEvents.push_back({playerId, cx, cz, false});
            }
        }
    }

    for (const auto& event : trackingEvents) {
        m_trackingChangeCallback(event.playerId, event.x, event.z, event.isTracking);
    }
}

void ChunkLoadTicketManager::_rebuildAllPlayerSources()
{
    for (const auto& [key, count] : m_playerSourceCounts) {
        MC_UNUSED(count);
        m_dirtyChunks.insert(key);
    }
    m_playerSourceCounts.clear();

    for (auto& [playerId, state] : m_playerStates) {
        if (!state.hasPosition) {
            continue;
        }

        const std::unordered_set<u64> oldTrackedChunks = state.trackedChunks;
        state.trackedChunks = _buildTrackedChunkSet(state.center.x, state.center.z);
        _applyTrackingDelta(playerId, oldTrackedChunks, state.trackedChunks);

        const u64 key = _posToKey(state.center.x, state.center.z);
        ++m_playerSourceCounts[key];
        m_dirtyChunks.insert(key);
    }
}

const ChunkTicketSet* ChunkLoadTicketManager::getChunkTickets(ChunkCoord x, ChunkCoord z) const
{
    u64 key = _posToKey(x, z);
    auto it = m_chunkTickets.find(key);
    if (it != m_chunkTickets.end()) {
        return &it->second;
    }
    return nullptr;
}

std::vector<PlayerId> ChunkLoadTicketManager::getTrackingPlayers(ChunkCoord x, ChunkCoord z) const
{
    u64 key = _posToKey(x, z);
    std::lock_guard<std::mutex> lock(m_trackingPlayersMutex);

    std::vector<PlayerId> result;
    auto it = m_chunkTrackingPlayers.find(key);
    if (it != m_chunkTrackingPlayers.end()) {
        result.assign(it->second.begin(), it->second.end());
    }
    return result;
}

bool ChunkLoadTicketManager::isPlayerTracking(PlayerId playerId, ChunkCoord x, ChunkCoord z) const
{
    u64 key = _posToKey(x, z);
    std::lock_guard<std::mutex> lock(m_trackingPlayersMutex);

    auto it = m_chunkTrackingPlayers.find(key);
    if (it == m_chunkTrackingPlayers.end()) {
        return false;
    }
    return it->second.find(playerId) != it->second.end();
}

bool ChunkLoadTicketManager::hasTrackingPlayers(u64 chunkKey) const
{
    std::lock_guard<std::mutex> lock(m_trackingPlayersMutex);

    auto it = m_chunkTrackingPlayers.find(chunkKey);
    return it != m_chunkTrackingPlayers.end() && !it->second.empty();
}

std::vector<ChunkPos> ChunkLoadTicketManager::getForcedChunks() const
{
    std::vector<ChunkPos> result;

    // 遍历所有票据集合，找出包含 FORCED 票据的区块
    for (const auto& [key, ticketSet] : m_chunkTickets) {
        for (const auto& ticket : ticketSet.tickets()) {
            if (ticket.typeName() == "forced") {
                ChunkCoord x = static_cast<ChunkCoord>(key >> 32);
                ChunkCoord z = static_cast<ChunkCoord>(key & 0xFFFFFFFF);
                result.emplace_back(x, z);
                break; // 一个区块只需添加一次
            }
        }
    }

    return result;
}

bool ChunkLoadTicketManager::isForcedChunk(ChunkCoord x, ChunkCoord z) const
{
    const ChunkTicketSet* tickets = getChunkTickets(x, z);
    if (tickets == nullptr) {
        return false;
    }

    for (const auto& ticket : tickets->tickets()) {
        if (ticket.typeName() == "forced") {
            return true;
        }
    }

    return false;
}

} // namespace mc::world::chunk
