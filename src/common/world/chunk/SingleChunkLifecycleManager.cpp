#include "SingleChunkLifecycleManager.hpp"
#include "common/util/assert/AssertAll.hpp"
#include <chrono>
#include <algorithm>

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
        if (status == ChunkStatuses::FULL) {
            m_lifecycleState.store(ChunkLifecycleState::Ready, std::memory_order_release);
        }

        // 回调
        if (m_statusChangeCallback) {
            m_statusChangeCallback(*this);
        }
    }
}

bool SingleChunkLifecycleManager::upgradeGenTarget(const ChunkStatus& target)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    MC_ASSERT_RELEASE_MSG(m_requestTarget != nullptr, "Request target should be initialized");

    // 只有新目标更高时才升级
    if (target.ordinal() > m_requestTarget->ordinal()) {
        m_requestTarget = &target;
        return true;
    }
    return false;
}

void SingleChunkLifecycleManager::setLevel(i32 level)
{
    const i32 oldLevel = m_level.exchange(level, std::memory_order_acq_rel);

    if (oldLevel != level && m_levelChangeCallback) {
        m_levelChangeCallback(*this);
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

    ChunkRequestControl control;

    if (!noActive && !targetUpgrade && !priorityUpgrade) {
        control.generation = m_requestGeneration;
        control.priority = m_requestPriority;
        control.targetStatus = m_requestTarget;
        control.cancelToken = m_cancelToken;
        control.shouldEnqueue = false;
        return control;
    }

    if (m_cancelToken) {
        m_cancelToken->store(true, std::memory_order_release);
    }

    ++m_requestGeneration;
    m_requestPriority = priority;
    m_requestTarget = &targetStatus;
    m_cancelToken = std::make_shared<std::atomic<bool>>(false);
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
        m_lifecycleState.store(ChunkLifecycleState::Cancelled, std::memory_order_release);
        return;
    }

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
    m_lifecycleState.store(ChunkLifecycleState::Cancelled, std::memory_order_release);
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
// 状态消费者回调 (Moonrise 风格)
// ============================================================================

void SingleChunkLifecycleManager::addStatusConsumer(const ChunkStatus& status, StatusConsumer consumer)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    const i32 ordinal = status.ordinal();
    if (ordinal >= 0 && ordinal < ChunkStatuses::COUNT) {
        // 如果已经完成该状态，立即调用
        if (m_status->isAtLeast(status)) {
            if (consumer) {
                consumer(m_generatingChunk.get());
            }
        } else {
            m_statusConsumers[ordinal].push_back(std::move(consumer));
        }
    }
}

void SingleChunkLifecycleManager::addFullStatusConsumer(FullChunkStatus status, FullStatusConsumer consumer)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    const i32 statusValue = static_cast<i32>(status);
    m_fullStatusConsumers[statusValue].push_back(std::move(consumer));
}

void SingleChunkLifecycleManager::notifyStatusComplete(const ChunkStatus& status, ChunkPrimer* primer)
{
    std::vector<StatusConsumer> consumers;

    {
        std::lock_guard<std::mutex> lock(m_mutex);

        const i32 ordinal = status.ordinal();
        if (ordinal >= 0 && ordinal < ChunkStatuses::COUNT) {
            // 获取并清除消费者
            auto it = m_statusConsumers.find(ordinal);
            if (it != m_statusConsumers.end()) {
                consumers = std::move(it->second);
                m_statusConsumers.erase(it);
            }
        }
    }

    // 在锁外调用消费者
    for (auto& consumer : consumers) {
        if (consumer) {
            consumer(primer);
        }
    }
}

// ============================================================================
// 优先级管理
// ============================================================================

void SingleChunkLifecycleManager::raisePriority(i32 priority)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    // 只有新优先级更高（数值更小）时才更新
    if (priority < m_requestPriority) {
        m_requestPriority = priority;
    }
}

void SingleChunkLifecycleManager::setPriorityValue(i32 priority)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_requestPriority = priority;
}

void SingleChunkLifecycleManager::lowerPriority(i32 priority)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    // 只有新优先级更低（数值更大）时才更新
    if (priority > m_requestPriority) {
        m_requestPriority = priority;
    }
}

i32 SingleChunkLifecycleManager::getEffectivePriority(i32 defaultPriority) const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_requestPriority != 0 ? m_requestPriority : defaultPriority;
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
