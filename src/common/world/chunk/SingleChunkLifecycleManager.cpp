#include "SingleChunkLifecycleManager.hpp"
#include <chrono>
#include "common/perfetto/TraceEvents.hpp"

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
    MC_TRACE_EVENT("server.initialization", "ServerChunkManager::initialize");

    GenerationCallback statusCallback;
    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_status != &status) {
        m_status = &status;
        if (status == ChunkStatuses::FULL) {
            m_lifecycleState.store(ChunkLifecycleState::Ready, std::memory_order_release);
        }

        statusCallback = m_statusChangeCallback;
    }

    if (statusCallback) {
        statusCallback(*this);
    }
}

void SingleChunkLifecycleManager::setLevel(i32 level)
{
    GenerationCallback levelCallback;
    const i32 oldLevel = m_level.exchange(level, std::memory_order_acq_rel);

    if (oldLevel != level) {
        levelCallback = m_levelChangeCallback;
    }

    if (levelCallback) {
        levelCallback(*this);
    }
}

bool SingleChunkLifecycleManager::hasActiveRequest() const
{
    const ChunkLifecycleState state = m_lifecycleState.load(std::memory_order_acquire);
    return state == ChunkLifecycleState::Queued || state == ChunkLifecycleState::Generating;
}

ChunkRequestControl SingleChunkLifecycleManager::upsertRequest(const ChunkStatus& targetStatus, i32 priority)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    const ChunkLifecycleState state = m_lifecycleState.load(std::memory_order_acquire);
    const bool noActive = (state != ChunkLifecycleState::Queued && state != ChunkLifecycleState::Generating);
    const bool targetUpgrade = targetStatus.ordinal() > m_requestTarget->ordinal();
    const bool priorityUpgrade = noActive || priority < m_requestPriority;
    const bool pendingButNotSubmitted = (state == ChunkLifecycleState::Queued && !m_requestSubmitted);

    ChunkRequestControl control;

    if (!noActive && !targetUpgrade && !priorityUpgrade) {
        control.generation = m_requestGeneration;
        control.priority = m_requestPriority;
        control.targetStatus = m_requestTarget;
        control.cancelToken = m_cancelToken;
        control.shouldEnqueue = pendingButNotSubmitted;
        return control;
    }

    if (m_cancelToken) {
        m_cancelToken->store(true, std::memory_order_release);
    }

    ++m_requestGeneration;
    m_requestPriority = priority;
    m_requestTarget = &targetStatus;
    m_cancelToken = std::make_shared<std::atomic<bool>>(false);
    m_requestSubmitted = false;
    m_lifecycleState.store(ChunkLifecycleState::Queued, std::memory_order_release);

    control.generation = m_requestGeneration;
    control.priority = m_requestPriority;
    control.targetStatus = m_requestTarget;
    control.cancelToken = m_cancelToken;
    control.shouldEnqueue = true;
    return control;
}

bool SingleChunkLifecycleManager::tryStartRequest(u64 generation)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (generation != m_requestGeneration) {
        return false;
    }

    if (m_cancelToken && m_cancelToken->load(std::memory_order_acquire)) {
        m_lifecycleState.store(ChunkLifecycleState::Cancelled, std::memory_order_release);
        return false;
    }

    m_lifecycleState.store(ChunkLifecycleState::Generating, std::memory_order_release);
    return true;
}

void SingleChunkLifecycleManager::finishRequest(u64 generation, bool success, bool cancelled)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (generation != m_requestGeneration) {
        return;
    }

    if (cancelled) {
        m_requestSubmitted = false;
        m_lifecycleState.store(ChunkLifecycleState::Cancelled, std::memory_order_release);
        return;
    }

    m_requestSubmitted = false;
    m_lifecycleState.store(success ? ChunkLifecycleState::Ready : ChunkLifecycleState::Failed,
                           std::memory_order_release);
}

void SingleChunkLifecycleManager::cancelActiveRequest()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_cancelToken) {
        m_cancelToken->store(true, std::memory_order_release);
    }

    ++m_requestGeneration;
    m_requestSubmitted = false;
    m_lifecycleState.store(ChunkLifecycleState::Cancelled, std::memory_order_release);
}

bool SingleChunkLifecycleManager::tryMarkRequestSubmitted(u64 generation)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (generation == m_requestGeneration && !m_requestSubmitted) {
        m_requestSubmitted = true;
        return true;
    }

    return false;
}

bool SingleChunkLifecycleManager::isGenerationCurrent(u64 generation) const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return generation == m_requestGeneration;
}

u64 SingleChunkLifecycleManager::requestGeneration() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_requestGeneration;
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

    m_lifecycleState.store(ChunkLifecycleState::Generating, std::memory_order_release);

    return m_generatingChunk.get();
}

std::unique_ptr<ChunkData> SingleChunkLifecycleManager::completeGeneration()
{
    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_generatingChunk) {
        m_chunkData = m_generatingChunk->toChunkData();
        m_generatingChunk.reset();
        m_status = &ChunkStatuses::FULL;
        m_lifecycleState.store(ChunkLifecycleState::Ready, std::memory_order_release);
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
