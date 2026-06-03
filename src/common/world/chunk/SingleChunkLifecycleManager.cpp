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

#include "SingleChunkLifecycleManager.hpp"
#include "../../util/assert/AssertAll.hpp"
#include <algorithm>
#include <limits>

namespace mc {

namespace {

[[nodiscard]] bool isHigherPriority(i32 lhs, i32 rhs)
{
    return lhs < rhs;
}

} // namespace

// ============================================================================
// 构造与基础访问
// ============================================================================

SingleChunkLifecycleManager::SingleChunkLifecycleManager(ChunkCoord x, ChunkCoord z)
    : m_x(x)
    , m_z(z)
{}

const ChunkStatus& SingleChunkLifecycleManager::status() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return *m_status;
}

bool SingleChunkLifecycleManager::hasCompletedStatus(const ChunkStatus& status) const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_status->isAtLeast(status);
}

SingleChunkLifecycleManager::SourceState SingleChunkLifecycleManager::sourceState() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_sourceState;
}

SingleChunkLifecycleManager::ExecutionState SingleChunkLifecycleManager::executionState() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_executionState;
}

bool SingleChunkLifecycleManager::isWaitingForNeighbors() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_executionState == ExecutionState::WaitingForNeighbors;
}

bool SingleChunkLifecycleManager::hasGeneratingChunk() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_generatingChunk != nullptr;
}

ChunkData* SingleChunkLifecycleManager::chunkData()
{
    return nullptr;
}

const ChunkData* SingleChunkLifecycleManager::chunkData() const
{
    return nullptr;
}

// ============================================================================
// 票据与玩家追踪
// ============================================================================

void SingleChunkLifecycleManager::addTicket(const ChunkLoadTicket& ticket)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_tickets.push_back(ticket);
}

void SingleChunkLifecycleManager::removeTicket(const ChunkLoadTicket& ticket)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = std::find_if(m_tickets.begin(), m_tickets.end(), [&ticket](const ChunkLoadTicket& current) {
        // 使用现有票据三元组判等，保持与旧行为一致。
        return current.typeName() == ticket.typeName() && current.chunkValue() == ticket.chunkValue() &&
            current.level() == ticket.level();
    });

    if (it != m_tickets.end()) {
        m_tickets.erase(it);
    }
}

size_t SingleChunkLifecycleManager::ticketCount() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_tickets.size();
}

bool SingleChunkLifecycleManager::hasTickets() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return !m_tickets.empty();
}

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

size_t SingleChunkLifecycleManager::trackingPlayerCount() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_trackingPlayers.size();
}

bool SingleChunkLifecycleManager::hasTrackingPlayers() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return !m_trackingPlayers.empty();
}

// ============================================================================
// 请求状态机
// ============================================================================

SingleChunkLifecycleManager::EnqueueDecision SingleChunkLifecycleManager::submitRequest(const ChunkStatus& targetStatus,
    i32 priority,
    std::function<void(bool, ChunkData*)> callback,
    std::shared_ptr<std::promise<ChunkData*>> promise)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    // 已经就绪时直接把等待者挂上，后续由调用方立即完成，不再走任何调度路径。
    if (m_sourceState == SourceState::Ready && m_status->isAtLeast(targetStatus)) {
        if (callback || promise) {
            m_waiters.push_back(Waiter{std::move(callback), std::move(promise)});
        }
        return buildDecisionLocked();
    }

    // 所有请求统一收敛到 lifecycle manager 内部，避免 ServerChunkManager 再维护一份并行状态。
    if (callback || promise) {
        m_waiters.push_back(Waiter{std::move(callback), std::move(promise)});
    }

    // 请求目标只允许单调提升；优先级采用更高优先级（数值更小）覆盖。
    if (targetStatus.ordinal() > m_requestedStatus->ordinal()) {
        m_requestedStatus = &targetStatus;
    }
    if (m_requestPriority == std::numeric_limits<i32>::max() || isHigherPriority(priority, m_requestPriority)) {
        m_requestPriority = priority;
    }

    // 第一次进入或取消后重新进入时，分配新的 generation 与取消令牌。
    if (!m_cancelToken) {
        ++m_requestGeneration;
        m_cancelToken = std::make_shared<std::atomic<bool>>(false);
    }

    // Ready 说明内存中已经有结果，但尚未通过上方快速路径命中，直接唤醒等待者即可。
    if (m_sourceState == SourceState::Ready) {
        return buildDecisionLocked();
    }

    // Unknown 是唯一允许触发一次"来源解析"的状态。
    if (m_sourceState == SourceState::Unknown) {
        m_sourceState = SourceState::ResolvingStorage;
        return buildDecisionLocked();
    }

    // 已确认存档不存在时，只剩生成链路。
    if (m_sourceState == SourceState::StorageMissing && m_executionState == ExecutionState::Idle) {
        m_executionState = ExecutionState::WaitingForNeighbors;
    }

    return buildDecisionLocked();
}

