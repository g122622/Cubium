#include "ServerChunkManager.hpp"
#include "ServerWorld.hpp"
#include "../sync/ChunkSendManager.hpp"
#include "common/perfetto/TraceEvents.hpp"
#include "common/util/assert/AssertAll.hpp"
#include "common/world/WorldConstants.hpp"
#include "common/world/storage/db/SectionCodec.hpp"
#include "common/world/storage/db/SectionKey.hpp"
#include "common/world/storage/section/SectionManager.hpp"
#include <chrono>
#include <cmath>
#include <spdlog/spdlog.h>

namespace mc::server {

namespace {

/**
 * @brief 将 ChunkData 适配为生成期 `IChunk` 视图
 *
 * 世界生成阶段需要从邻居窗口里读取统一的 `IChunk` 接口，
 * 而内存里已完成的区块实际类型是 `ChunkData`。这里保留一个轻量适配器，
 * 仅用于生成窗口拼装，不引入兼容层语义。
 */
class ChunkDataChunkAdapter : public IChunk {
public:
    explicit ChunkDataChunkAdapter(std::shared_ptr<ChunkData> chunk)
        : m_chunk(std::move(chunk))
        , m_status(m_chunk && m_chunk->isFullyGenerated() ? ChunkLoadStatus::Generated : ChunkLoadStatus::Generating)
    {
    }

    [[nodiscard]] ChunkCoord x() const override { return m_chunk ? m_chunk->x() : 0; }
    [[nodiscard]] ChunkCoord z() const override { return m_chunk ? m_chunk->z() : 0; }
    [[nodiscard]] ChunkPos pos() const override { return m_chunk ? m_chunk->pos() : ChunkPos(0, 0); }

    [[nodiscard]] const BlockState* getBlockState(BlockCoord x, BlockCoord y, BlockCoord z) const override {
        return m_chunk ? m_chunk->getBlockState(x, y, z) : nullptr;
    }

    void setBlockState(BlockCoord x, BlockCoord y, BlockCoord z, const BlockState* state) override {
        MC_ASSERT_RELEASE(m_chunk != nullptr);
        m_chunk->setBlockState(x, y, z, state);
    }

    [[nodiscard]] u32 getBlockStateId(BlockCoord x, BlockCoord y, BlockCoord z) const override {
        return m_chunk ? m_chunk->getBlockStateId(x, y, z) : 0;
    }

    void setBlockStateId(BlockCoord x, BlockCoord y, BlockCoord z, u32 stateId) override {
        MC_ASSERT_RELEASE(m_chunk != nullptr);
        m_chunk->setBlockStateId(x, y, z, stateId);
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
        MC_ASSERT_RELEASE(m_chunk != nullptr);
        return m_chunk->createSection(index);
    }

    [[nodiscard]] const ChunkSection* const* getSections() const override {
        MC_ASSERT_RELEASE(m_chunk != nullptr);
        return m_chunk->getSections();
    }

    [[nodiscard]] BiomeId getBiomeAtBlock(BlockCoord x, BlockCoord y, BlockCoord z) const override {
        MC_ASSERT_RELEASE(m_chunk != nullptr);
        return m_chunk->getBiomeAtBlock(x, y, z);
    }

    [[nodiscard]] BlockCoord getTopBlockY(HeightmapType type, BlockCoord x, BlockCoord z) const override {
        MC_ASSERT_RELEASE(m_chunk != nullptr);
        return m_chunk->getTopBlockY(type, x, z);
    }

    void updateHeightmap(HeightmapType type, BlockCoord x, BlockCoord y, BlockCoord z, const BlockState* state) override {
        MC_UNUSED(type);
        MC_UNUSED(y);
        MC_UNUSED(state);
        MC_ASSERT_RELEASE(m_chunk != nullptr);
        m_chunk->updateHeightMap(x, z);
    }

    [[nodiscard]] ChunkLoadStatus getStatus() const override {
        return m_status;
    }

    void setStatus(ChunkLoadStatus status) override {
        m_status = status;
    }

    [[nodiscard]] bool isModified() const override {
        MC_ASSERT_RELEASE(m_chunk != nullptr);
        return m_chunk->isDirty();
    }

