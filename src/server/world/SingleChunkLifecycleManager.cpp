/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM OR IN CONNECTION WITH
 * THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#include "SingleChunkLifecycleManager.hpp"
#include "common/profiler/TraceEvents.hpp"
#include "common/util/assert/AssertAll.hpp"
#include "common/world/chunk/data/ChunkPrimer.hpp"
#include <algorithm>
#include <limits>

using namespace mc::trace;

namespace mc::world::chunk {

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

const ChunkStatus& SingleChunkLifecycleManager::getCurrentGenStatus() const
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    return *m_currentGenStatus;
}

bool SingleChunkLifecycleManager::hasCompletedStatus(const ChunkStatus& status) const
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    return m_currentGenStatus->isAtLeast(status);
}

ChunkPrimer* SingleChunkLifecycleManager::getCurrentChunk() const
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    return m_currentChunk.get();
}

void SingleChunkLifecycleManager::setCurrentChunk(std::unique_ptr<ChunkPrimer> chunk)
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    m_currentChunk = std::move(chunk);
}

std::unique_ptr<ChunkPrimer> SingleChunkLifecycleManager::releaseCurrentChunk()
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    return std::move(m_currentChunk);
}

ChunkPrimer* SingleChunkLifecycleManager::getChunkIfPresentUnchecked(const ChunkStatus& status) const
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    // Cubium ChunkPrimer 累积式：m_currentChunk 已含所有 ≤ currentGenStatus 的状态数据。
    // 若 currentGenStatus >= status，返回 m_currentChunk；否则返回 nullptr。
    if (m_currentChunk != nullptr && m_currentGenStatus->isAtLeast(status)) {
        return m_currentChunk.get();
    }
    return nullptr;
}

void SingleChunkLifecycleManager::onChunkGenComplete(const ChunkStatus& completedStatus)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Chunk,
        "SingleChunkLifecycleManager::onChunkGenComplete",
        "completedStatus",
        completedStatus.name());
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    // Cubium ChunkPrimer 累积式：primer 是同一对象（_executeStepTask 修改同一 primer），
    // 只需推进 m_currentGenStatus，无需重存区块（对齐 Moonrise onChunkGenComplete 的 currentGenStatus 赋值）。
    if (completedStatus.isAfter(*m_currentGenStatus)) {
        m_currentGenStatus = &completedStatus;
    }
}

void SingleChunkLifecycleManager::completeStatusTo(const ChunkStatus& target)
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    if (target.isAfter(*m_currentGenStatus)) {
        m_currentGenStatus = &target;
    }
}

void SingleChunkLifecycleManager::setStatus(const ChunkStatus& status)
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    if (status.isAfter(*m_currentGenStatus)) {
        m_currentGenStatus = &status;
    }
}

bool SingleChunkLifecycleManager::acquireStatusBump(const ChunkStatus& target)
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    const ChunkStatus* parent = target.parent();
    if (parent == nullptr || !m_currentGenStatus->isAtLeast(*parent)) {
        return false;
    }
    if (m_currentGenStatus->isAtLeast(target)) {
        return false;
    }
    m_currentGenStatus = &target;
    return true;
}

// ============================================================================
// 双向邻居依赖图
// ============================================================================

void SingleChunkLifecycleManager::addBlockingNeighbour(SingleChunkLifecycleManager* neighbour)
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    if (neighbour != nullptr) {
        m_blockingNeighbours.insert(neighbour);
    }
}

void SingleChunkLifecycleManager::addWaitingNeighbour(
    SingleChunkLifecycleManager* neighbour, const ChunkStatus* requiredStatus)
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    if (neighbour != nullptr) {
        m_waitingNeighbours[neighbour] = requiredStatus;
    }
}

void SingleChunkLifecycleManager::removeBlockingNeighbour(SingleChunkLifecycleManager* neighbour)
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    if (neighbour != nullptr) {
        m_blockingNeighbours.erase(neighbour);
    }
}

void SingleChunkLifecycleManager::clearBlockingNeighbours()
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    m_blockingNeighbours.clear();
}

std::vector<SingleChunkLifecycleManager*> SingleChunkLifecycleManager::clearSatisfiedWaitingNeighbours(
    const ChunkStatus& completedStatus)
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    std::vector<SingleChunkLifecycleManager*> removed;
    for (auto it = m_waitingNeighbours.begin(); it != m_waitingNeighbours.end();) {
        const ChunkStatus* requiredStatus = it->second;
        if (requiredStatus == nullptr || completedStatus.isAtLeast(*requiredStatus)) {
            removed.push_back(it->first);
            it = m_waitingNeighbours.erase(it);
        } else {
            ++it;
        }
    }
    return removed;
}

