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

#include "ChunkLoadTicketManager.hpp"
#include "ChunkLoadTicket.hpp"
#include "common/perfetto/TraceEvents.hpp"
#include <algorithm>
#include <mutex>
#include <spdlog/spdlog.h>

namespace {

struct TrackingChangeEvent {
    mc::PlayerId playerId;
    mc::ChunkCoord x;
    mc::ChunkCoord z;
    bool isTracking;
};

} // namespace

namespace mc::world {

// ============================================================================
// 预定义票据类型
// ============================================================================

namespace TicketTypes {
// 玩家加载票据
const ChunkLoadTicketType<ChunkPos> PLAYER = ChunkLoadTicketType<ChunkPos>::create("player");

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

void initializeTicketTypes()
{
    // 票据类型已在静态初始化时创建，此函数保留用于未来扩展
}
} // namespace TicketTypes

// ============================================================================
// ChunkTicketSet 实现
// ============================================================================

void ChunkTicketSet::addTicket(ChunkLoadTicket ticket)
{
    // 检查是否已存在相同的票据
    for (const auto& t : m_tickets) {
        if (t == ticket) {
            return; // 已存在，不重复添加
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

i32 ChunkTicketSet::getMinLevel() const
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
    // 设置距离图回调（用于强制加载票据等）
    m_distanceGraph.setLevelChangeCallback([this](ChunkCoord x, ChunkCoord z, i32 oldLevel, i32 newLevel) {
        if (m_levelChangeCallback) {
            m_levelChangeCallback(x, z, oldLevel, newLevel);
        }
    });
}

void ChunkLoadTicketManager::setupTrackerCallback(PlayerChunkTracker* tracker)
{
    tracker->setLevelChangeCallback([this](ChunkCoord x, ChunkCoord z, i32 oldLevel, i32 newLevel) {
        if (m_levelChangeCallback) {
            m_levelChangeCallback(x, z, oldLevel, newLevel);
        }
    });
}

void ChunkLoadTicketManager::addTicket(ChunkPos pos, ChunkLoadTicket ticket)
{
    u64 key = posToKey(pos.x, pos.z);

    auto& ticketSet = m_chunkTickets[key];
    ticketSet.addTicket(std::move(ticket));

    // 标记区块为脏
    m_dirtyChunks.insert(key);
}

void ChunkLoadTicketManager::removeTicket(ChunkPos pos, const ChunkLoadTicket& ticket)
{
    u64 key = posToKey(pos.x, pos.z);

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

    // 获取旧的追踪区块集合
    std::unordered_set<u64> oldChunks;
    auto trackerIt = m_playerTrackers.find(playerId);
    if (trackerIt != m_playerTrackers.end()) {
        oldChunks = trackerIt->second->chunksInRange();
    }

    // 检查玩家是否已存在
    auto posIt = m_playerPositions.find(playerId);
    if (posIt != m_playerPositions.end()) {
        ChunkPos oldPos = posIt->second;

        if (oldPos.x == x && oldPos.z == z) {
            return; // 位置没变
        }

        // 移除旧位置的票据
        ChunkLoadTicket oldTicket(TicketTypes::PLAYER, PLAYER_TICKET_LEVEL, oldPos);
        removeTicket(oldPos, oldTicket);

        // 更新玩家追踪器位置
        if (trackerIt != m_playerTrackers.end()) {
            trackerIt->second->setPlayerPosition(x, z);
        }
    } else {
        // 创建新的玩家追踪器
        auto tracker = std::make_unique<PlayerChunkTracker>(m_viewDistance);
        setupTrackerCallback(tracker.get());
        tracker->setPlayerPosition(x, z);
        m_playerTrackers[playerId] = std::move(tracker);
    }

    // 更新玩家位置
    m_playerPositions[playerId] = newPos;

    // 添加新位置的票据
    ChunkLoadTicket newTicket(TicketTypes::PLAYER, PLAYER_TICKET_LEVEL, newPos);
    addTicket(newPos, newTicket);

    // 获取新的追踪区块集合
    std::unordered_set<u64> newChunks;
    trackerIt = m_playerTrackers.find(playerId);
    if (trackerIt != m_playerTrackers.end()) {
        newChunks = trackerIt->second->chunksInRange();
    }

    // 计算差异并更新区块追踪映射
    std::vector<TrackingChangeEvent> trackingEvents;
    {
        std::lock_guard<std::mutex> lock(m_trackingPlayersMutex);

        // 进入的区块
        for (u64 key : newChunks) {
            if (oldChunks.find(key) == oldChunks.end()) {
                m_chunkTrackingPlayers[key].insert(playerId);

                // 先收集变化，锁外再派发回调，避免回调里再次查询 tracking 数据时自锁
                if (m_trackingChangeCallback) {
                    ChunkCoord cx = static_cast<ChunkCoord>(key >> 32);
                    ChunkCoord cz = static_cast<ChunkCoord>(key & 0xFFFFFFFF);
                    trackingEvents.push_back({playerId, cx, cz, true});
                }
            }
        }

        // 离开的区块
        for (u64 key : oldChunks) {
            if (newChunks.find(key) == newChunks.end()) {
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
    }

    for (const auto& event : trackingEvents) {
        m_trackingChangeCallback(event.playerId, event.x, event.z, event.isTracking);
    }

    // 处理更新
    processUpdates();
}

void ChunkLoadTicketManager::removePlayer(PlayerId playerId)
{
    // 获取玩家追踪的区块
    std::unordered_set<u64> trackedChunks;
    auto trackerIt = m_playerTrackers.find(playerId);
    if (trackerIt != m_playerTrackers.end()) {
        trackedChunks = trackerIt->second->chunksInRange();
    }

    // 从区块追踪映射中移除玩家，触发离开回调
    std::vector<TrackingChangeEvent> trackingEvents;
    {
        std::lock_guard<std::mutex> lock(m_trackingPlayersMutex);
        for (u64 key : trackedChunks) {
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

    // 移除玩家票据
    auto posIt = m_playerPositions.find(playerId);
    if (posIt != m_playerPositions.end()) {
        ChunkPos pos = posIt->second;
        ChunkLoadTicket ticket(TicketTypes::PLAYER, PLAYER_TICKET_LEVEL, pos);
        removeTicket(pos, ticket);
        m_playerPositions.erase(posIt);
    }

    // 移除玩家追踪器
    m_playerTrackers.erase(playerId);

    // 处理更新
    processUpdates();
}

i32 ChunkLoadTicketManager::getChunkLevel(ChunkCoord x, ChunkCoord z) const
{
    u64 key = posToKey(x, z);

    // 首先检查票据集合
    auto it = m_chunkTickets.find(key);
    if (it != m_chunkTickets.end()) {
        i32 ticketLevel = it->second.getMinLevel();
        if (ticketLevel < ChunkDistanceGraph::MAX_LEVEL) {
            return ticketLevel;
        }
    }

    // 检查所有玩家追踪器，找到最低级别
    i32 minLevel = ChunkDistanceGraph::MAX_LEVEL;
    for (const auto& [playerId, tracker] : m_playerTrackers) {
        i32 level = tracker->getLevel(x, z);
        if (level < minLevel) {
            minLevel = level;
        }
    }

    if (minLevel < ChunkDistanceGraph::MAX_LEVEL) {
        return minLevel;
    }

    // 最后检查距离图（用于强制加载等票据）
    return m_distanceGraph.getLevel(x, z);
}

void ChunkLoadTicketManager::tick()
{
    ++m_currentTime;

    // 清理过期票据
    for (auto& [key, ticketSet] : m_chunkTickets) {
        ticketSet.removeExpired(m_currentTime);
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
    const i32 clampedDistance = std::clamp(distance, 2, 32);

    if (m_viewDistance == clampedDistance) {
        return;
    }

    m_viewDistance = clampedDistance;

    // 更新所有玩家追踪器并处理追踪变化
    for (auto& [playerId, tracker] : m_playerTrackers) {
        // 获取旧的追踪区块
        std::unordered_set<u64> oldChunks = tracker->chunksInRange();

        // 更新视距
        tracker->setViewDistance(clampedDistance);

        // 获取新的追踪区块
        std::unordered_set<u64> newChunks = tracker->chunksInRange();

        // 计算差异并更新区块追踪映射
        std::vector<TrackingChangeEvent> trackingEvents;
        {
            std::lock_guard<std::mutex> lock(m_trackingPlayersMutex);

            // 进入的区块
            for (u64 key : newChunks) {
                if (oldChunks.find(key) == oldChunks.end()) {
                    m_chunkTrackingPlayers[key].insert(playerId);

                    if (m_trackingChangeCallback) {
                        ChunkCoord cx = static_cast<ChunkCoord>(key >> 32);
                        ChunkCoord cz = static_cast<ChunkCoord>(key & 0xFFFFFFFF);
                        trackingEvents.push_back({playerId, cx, cz, true});
                    }
                }
            }

            // 离开的区块
            for (u64 key : oldChunks) {
                if (newChunks.find(key) == newChunks.end()) {
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
        }

        for (const auto& event : trackingEvents) {
            m_trackingChangeCallback(event.playerId, event.x, event.z, event.isTracking);
        }
    }

    // 处理更新
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
        addTicket(pos, ticket);
    } else {
        removeTicket(pos, ticket);
    }

    processUpdates();
}

void ChunkLoadTicketManager::processUpdates()
{
    MC_TRACE_EVENT("server.chunk", "ChunkLoadTicketManager::processUpdates", "dirtyChunkCount", m_dirtyChunks.size(), "playerCount", m_playerTrackers.size());

    // 处理所有玩家追踪器的更新
    for (auto& [playerId, tracker] : m_playerTrackers) {
        tracker->processUpdates(1000);
    }

    // 更新脏区块的级别
    for (u64 key : m_dirtyChunks) {
        ChunkCoord x = static_cast<ChunkCoord>(key >> 32);
        ChunkCoord z = static_cast<ChunkCoord>(key & 0xFFFFFFFF);

        // 获取票据的最低级别
        auto it = m_chunkTickets.find(key);
        i32 ticketLevel = ChunkDistanceGraph::MAX_LEVEL;
        if (it != m_chunkTickets.end()) {
            ticketLevel = it->second.getMinLevel();
        }

        // 更新距离图
        bool isDecreasing = ticketLevel < m_distanceGraph.getLevel(x, z);
        m_distanceGraph.updateSourceLevel(x, z, ticketLevel, isDecreasing);
    }
    m_dirtyChunks.clear();

    // 处理距离图更新
    m_distanceGraph.processUpdates(1000); // 每次最多处理 1000 个更新
}

const ChunkTicketSet* ChunkLoadTicketManager::getChunkTickets(ChunkCoord x, ChunkCoord z) const
{
    u64 key = posToKey(x, z);
    auto it = m_chunkTickets.find(key);
    if (it != m_chunkTickets.end()) {
        return &it->second;
    }
    return nullptr;
}

std::vector<PlayerId> ChunkLoadTicketManager::getTrackingPlayers(ChunkCoord x, ChunkCoord z) const
{
    u64 key = posToKey(x, z);
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
    u64 key = posToKey(x, z);
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

} // namespace mc::world