    void setModified(bool modified) override {
        MC_ASSERT_RELEASE(m_chunk != nullptr);
        m_chunk->setDirty(modified);
    }

private:
    std::shared_ptr<ChunkData> m_chunk;
    ChunkLoadStatus m_status = ChunkLoadStatus::Generated;
};

/**
 * @brief 统一完成等待者集合
 *
 * @param waiters 待完成等待者
 * @param success 是否成功
 * @param chunk 返回区块
 */
void fulfillWaiters(std::vector<SingleChunkLifecycleManager::Waiter> waiters, bool success, ChunkData* chunk)
{
    for (auto& waiter : waiters) {
        if (waiter.promise) {
            waiter.promise->set_value(success ? chunk : nullptr);
        }
        if (waiter.callback) {
            waiter.callback(success, success ? chunk : nullptr);
        }
    }
}

} // namespace

// ============================================================================
// 构造与析构
// ============================================================================

ServerChunkManager::ServerChunkManager(ServerWorld& world, std::unique_ptr<IChunkGenerator> generator)
    : m_world(&world)
    , m_generator(std::move(generator))
{
    m_ticketManager.setLevelChangeCallback([this](ChunkCoord x, ChunkCoord z, i32 oldLevel, i32 newLevel) {
        onTicketLevelChanged(x, z, oldLevel, newLevel);
    });
}

ServerChunkManager::ServerChunkManager(std::unique_ptr<IChunkGenerator> generator)
    : m_world(nullptr)
    , m_generator(std::move(generator))
{
    m_ticketManager.setLevelChangeCallback([this](ChunkCoord x, ChunkCoord z, i32 oldLevel, i32 newLevel) {
        onTicketLevelChanged(x, z, oldLevel, newLevel);
    });
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
    MC_TRACE_EVENT("server.initialization", "ServerChunkManager::initialize");
    return {};
}

void ServerChunkManager::shutdown()
{
    std::vector<std::unique_ptr<SingleChunkLifecycleManager>> lifecycleManagers;
    {
        std::lock_guard<std::mutex> lock(m_lifecycleManagersMutex);
        lifecycleManagers.reserve(m_lifecycleManagers.size());
        for (auto& [key, lifecycleManager] : m_lifecycleManagers) {
            MC_UNUSED(key);
            if (lifecycleManager) {
                lifecycleManager->cancelActiveWork();
                lifecycleManagers.push_back(std::move(lifecycleManager));
            }
        }
        m_lifecycleManagers.clear();
    }

    for (auto& lifecycleManager : lifecycleManagers) {
        if (lifecycleManager) {
            failWaiters(lifecycleManager->takeAllWaiters());
        }
    }

    std::lock_guard<std::mutex> lock(m_chunksMutex);
    m_chunks.clear();
}

// ============================================================================
// 内存查询
// ============================================================================

ChunkData* ServerChunkManager::tryToGetChunkInMem(ChunkCoord x, ChunkCoord z)
{
    return tryToGetChunkSharedInMem(x, z).get();
}

const ChunkData* ServerChunkManager::tryToGetChunkInMem(ChunkCoord x, ChunkCoord z) const
{
    return tryToGetChunkSharedInMem(x, z).get();
}

std::shared_ptr<ChunkData> ServerChunkManager::tryToGetChunkSharedInMem(ChunkCoord x, ChunkCoord z)
{
    const u64 key = posToKey(x, z);
    std::lock_guard<std::mutex> lock(m_chunksMutex);
    auto it = m_chunks.find(key);
    return it != m_chunks.end() ? it->second : std::shared_ptr<ChunkData>{};
}

std::shared_ptr<const ChunkData> ServerChunkManager::tryToGetChunkSharedInMem(ChunkCoord x, ChunkCoord z) const
{
    const u64 key = posToKey(x, z);
    std::lock_guard<std::mutex> lock(m_chunksMutex);
    auto it = m_chunks.find(key);
    return it != m_chunks.end() ? it->second : std::shared_ptr<const ChunkData>{};
}

bool ServerChunkManager::hasChunkInMem(ChunkCoord x, ChunkCoord z) const
{
    return tryToGetChunkInMem(x, z) != nullptr;
}

// ============================================================================
// 请求入口
// ============================================================================

ChunkData* ServerChunkManager::requestChunkSync(ChunkCoord x, ChunkCoord z, const ChunkStatus& targetStatus)
{
    if (ChunkData* chunk = tryToGetChunkInMem(x, z)) {
        return chunk;
    }

    std::lock_guard<std::mutex> generationLock(m_syncGenerationMutex);

    if (ChunkData* chunk = tryToGetChunkInMem(x, z)) {
        return chunk;
    }

    if (m_world && m_world->isStorageOpen()) {
        if (auto loadedChunk = tryToLoadChunkFromStorageSync(x, z)) {
            SingleChunkLifecycleManager& lifecycleManager = getOrCreateLifecycleManager(x, z);
            lifecycleManager.markLoadedFromStorageReady();
            return storeChunkInMemorySync(x, z, std::move(loadedChunk));
        }
    }

    SingleChunkLifecycleManager& lifecycleManager = getOrCreateLifecycleManager(x, z);
    const i32 priority = computeSchedulePriority(x, z, targetStatus, m_ticketManager.getChunkLevel(x, z));
    MC_UNUSED(priority);
    executeGenerationSync(lifecycleManager, targetStatus);
    return tryToGetChunkInMem(x, z);
}

ChunkData* ServerChunkManager::requestFullChunkSync(ChunkCoord x, ChunkCoord z)
{
    return requestChunkSync(x, z, ChunkStatuses::FULL);
}

std::future<ChunkData*> ServerChunkManager::requestChunkAsync(ChunkCoord x, ChunkCoord z, const ChunkStatus& targetStatus)
{
    auto promise = std::make_shared<std::promise<ChunkData*>>();
    auto future = promise->get_future();
    submitChunkRequest(x, z, targetStatus, {}, promise);
    return future;
}

void ServerChunkManager::requestChunkAsync(
    ChunkCoord x,
    ChunkCoord z,
    const ChunkStatus& targetStatus,
    ChunkCallback callback)
{
    submitChunkRequest(x, z, targetStatus, std::move(callback), {});
}

void ServerChunkManager::submitChunkRequest(
    ChunkCoord x,
    ChunkCoord z,
    const ChunkStatus& targetStatus,
    ChunkCallback callback,
    std::shared_ptr<std::promise<ChunkData*>> promise)
{
    MC_TRACE_EVENT(
        "server.chunk",
        "ServerChunkManager::submitChunkRequest",
        "x", x,
        "z", z,
        "targetStatus", targetStatus.name());

    if (ChunkData* chunk = tryToGetChunkInMem(x, z)) {
        std::vector<SingleChunkLifecycleManager::Waiter> waiters;
        waiters.emplace_back(SingleChunkLifecycleManager::Waiter{std::move(callback), std::move(promise)});
        fulfillWaiters(std::move(waiters), true, chunk);
        return;
    }

    SingleChunkLifecycleManager& lifecycleManager = getOrCreateLifecycleManager(x, z);
    const i32 priority = computeSchedulePriority(x, z, targetStatus, m_ticketManager.getChunkLevel(x, z));
    auto decision = lifecycleManager.submitRequest(targetStatus, priority, std::move(callback), std::move(promise));
    advanceChunkState(lifecycleManager, decision);
}

// ============================================================================
// 状态机推进
// ============================================================================

void ServerChunkManager::advanceChunkState(
    SingleChunkLifecycleManager& lifecycleManager,
    const SingleChunkLifecycleManager::EnqueueDecision& decision)
{
    if (decision.shouldWakeReadyWaiters) {
        completeReadyWaiters(lifecycleManager);
        return;
    }

    if (decision.shouldResolveStorage) {
        resolveChunkSourceSync(lifecycleManager);
        return;
    }

    if (decision.shouldQueueGeneration) {
        enqueueChunkGenerationAsync(lifecycleManager, decision);
    }
}

void ServerChunkManager::resolveChunkSourceSync(SingleChunkLifecycleManager& lifecycleManager)
{
    const ChunkCoord x = lifecycleManager.x();
    const ChunkCoord z = lifecycleManager.z();

    std::unique_ptr<ChunkData> loadedChunk;
    if (m_world && m_world->isStorageOpen()) {
        loadedChunk = tryToLoadChunkFromStorageSync(x, z);
    }

    auto decision = lifecycleManager.noteStorageResolved(loadedChunk != nullptr);
    if (loadedChunk) {
        lifecycleManager.markLoadedFromStorageReady();
        ChunkData* stored = storeChunkInMemorySync(x, z, std::move(loadedChunk));
        MC_UNUSED(stored);
        completeReadyWaiters(lifecycleManager);
        wakeBlockedNeighborsAsync(x, z);
        return;
    }

    if (const ChunkStatus* prerequisiteStatus = getNeighborPrerequisiteStatus(lifecycleManager.requestedStatus())) {
        decision = lifecycleManager.noteNeighborProgress(areNeighborsReady(x, z, *prerequisiteStatus));
    } else {
        decision = lifecycleManager.noteNeighborProgress(true);
    }

    advanceChunkState(lifecycleManager, decision);
}

void ServerChunkManager::enqueueChunkGenerationAsync(
    SingleChunkLifecycleManager& lifecycleManager,
    const SingleChunkLifecycleManager::EnqueueDecision& decision)
{
    const ChunkCoord x = lifecycleManager.x();
    const ChunkCoord z = lifecycleManager.z();

    // 同步路径直接生成，避免在没有 worker 池时把请求永久挂起。
    if (m_workerPool == nullptr || !m_workerPool->isRunning()) {
        std::lock_guard<std::mutex> generationLock(m_syncGenerationMutex);
        lifecycleManager.noteGenerationStarted(decision.generation);
        executeGenerationSync(lifecycleManager, *decision.targetStatus);
        auto completionDecision = lifecycleManager.noteGenerationFinished(
            decision.generation,
            SingleChunkLifecycleManager::CompletionState::Succeeded);
        MC_UNUSED(completionDecision);
        completeReadyWaiters(lifecycleManager);
        wakeBlockedNeighborsAsync(x, z);
        return;
    }

    lifecycleManager.noteGenerationQueued(decision.generation);

    auto task = std::make_unique<ChunkGenerateTask>(
        x,
        z,
        *decision.targetStatus,
        [this](ChunkPrimer& chunk, const ChunkStatus& targetStatus, const std::atomic<bool>& cancelSignal) {
            const auto& allStatuses = ChunkStatus::getAll();
            for (const auto& status : allStatuses) {
                if (cancelSignal.load(std::memory_order_acquire)) {
                    return;
                }
                if (status.ordinal() > targetStatus.ordinal()) {
                    break;
                }
                if (chunk.hasCompletedStatus(status)) {
                    continue;
                }

                const i32 regionRadius = std::max(0, status.taskRange());
                std::vector<IChunk*> neighbors(static_cast<size_t>((regionRadius * 2 + 1) * (regionRadius * 2 + 1)), nullptr);
                std::vector<std::unique_ptr<IChunk>> neighborAdapters(neighbors.size());
                collectNeighborChunks(chunk.x(), chunk.z(), regionRadius, &chunk, neighbors, neighborAdapters);
                WorldGenRegion region(chunk.x(), chunk.z(), regionRadius, std::move(neighbors));

                if (status == ChunkStatuses::STRUCTURE_STARTS) {
                    m_generator->generateStructureStarts(region, chunk);
                } else if (status == ChunkStatuses::STRUCTURE_REFERENCES) {
                    m_generator->generateStructureReferences(region, chunk);
                } else if (status == ChunkStatuses::BIOMES) {
                    m_generator->generateBiomes(region, chunk);
                } else if (status == ChunkStatuses::NOISE) {
                    m_generator->generateNoise(region, chunk);
                } else if (status == ChunkStatuses::SURFACE) {
                    m_generator->buildSurface(region, chunk);
                } else if (status == ChunkStatuses::CARVERS) {
                    m_generator->applyCarvers(region, chunk, false);
                } else if (status == ChunkStatuses::LIQUID_CARVERS) {
                    m_generator->applyCarvers(region, chunk, true);
                } else if (status == ChunkStatuses::FEATURES) {
                    m_generator->placeFeatures(region, chunk);
                } else if (status == ChunkStatuses::HEIGHTMAPS) {
                    chunk.updateAllHeightmaps();
                }

                chunk.setChunkStatus(status);
            }

            if (!cancelSignal.load(std::memory_order_acquire) && chunk.hasCompletedStatus(ChunkStatuses::HEIGHTMAPS)) {
                std::vector<SpawnedEntityData> entities;
                constexpr i32 spawnRegionRadius = 1;
                std::vector<IChunk*> neighbors(static_cast<size_t>((spawnRegionRadius * 2 + 1) * (spawnRegionRadius * 2 + 1)), nullptr);
                std::vector<std::unique_ptr<IChunk>> neighborAdapters(neighbors.size());
                collectNeighborChunks(chunk.x(), chunk.z(), spawnRegionRadius, &chunk, neighbors, neighborAdapters);
                WorldGenRegion region(chunk.x(), chunk.z(), spawnRegionRadius, std::move(neighbors));
                m_generator->spawnInitialMobs(region, chunk, entities);
                for (auto& entityData : entities) {
                    chunk.addSpawnedEntity(std::move(entityData));
                }
            }
        });

    m_workerPool->submit(
        std::move(task),
        [this, x, z, generation = decision.generation](bool success, util::ITask* task) {
            SingleChunkLifecycleManager* lifecycleManager = findLifecycleManager(x, z);
            if (!lifecycleManager || !lifecycleManager->isGenerationCurrent(generation)) {
                return;
            }

            lifecycleManager->noteGenerationStarted(generation);

            ChunkData* storedChunk = nullptr;
            auto completionState = SingleChunkLifecycleManager::CompletionState::Failed;

            if (success && task) {
                auto* generationTask = static_cast<ChunkGenerateTask*>(task);
                auto result = generationTask->takeResult();
                if (result) {
                    storedChunk = finalizeGeneratedChunkSync(x, z, *result);
                    if (storedChunk != nullptr) {
                        completionState = SingleChunkLifecycleManager::CompletionState::Succeeded;
                    }
                }
            }

            auto completionDecision = lifecycleManager->noteGenerationFinished(generation, completionState);
            if (completionState == SingleChunkLifecycleManager::CompletionState::Succeeded) {
                completeReadyWaiters(*lifecycleManager);
                wakeBlockedNeighborsAsync(x, z);
            } else {
                MC_UNUSED(completionDecision);
            }
        },
        static_cast<util::TaskPriority>(decision.priority),
        decision.cancelToken);
}

void ServerChunkManager::completeReadyWaiters(SingleChunkLifecycleManager& lifecycleManager)
{
    ChunkData* chunk = tryToGetChunkInMem(lifecycleManager.x(), lifecycleManager.z());
    if (!chunk) {
        chunk = lifecycleManager.chunkData();
    }
    fulfillWaiters(lifecycleManager.takeReadyWaiters(), chunk != nullptr, chunk);
}

void ServerChunkManager::failWaiters(std::vector<SingleChunkLifecycleManager::Waiter> waiters)
{
    fulfillWaiters(std::move(waiters), false, nullptr);
}

// ============================================================================
// 生命周期管理器访问
// ============================================================================

SingleChunkLifecycleManager& ServerChunkManager::getOrCreateLifecycleManager(ChunkCoord x, ChunkCoord z)
{
    const u64 key = posToKey(x, z);
    std::lock_guard<std::mutex> lock(m_lifecycleManagersMutex);
    auto it = m_lifecycleManagers.find(key);
    if (it != m_lifecycleManagers.end()) {
        return *it->second;
    }

    auto lifecycleManager = std::make_unique<SingleChunkLifecycleManager>(x, z);
    auto* ptr = lifecycleManager.get();
    m_lifecycleManagers[key] = std::move(lifecycleManager);
    return *ptr;
}

SingleChunkLifecycleManager* ServerChunkManager::findLifecycleManager(ChunkCoord x, ChunkCoord z)
{
    const u64 key = posToKey(x, z);
    std::lock_guard<std::mutex> lock(m_lifecycleManagersMutex);
    auto it = m_lifecycleManagers.find(key);
    return it != m_lifecycleManagers.end() ? it->second.get() : nullptr;
}

const SingleChunkLifecycleManager* ServerChunkManager::findLifecycleManager(ChunkCoord x, ChunkCoord z) const
{
    const u64 key = posToKey(x, z);
    std::lock_guard<std::mutex> lock(m_lifecycleManagersMutex);
    auto it = m_lifecycleManagers.find(key);
    return it != m_lifecycleManagers.end() ? it->second.get() : nullptr;
}

// ============================================================================
// 票据与唤醒
// ============================================================================

void ServerChunkManager::onTicketLevelChanged(ChunkCoord x, ChunkCoord z, i32 oldLevel, i32 newLevel)
{
    MC_UNUSED(oldLevel);

    SingleChunkLifecycleManager& lifecycleManager = getOrCreateLifecycleManager(x, z);
    lifecycleManager.setLevel(newLevel);

    if (newLevel <= world::ChunkLoadTicketManager::MAX_LOADED_LEVEL) {
        const ChunkStatus& targetStatus = lifecycleManager.requestedStatus().ordinal() > ChunkStatuses::EMPTY.ordinal()
            ? lifecycleManager.requestedStatus()
            : ChunkStatuses::FULL;
        submitChunkRequest(x, z, targetStatus, {}, {});
        return;
    }

    lifecycleManager.cancelActiveWork();
    failWaiters(lifecycleManager.takeAllWaiters());
}

void ServerChunkManager::wakeBlockedNeighborsAsync(ChunkCoord x, ChunkCoord z)
{
    MC_TRACE_EVENT("server.chunk", "ServerChunkManager::wakeBlockedNeighborsAsync");

    static constexpr i32 retryRadius = 8;
    std::vector<SingleChunkLifecycleManager*> neighbors;

    {
        std::lock_guard<std::mutex> lock(m_lifecycleManagersMutex);
        neighbors.reserve(static_cast<size_t>((retryRadius * 2 + 1) * (retryRadius * 2 + 1)));
        for (i32 dz = -retryRadius; dz <= retryRadius; ++dz) {
            for (i32 dx = -retryRadius; dx <= retryRadius; ++dx) {
                if (dx == 0 && dz == 0) {
                    continue;
                }
                auto it = m_lifecycleManagers.find(posToKey(x + dx, z + dz));
                if (it != m_lifecycleManagers.end()) {
                    neighbors.push_back(it->second.get());
                }
            }
        }
    }

    for (SingleChunkLifecycleManager* neighbor : neighbors) {
        if (!neighbor || !neighbor->isWaitingForNeighbors()) {
            continue;
        }

        const ChunkStatus* prerequisiteStatus = getNeighborPrerequisiteStatus(neighbor->requestedStatus());
        const bool neighborsReady = prerequisiteStatus == nullptr
            ? true
            : areNeighborsReady(neighbor->x(), neighbor->z(), *prerequisiteStatus);
        advanceChunkState(*neighbor, neighbor->noteNeighborProgress(neighborsReady));
    }
}

// ============================================================================
// 同步生成与邻居依赖
// ============================================================================

void ServerChunkManager::executeGenerationSync(
    SingleChunkLifecycleManager& lifecycleManager,
    const ChunkStatus& targetStatus)
{
    ChunkPrimer* primer = lifecycleManager.createGeneratingChunk();
    MC_ASSERT_RELEASE(primer != nullptr);

    const auto& allStatuses = ChunkStatus::getAll();
    for (const auto& status : allStatuses) {
        if (status.ordinal() > targetStatus.ordinal()) {
            break;
        }
        if (primer->hasCompletedStatus(status)) {
            continue;
        }

        const i32 regionRadius = std::max(0, status.taskRange());
        std::vector<IChunk*> neighbors(static_cast<size_t>((regionRadius * 2 + 1) * (regionRadius * 2 + 1)), nullptr);
        std::vector<std::unique_ptr<IChunk>> neighborAdapters(neighbors.size());
        collectNeighborChunks(lifecycleManager.x(), lifecycleManager.z(), regionRadius, primer, neighbors, neighborAdapters);
        WorldGenRegion region(lifecycleManager.x(), lifecycleManager.z(), regionRadius, std::move(neighbors));

        if (status == ChunkStatuses::STRUCTURE_STARTS) {
            m_generator->generateStructureStarts(region, *primer);
        } else if (status == ChunkStatuses::STRUCTURE_REFERENCES) {
            m_generator->generateStructureReferences(region, *primer);
        } else if (status == ChunkStatuses::BIOMES) {
            m_generator->generateBiomes(region, *primer);
        } else if (status == ChunkStatuses::NOISE) {
            m_generator->generateNoise(region, *primer);
        } else if (status == ChunkStatuses::SURFACE) {
            m_generator->buildSurface(region, *primer);
        } else if (status == ChunkStatuses::CARVERS) {
            m_generator->applyCarvers(region, *primer, false);
        } else if (status == ChunkStatuses::LIQUID_CARVERS) {
            m_generator->applyCarvers(region, *primer, true);
        } else if (status == ChunkStatuses::FEATURES) {
            m_generator->placeFeatures(region, *primer);
        } else if (status == ChunkStatuses::HEIGHTMAPS) {
            primer->updateAllHeightmaps();
        }

        primer->setChunkStatus(status);
    }

    if (primer->hasCompletedStatus(ChunkStatuses::HEIGHTMAPS)) {
        std::vector<SpawnedEntityData> entities;
        constexpr i32 spawnRegionRadius = 1;
        std::vector<IChunk*> neighbors(static_cast<size_t>((spawnRegionRadius * 2 + 1) * (spawnRegionRadius * 2 + 1)), nullptr);
        std::vector<std::unique_ptr<IChunk>> neighborAdapters(neighbors.size());
        collectNeighborChunks(lifecycleManager.x(), lifecycleManager.z(), spawnRegionRadius, primer, neighbors, neighborAdapters);
        WorldGenRegion region(lifecycleManager.x(), lifecycleManager.z(), spawnRegionRadius, std::move(neighbors));
        m_generator->spawnInitialMobs(region, *primer, entities);

        for (auto& entityData : entities) {
            primer->addSpawnedEntity(std::move(entityData));
        }
    }

    auto data = lifecycleManager.completeGeneration();
    MC_ASSERT_RELEASE(data != nullptr);
    ChunkData* stored = storeChunkInMemorySync(lifecycleManager.x(), lifecycleManager.z(), std::move(data));
    MC_ASSERT_RELEASE(stored != nullptr);
}

bool ServerChunkManager::areNeighborsReady(
    ChunkCoord x,
    ChunkCoord z,
    const ChunkStatus& prerequisiteStatus) const
{
    if (prerequisiteStatus.taskRange() <= 0) {
        return true;
    }

    const ChunkStatus* requiredStatus = prerequisiteStatus.parent();
    if (!requiredStatus) {
        requiredStatus = &prerequisiteStatus;
    }

    const i32 range = prerequisiteStatus.taskRange();
    for (i32 dz = -range; dz <= range; ++dz) {
        for (i32 dx = -range; dx <= range; ++dx) {
            if (dx == 0 && dz == 0) {
                continue;
            }

            const SingleChunkLifecycleManager* neighbor = findLifecycleManager(x + dx, z + dz);
            if (!neighbor || !neighbor->hasCompletedStatus(*requiredStatus)) {
                return false;
            }
        }
    }

    return true;
}

const ChunkStatus* ServerChunkManager::getNeighborPrerequisiteStatus(const ChunkStatus& targetStatus) const
{
    const auto& allStatuses = ChunkStatus::getAll();
    const ChunkStatus* prerequisiteStage = nullptr;
    i32 prerequisiteRange = 0;

    for (const auto& status : allStatuses) {
        if (status.ordinal() > targetStatus.ordinal()) {
            break;
        }

        const i32 statusRange = status.taskRange();
        if (statusRange > prerequisiteRange ||
            (statusRange == prerequisiteRange && prerequisiteStage && status.ordinal() > prerequisiteStage->ordinal())) {
            prerequisiteRange = statusRange;
            prerequisiteStage = &status;
        }
    }

    if (prerequisiteRange <= 0) {
        return nullptr;
    }

    const ChunkStatus* prerequisiteStatus = prerequisiteStage ? prerequisiteStage->parent() : nullptr;
    return prerequisiteStatus ? prerequisiteStatus : prerequisiteStage;
}

i32 ServerChunkManager::computeSchedulePriority(
    ChunkCoord x,
    ChunkCoord z,
    const ChunkStatus& targetStatus,
    i32 ticketLevel) const
{
    const i32 normalizedLevel = std::clamp(ticketLevel, 0, world::ChunkDistanceGraph::MAX_LEVEL);
    const i32 statusPenalty = std::max(0, ChunkStatuses::FULL.ordinal() - targetStatus.ordinal());
    const i32 spatialPenalty = static_cast<i32>((std::abs(x) + std::abs(z)) & 0xFF);
    return normalizedLevel * 1024 + statusPenalty * 32 + spatialPenalty;
}

// ============================================================================
// 邻居窗口
// ============================================================================

void ServerChunkManager::collectNeighborChunks(
    ChunkCoord x,
    ChunkCoord z,
    i32 radius,
    IChunk* centerChunk,
    std::vector<IChunk*>& neighbors,
    std::vector<std::unique_ptr<IChunk>>& neighborAdapters)
{
    const i32 diameter = radius * 2 + 1;
    MC_ASSERT_RELEASE(centerChunk != nullptr);
    MC_ASSERT_RELEASE(static_cast<i32>(neighbors.size()) == diameter * diameter);
    MC_ASSERT_RELEASE(static_cast<i32>(neighborAdapters.size()) == diameter * diameter);

    for (i32 dz = -radius; dz <= radius; ++dz) {
        for (i32 dx = -radius; dx <= radius; ++dx) {
            const size_t index = static_cast<size_t>((dz + radius) * diameter + (dx + radius));

            if (dx == 0 && dz == 0) {
                neighbors[index] = centerChunk;
                continue;
            }

            if (auto loadedChunk = tryToGetChunkSharedInMem(x + dx, z + dz)) {
                neighborAdapters[index] = std::make_unique<ChunkDataChunkAdapter>(std::move(loadedChunk));
                neighbors[index] = neighborAdapters[index].get();
                continue;
            }

            neighborAdapters[index] = std::make_unique<ChunkPrimer>(x + dx, z + dz);
            neighbors[index] = neighborAdapters[index].get();
        }
    }
}

// ============================================================================
// 存储与发布
// ============================================================================

ChunkData* ServerChunkManager::storeChunkInMemorySync(ChunkCoord x, ChunkCoord z, std::unique_ptr<ChunkData> data)
{
    MC_ASSERT_RELEASE(data != nullptr);
    std::shared_ptr<ChunkData> sharedChunk(std::move(data));
    MC_ASSERT_RELEASE(sharedChunk != nullptr);

    ChunkData* stored = nullptr;
    {
        std::lock_guard<std::mutex> lock(m_chunksMutex);
        auto& slot = m_chunks[posToKey(x, z)];
        if (slot && slot.get() == sharedChunk.get()) {
            stored = slot.get();
        } else {
            slot = std::move(sharedChunk);
            stored = slot.get();
        }
    }

    if (SingleChunkLifecycleManager* lifecycleManager = findLifecycleManager(x, z)) {
        lifecycleManager->setStatus(ChunkStatuses::FULL);
    }

    if (m_chunkLoadedCallback && stored) {
        m_chunkLoadedCallback(x, z);
    }

    return stored;
}

ChunkData* ServerChunkManager::finalizeGeneratedChunkSync(ChunkCoord x, ChunkCoord z, ChunkPrimer& primer)
{
    std::vector<SpawnedEntityData> spawnedEntities;
    if (primer.spawnedEntityCount() > 0) {
        spawnedEntities = std::move(primer.spawnedEntities());
    }

    auto data = primer.toChunkData();
    if (!data) {
        return nullptr;
    }

    if (SingleChunkLifecycleManager* lifecycleManager = findLifecycleManager(x, z)) {
        lifecycleManager->markGenerationReady();
    }

    ChunkData* stored = storeChunkInMemorySync(x, z, std::move(data));
    if (stored && !spawnedEntities.empty()) {
        if (m_world) {
            m_world->spawnEntitiesFromChunkGeneration(spawnedEntities);
        } else if (m_entitySpawnCallback) {
            m_entitySpawnCallback(spawnedEntities);
        }
    }

    return stored;
}

void ServerChunkManager::saveChunkSectionsSync(const ChunkData& chunk)
{
    if (!m_world || !m_world->isStorageOpen()) {
        return;
    }

    auto& storageService = m_world->storage();
    auto dimension = m_world->dimension();
    auto& sectionManager = storageService.sectionManager(dimension);

    std::vector<BiomeId> biomes;
    {
        const auto biomeBytes = chunk.getBiomes().serialize();
        biomes.reserve(biomeBytes.size() / 2);
        for (size_t i = 0; i + 1 < biomeBytes.size(); i += 2) {
            const u16 low = static_cast<u16>(biomeBytes[i]);
            const u16 high = static_cast<u16>(biomeBytes[i + 1]);
            biomes.push_back(static_cast<BiomeId>(low | (high << 8)));
        }
    }

    for (i8 sectionY = 0; sectionY < world::CHUNK_SECTIONS; ++sectionY) {
        const ChunkSection* section = chunk.getSection(sectionY);
        if (!section) {
            continue;
        }

        world::storage::SectionKey sectionKey(chunk.x(), chunk.z(), sectionY, dimension);
        auto sectionDataResult = world::storage::SectionCodec::fromChunkSection(*section, sectionKey, biomes);
        if (!sectionDataResult.success()) {
            continue;
        }

        auto saveResult = sectionManager.saveSectionSync(sectionKey, sectionDataResult.value());
        if (saveResult.failed()) {
            spdlog::info(
                "保存 section 失败 ({}, {}, {}): {}",
                chunk.x(),
                sectionY,
                chunk.z(),
                saveResult.error().message());
        }
    }
}

std::unique_ptr<ChunkData> ServerChunkManager::tryToLoadChunkFromStorageSync(ChunkCoord x, ChunkCoord z)
{
    MC_TRACE_EVENT("server.chunk", "ServerChunkManager::tryToLoadChunkFromStorageSync");

    if (!m_world || !m_world->isStorageOpen()) {
        return nullptr;
    }

    auto& storageService = m_world->storage();
    auto dimension = m_world->dimension();
    auto& sectionManager = storageService.sectionManager(dimension);

    auto chunk = std::make_unique<ChunkData>(x, z);
    bool hasAnySection = false;
    bool hasBiomes = false;
    mc::BiomeContainer biomeContainer;

    for (i8 sectionY = 0; sectionY < world::CHUNK_SECTIONS; ++sectionY) {
        world::storage::SectionKey sectionKey(x, z, sectionY, dimension);
        auto loadResult = sectionManager.loadSectionSync(sectionKey);
        if (loadResult.failed() || !loadResult.value()) {
            continue;
        }

        const auto sectionData = loadResult.value();
        if (!sectionData) {
            continue;
        }

        if (!hasBiomes && sectionData->biomes.size() == mc::BiomeContainer::BIOME_SIZE) {
            for (i32 biomeY = 0; biomeY < mc::BiomeContainer::BIOME_HEIGHT; ++biomeY) {
                for (i32 biomeZ = 0; biomeZ < mc::BiomeContainer::BIOME_DEPTH; ++biomeZ) {
                    for (i32 biomeX = 0; biomeX < mc::BiomeContainer::BIOME_WIDTH; ++biomeX) {
                        const size_t biomeIndex = static_cast<size_t>(
                            biomeY * mc::BiomeContainer::BIOME_WIDTH * mc::BiomeContainer::BIOME_DEPTH +
                            biomeZ * mc::BiomeContainer::BIOME_WIDTH +
                            biomeX);
                        biomeContainer.setBiome(biomeX, biomeY, biomeZ, sectionData->biomes[biomeIndex]);
                    }
                }
            }
            hasBiomes = true;
        }

        ChunkSection* section = chunk->createSection(sectionY);
        if (!section) {
            continue;
        }

        auto applyResult = world::storage::SectionCodec::toChunkSection(*sectionData, *section);
        if (applyResult.failed()) {
            spdlog::info(
                "应用 section 数据失败 ({}, {}, {}): {}",
                x,
                sectionY,
                z,
                applyResult.error().message());
            continue;
        }

        hasAnySection = true;
    }

    if (!hasAnySection) {
        return nullptr;
    }

    if (hasBiomes) {
        chunk->setBiomes(std::move(biomeContainer));
    }

    chunk->setLoaded(true);
    chunk->setFullyGenerated(true);
    chunk->setDirty(false);
    return chunk;
}

// ============================================================================
// 卸载
// ============================================================================

void ServerChunkManager::unloadChunkSync(ChunkCoord x, ChunkCoord z)
{
    const u64 key = posToKey(x, z);

    if (m_world && m_world->isStorageOpen()) {
        std::shared_ptr<ChunkData> chunkToSave;
        {
            std::lock_guard<std::mutex> lock(m_chunksMutex);
            auto it = m_chunks.find(key);
            if (it != m_chunks.end() && it->second && it->second->isDirty()) {
                chunkToSave = it->second;
            }
        }

        if (chunkToSave) {
            saveChunkSectionsSync(*chunkToSave);
            chunkToSave->setDirty(false);
        }
    }

    if (m_chunkSendManager) {
        m_chunkSendManager->onChunkPreUnload(x, z);
    }
    if (m_chunkUnloadedCallback) {
        m_chunkUnloadedCallback(x, z);
    }

    std::unique_ptr<SingleChunkLifecycleManager> lifecycleManager;
    {
        std::lock_guard<std::mutex> lock(m_lifecycleManagersMutex);
        auto it = m_lifecycleManagers.find(key);
        if (it != m_lifecycleManagers.end()) {
            lifecycleManager = std::move(it->second);
            m_lifecycleManagers.erase(it);
        }
    }

    if (lifecycleManager) {
        lifecycleManager->cancelActiveWork();
        failWaiters(lifecycleManager->takeAllWaiters());
    }

    {
        std::lock_guard<std::mutex> lock(m_chunksMutex);
        m_chunks.erase(key);
    }
}

void ServerChunkManager::checkChunkUnloading()
{
    std::vector<u64> toUnload;

    {
        std::lock_guard<std::mutex> lock(m_lifecycleManagersMutex);
        for (const auto& [key, lifecycleManager] : m_lifecycleManagers) {
            if (!lifecycleManager) {
                continue;
            }

            if (!lifecycleManager->shouldLoad() &&
                !m_ticketManager.hasTrackingPlayers(key) &&
                !lifecycleManager->hasGeneratingChunk()) {
                toUnload.push_back(key);
            }
        }
    }

    for (u64 key : toUnload) {
        auto chunkId = ChunkId::fromId(key);
        unloadChunkSync(chunkId.x, chunkId.z);
    }
}

// ============================================================================
// 票据与 tick
// ============================================================================

void ServerChunkManager::updatePlayerPosition(PlayerId player, f64 x, f64 z)
{
    const ChunkCoord chunkX = static_cast<ChunkCoord>(std::floor(x / world::CHUNK_WIDTH));
    const ChunkCoord chunkZ = static_cast<ChunkCoord>(std::floor(z / world::CHUNK_WIDTH));
    m_ticketManager.updatePlayerPosition(player, chunkX, chunkZ);
}

void ServerChunkManager::removePlayer(PlayerId player)
{
    m_ticketManager.removePlayer(player);
}

void ServerChunkManager::forceChunk(ChunkCoord x, ChunkCoord z, bool force)
{
    m_ticketManager.forceChunk(x, z, force);
}

void ServerChunkManager::setViewDistance(i32 distance)
{
    m_ticketManager.setViewDistance(distance);
}

void ServerChunkManager::tick()
{
    ++m_currentTick;
    processTicketUpdatesSync();

    if (m_currentTick - m_lastUnloadCheckTick >= UNLOAD_CHECK_INTERVAL_TICKS) {
        checkChunkUnloading();
        m_lastUnloadCheckTick = m_currentTick;
    }
}

// ============================================================================
// 统计
// ============================================================================

size_t ServerChunkManager::loadedChunkCount() const
{
    std::lock_guard<std::mutex> lock(m_chunksMutex);
    return m_chunks.size();
}

void ServerChunkManager::forEachLoadedChunk(const std::function<bool(ChunkData&)>& callback)
{
    std::vector<std::shared_ptr<ChunkData>> chunks;
    {
        std::lock_guard<std::mutex> lock(m_chunksMutex);
        chunks.reserve(m_chunks.size());
        for (const auto& [key, chunk] : m_chunks) {
            MC_UNUSED(key);
            if (chunk) {
                chunks.push_back(chunk);
            }
        }
    }

    for (auto& chunk : chunks) {
        if (chunk && !callback(*chunk)) {
            break;
        }
    }
}

void ServerChunkManager::forEachLoadedChunk(const std::function<bool(const ChunkData&)>& callback) const
{
    std::vector<std::shared_ptr<const ChunkData>> chunks;
    {
        std::lock_guard<std::mutex> lock(m_chunksMutex);
        chunks.reserve(m_chunks.size());
        for (const auto& [key, chunk] : m_chunks) {
            MC_UNUSED(key);
            if (chunk) {
                chunks.push_back(chunk);
            }
        }
    }

    for (const auto& chunk : chunks) {
        if (chunk && !callback(*chunk)) {
            break;
        }
    }
}

size_t ServerChunkManager::lifecycleManagerCount() const
{
    std::lock_guard<std::mutex> lock(m_lifecycleManagersMutex);
    return m_lifecycleManagers.size();
}

size_t ServerChunkManager::pendingTaskCount() const
{
    return m_workerPool ? m_workerPool->pendingTaskCount() : 0;
}

} // namespace mc::server
