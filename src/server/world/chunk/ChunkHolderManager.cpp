#include "server/world/chunk/ChunkHolderManager.hpp"
#include "server/world/ServerWorld.hpp"
#include "common/world/chunk/ChunkData.hpp"
#include "common/world/chunk/ChunkLoadTicket.hpp"
#include "common/util/assert/AssertAll.hpp"
#include <algorithm>

namespace mc {

ChunkHolderManager::ChunkHolderManager(server::ServerWorld* world, world::ThreadedTicketLevelPropagator& ticketPropagator)
    : m_world(world)
    , m_ticketPropagator(ticketPropagator)
    , m_ticketLock(6)  // 64x64 区块为一个区域
{
    // 设置票据级别变化回调
    m_ticketPropagator.setLevelChangeCallback(
        [this](i32 x, i32 z, i32 oldLevel, i32 newLevel) {
            onTicketLevelChanged(x, z, oldLevel, newLevel);
        });
}

// ============================================================================
// 区块持有者访问
// ============================================================================

SingleChunkLifecycleManager* ChunkHolderManager::getChunkHolder(ChunkCoord x, ChunkCoord z) {
    std::lock_guard<std::mutex> lock(m_holdersMutex);

    u64 key = makeKey(x, z);
    auto it = m_holders.find(key);
    if (it == m_holders.end()) {
        return nullptr;
    }
    return it->second.get();
}

const SingleChunkLifecycleManager* ChunkHolderManager::getChunkHolder(ChunkCoord x, ChunkCoord z) const {
    std::lock_guard<std::mutex> lock(m_holdersMutex);

    u64 key = makeKey(x, z);
    auto it = m_holders.find(key);
    if (it == m_holders.end()) {
        return nullptr;
    }
    return it->second.get();
}

SingleChunkLifecycleManager* ChunkHolderManager::getOrCreateChunkHolder(ChunkCoord x, ChunkCoord z) {
    std::lock_guard<std::mutex> lock(m_holdersMutex);

    u64 key = makeKey(x, z);
    auto it = m_holders.find(key);
    if (it != m_holders.end()) {
        return it->second.get();
    }

    // 创建新的持有者
    auto holder = std::make_unique<SingleChunkLifecycleManager>(x, z);
    auto* ptr = holder.get();
    m_holders[key] = std::move(holder);

    return ptr;
}

bool ChunkHolderManager::hasChunkHolder(ChunkCoord x, ChunkCoord z) const {
    std::lock_guard<std::mutex> lock(m_holdersMutex);

    u64 key = makeKey(x, z);
    return m_holders.find(key) != m_holders.end();
}

size_t ChunkHolderManager::holderCount() const {
    std::lock_guard<std::mutex> lock(m_holdersMutex);
    return m_holders.size();
}

void ChunkHolderManager::forEachHolder(const std::function<void(SingleChunkLifecycleManager&)>& callback) {
    std::lock_guard<std::mutex> lock(m_holdersMutex);

    for (auto& [key, holder] : m_holders) {
        if (holder) {
            callback(*holder);
        }
    }
}

void ChunkHolderManager::forEachHolder(const std::function<void(const SingleChunkLifecycleManager&)>& callback) const {
    std::lock_guard<std::mutex> lock(m_holdersMutex);

    for (const auto& [key, holder] : m_holders) {
        if (holder) {
            callback(*holder);
        }
    }
}

// ============================================================================
// 票据操作
// ============================================================================

void ChunkHolderManager::addTicket(ChunkCoord x, ChunkCoord z, i32 level, const String& ticketType) {
    MC_ASSERT(level >= 1 && level <= 62);

    // 设置票据源
    m_ticketPropagator.setSource(x, z, level);

    // 添加到持有者
    auto* holder = getOrCreateChunkHolder(x, z);
    if (holder) {
        // 创建一个简单的票据
        // 注意: 使用 PLAYER 类型创建票据，级别由参数指定
        ChunkLoadTicket ticket(world::TicketTypes::PLAYER, level, ChunkPos(x, z));
        holder->addTicket(ticket);
    }

    m_hasPendingUpdates.store(true, std::memory_order_release);
}

void ChunkHolderManager::removeTicket(ChunkCoord x, ChunkCoord z, i32 level, const String& ticketType) {
    MC_ASSERT(level >= 1 && level <= 62);

    // 移除票据源
    m_ticketPropagator.removeSource(x, z);

    // 从持有者移除
    auto* holder = getChunkHolder(x, z);
    if (holder) {
        ChunkLoadTicket ticket(world::TicketTypes::PLAYER, level, ChunkPos(x, z));
        holder->removeTicket(ticket);
    }

    m_hasPendingUpdates.store(true, std::memory_order_release);
}

bool ChunkHolderManager::processTicketUpdates() {
    if (!m_hasPendingUpdates.exchange(false, std::memory_order_acq_rel)) {
        return false;
    }

    // 遍历所有 Section 处理更新
    bool hasUpdates = false;
    for (i32 sectionX = -10; sectionX <= 10; ++sectionX) {
        for (i32 sectionZ = -10; sectionZ <= 10; ++sectionZ) {
            std::vector<std::pair<u64, u8>> updatedPositions;
            if (m_ticketPropagator.performUpdate(sectionX, sectionZ, m_ticketLock, updatedPositions)) {
                hasUpdates = true;
            }
        }
    }

    return hasUpdates;
}

bool ChunkHolderManager::processTicketUpdates(i32 sectionX, i32 sectionZ) {
    std::vector<std::pair<u64, u8>> updatedPositions;
    bool hasUpdates = m_ticketPropagator.performUpdate(sectionX, sectionZ, m_ticketLock, updatedPositions);

    if (!updatedPositions.empty()) {
        for (const auto& [key, level] : updatedPositions) {
            ChunkCoord x, z;
            fromKey(key, x, z);

            auto* holder = getChunkHolder(x, z);
            if (holder) {
                holder->setLevel(static_cast<i32>(level));
            }
        }
    }

    return hasUpdates;
}

// ============================================================================
// 卸载队列
// ============================================================================

void ChunkHolderManager::queueUnload(SingleChunkLifecycleManager* holder) {
    if (!holder) {
        return;
    }

    std::lock_guard<std::mutex> lock(m_unloadMutex);

    // 检查是否已在队列中
    auto it = std::find(m_unloadQueue.begin(), m_unloadQueue.end(), holder);
    if (it == m_unloadQueue.end()) {
        holder->setQueuedForUnload(true);
        m_unloadQueue.push_back(holder);
    }
}

size_t ChunkHolderManager::processUnloadQueue(size_t maxCount) {
    std::vector<SingleChunkLifecycleManager*> toUnload;

    {
        std::lock_guard<std::mutex> lock(m_unloadMutex);

        // 收集需要卸载的区块
        size_t count = 0;
        for (auto it = m_unloadQueue.begin(); it != m_unloadQueue.end() && count < maxCount; ) {
            auto* holder = *it;

            // 检查是否仍然应该卸载
            if (holder && !holder->shouldLoad() && !holder->hasTickets()) {
                toUnload.push_back(holder);
                it = m_unloadQueue.erase(it);
                ++count;
            } else {
                if (holder) {
                    holder->setQueuedForUnload(false);
                }
                it = m_unloadQueue.erase(it);
            }
        }
    }

    // 执行卸载
    for (auto* holder : toUnload) {
        if (!holder) {
            continue;
        }

        // 从持有者映射中移除
        std::lock_guard<std::mutex> lock(m_holdersMutex);

        u64 key = makeKey(holder->x(), holder->z());
        m_holders.erase(key);
    }

    return toUnload.size();
}

size_t ChunkHolderManager::unloadQueueSize() const {
    std::lock_guard<std::mutex> lock(m_unloadMutex);
    return m_unloadQueue.size();
}

void ChunkHolderManager::clearUnloadQueue() {
    std::lock_guard<std::mutex> lock(m_unloadMutex);

    for (auto* holder : m_unloadQueue) {
        if (holder) {
            holder->setQueuedForUnload(false);
        }
    }
    m_unloadQueue.clear();
}

// ============================================================================
// 私有方法
// ============================================================================

void ChunkHolderManager::onTicketLevelChanged(ChunkCoord x, ChunkCoord z, i32 oldLevel, i32 newLevel) {
    // 更新持有者级别
    auto* holder = getChunkHolder(x, z);
    if (holder) {
        holder->setLevel(newLevel);
    }

    // 调用回调
    if (m_levelChangeCallback) {
        m_levelChangeCallback(x, z, oldLevel, newLevel);
    }

    // 如果级别变为卸载级别，加入卸载队列
    if (newLevel > 33 && holder && !holder->hasTickets()) {
        queueUnload(holder);
    }
}

} // namespace mc