std::vector<std::pair<SingleChunkLifecycleManager*, const ChunkStatus*>>
SingleChunkLifecycleManager::takeWaitingNeighbours()
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    std::vector<std::pair<SingleChunkLifecycleManager*, const ChunkStatus*>> result;
    result.reserve(m_waitingNeighbours.size());
    for (auto& [neighbour, requiredStatus] : m_waitingNeighbours) {
        result.emplace_back(neighbour, requiredStatus);
    }
    m_waitingNeighbours.clear();
    return result;
}

void SingleChunkLifecycleManager::removeWaitingNeighbour(SingleChunkLifecycleManager* neighbour)
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    if (neighbour != nullptr) {
        m_waitingNeighbours.erase(neighbour);
    }
}

std::vector<SingleChunkLifecycleManager*> SingleChunkLifecycleManager::blockingNeighbours() const
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    std::vector<SingleChunkLifecycleManager*> result;
    result.reserve(m_blockingNeighbours.size());
    for (auto* neighbour : m_blockingNeighbours) {
        result.push_back(neighbour);
    }
    return result;
}

// ============================================================================
// 生成任务跟踪
// ============================================================================

const ChunkStatus& SingleChunkLifecycleManager::requestedGenStatus() const
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    return *m_requestedGenStatus;
}

bool SingleChunkLifecycleManager::upgradeGenTarget(const ChunkStatus& newTarget)
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    if (newTarget.ordinal() > m_requestedGenStatus->ordinal()) {
        m_requestedGenStatus = &newTarget;
        return true;
    }
    return false;
}

void SingleChunkLifecycleManager::setGenerationTask(
    mc::server::ChunkProgressionTask* task, const ChunkStatus& scheduledStatus)
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    MC_ASSERT_RELEASE(m_generationTask == nullptr);
    m_generationTask = task;
    m_scheduledStatus = &scheduledStatus;
}

void SingleChunkLifecycleManager::clearGenerationTask()
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    m_generationTask = nullptr;
    m_scheduledStatus = nullptr;
}

bool SingleChunkLifecycleManager::isSafeToUnload() const
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    // m_neighboursUsingThisChunk 用原子读写（worker 线程 add/remove 持调度锁，但不持本 m_mutex），
    // 此处持 m_mutex 读取保证与依赖图操作（addWaitingNeighbour/removeBlockingNeighbour）的可见性。
    // 注意：最终卸载一致性由 unloadChunkSync 持调度锁重新检查 isSafeToUnload 保证
    // （见 ServerChunkManager::unloadChunkSync）。
    return m_neighboursUsingThisChunk.load(std::memory_order::acquire) == 0 && m_generationTask == nullptr &&
        !hasFailedGeneration() && m_blockingNeighbours.empty() && m_waitingNeighbours.empty();
}

// ============================================================================
// 票据与玩家追踪
// ============================================================================

SingleChunkLifecycleManager::SourceState SingleChunkLifecycleManager::sourceState() const
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    return m_sourceState;
}

void SingleChunkLifecycleManager::addTicket(const ChunkLoadTicket& ticket)
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    m_tickets.push_back(ticket);
}

void SingleChunkLifecycleManager::removeTicket(const ChunkLoadTicket& ticket)
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    auto it = std::find_if(m_tickets.begin(), m_tickets.end(), [&ticket](const ChunkLoadTicket& current) {
        return current.typeName() == ticket.typeName() && current.chunkValue() == ticket.chunkValue() &&
            current.level() == ticket.level();
    });

    if (it != m_tickets.end()) {
        m_tickets.erase(it);
    }
}

size_t SingleChunkLifecycleManager::ticketCount() const
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    return m_tickets.size();
}

bool SingleChunkLifecycleManager::hasTickets() const
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    return !m_tickets.empty();
}

void SingleChunkLifecycleManager::addTrackingPlayer(PlayerId player)
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    m_trackingPlayers.insert(player);
}

void SingleChunkLifecycleManager::removeTrackingPlayer(PlayerId player)
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    m_trackingPlayers.erase(player);
}

size_t SingleChunkLifecycleManager::trackingPlayerCount() const
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    return m_trackingPlayers.size();
}

bool SingleChunkLifecycleManager::hasTrackingPlayers() const
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    return !m_trackingPlayers.empty();
}

