#include "ServerChunkManager.hpp"
#include "ServerWorld.hpp"
#include "../sync/ChunkSendManager.hpp"
#include "../../common/world/WorldConstants.hpp"
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
        MC_ASSERT_RELEASE(m_chunk);
        return m_chunk->getSections();
    }

    [[nodiscard]] BiomeId getBiomeAtBlock(BlockCoord x, BlockCoord y, BlockCoord z) const override {
        MC_ASSERT_RELEASE(m_chunk);
        return m_chunk->getBiomeAtBlock(x, y, z);
    }

    [[nodiscard]] BlockCoord getTopBlockY(HeightmapType type, BlockCoord x, BlockCoord z) const override {
        MC_ASSERT_RELEASE(m_chunk);
        return m_chunk->getTopBlockY(type, x, z);
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
        MC_ASSERT_RELEASE(m_chunk);
        return m_chunk->isDirty();
    }

    void setModified(bool modified) override {
        if (m_chunk) {
            m_chunk->setDirty(modified);
        } else {
            MC_ASSERT_RELEASE(false);
        }
    }

private:
    std::shared_ptr<ChunkData> m_chunk;
    ChunkLoadStatus m_status = ChunkLoadStatus::Generated;
};

} // namespace

// ============================================================================
// 构造与析构
// ============================================================================

ServerChunkManager::ServerChunkManager(ServerWorld& world, std::unique_ptr<IChunkGenerator> generator)
    : m_world(&world)
    , m_generator(std::move(generator))
    , m_workerPool(-1)  // 自动检测线程数
{
    // 设置票据管理器回调
    m_ticketManager.setLevelChangeCallback([this](ChunkCoord x, ChunkCoord z, i32 oldLevel, i32 newLevel) {
        onTicketLevelChanged(x, z, oldLevel, newLevel);
    });
}

ServerChunkManager::ServerChunkManager(std::unique_ptr<IChunkGenerator> generator)
    : m_world(nullptr)
    , m_generator(std::move(generator))
    , m_workerPool(-1)  // 自动检测线程数
{
    // 设置票据管理器回调
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

    // 启动 Worker 线程池
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
    std::lock_guard<std::mutex> lock(m_singleChunkLifecycleManagersMutex);
    m_singleChunkLifecycleManagers.clear();

    // 清理区块缓存
    std::lock_guard<std::mutex> chunksLock(m_chunksMutex);
    m_chunks.clear();
}

// ============================================================================
// Worker 管理
// ============================================================================