SingleChunkLifecycleManager::EnqueueDecision SingleChunkLifecycleManager::noteNeighborProgress(bool neighborsReady)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    // 邻居推进事件只允许影响"等待邻居"的执行状态，不得改写来源状态。
    if (m_sourceState != SourceState::StorageMissing) {
        return buildDecisionLocked();
    }

    if (m_executionState != ExecutionState::WaitingForNeighbors && m_executionState != ExecutionState::Idle) {
        return buildDecisionLocked();
    }

    m_executionState = neighborsReady ? ExecutionState::Queued : ExecutionState::WaitingForNeighbors;
    return buildDecisionLocked();
}

SingleChunkLifecycleManager::EnqueueDecision SingleChunkLifecycleManager::noteStorageResolved(bool foundInStorage)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    MC_ASSERT_RELEASE(m_sourceState == SourceState::ResolvingStorage);

    if (foundInStorage) {
        m_sourceState = SourceState::LoadedFromStorage;
        m_executionState = ExecutionState::Idle;
    } else {
        m_sourceState = SourceState::StorageMissing;
        m_executionState = ExecutionState::WaitingForNeighbors;
    }

    return buildDecisionLocked();
}

SingleChunkLifecycleManager::EnqueueDecision SingleChunkLifecycleManager::noteGenerationQueued(u64 generation)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (generation != m_requestGeneration) {
        return buildDecisionLocked();
    }

    m_submittedGeneration = generation;
    m_executionState = ExecutionState::Queued;
    return buildDecisionLocked();
}

SingleChunkLifecycleManager::EnqueueDecision SingleChunkLifecycleManager::noteGenerationStarted(u64 generation)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (generation != m_requestGeneration) {
        return buildDecisionLocked();
    }

    // worker 真正开始执行后，状态切换到 Generating；这一步只做状态登记，不处理结果。
    m_executionState = ExecutionState::Generating;
    return buildDecisionLocked();
}

SingleChunkLifecycleManager::EnqueueDecision SingleChunkLifecycleManager::noteGenerationFinished(
    u64 generation, CompletionState completionState)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (generation != m_requestGeneration) {
        return buildDecisionLocked();
    }

    if (completionState == CompletionState::Succeeded) {
        m_sourceState = SourceState::Ready;
        m_executionState = ExecutionState::Idle;
        m_requestPriority = std::numeric_limits<i32>::max();
        return buildDecisionLocked();
    }

    // 失败或取消后，保留"来源已确认缺失"的事实，只重置执行状态，允许后续重新请求。
    m_sourceState = SourceState::StorageMissing;
    m_executionState = ExecutionState::Idle;
    clearActiveGenerationLocked();
    return buildDecisionLocked();
}

SingleChunkLifecycleManager::EnqueueDecision SingleChunkLifecycleManager::cancelActiveWork()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    clearActiveGenerationLocked();
    m_executionState = ExecutionState::Idle;
    return buildDecisionLocked();
}

std::vector<SingleChunkLifecycleManager::Waiter> SingleChunkLifecycleManager::takeReadyWaiters()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_sourceState != SourceState::Ready) {
        return {};
    }

    std::vector<Waiter> waiters;
    waiters.swap(m_waiters);
    return waiters;
}