// ============================================================================
// 请求聚合
// ============================================================================

SingleChunkLifecycleManager::EnqueueDecision SingleChunkLifecycleManager::submitRequest(const ChunkStatus& targetStatus,
    i32 priority,
    std::function<void(bool, ChunkData*)> callback,
    std::shared_ptr<std::promise<ChunkData*>> promise)
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);

    // 已经达到或超过请求状态，且来源已就绪时，直接把等待者挂上。
    if (m_sourceState == SourceState::Ready && m_currentGenStatus->isAtLeast(targetStatus)) {
        if (callback || promise) {
            m_waiters.push_back(Waiter{std::move(callback), std::move(promise)});
        }
        return _buildDecisionLocked();
    }

    // 收敛请求：目标单调提升，优先级取更高（数值更小）
    if (targetStatus.ordinal() > m_requestedGenStatus->ordinal()) {
        m_requestedGenStatus = &targetStatus;
    }
    if (m_requestPriority == std::numeric_limits<i32>::max() || isHigherPriority(priority, m_requestPriority)) {
        m_requestPriority = priority;
    }

    // 首次进入或取消后重新进入时，分配新的 generation 与取消令牌。
    // 若 m_abortSignal 为 true（已被 cancelActiveWork 取消），分配新的 false 令牌，
    // 使重新请求的任务不被旧取消标志影响。
    if (!m_abortSignal || m_abortSignal->load(std::memory_order::acquire)) {
        ++m_requestGeneration;
        m_abortSignal = std::make_shared<std::atomic<bool>>(false);
    }

    if (callback || promise) {
        m_waiters.push_back(Waiter{std::move(callback), std::move(promise)});
    }

    // Ready 说明内存中已有结果但上方快速路径未命中，唤醒等待者
    if (m_sourceState == SourceState::Ready) {
        return _buildDecisionLocked();
    }

    // Unknown 是唯一允许触发一次来源解析的状态
    if (m_sourceState == SourceState::Unknown) {
        m_sourceState = SourceState::ResolvingStorage;
        EnqueueDecision decision = _buildDecisionLocked();
        decision.shouldResolveStorage = true;
        return decision;
    }

    // 已确认存档不存在 → 走生成链路（由 ChunkTaskScheduler 调度）
    if (m_sourceState == SourceState::StorageMissing) {
        return _buildDecisionLocked();
    }

    return _buildDecisionLocked();
}

SingleChunkLifecycleManager::EnqueueDecision SingleChunkLifecycleManager::noteStorageResolved(bool foundInStorage)
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    MC_ASSERT_RELEASE(m_sourceState == SourceState::ResolvingStorage);

    if (foundInStorage) {
        m_sourceState = SourceState::LoadedFromStorage;
    } else {
        m_sourceState = SourceState::StorageMissing;
    }

    return _buildDecisionLocked();
}

SingleChunkLifecycleManager::EnqueueDecision SingleChunkLifecycleManager::cancelActiveWork()
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    // 设置取消标志（运行中/排队中的任务持有此 shared_ptr，检测到 true 后 onCancel→cancelGeneration 清理）。
    // 不清除 m_abortSignal 为 null：保留 true 状态，使 cancelActiveWork 之后 onChunkGenComplete 的自重调度
    // （schedule→submitTask→holder.abortSignal()）捕获到此 true 标志，新任务立即被取消，
    // 避免"cancelActiveWork 清空 abortSignal 后自重调度提交不可取消任务"的竞态。
    // 重新请求（submitRequest）时若发现 m_abortSignal 为 true，会分配新的 false 令牌。
    if (m_abortSignal) {
        m_abortSignal->store(true, std::memory_order::release);
    }
    ++m_requestGeneration;
    m_generationTask = nullptr;
    m_scheduledStatus = nullptr;
    m_requestPriority = std::numeric_limits<i32>::max();
    return _buildDecisionLocked();
}

std::vector<SingleChunkLifecycleManager::Waiter> SingleChunkLifecycleManager::takeReadyWaiters()
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    if (m_sourceState != SourceState::Ready) {
        return {};
    }
    std::vector<Waiter> waiters;
    waiters.swap(m_waiters);
    return waiters;
}

std::vector<SingleChunkLifecycleManager::Waiter> SingleChunkLifecycleManager::takeAllWaiters()
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    std::vector<Waiter> waiters;
    waiters.swap(m_waiters);
    return waiters;
}

// ============================================================================
// 存档恢复
// ============================================================================