void ServerChunkManager::startWorkers()
{
    // 设置生成器函数
    m_workerPool.setGenerator([this](ChunkPrimer& chunk, const ChunkStatus& targetStatus, const std::atomic<bool>& cancelSignal) {
        // 按阶段生成
        const auto& allStatuses = ChunkStatus::getAll();
        for (const auto& status : allStatuses) {
            if (cancelSignal.load(std::memory_order_acquire)) {
                return;
            }

            if (status.ordinal() > targetStatus.ordinal()) {
                break;
            }

            if (!chunk.hasCompletedStatus(status)) {
                const i32 regionRadius = std::max(0, status.taskRange());
                std::vector<IChunk*> chunks;
                std::vector<std::unique_ptr<IChunk>> neighborAdapters;
                chunks.resize(static_cast<size_t>((regionRadius * 2 + 1) * (regionRadius * 2 + 1)), nullptr);
                neighborAdapters.resize(chunks.size());

                getNeighborChunks(chunk.x(), chunk.z(), regionRadius, &chunk, chunks, neighborAdapters);
                WorldGenRegion region(chunk.x(), chunk.z(), regionRadius, std::move(chunks));

                // 执行生成
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
                    // 异步路径：暂时跳过邻居检查
                    // TODO: 实现完整的两阶段生成系统
                    // 第一阶段：所有区块生成到 LIQUID_CARVERS
                    // 第二阶段：批量执行 FEATURES
                    m_generator->placeFeatures(region, chunk);
                } else if (status == ChunkStatuses::LIGHT) {
                    // LIGHT 阶段：区块生成系统中的占位阶段
                    // 真正的光照计算在区块加载后由 WorldLightManager::lightChunk() 完成
                    // 参考 Moonrise: ChunkLightTask 在区块加载后异步执行光照
                    MC_TRACE_EVENT("world.chunk_gen", "Lighting");
                    // 不在此处执行光照计算，因为需要访问已加载的邻居区块
                    // 光照将在 storeGeneratedChunkToMem 触发的回调中完成
                } else if (status == ChunkStatuses::SPAWN) {
                    // SPAWN 阶段：计算生物生成点
                    // 参考 MC 1.16.5: SPAWN 阶段计算初始生成位置
                    // TODO 目前简化实现
                    MC_TRACE_EVENT("world.chunk_gen", "Spawn");
                } else if (status == ChunkStatuses::HEIGHTMAPS) {
                    MC_TRACE_EVENT("world.chunk_gen", "Heightmaps");
                    chunk.updateAllHeightmaps();
                }

                chunk.setChunkStatus(status);
            }
        }

        if (cancelSignal.load(std::memory_order_acquire)) {
            return;
        }

        // 在高度图阶段完成后调用 spawnInitialMobs。
        // 生成点计算依赖 MotionBlockingNoLeaves 等高度图，必须等 HEIGHTMAPS 执行完。
        if (chunk.hasCompletedStatus(ChunkStatuses::HEIGHTMAPS)) {
            if (cancelSignal.load(std::memory_order_acquire)) {
                return;
            }
            MC_TRACE_EVENT("world.chunk_gen", "SpawnInitialMobs");
            std::vector<SpawnedEntityData> entities;
            std::vector<IChunk*> chunks;
            std::vector<std::unique_ptr<IChunk>> neighborAdapters;
            constexpr i32 spawnRegionRadius = 1;
            chunks.resize(static_cast<size_t>((spawnRegionRadius * 2 + 1) * (spawnRegionRadius * 2 + 1)), nullptr);
            neighborAdapters.resize(chunks.size());

            getNeighborChunks(chunk.x(), chunk.z(), spawnRegionRadius, &chunk, chunks, neighborAdapters);
            WorldGenRegion region(chunk.x(), chunk.z(), spawnRegionRadius, std::move(chunks));
            m_generator->spawnInitialMobs(region, chunk, entities);

            // 将生成的实体数据存储到 ChunkPrimer 中
            for (auto& entityData : entities) {
                // spdlog::info("Chunk ({}, {}): Spawned entity {} at ({}, {}, {}) with reason {}",
                //              chunk.x(), chunk.z(),
                //              entityData.entityTypeId, entityData.x, entityData.y, entityData.z,
                //              entityData.spawnReason);
                chunk.addSpawnedEntity(std::move(entityData));
            }
        }

        // 注意：实体数据将在 finalizeChunkGeneration 中提取并添加到世界
    });

    m_workerPool.start();
}

