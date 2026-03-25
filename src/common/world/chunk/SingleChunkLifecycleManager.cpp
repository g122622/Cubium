#include "SingleChunkLifecycleManager.hpp"
#include <chrono>

namespace mc {

// ============================================================================
// SingleChunkLifecycleManager 实现
// ============================================================================

SingleChunkLifecycleManager::SingleChunkLifecycleManager(ChunkCoord x, ChunkCoord z)
    : m_x(x)
    , m_z(z)
{
    // 初始化 Future
    for (i32 i = 0; i < ChunkStatuses::COUNT; ++i) {
        m_futureInitialized[i] = false;
    }
}

// ============================================================================
// 状态管理
// ============================================================================

void SingleChunkLifecycleManager::setStatus(const ChunkStatus& status)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_status != &status) {
        m_status = &status;

        // 回调
        if (m_statusChangeCallback) {
            m_statusChangeCallback(*this);
        }
    }
}

void SingleChunkLifecycleManager::setLevel(i32 level)
{
    const i32 oldLevel = m_level.exchange(level, std::memory_order_acq_rel);

    if (oldLevel != level && m_levelChangeCallback) {
        m_levelChangeCallback(*this);
    }
}

// ============================================================================
// 区块数据访问
// ============================================================================

ChunkPrimer* SingleChunkLifecycleManager::createGeneratingChunk()
{
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!m_generatingChunk) {
        m_generatingChunk = std::make_unique<ChunkPrimer>(m_x, m_z);
    }

    return m_generatingChunk.get();
}

std::unique_ptr<ChunkData> SingleChunkLifecycleManager::completeGeneration()
{
    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_generatingChunk) {
        m_chunkData = m_generatingChunk->toChunkData();
        m_generatingChunk.reset();
        m_status = &ChunkStatuses::FULL;
    }

    return std::move(m_chunkData);
}

// ============================================================================
// Future 管理
// ============================================================================

SingleChunkLifecycleManager::ChunkFuture SingleChunkLifecycleManager::getChunkFuture(const ChunkStatus& status)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    const i32 ordinal = status.ordinal();

    if (ordinal < 0 || ordinal >= ChunkStatuses::COUNT) {
        // 返回错误 Future
        std::promise<ChunkResult> errorPromise;
        errorPromise.set_value(Error::GenerationFailed);
        return errorPromise.get_future().share();
    }

    // 如果已经完成
    if (m_status->isAtLeast(status)) {
        std::promise<ChunkResult> readyPromise;
        readyPromise.set_value(m_generatingChunk.get());
        return readyPromise.get_future().share();
    }

    // 初始化 Future
    if (!m_futureInitialized[ordinal]) {
        m_futures[ordinal] = m_promises[ordinal].get_future().share();
        m_futureInitialized[ordinal] = true;
    }

    return m_futures[ordinal];
}

void SingleChunkLifecycleManager::completeFuture(const ChunkStatus& status, ChunkPrimer* chunk)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    const i32 ordinal = status.ordinal();

    if (ordinal >= 0 && ordinal < ChunkStatuses::COUNT) {
        if (m_futureInitialized[ordinal]) {
            m_promises[ordinal].set_value(chunk);
        } else {
            // 如果 Future 未初始化，设置一个已完成的值
            m_promises[ordinal].set_value(chunk);
            m_futures[ordinal] = m_promises[ordinal].get_future().share();
            m_futureInitialized[ordinal] = true;
        }
    }
}

void SingleChunkLifecycleManager::failFuture(const ChunkStatus& status, Error error)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    const i32 ordinal = status.ordinal();

    if (ordinal >= 0 && ordinal < ChunkStatuses::COUNT) {
        if (m_futureInitialized[ordinal]) {
            m_promises[ordinal].set_value(error);
        }
    }
}

// ============================================================================
// 票据管理
// ============================================================================

void SingleChunkLifecycleManager::addTicket(const ChunkLoadTicket& ticket)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_tickets.push_back(ticket);
}

void SingleChunkLifecycleManager::removeTicket(const ChunkLoadTicket& ticket)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = std::find_if(m_tickets.begin(), m_tickets.end(),
        [&ticket](const ChunkLoadTicket& t) {
            // 比较票据类型和值
            return t.typeName() == ticket.typeName() &&
                   t.chunkValue() == ticket.chunkValue() &&
                   t.level() == ticket.level();
        });

    if (it != m_tickets.end()) {
        m_tickets.erase(it);
    }
}

// ============================================================================
// 玩家追踪
// ============================================================================

void SingleChunkLifecycleManager::addTrackingPlayer(PlayerId player)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_trackingPlayers.insert(player);
}

void SingleChunkLifecycleManager::removeTrackingPlayer(PlayerId player)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_trackingPlayers.erase(player);
}

} // namespace mc