void SingleChunkLifecycleManager::markLoadedFromStorageReady(const ChunkStatus& persistedStatus)
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    if (persistedStatus.isAfter(*m_currentGenStatus)) {
        m_currentGenStatus = &persistedStatus;
    }
    m_sourceState = SourceState::Ready;
    m_requestPriority = std::numeric_limits<i32>::max();
}

std::shared_ptr<std::atomic<bool>> SingleChunkLifecycleManager::abortSignal() const
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    return m_abortSignal;
}

i32 SingleChunkLifecycleManager::requestPriority() const
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    return m_requestPriority;
}

bool SingleChunkLifecycleManager::reviveForScheduling()
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    // 仅当处于取消态（abortSignal 为 true）时复活：分配新的 false 令牌。
    // 与 submitRequest 的令牌重置逻辑一致（见 submitRequest 第 352-355 行）。
    if (m_abortSignal && m_abortSignal->load(std::memory_order::acquire)) {
        ++m_requestGeneration;
        m_abortSignal = std::make_shared<std::atomic<bool>>(false);
        return true;
    }
    return false;
}

// ============================================================================
// 内部工具
// ============================================================================

SingleChunkLifecycleManager::EnqueueDecision SingleChunkLifecycleManager::_buildDecisionLocked() const
{
    EnqueueDecision decision;
    decision.targetStatus = m_requestedGenStatus;
    decision.abortSignal = m_abortSignal;

    // Ready 且已达到请求状态：只需唤醒等待者
    if (m_sourceState == SourceState::Ready && m_currentGenStatus->isAtLeast(*m_requestedGenStatus)) {
        decision.shouldWakeReadyWaiters = !m_waiters.empty();
        return decision;
    }

    // 来源解析是一次性的：Unknown→ResolvingStorage 那一跳由 submitRequest 直接置 shouldResolveStorage=true。
    // ResolvingStorage 状态下（其他线程并发 submitRequest 见到，解析在途）返回 no-op：waiter 已加入 m_waiters，
    // 在途线程的 noteStorageResolved→_completeReadyWaiters（存档命中）或 _scheduleGeneration（存档缺失）会唤醒。
    // 若此处置 shouldResolveStorage=true，并发 submitRequest 会重复触发 _resolveChunkSourceSync→noteStorageResolved，
    // 第二次 noteStorageResolved 见到非 ResolvingStorage（已被第一次推进到 LoadedFromStorage/StorageMissing）
    // 触发 m_sourceState != ResolvingStorage 断言。
    if (m_sourceState == SourceState::ResolvingStorage) {
        return decision;
    }

    // 已达到请求状态、但尚未发布到 Ready/FULL 的稳态：区块数据已在 m_currentChunk（primer）中可用，
    // 但因请求的目标是中间状态（非 FULL，未走 _finalizeGeneratedChunkSync→markLoadedFromStorageReady），
    // sourceState 停在 StorageMissing（生成路径）或 LoadedFromStorage（存档命中瞬态，理论应被
    // markLoadedFromStorageReady 立即推进到 Ready，此处兜底）。此时新到达的请求 waiter 若仅靠下方
    // shouldScheduleGeneration 分支（要求 gen<req）会被永久悬挂——_buildDecisionLocked 对 gen>=req 返回
    // no-op，waiter 既不被完成也不被重新调度（_completeReadyWaiters 的 takeReadyWaiters 要求 Ready）。
    // 对齐 Moonrise：schedule 中 currentGenStatus.isOrAfter(targetStatus) 时直接返回（请求者经
    // getChunkIfPresentUnchecked 快速路径拿到已生成 chunk）。Cubium 在 _completeReadyWaiters 用
    // takeAllWaiters 取出 waiter，从 primer 的 ChunkData 完成它们（与 _publishGeneratedChunk 同语义）。
    if (m_sourceState != SourceState::Ready && m_currentGenStatus->isAtLeast(*m_requestedGenStatus) &&
        m_generationTask == nullptr && m_currentChunk != nullptr) {
        decision.shouldWakeReadyWaiters = !m_waiters.empty();
        return decision;
    }

    // 存档缺失后，若尚未达到请求状态且没有进行中任务，触发生成调度
    if (m_sourceState == SourceState::StorageMissing && m_currentGenStatus->isBefore(*m_requestedGenStatus) &&
        m_generationTask == nullptr) {
        decision.shouldScheduleGeneration = true;
    }

    return decision;
}

} // namespace mc::world::chunk
