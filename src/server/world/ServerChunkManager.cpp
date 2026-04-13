#include "ServerChunkManager.hpp"
#include "ServerWorld.hpp"
#include "../sync/ChunkSendManager.hpp"
#include "../../common/world/WorldConstants.hpp"
#include "../../common/world/chunk/ChunkPrimer.hpp"
#include "../../common/world/chunk/ChunkStatus.hpp"
#include "../../common/world/gen/chunk/IChunkGenerator.hpp"
#include "chunk/task/ChunkProgressionTask.hpp"
#include <chrono>
#include <spdlog/spdlog.h>
#include "../../common/perfetto/TraceEvents.hpp"
#include "common/util/assert/AssertAll.hpp"

namespace mc::server {

namespace {

class ChunkDataChunkAdapter : public IChunk {
public:
    explicit ChunkDataChunkAdapter(std::shared_ptr<ChunkData> chunk)
        : m_chunk(std::move(chunk))
        , m_status(m_chunk && m_chunk->isFullyGenerated() ? ChunkLoadStatus::Generated : ChunkLoadStatus::Generating) {}

    [[nodiscard]] ChunkCoord x() const override { return m_chunk ? m_chunk->x() : 0; }
    [[nodiscard]] ChunkCoord z() const override { return m_chunk ? m_chunk->z() : 0; }
    [[nodiscard]] ChunkPos pos() const override { return m_chunk ? m_chunk->pos() : ChunkPos(0, 0); }

    [[nodiscard]] const BlockState* getBlock(BlockCoord x, BlockCoord y, BlockCoord z) const override {
        return m_chunk ? m_chunk->getBlock(x, y, z) : nullptr;
    }

    void setBlock(BlockCoord x, BlockCoord y, BlockCoord z, const BlockState* state) override {
        if (m_chunk) {
            m_chunk->setBlock(x, y, z, state);
        }
    }

    [[nodiscard]] u32 getBlockStateId(BlockCoord x, BlockCoord y, BlockCoord z) const override {
        return m_chunk ? m_chunk->getBlockStateId(x, y, z) : 0;
    }

    void setBlockStateId(BlockCoord x, BlockCoord y, BlockCoord z, u32 stateId) override {
        if (m_chunk) {
            m_chunk->setBlockStateId(x, y, z, stateId);
        }
    }

    [[nodiscard]] ChunkSection* getSection(i32 index) override {
        return m_chunk ? m_chunk->getSection(index) : nullptr;
    }

    [[nodiscard]] const ChunkSection* getSection(i32 index) const override {
        return m_chunk ? m_chunk->getSection(index) : nullptr;
    }

    [[nodiscard]] bool hasSection(i32 index) const override {
        return m_chunk ? m_chunk->hasSection(index) : false;
    }

    ChunkSection* createSection(i32 index) override {
        return m_chunk ? m_chunk->createSection(index) : nullptr;
    }

    [[nodiscard]] const ChunkSection* const* getSections() const override {
        return m_chunk ? m_chunk->getSections() : nullptr;
    }

    [[nodiscard]] BiomeId getBiomeAtBlock(BlockCoord x, BlockCoord y, BlockCoord z) const override {
        return m_chunk ? m_chunk->getBiomeAtBlock(x, y, z) : Biomes::Plains;
    }

    [[nodiscard]] BlockCoord getTopBlockY(HeightmapType type, BlockCoord x, BlockCoord z) const override {
        (void)type;
        return m_chunk ? m_chunk->getHighestBlock(x, z) : 0;
    }

    void updateHeightmap(HeightmapType type, BlockCoord x, BlockCoord y, BlockCoord z, const BlockState* state) override {
        (void)type;
        (void)y;
        (void)state;
        if (m_chunk) {
            m_chunk->updateHeightMap(x, z);
        }
    }

    [[nodiscard]] ChunkLoadStatus getStatus() const override {
        return m_status;
    }

    void setStatus(ChunkLoadStatus status) override {
        m_status = status;
    }

    [[nodiscard]] bool isModified() const override {
        return m_chunk ? m_chunk->isDirty() : false;
    }