void ServerChunkManager::stopWorkers()
{
    m_workerPool.shutdown();
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
    // spdlog::debug("Requesting chunk synchronously at ({}, {}), current m_chunks size: {}", x, z, m_chunks.size());

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
    SingleChunkLifecycleManager* singleChunkLifecycleManager = getOrCreateSingleChunkLifecycleManager(x, z);
    if (!singleChunkLifecycleManager) {
        return nullptr;
    }

    // 检查是否已完成
    if (ChunkData* data = singleChunkLifecycleManager->getChunkData()) {
        return data;
    }

    // 同步生成到 FULL 状态，确保与异步路径结果一致
    executeGenerationTask(*singleChunkLifecycleManager, ChunkStatuses::FULL);

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

    {
        std::lock_guard<std::mutex> lock(m_singleChunkLifecycleManagersMutex);
        m_singleChunkLifecycleManagers.erase(key);
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
    SingleChunkLifecycleManager* singleChunkLifecycleManager = getOrCreateSingleChunkLifecycleManager(x, z);
    if (!singleChunkLifecycleManager) {
        promise->set_value(nullptr);
        return future;
    }

    // 调度异步生成
    const ChunkStatus& target = targetStatus ? *targetStatus : ChunkStatuses::FULL;

    requestChunkGeneration(x, z, target,
                           computeSchedulePriority(x, z, target, m_ticketManager.getChunkLevel(x, z)),
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
    SingleChunkLifecycleManager* singleChunkLifecycleManager = getOrCreateSingleChunkLifecycleManager(x, z);
    if (!singleChunkLifecycleManager) {
        if (callback) callback(false, nullptr);
        return;
    }

    // 调度异步生成
    const ChunkStatus& target = targetStatus ? *targetStatus : ChunkStatuses::FULL;

    requestChunkGeneration(x, z, target,
                           computeSchedulePriority(x, z, target, m_ticketManager.getChunkLevel(x, z)),
                           std::move(callback),
                           nullptr);
}

// ============================================================================
// 区块持有者
// ============================================================================

SingleChunkLifecycleManager* ServerChunkManager::getOrCreateSingleChunkLifecycleManager(ChunkCoord x, ChunkCoord z)
{
    const u64 key = posToKey(x, z);

    std::lock_guard<std::mutex> lock(m_singleChunkLifecycleManagersMutex);

    auto it = m_singleChunkLifecycleManagers.find(key);
    if (it != m_singleChunkLifecycleManagers.end()) {
        return it->second.get();
    }

    auto singleChunkLifecycleManager = std::make_unique<SingleChunkLifecycleManager>(x, z);
    singleChunkLifecycleManager->setStatusChangeCallback([this](SingleChunkLifecycleManager& manager) {
        onChunkStatusChanged(manager.x(), manager.z(), manager.getStatus());
    });

    auto* ptr = singleChunkLifecycleManager.get();
    m_singleChunkLifecycleManagers[key] = std::move(singleChunkLifecycleManager);
    return ptr;
}

SingleChunkLifecycleManager* ServerChunkManager::getSingleChunkLifecycleManager(ChunkCoord x, ChunkCoord z)
{
    const u64 key = posToKey(x, z);

    std::lock_guard<std::mutex> lock(m_singleChunkLifecycleManagersMutex);
    auto it = m_singleChunkLifecycleManagers.find(key);
    return it != m_singleChunkLifecycleManagers.end() ? it->second.get() : nullptr;
}

const SingleChunkLifecycleManager* ServerChunkManager::getSingleChunkLifecycleManager(ChunkCoord x, ChunkCoord z) const
{
    const u64 key = posToKey(x, z);

    std::lock_guard<std::mutex> lock(m_singleChunkLifecycleManagersMutex);
    auto it = m_singleChunkLifecycleManagers.find(key);
    return it != m_singleChunkLifecycleManagers.end() ? it->second.get() : nullptr;
}

// ============================================================================
// 票据管理
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

// ============================================================================
// 主循环
// ============================================================================

void ServerChunkManager::tick()
{
    MC_TRACE_EVENT("server.chunk", "ChunkManagerTick");

    ++m_currentTick;

    // 处理票据更新
    m_ticketManager.tick();

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

void ServerChunkManager::scheduleGeneration(SingleChunkLifecycleManager& singleChunkLifecycleManager, const ChunkStatus& targetStatus)
{
    // 如果已经达到目标状态、已有缓存结果或已有正在使用的 primer，则不重复调度
    if (singleChunkLifecycleManager.hasCompletedStatus(targetStatus) ||
        getChunk(singleChunkLifecycleManager.x(), singleChunkLifecycleManager.z()) != nullptr) {
        return;
    }

    if (!singleChunkLifecycleManager.shouldLoad()) {
        cancelPendingGeneration(singleChunkLifecycleManager.x(), singleChunkLifecycleManager.z());
        return;
    }

    if (const ChunkStatus* prerequisiteStatus = getNeighborPrerequisiteStatus(targetStatus)) {
        if (!checkNeighborsReady(singleChunkLifecycleManager.x(), singleChunkLifecycleManager.z(), *prerequisiteStatus)) {
            return;
        }
    }

    const i32 ticketLevel = m_ticketManager.getChunkLevel(singleChunkLifecycleManager.x(),
                                                           singleChunkLifecycleManager.z());
    const i32 priority = computeSchedulePriority(singleChunkLifecycleManager.x(),
                                                 singleChunkLifecycleManager.z(),
                                                 targetStatus,
                                                 ticketLevel);

    requestChunkGeneration(singleChunkLifecycleManager.x(),
                           singleChunkLifecycleManager.z(),
                           targetStatus,
                           priority,
                           nullptr,
                           nullptr);
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

    // 这里返回“高邻域阶段的父阶段”作为调度前置，避免在初始阶段形成互相等待。
    // 例如 FULL 的最大邻域阶段是 FEATURES（range=8），其父阶段是 LIQUID_CARVERS（range=0），
    // 可先让任务进入 worker 推进流水线，再由状态回调驱动后续重试。
    const ChunkStatus* prerequisiteStatus = prerequisiteStage ? prerequisiteStage->parent() : nullptr;
    return prerequisiteStatus ? prerequisiteStatus : prerequisiteStage;
}

void ServerChunkManager::requestChunkGeneration(ChunkCoord x, ChunkCoord z,
                                                const ChunkStatus& targetStatus,
                                                i32 priority,
                                                ChunkCallback callback,
                                                std::shared_ptr<std::promise<ChunkData*>> promise)
{
    MC_ASSERT_RELEASE_MSG(targetStatus.ordinal() >= 0, "Invalid target chunk status ordinal");

    const u64 key = posToKey(x, z);
    SingleChunkLifecycleManager* singleChunkLifecycleManager = getOrCreateSingleChunkLifecycleManager(x, z);
    if (!singleChunkLifecycleManager) {
        if (promise) {
            promise->set_value(nullptr);
        }
        if (callback) {
            callback(false, nullptr);
        }
        return;
    }

    ChunkRequestControl control = singleChunkLifecycleManager->upsertRequest(targetStatus, priority);

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

    if (const ChunkStatus* prerequisiteStatus = getNeighborPrerequisiteStatus(targetStatus)) {
        if (!checkNeighborsReady(x, z, *prerequisiteStatus)) {
            return;
        }
    }

    if (!control.shouldEnqueue) {
        return;
    }

    if (!singleChunkLifecycleManager->tryMarkRequestSubmitted(control.generation)) {
        return;
    }

    m_workerPool.submitGenerate(x, z, targetStatus,
        [this, key, x, z, generation = control.generation](bool success, ChunkPrimer* primer) {
            ChunkData* stored = nullptr;

            SingleChunkLifecycleManager* singleChunkLifecycleManager = getSingleChunkLifecycleManager(x, z);
            if (singleChunkLifecycleManager && !singleChunkLifecycleManager->tryStartRequest(generation)) {
                success = false;
            }

            if (singleChunkLifecycleManager && !singleChunkLifecycleManager->isGenerationCurrent(generation)) {
                success = false;
            }

            if (success && primer) {
                stored = finalizeChunkGeneration(x, z, *primer);
            }

            PendingGeneration pending;
            bool staleGeneration = false;
            {
                std::lock_guard<std::mutex> lock(m_pendingGenerationsMutex);
                auto it = m_pendingGenerations.find(key);
                if (it != m_pendingGenerations.end()) {
                    if (it->second.generation != generation) {
                        staleGeneration = true;
                    } else {
                        pending = std::move(it->second);
                        m_pendingGenerations.erase(it);
                    }
                }
            }

            if (staleGeneration) {
                return;
            }

            const bool callbackSuccess = stored != nullptr;

            if (singleChunkLifecycleManager) {
                const bool cancelled = !callbackSuccess;
                singleChunkLifecycleManager->finishRequest(generation, callbackSuccess, cancelled);
            }

            for (auto& p : pending.promises) {
                if (p) {
                    p->set_value(stored);
                }
            }

            for (auto& cb : pending.callbacks) {
                if (cb) {
                    cb(callbackSuccess, stored);
                }
            }
        },
        control.cancelToken,
        priority);
}

void ServerChunkManager::cancelPendingGeneration(ChunkCoord x, ChunkCoord z)
{
    const u64 key = posToKey(x, z);

    if (SingleChunkLifecycleManager* singleChunkLifecycleManager = getSingleChunkLifecycleManager(x, z)) {
        singleChunkLifecycleManager->cancelActiveRequest();
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

    SingleChunkLifecycleManager* singleChunkLifecycleManager = getOrCreateSingleChunkLifecycleManager(x, z);
    if (!singleChunkLifecycleManager) {
        return;
    }

    singleChunkLifecycleManager->setLevel(newLevel);

    if (newLevel <= world::ChunkLoadTicketManager::MAX_LOADED_LEVEL) {
        scheduleGeneration(*singleChunkLifecycleManager, ChunkStatuses::FULL);
        return;
    }

    cancelPendingGeneration(x, z);
}

i32 ServerChunkManager::computeSchedulePriority(ChunkCoord x, ChunkCoord z,
                                                const ChunkStatus& targetStatus,
                                                i32 ticketLevel) const
{
    const i32 normalizedLevel = std::clamp(ticketLevel, 0, world::ChunkDistanceGraph::MAX_LEVEL);
    const i32 statusPenalty = std::max(0, ChunkStatuses::FULL.ordinal() - targetStatus.ordinal());
    const i32 spatialPenalty = static_cast<i32>((std::abs(x) + std::abs(z)) & 0xFF);
    return normalizedLevel * 1024 + statusPenalty * 32 + spatialPenalty;
}

void ServerChunkManager::executeGenerationTask(SingleChunkLifecycleManager& singleChunkLifecycleManager, const ChunkStatus& status)
{
    // 创建区块生成器
    ChunkPrimer* primer = singleChunkLifecycleManager.createGeneratingChunk();
    if (!primer) {
        return;
    }

    // 按阶段生成
    const auto& allStatuses = ChunkStatus::getAll();
    for (const auto& s : allStatuses) {
        if (s.ordinal() > status.ordinal()) {
            break;
        }

        if (!primer->hasCompletedStatus(s)) {
            const i32 regionRadius = std::max(0, s.taskRange());
            std::vector<IChunk*> chunks;
            std::vector<std::unique_ptr<IChunk>> neighborAdapters;
            chunks.resize(static_cast<size_t>((regionRadius * 2 + 1) * (regionRadius * 2 + 1)), nullptr);
            neighborAdapters.resize(chunks.size());

            getNeighborChunks(singleChunkLifecycleManager.x(), singleChunkLifecycleManager.z(), regionRadius, primer, chunks, neighborAdapters);
            WorldGenRegion region(singleChunkLifecycleManager.x(), singleChunkLifecycleManager.z(), regionRadius, std::move(chunks));

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
                // LIGHT 阶段：区块生成系统中的占位阶段
                // 真正的光照计算在区块加载后由 WorldLightManager::lightChunk() 完成
            } else if (s == ChunkStatuses::SPAWN) {
                // SPAWN 阶段：计算生物生成点
                MC_TRACE_EVENT("world.chunk_gen", "Spawn");
            } else if (s == ChunkStatuses::HEIGHTMAPS) {
                primer->updateAllHeightmaps();
            }

            primer->setChunkStatus(s);
        }
    }

    // 在高度图阶段完成后调用 spawnInitialMobs。
    // 生成点计算依赖 MotionBlockingNoLeaves 等高度图，必须等 HEIGHTMAPS 执行完。
    if (primer->hasCompletedStatus(ChunkStatuses::HEIGHTMAPS)) {
        std::vector<SpawnedEntityData> entities;
        std::vector<IChunk*> chunks;
        std::vector<std::unique_ptr<IChunk>> neighborAdapters;
        constexpr i32 spawnRegionRadius = 1;
        chunks.resize(static_cast<size_t>((spawnRegionRadius * 2 + 1) * (spawnRegionRadius * 2 + 1)), nullptr);
        neighborAdapters.resize(chunks.size());

        getNeighborChunks(singleChunkLifecycleManager.x(), singleChunkLifecycleManager.z(), spawnRegionRadius, primer, chunks, neighborAdapters);
        WorldGenRegion region(singleChunkLifecycleManager.x(), singleChunkLifecycleManager.z(), spawnRegionRadius, std::move(chunks));
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
    auto data = singleChunkLifecycleManager.completeGeneration();
    if (data) {
        // 存储到缓存
        ChunkData* stored = storeGeneratedChunkToMem(singleChunkLifecycleManager.x(), singleChunkLifecycleManager.z(), std::move(data));

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

    // 检查邻居区块
    const i32 range = status.taskRange();
    for (i32 dz = -range; dz <= range; ++dz) {
        for (i32 dx = -range; dx <= range; ++dx) {
            if (dx == 0 && dz == 0) continue;

            const SingleChunkLifecycleManager* neighbor = getSingleChunkLifecycleManager(x + dx, z + dz);
            if (!neighbor || !neighbor->hasCompletedStatus(*requiredStatus)) {
                return false;
            }
        }
    }

    return true;
}

void ServerChunkManager::onChunkStatusChanged(ChunkCoord x, ChunkCoord z, const ChunkStatus& /*status*/)
{
    static constexpr i32 retryRadius = 8;

    std::vector<SingleChunkLifecycleManager*> neighbors;
    neighbors.reserve(static_cast<size_t>((retryRadius * 2 + 1) * (retryRadius * 2 + 1)));

    {
        std::lock_guard<std::mutex> lock(m_singleChunkLifecycleManagersMutex);
        for (i32 dz = -retryRadius; dz <= retryRadius; ++dz) {
            for (i32 dx = -retryRadius; dx <= retryRadius; ++dx) {
                if (dx == 0 && dz == 0) {
                    continue;
                }

                auto it = m_singleChunkLifecycleManagers.find(posToKey(x + dx, z + dz));
                if (it != m_singleChunkLifecycleManagers.end()) {
                    neighbors.push_back(it->second.get());
                }
            }
        }
    }

    for (SingleChunkLifecycleManager* neighbor : neighbors) {
        if (neighbor) {
            scheduleGeneration(*neighbor, ChunkStatuses::FULL);
        }
    }
}

void ServerChunkManager::getNeighborChunks(
    ChunkCoord x,
    ChunkCoord z,
    i32 radius,
    IChunk* centerChunk,
    std::vector<IChunk*>& neighbors,
    std::vector<std::unique_ptr<IChunk>>& neighborAdapters)
{
    const i32 diameter = radius * 2 + 1;
    MC_ASSERT_RELEASE(centerChunk);
    MC_ASSERT_RELEASE(static_cast<i32>(neighbors.size()) == diameter * diameter);
    MC_ASSERT_RELEASE(static_cast<i32>(neighborAdapters.size()) == diameter * diameter);

    for (i32 dz = -radius; dz <= radius; ++dz) {
        for (i32 dx = -radius; dx <= radius; ++dx) {
            const size_t index = static_cast<size_t>((dz + radius) * diameter + (dx + radius));

            if (dx == 0 && dz == 0) {
                neighbors[index] = centerChunk;
                continue;
            }

            SingleChunkLifecycleManager* singleChunkLifecycleManager = getSingleChunkLifecycleManager(x + dx, z + dz);
            if (singleChunkLifecycleManager) {
                if (auto data = getChunkShared(x + dx, z + dz)) {
                    neighborAdapters[index] = std::make_unique<ChunkDataChunkAdapter>(std::move(data));
                    neighbors[index] = neighborAdapters[index].get();
                } else {
                    // 邻居暂不可用时放入同坐标占位 ChunkPrimer，避免 WorldGenRegion 热路径出现空指针断言。
                    neighborAdapters[index] = std::make_unique<ChunkPrimer>(x + dx, z + dz);
                    neighbors[index] = neighborAdapters[index].get();
                }
            } else {
                // 尚未创建生命周期管理器的邻居同样使用占位区块填充，保持窗口完整性。
                neighborAdapters[index] = std::make_unique<ChunkPrimer>(x + dx, z + dz);
                neighbors[index] = neighborAdapters[index].get();
            }
        }
    }
}

ChunkData* ServerChunkManager::storeGeneratedChunkToMem(ChunkCoord x, ChunkCoord z, std::unique_ptr<ChunkData> data)
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

    if (SingleChunkLifecycleManager* singleChunkLifecycleManager = getSingleChunkLifecycleManager(x, z)) {
        singleChunkLifecycleManager->setStatus(ChunkStatuses::FULL);
    }

    // 调用区块加载回调（用于光照初始化等）
    if (m_chunkLoadedCallback && stored) {
        m_chunkLoadedCallback(x, z);
    }

    return stored;
}

void ServerChunkManager::processCompletedTasks()
{
    // Worker 线程池会自动处理完成的任务
    // 这里可以添加额外的后处理逻辑
}

void ServerChunkManager::checkChunkUnloading()
{
    // spdlog::info("[ServerChunkManager] Checking chunk unloading, current loaded chunks: {}, singleChunkLifecycleManagers: {}",
    //               loadedChunkCount(), singleChunkLifecycleManagerCount());
    MC_TRACE_EVENT(
        "server.chunk",
        "CheckUnloading",
        "loadedChunkCount", loadedChunkCount(),
        "singleChunkLifecycleManagerCount", singleChunkLifecycleManagerCount()
    );

    // 检查所有持有者
    std::vector<u64> toUnload;

    {
        std::lock_guard<std::mutex> lock(m_singleChunkLifecycleManagersMutex);
        for (const auto& [key, singleChunkLifecycleManager] : m_singleChunkLifecycleManagers) {
            // 没有票据且没有玩家追踪且没有正在生成
            if (!singleChunkLifecycleManager->shouldLoad() &&
                !m_ticketManager.hasTrackingPlayers(key) &&
                !singleChunkLifecycleManager->hasGeneratingChunk()) {
                toUnload.push_back(key);
            }
        }
    }

    // 卸载区块
    for (u64 key : toUnload) {
        auto chunkId = ChunkId::fromId(key);

        cancelPendingGeneration(chunkId.x, chunkId.z);

        // 在卸载前通知 ChunkSendManager 发送卸载包
        if (m_chunkSendManager) {
            m_chunkSendManager->onChunkPreUnload(chunkId.x, chunkId.z);
        }

        // 通知村庄管理器区块卸载（用于清理 POI 等）
        if (m_chunkUnloadedCallback) {
            m_chunkUnloadedCallback(chunkId.x, chunkId.z);
        }

        // spdlog::info("[ServerChunkManager] Unloading chunk: ({}, {})", chunkId.x, chunkId.z);
        ChunkPos chunkPos{chunkId.x, chunkId.z};
        MC_TRACE_INSTANT("server.chunk",
            "UnloadChunk",
            "x", chunkId.x,
            "z", chunkId.z,
            [flow = ::perfetto::Flow::ProcessScoped(chunkPos.toId())](::perfetto::EventContext ctx) {
                flow(ctx);
        });
        
        unloadChunk(chunkId.x, chunkId.z);
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

    ChunkData* stored = storeGeneratedChunkToMem(x, z, std::move(data));

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

size_t ServerChunkManager::singleChunkLifecycleManagerCount() const
{
    std::lock_guard<std::mutex> lock(m_singleChunkLifecycleManagersMutex);
    return m_singleChunkLifecycleManagers.size();
}

size_t ServerChunkManager::pendingTaskCount() const
{
    return m_workerPool.pendingTaskCount();
}

} // namespace mc::server