std::vector<SingleChunkLifecycleManager::Waiter> SingleChunkLifecycleManager::takeAllWaiters()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<Waiter> waiters;
    waiters.swap(m_waiters);
    return waiters;
}

// ============================================================================
// 区块数据状态
// ============================================================================

ChunkPrimer* SingleChunkLifecycleManager::createGeneratingChunk()
{
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!m_generatingChunk) {
        // 只有真正进入生成阶段时才创建 ChunkPrimer，避免"来源解析"和"等待邻居"阶段过早分配。
        m_generatingChunk = std::make_unique<ChunkPrimer>(m_x, m_z);
    }

    m_executionState = ExecutionState::Generating;
    return m_generatingChunk.get();
}

std::unique_ptr<ChunkData> SingleChunkLifecycleManager::completeGeneration()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    MC_ASSERT_RELEASE(m_generatingChunk != nullptr);

    auto chunkData = m_generatingChunk->toChunkData();
    m_generatingChunk.reset();
    m_status = &ChunkStatuses::FULL;
    m_sourceState = SourceState::Ready;
    m_executionState = ExecutionState::Idle;
    m_requestPriority = std::numeric_limits<i32>::max();
    return chunkData;
}

void SingleChunkLifecycleManager::markGenerationReady()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_generatingChunk.reset();
    m_status = &ChunkStatuses::FULL;
    m_sourceState = SourceState::Ready;
    m_executionState = ExecutionState::Idle;
    m_requestPriority = std::numeric_limits<i32>::max();
}

void SingleChunkLifecycleManager::markLoadedFromStorageReady()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    // 存档恢复完成后，真正的 ChunkData 所有权由外层缓存持有；
    // lifecycle manager 这里只记录"来源已解析并且区块已经就绪"。
    m_status = &ChunkStatuses::FULL;
    m_sourceState = SourceState::Ready;
    m_executionState = ExecutionState::Idle;
    m_requestPriority = std::numeric_limits<i32>::max();
}

void SingleChunkLifecycleManager::setStatus(const ChunkStatus& status)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_status = &status;
}

bool SingleChunkLifecycleManager::isGenerationCurrent(u64 generation) const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return generation == m_requestGeneration;
}

i32 SingleChunkLifecycleManager::requestPriority() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_requestPriority;
}

const ChunkStatus& SingleChunkLifecycleManager::requestedStatus() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return *m_requestedStatus;
}

std::shared_ptr<std::atomic<bool>> SingleChunkLifecycleManager::cancelToken() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_cancelToken;
}

// ============================================================================
// 内部工具
// ============================================================================

SingleChunkLifecycleManager::EnqueueDecision SingleChunkLifecycleManager::buildDecisionLocked() const
{
    EnqueueDecision decision;
    decision.generation = m_requestGeneration;
    decision.priority = m_requestPriority == std::numeric_limits<i32>::max() ? 0 : m_requestPriority;
    decision.targetStatus = m_requestedStatus;
    decision.cancelToken = m_cancelToken;

    // Ready 且已有内存 chunk 时，只需要唤醒等待者，不应再触发任何 IO 或生成。
    if (m_sourceState == SourceState::Ready) {
        decision.shouldWakeReadyWaiters = !m_waiters.empty();
        return decision;
    }

    // 来源解析是一次性的；只有 Unknown -> ResolvingStorage 那一跳会产生该动作。
    if (m_sourceState == SourceState::ResolvingStorage) {
        decision.shouldResolveStorage = true;
        return decision;
    }

    // 存档缺失后，只有在邻居条件已满足且尚未进入 worker 时，才允许排队生成。
    if (m_sourceState == SourceState::StorageMissing && m_executionState == ExecutionState::Queued &&
        m_cancelToken != nullptr) {
        decision.shouldQueueGeneration = true;
    }

    return decision;
}

void SingleChunkLifecycleManager::clearActiveGenerationLocked()
{
    if (m_cancelToken) {
        m_cancelToken->store(true, std::memory_order_release);
    }

    ++m_requestGeneration;
    m_submittedGeneration = 0;
    m_cancelToken = nullptr;
    m_generatingChunk.reset();
    m_requestPriority = std::numeric_limits<i32>::max();
}

} // namespace mc