    void setModified(bool modified) override {
        if (m_chunk) {
            m_chunk->setDirty(modified);
        }
    }

private:
    std::shared_ptr<ChunkData> m_chunk;
    ChunkLoadStatus m_status = ChunkLoadStatus::Generated;
};

// 票据级别常量
constexpr i32 MAX_TICKET_LEVEL = 33;  // 区块加载最大级别
constexpr i32 FORCED_TICKET_LEVEL = 31;  // 强制加载级别

} // namespace

// ============================================================================
// 构造与析构
// ============================================================================

ServerChunkManager::ServerChunkManager(ServerWorld& world, std::unique_ptr<IChunkGenerator> generator)
    : m_world(&world)
    , m_generator(std::move(generator))
    , m_ticketPropagator()
    , m_holderManager(std::make_unique<ChunkHolderManager>(&world, m_ticketPropagator))
    , m_taskScheduler(std::make_unique<ChunkTaskScheduler>(world, -1))
{
    // 设置票据级别变化回调
    m_holderManager->setLevelChangeCallback(
        [this](ChunkCoord x, ChunkCoord z, i32 oldLevel, i32 newLevel) {
            onTicketLevelChanged(x, z, oldLevel, newLevel);
        });
}

ServerChunkManager::ServerChunkManager(std::unique_ptr<IChunkGenerator> generator)
    : m_world(nullptr)
    , m_generator(std::move(generator))
    , m_ticketPropagator()
    , m_holderManager(std::make_unique<ChunkHolderManager>(nullptr, m_ticketPropagator))
    , m_taskScheduler(nullptr)  // 没有 ServerWorld 时无法创建调度器
{
    // 注意：无 ServerWorld 模式需要后续设置回调
}

ServerChunkManager::~ServerChunkManager()
{
    shutdown();
}

// ============================================================================
// 生命周期
// ============================================================================

Result<void> ServerChunkManager::initialize()
{
    // 启动任务调度器
    startWorkers();

    return {};
}

void ServerChunkManager::shutdown()
{
    stopWorkers();

    // 清理挂起请求，避免 future 永久阻塞
    std::unordered_map<u64, PendingGeneration> pending;
    {
        std::lock_guard<std::mutex> lock(m_pendingGenerationsMutex);
        pending.swap(m_pendingGenerations);
    }
    for (auto& [key, entry] : pending) {
        (void)key;
        if (entry.cancelToken) {
            entry.cancelToken->store(true, std::memory_order_release);
        }
        for (auto& p : entry.promises) {
            if (p) {
                p->set_value(nullptr);
            }
        }
        for (auto& cb : entry.callbacks) {
            if (cb) {
                cb(false, nullptr);
            }
        }
    }

    // 清理区块持有者
    m_holderManager.reset();

    // 清理区块缓存
    std::lock_guard<std::mutex> chunksLock(m_chunksMutex);
    m_chunks.clear();
}

// ============================================================================
// Worker 管理
// ============================================================================

void ServerChunkManager::startWorkers()
{
    if (m_taskScheduler) {
        m_taskScheduler->start();
    }
}

void ServerChunkManager::stopWorkers()
{
    if (m_taskScheduler) {
        m_taskScheduler->shutdown();
    }
}

bool ServerChunkManager::workersRunning() const
{
    return m_taskScheduler && !m_taskScheduler->hasShutdown();
}

// ============================================================================
// 区块访问（同步）
// ============================================================================

ChunkData* ServerChunkManager::getChunk(ChunkCoord x, ChunkCoord z)
{
    return getChunkShared(x, z).get();
}

const ChunkData* ServerChunkManager::getChunk(ChunkCoord x, ChunkCoord z) const
{
    return getChunkShared(x, z).get();
}

std::shared_ptr<ChunkData> ServerChunkManager::getChunkShared(ChunkCoord x, ChunkCoord z)
{
    const u64 key = posToKey(x, z);

    std::lock_guard<std::mutex> lock(m_chunksMutex);
    auto it = m_chunks.find(key);
    if (it != m_chunks.end()) {
        return it->second;
    }
    return {};
}

std::shared_ptr<const ChunkData> ServerChunkManager::getChunkShared(ChunkCoord x, ChunkCoord z) const
{
    const u64 key = posToKey(x, z);

    std::lock_guard<std::mutex> lock(m_chunksMutex);
    auto it = m_chunks.find(key);
    if (it != m_chunks.end()) {
        return it->second;
    }
    return {};
}

bool ServerChunkManager::hasChunk(ChunkCoord x, ChunkCoord z) const
{
    const u64 key = posToKey(x, z);

    std::lock_guard<std::mutex> lock(m_chunksMutex);
    return m_chunks.find(key) != m_chunks.end();
}

ChunkData* ServerChunkManager::getChunkSync(ChunkCoord x, ChunkCoord z)
{
    spdlog::debug("Requesting chunk synchronously at ({}, {}), current m_chunks size: {}", x, z, m_chunks.size());

    // 先检查缓存
    if (ChunkData* cached = getChunk(x, z)) {
        return cached;
    }

    std::lock_guard<std::mutex> generationLock(m_syncGenerationMutex);

    // 进入同步生成临界区后再次检查缓存，避免重复生成
    if (ChunkData* cached = getChunk(x, z)) {
        return cached;
    }

    // 获取持有者
    SingleChunkLifecycleManager* holder = getOrCreateChunkHolder(x, z);
    if (!holder) {
        return nullptr;
    }

    // 检查是否已完成
    if (ChunkData* data = holder->getChunkData()) {
        return data;
    }

    // 同步生成到 FULL 状态，确保与异步路径结果一致
    executeGenerationTask(*holder, ChunkStatuses::FULL);

    // 返回缓存中的结果
    return getChunk(x, z);
}

void ServerChunkManager::unloadChunk(ChunkCoord x, ChunkCoord z)
{
    const u64 key = posToKey(x, z);

    {
        std::lock_guard<std::mutex> lock(m_chunksMutex);
        m_chunks.erase(key);
    }

    // 从持有者管理器移除
    // 先清除票据，让区块可以被卸载
    if (m_holderManager) {
        auto* holder = m_holderManager->getChunkHolder(x, z);
        if (holder) {
            holder->setLevel(MAX_TICKET_LEVEL + 1);  // 设置为卸载级别
            holder->setQueuedForUnload(true);
        }
    }
}

// ============================================================================
// 区块访问（异步）
// ============================================================================

std::future<ChunkData*> ServerChunkManager::getChunkAsync(ChunkCoord x, ChunkCoord z,
                                                           const ChunkStatus* targetStatus)
{
    auto promise = std::make_shared<std::promise<ChunkData*>>();
    auto future = promise->get_future();

    if (auto cached = getChunkShared(x, z)) {
        promise->set_value(cached.get());
        return future;
    }

    // 获取或创建持有者
    SingleChunkLifecycleManager* holder = getOrCreateChunkHolder(x, z);
    if (!holder) {
        promise->set_value(nullptr);
        return future;
    }

    // 调度异步生成
    const ChunkStatus& target = targetStatus ? *targetStatus : ChunkStatuses::FULL;

    requestChunkGeneration(x, z, target,
                           computeSchedulePriority(x, z, target, getTicketLevel(x, z)),
                           nullptr,
                           promise);

    return future;
}

void ServerChunkManager::getChunkAsync(ChunkCoord x, ChunkCoord z, ChunkCallback callback,
                                        const ChunkStatus* targetStatus)
{
    if (auto cached = getChunkShared(x, z)) {
        if (callback) callback(true, cached.get());
        return;
    }

    // 获取或创建持有者
    SingleChunkLifecycleManager* holder = getOrCreateChunkHolder(x, z);
    if (!holder) {
        if (callback) callback(false, nullptr);
        return;
    }

    // 调度异步生成
    const ChunkStatus& target = targetStatus ? *targetStatus : ChunkStatuses::FULL;

    requestChunkGeneration(x, z, target,
                           computeSchedulePriority(x, z, target, getTicketLevel(x, z)),
                           std::move(callback),
                           nullptr);
}

// ============================================================================
// 区块持有者
// ============================================================================

SingleChunkLifecycleManager* ServerChunkManager::getOrCreateChunkHolder(ChunkCoord x, ChunkCoord z)
{
    return m_holderManager->getOrCreateChunkHolder(x, z);
}

SingleChunkLifecycleManager* ServerChunkManager::getChunkHolder(ChunkCoord x, ChunkCoord z)
{
    return m_holderManager->getChunkHolder(x, z);
}

const SingleChunkLifecycleManager* ServerChunkManager::getChunkHolder(ChunkCoord x, ChunkCoord z) const
{
    return m_holderManager->getChunkHolder(x, z);
}

// ============================================================================
// 票据管理
// ============================================================================

void ServerChunkManager::updatePlayerPosition(PlayerId player, f64 x, f64 z)
{
    const ChunkCoord chunkX = static_cast<ChunkCoord>(std::floor(x / world::CHUNK_WIDTH));
    const ChunkCoord chunkZ = static_cast<ChunkCoord>(std::floor(z / world::CHUNK_WIDTH));

    // 获取旧位置用于票据更新
    const world::PlayerChunkTracker* oldTracker = m_trackingManager.getPlayerTracker(player);
    ChunkCoord oldChunkX = 0;
    ChunkCoord oldChunkZ = 0;
    bool hadOldPos = false;
    i32 oldViewDistance = m_viewDistance;

    if (oldTracker && oldTracker->hasPosition()) {
        oldChunkX = oldTracker->playerX();
        oldChunkZ = oldTracker->playerZ();
        oldViewDistance = oldTracker->viewDistance();
        hadOldPos = true;

        // 移除旧位置的票据
        for (i32 dz = -oldViewDistance; dz <= oldViewDistance; ++dz) {
            for (i32 dx = -oldViewDistance; dx <= oldViewDistance; ++dx) {
                ChunkCoord cx = oldChunkX + dx;
                ChunkCoord cz = oldChunkZ + dz;

                i32 dist = std::max(std::abs(dx), std::abs(dz));
                i32 level = MAX_TICKET_LEVEL - (oldViewDistance - dist);

                if (level > 0 && level < MAX_TICKET_LEVEL) {
                    m_holderManager->removeTicket(cx, cz, level, "player_view");
                }
            }
        }
    }

    // 更新追踪管理器（会触发追踪变化回调）
    m_trackingManager.updatePlayerPosition(player, chunkX, chunkZ);

    // 添加新位置的票据
    for (i32 dz = -m_viewDistance; dz <= m_viewDistance; ++dz) {
        for (i32 dx = -m_viewDistance; dx <= m_viewDistance; ++dx) {
            ChunkCoord cx = chunkX + dx;
            ChunkCoord cz = chunkZ + dz;

            i32 dist = std::max(std::abs(dx), std::abs(dz));
            i32 level = MAX_TICKET_LEVEL - (m_viewDistance - dist);

            if (level > 0 && level < MAX_TICKET_LEVEL) {
                m_holderManager->addTicket(cx, cz, level, "player_view");
            }
        }
    }
}

void ServerChunkManager::removePlayer(PlayerId player)
{
    // 获取旧位置用于票据移除
    const world::PlayerChunkTracker* tracker = m_trackingManager.getPlayerTracker(player);
    if (tracker && tracker->hasPosition()) {
        ChunkCoord chunkX = tracker->playerX();
        ChunkCoord chunkZ = tracker->playerZ();
        i32 viewDistance = tracker->viewDistance();

        // 移除视距范围内的票据
        for (i32 dz = -viewDistance; dz <= viewDistance; ++dz) {
            for (i32 dx = -viewDistance; dx <= viewDistance; ++dx) {
                ChunkCoord cx = chunkX + dx;
                ChunkCoord cz = chunkZ + dz;

                i32 dist = std::max(std::abs(dx), std::abs(dz));
                i32 level = MAX_TICKET_LEVEL - (viewDistance - dist);

                if (level > 0 && level < MAX_TICKET_LEVEL) {
                    m_holderManager->removeTicket(cx, cz, level, "player_view");
                }
            }
        }
    }

    // 移除追踪管理器中的玩家
    m_trackingManager.removePlayer(player);
}

void ServerChunkManager::forceChunk(ChunkCoord x, ChunkCoord z, bool force)
{
    std::lock_guard<std::mutex> lock(m_forcedMutex);

    u64 key = posToKey(x, z);

    if (force) {
        if (m_forcedChunks.find(key) == m_forcedChunks.end()) {
            m_forcedChunks.insert(key);
            m_holderManager->addTicket(x, z, FORCED_TICKET_LEVEL, "forced");
        }
    } else {
        if (m_forcedChunks.find(key) != m_forcedChunks.end()) {
            m_forcedChunks.erase(key);
            m_holderManager->removeTicket(x, z, FORCED_TICKET_LEVEL, "forced");
        }
    }
}

void ServerChunkManager::setViewDistance(i32 distance)
{
    m_viewDistance = distance;
    m_trackingManager.setDefaultViewDistance(distance);
}

bool ServerChunkManager::shouldChunkLoad(ChunkCoord x, ChunkCoord z) const
{
    i32 level = getTicketLevel(x, z);
    return level > 0 && level <= MAX_TICKET_LEVEL;
}

void ServerChunkManager::processTicketUpdates()
{
    m_holderManager->processTicketUpdates();
}

i32 ServerChunkManager::getTicketLevel(ChunkCoord x, ChunkCoord z) const
{
    return m_ticketPropagator.getLevel(x, z);
}

// ============================================================================
// 主循环
// ============================================================================

void ServerChunkManager::tick()
{
    MC_TRACE_EVENT("server.chunk", "ChunkManagerTick");

    ++m_currentTick;

    // 处理票据更新
    processTicketUpdates();

    // 执行主线程任务
    if (m_taskScheduler) {
        m_taskScheduler->executeAllRecentlyQueuedMainThreadTasks();
    }

    // 处理完成的异步任务
    processCompletedTasks();

    // 检查区块卸载
    if (m_currentTick - m_lastUnloadCheck >= UNLOAD_CHECK_INTERVAL) {
        checkChunkUnloading();
        m_lastUnloadCheck = m_currentTick;
    }
}

// ============================================================================
// 内部方法
// ============================================================================

void ServerChunkManager::scheduleGeneration(SingleChunkLifecycleManager& holder, const ChunkStatus& targetStatus)
{
    // 如果已经达到目标状态、已有缓存结果或已有正在使用的 primer，则不重复调度
    if (holder.hasCompletedStatus(targetStatus) ||
        getChunk(holder.x(), holder.z()) != nullptr) {
        return;
    }

    if (!holder.shouldLoad()) {
        cancelPendingGeneration(holder.x(), holder.z());
        return;
    }

    const i32 ticketLevel = getTicketLevel(holder.x(), holder.z());
    const Priority priority = computeSchedulePriority(holder.x(), holder.z(), targetStatus, ticketLevel);

    requestChunkGeneration(holder.x(), holder.z(), targetStatus, priority, nullptr, nullptr);
}

void ServerChunkManager::requestChunkGeneration(ChunkCoord x, ChunkCoord z,
                                                const ChunkStatus& targetStatus,
                                                Priority priority,
                                                ChunkCallback callback,
                                                std::shared_ptr<std::promise<ChunkData*>> promise)
{
    MC_ASSERT_RELEASE_MSG(targetStatus.ordinal() >= 0, "Invalid target chunk status ordinal");

    const u64 key = posToKey(x, z);
    SingleChunkLifecycleManager* holder = getOrCreateChunkHolder(x, z);
    if (!holder) {
        if (promise) {
            promise->set_value(nullptr);
        }
        if (callback) {
            callback(false, nullptr);
        }
        return;
    }

    ChunkRequestControl control = holder->upsertRequest(targetStatus, static_cast<i32>(priority));

    PendingGeneration stalePending;
    bool hasStalePending = false;

    {
        std::lock_guard<std::mutex> lock(m_pendingGenerationsMutex);
        auto& entry = m_pendingGenerations[key];

        if (entry.generation != 0 && entry.generation != control.generation) {
            stalePending = std::move(entry);
            hasStalePending = true;
            entry = PendingGeneration{};
        }

        entry.generation = control.generation;
        entry.cancelToken = control.cancelToken;
        if (callback) {
            entry.callbacks.push_back(std::move(callback));
        }
        if (promise) {
            entry.promises.push_back(std::move(promise));
        }
    }

    if (hasStalePending) {
        for (auto& p : stalePending.promises) {
            if (p) {
                p->set_value(nullptr);
            }
        }
        for (auto& cb : stalePending.callbacks) {
            if (cb) {
                cb(false, nullptr);
            }
        }
    }

    if (!control.shouldEnqueue) {
        return;
    }

    // 调度任务
    if (m_taskScheduler && !m_taskScheduler->hasShutdown()) {
        m_taskScheduler->scheduleChunkTask(x, z,
            [this, key, x, z, generation = control.generation, targetStatus = &targetStatus]() {
                SingleChunkLifecycleManager* holder = getChunkHolder(x, z);
                if (!holder || !holder->tryStartRequest(generation)) {
                    return;
                }

                if (!holder->isGenerationCurrent(generation)) {
                    return;
                }

                // 执行生成
                executeGenerationTask(*holder, *targetStatus);

                // 完成请求
                holder->finishRequest(generation, true, false);

                // 处理回调
                PendingGeneration pending;
                {
                    std::lock_guard<std::mutex> lock(m_pendingGenerationsMutex);
                    auto it = m_pendingGenerations.find(key);
                    if (it != m_pendingGenerations.end()) {
                        if (it->second.generation == generation) {
                            pending = std::move(it->second);
                            m_pendingGenerations.erase(it);
                        }
                    }
                }

                ChunkData* chunk = getChunk(x, z);
                for (auto& p : pending.promises) {
                    if (p) {
                        p->set_value(chunk);
                    }
                }
                for (auto& cb : pending.callbacks) {
                    if (cb) {
                        cb(chunk != nullptr, chunk);
                    }
                }
            },
            priority);
    }
}

void ServerChunkManager::cancelPendingGeneration(ChunkCoord x, ChunkCoord z)
{
    const u64 key = posToKey(x, z);

    if (SingleChunkLifecycleManager* holder = getChunkHolder(x, z)) {
        holder->cancelActiveRequest();
    }

    PendingGeneration pending;
    bool hasPending = false;
    {
        std::lock_guard<std::mutex> lock(m_pendingGenerationsMutex);
        auto it = m_pendingGenerations.find(key);
        if (it != m_pendingGenerations.end()) {
            pending = std::move(it->second);
            m_pendingGenerations.erase(it);
            hasPending = true;
        }
    }

    if (hasPending) {
        if (pending.cancelToken) {
            pending.cancelToken->store(true, std::memory_order_release);
        }
        for (auto& p : pending.promises) {
            if (p) {
                p->set_value(nullptr);
            }
        }
        for (auto& cb : pending.callbacks) {
            if (cb) {
                cb(false, nullptr);
            }
        }
    }
}

void ServerChunkManager::onTicketLevelChanged(ChunkCoord x, ChunkCoord z, i32 oldLevel, i32 newLevel)
{
    (void)oldLevel;

    SingleChunkLifecycleManager* holder = getOrCreateChunkHolder(x, z);
    if (!holder) {
        return;
    }

    holder->setLevel(newLevel);

    if (newLevel <= MAX_TICKET_LEVEL) {
        scheduleGeneration(*holder, ChunkStatuses::FULL);
        return;
    }

    cancelPendingGeneration(x, z);
}

Priority ServerChunkManager::computeSchedulePriority(ChunkCoord x, ChunkCoord z,
                                                const ChunkStatus& targetStatus,
                                                i32 ticketLevel) const
{
    // 票据级别越低（越重要），优先级越高
    const i32 normalizedLevel = std::clamp(ticketLevel, 0, MAX_TICKET_LEVEL);
    const i32 statusPenalty = std::max(0, ChunkStatuses::FULL.ordinal() - targetStatus.ordinal());

    // 根据票据级别和状态计算优先级
    // BLOCKING = 0 最高，LOWEST = 6 最低
    i32 priorityValue = (normalizedLevel / 5) + statusPenalty;
    priorityValue = std::clamp(priorityValue, 0, static_cast<i32>(Priority::LOWEST));

    return static_cast<Priority>(priorityValue);
}

void ServerChunkManager::executeGenerationTask(SingleChunkLifecycleManager& holder, const ChunkStatus& status)
{
    // 创建区块生成器
    ChunkPrimer* primer = holder.createGeneratingChunk();
    if (!primer) {
        return;
    }

    // 创建 WorldGenRegion（简化版）
    std::array<IChunk*, 9> chunks{};
    std::array<std::unique_ptr<IChunk>, 9> neighborAdapters{};
    chunks[4] = primer;

    // 获取邻居区块
    getNeighborChunks(holder.x(), holder.z(), chunks, neighborAdapters);

    WorldGenRegion region(holder.x(), holder.z(), chunks);

    // 按阶段生成
    const auto& allStatuses = ChunkStatus::getAll();
    for (const auto& s : allStatuses) {
        if (s.ordinal() > status.ordinal()) {
            break;
        }

        if (!primer->hasCompletedStatus(s)) {
            // 执行生成
            if (s == ChunkStatuses::STRUCTURE_STARTS) {
                m_generator->generateStructureStarts(region, *primer);
            } else if (s == ChunkStatuses::STRUCTURE_REFERENCES) {
                m_generator->generateStructureReferences(region, *primer);
            } else if (s == ChunkStatuses::BIOMES) {
                m_generator->generateBiomes(region, *primer);
            } else if (s == ChunkStatuses::NOISE) {
                m_generator->generateNoise(region, *primer);
            } else if (s == ChunkStatuses::SURFACE) {
                m_generator->buildSurface(region, *primer);
            } else if (s == ChunkStatuses::CARVERS) {
                m_generator->applyCarvers(region, *primer, false);
            } else if (s == ChunkStatuses::LIQUID_CARVERS) {
                m_generator->applyCarvers(region, *primer, true);
            } else if (s == ChunkStatuses::FEATURES) {
                // 同步路径：不检查邻居依赖，直接执行
                m_generator->placeFeatures(region, *primer);
            } else if (s == ChunkStatuses::LIGHT) {
                // 光照计算
                auto* lightManager = m_world ? m_world->lightManager() : nullptr;
                if (lightManager) {
                    // 参考 Moonrise：区分已光照和未光照区块
                    if (primer->isLightCorrect()) {
                        // 已正确光照：只需重新加载光照数据并检查边缘
                        std::vector<bool> emptySections;
                        for (i32 i = 0; i < ChunkData::SECTIONS; ++i) {
                            emptySections.push_back(!primer->hasSection(i) ||
                                                   primer->getSection(i) == nullptr ||
                                                   primer->getSection(i)->isEmpty());
                        }
                        lightManager->forceLoadInChunk(primer, emptySections);
                        lightManager->checkChunkEdges(holder.x(), holder.z());
                    } else {
                        // 需要完整光照计算
                        primer->setLightCorrect(false);
                        lightManager->lightChunk(primer, true);
                        primer->setLightCorrect(true);
                    }
                }
            } else if (s == ChunkStatuses::SPAWN) {
                // SPAWN 阶段：计算生物生成点
                MC_TRACE_EVENT("world.chunk_gen", "Spawn");
            } else if (s == ChunkStatuses::HEIGHTMAPS) {
                primer->updateAllHeightmaps();
            }

            primer->setChunkStatus(s);
        }
    }

    // 在区块生成完成后调用 spawnInitialMobs
    // 参考 MC 1.16.5 performWorldGenSpawning
    if (primer->hasCompletedStatus(ChunkStatuses::FEATURES)) {
        std::vector<SpawnedEntityData> entities;
        m_generator->spawnInitialMobs(region, *primer, entities);

        // 将生成的实体数据存储到 ChunkPrimer 中
        for (auto& entityData : entities) {
            primer->addSpawnedEntity(std::move(entityData));
        }
    }

    // 提取生成的实体数据（在 completeGeneration 之前）
    std::vector<SpawnedEntityData> spawnedEntities;
    if (primer->spawnedEntityCount() > 0) {
        spawnedEntities = std::move(primer->spawnedEntities());
    }

    // 完成生成
    auto data = holder.completeGeneration();
    if (data) {
        // 存储到缓存
        ChunkData* stored = storeGeneratedChunk(holder.x(), holder.z(), std::move(data));

        // 将生成的实体添加到世界
        if (stored && m_world && !spawnedEntities.empty()) {
            m_world->spawnEntitiesFromChunkGeneration(spawnedEntities);
        }
    }
}

bool ServerChunkManager::checkNeighborsReady(ChunkCoord x, ChunkCoord z, const ChunkStatus& status) const
{
    // taskRange 为 0 表示不需要邻居
    if (status.taskRange() == 0) {
        return true;
    }

    // 需要邻居完成的是前一阶段（parent），而不是当前阶段
    // 这避免了循环等待死锁：FEATURES 需要邻居完成 CARVERS
    // 参考 MC ChunkStatus.outputParent 的设计
    const ChunkStatus* requiredStatus = status.parent();
    if (!requiredStatus) {
        requiredStatus = &status;
    }

    // 检查 8 个邻居
    const i32 range = status.taskRange();
    for (i32 dz = -range; dz <= range; ++dz) {
        for (i32 dx = -range; dx <= range; ++dx) {
            if (dx == 0 && dz == 0) continue;

            const SingleChunkLifecycleManager* neighbor = getChunkHolder(x + dx, z + dz);
            if (!neighbor || !neighbor->hasCompletedStatus(*requiredStatus)) {
                return false;
            }
        }
    }

    return true;
}

void ServerChunkManager::getNeighborChunks(
    ChunkCoord x,
    ChunkCoord z,
    std::array<IChunk*, 9>& neighbors,
    std::array<std::unique_ptr<IChunk>, 9>& neighborAdapters)
{
    // 索引顺序：0=NW, 1=N, 2=NE, 3=W, 4=中心, 5=E, 6=SW, 7=S, 8=SE
    // 偏移量（dx, dz）
    static constexpr i32 offsets[9][2] = {
        {-1, -1}, {0, -1}, {1, -1},  // NW, N, NE
        {-1, 0},  {0, 0},  {1, 0},   // W, 中心, E
        {-1, 1},  {0, 1},  {1, 1}    // SW, S, SE
    };

    for (size_t i = 0; i < 9; ++i) {
        // 跳过中心位置（调用者已设置）
        if (i == 4) {
            continue;
        }

        SingleChunkLifecycleManager* holder = getChunkHolder(x + offsets[i][0], z + offsets[i][1]);
        if (holder) {
            if (auto data = getChunkShared(x + offsets[i][0], z + offsets[i][1])) {
                neighborAdapters[i] = std::make_unique<ChunkDataChunkAdapter>(std::move(data));
                neighbors[i] = neighborAdapters[i].get();
            } else {
                neighborAdapters[i].reset();
                neighbors[i] = nullptr;
            }
        } else {
            neighborAdapters[i].reset();
            neighbors[i] = nullptr;
        }
    }
}

ChunkData* ServerChunkManager::storeGeneratedChunk(ChunkCoord x, ChunkCoord z, std::unique_ptr<ChunkData> data)
{
    MC_ASSERT_RELEASE_MSG(data, "Generated chunk data should not be null");

    std::shared_ptr<ChunkData> sharedData(std::move(data));

    ChunkData* stored = nullptr;
    {
        std::lock_guard<std::mutex> lock(m_chunksMutex);
        auto& slot = m_chunks[posToKey(x, z)];
        slot = std::move(sharedData);
        stored = slot.get();
    }

    if (SingleChunkLifecycleManager* holder = getChunkHolder(x, z)) {
        holder->setStatus(ChunkStatuses::FULL);
    }

    // 调用区块加载回调（用于光照初始化等）
    if (m_chunkLoadedCallback && stored) {
        m_chunkLoadedCallback(x, z);
    }

    return stored;
}

void ServerChunkManager::processCompletedTasks()
{
    // 任务调度器会自动处理完成的任务
    // 这里可以添加额外的后处理逻辑
}

void ServerChunkManager::checkChunkUnloading()
{
    MC_TRACE_EVENT(
        "server.chunk",
        "CheckUnloading",
        "loadedChunkCount", loadedChunkCount(),
        "holderCount", holderCount()
    );

    // 处理卸载队列
    if (m_holderManager) {
        m_holderManager->processUnloadQueue(100);
    }

    // 检查所有持有者
    std::vector<u64> toUnload;

    m_holderManager->forEachHolder([this, &toUnload](SingleChunkLifecycleManager& holder) {
        // 没有票据且没有玩家追踪且没有正在生成
        if (!holder.shouldLoad() &&
            !holder.hasTickets() &&
            !holder.hasGeneratingChunk()) {

            // 检查是否有玩家追踪
            u64 key = posToKey(holder.x(), holder.z());
            if (!m_trackingManager.hasTrackingPlayers(key)) {
                toUnload.push_back(key);
            }
        }
    });

    // 卸载区块
    for (u64 key : toUnload) {
        ChunkCoord x, z;
        keyToPos(key, x, z);

        cancelPendingGeneration(x, z);

        // 在卸载前通知 ChunkSendManager 发送卸载包
        if (m_chunkSendManager) {
            m_chunkSendManager->onChunkPreUnload(x, z);
        }

        // 通知村庄管理器区块卸载（用于清理 POI 等）
        if (m_chunkUnloadedCallback) {
            m_chunkUnloadedCallback(x, z);
        }

        ChunkPos chunkPos{x, z};
        MC_TRACE_INSTANT("server.chunk",
            "UnloadChunk",
            "x", x,
            "z", z,
            [flow = ::perfetto::Flow::ProcessScoped(chunkPos.toId())](::perfetto::EventContext ctx) {
                flow(ctx);
        });

        unloadChunk(x, z);
    }
}

ChunkData* ServerChunkManager::finalizeChunkGeneration(ChunkCoord x, ChunkCoord z, ChunkPrimer& primer)
{
    // 提取生成的实体数据（在 toChunkData 之前）
    std::vector<SpawnedEntityData> spawnedEntities;
    if (primer.spawnedEntityCount() > 0) {
        spawnedEntities = std::move(primer.spawnedEntities());
    }

    // 完成生成
    auto data = primer.toChunkData();
    if (!data) {
        return nullptr;
    }

    ChunkData* stored = storeGeneratedChunk(x, z, std::move(data));

    // 将生成的实体添加到世界
    if (stored && !spawnedEntities.empty()) {
        if (m_world) {
            m_world->spawnEntitiesFromChunkGeneration(spawnedEntities);
        } else if (m_entitySpawnCallback) {
            m_entitySpawnCallback(spawnedEntities);
        }
    }

    return stored;
}

// ============================================================================
// 统计
// ============================================================================

size_t ServerChunkManager::loadedChunkCount() const
{
    std::lock_guard<std::mutex> lock(m_chunksMutex);
    return m_chunks.size();
}

size_t ServerChunkManager::holderCount() const
{
    return m_holderManager ? m_holderManager->holderCount() : 0;
}

size_t ServerChunkManager::pendingTaskCount() const
{
    if (m_taskScheduler) {
        size_t count = m_taskScheduler->getMainThreadExecutor().size();
        count += m_taskScheduler->getRadiusAwareScheduler().size();
        return count;
    }
    return 0;
}

} // namespace mc::server
