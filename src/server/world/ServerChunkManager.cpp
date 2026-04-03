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
        // 创建 WorldGenRegion（简化版）
        std::array<IChunk*, 9> chunks{};
        std::array<std::unique_ptr<IChunk>, 9> neighborAdapters{};
        chunks[4] = &chunk;  // 中心区块

        // 获取邻居区块（如果可用）
        getNeighborChunks(chunk.x(), chunk.z(), chunks, neighborAdapters);

        WorldGenRegion region(chunk.x(), chunk.z(), chunks);

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
                    MC_TRACE_EVENT("world.chunk_gen", "Lighting");
                    // 光照初始化
                    chunk.initializeSkyLight();
                    chunk.initializeBlockLight();
                } else if (status == ChunkStatuses::SPAWN) {
                    // SPAWN 阶段：计算生物生成点
                    // 参考 MC 1.16.5: SPAWN 阶段计算初始生成位置
                    // 目前简化实现
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

        // 在区块生成完成后调用 spawnInitialMobs
        // 参考 MC 1.16.5 performWorldGenSpawning
        // 注意：这里我们已经在 FEATURES 阶段之后，地形已经完整生成
        if (chunk.hasCompletedStatus(ChunkStatuses::FEATURES)) {
            if (cancelSignal.load(std::memory_order_acquire)) {
                return;
            }
            MC_TRACE_EVENT("world.chunk_gen", "SpawnInitialMobs");
            std::vector<SpawnedEntityData> entities;
            m_generator->spawnInitialMobs(region, chunk, entities);

            // 将生成的实体数据存储到 ChunkPrimer 中
            for (auto& entityData : entities) {
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

    if (!control.shouldEnqueue) {
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

    // 创建 WorldGenRegion（简化版）
    std::array<IChunk*, 9> chunks{};
    std::array<std::unique_ptr<IChunk>, 9> neighborAdapters{};
    chunks[4] = primer;

    // 获取邻居区块
    getNeighborChunks(singleChunkLifecycleManager.x(), singleChunkLifecycleManager.z(), chunks, neighborAdapters);

    WorldGenRegion region(singleChunkLifecycleManager.x(), singleChunkLifecycleManager.z(), chunks);

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
                // 光照初始化
                primer->initializeSkyLight();
                primer->initializeBlockLight();
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

    // 检查 8 个邻居
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

    for (i32 i = 0; i < 9; ++i) {
        // 跳过中心位置（调用者已设置）
        if (i == 4) {
            continue;
        }

        SingleChunkLifecycleManager* singleChunkLifecycleManager = getSingleChunkLifecycleManager(x + offsets[i][0], z + offsets[i][1]);
        if (singleChunkLifecycleManager) {
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
    MC_TRACE_EVENT("server.chunk", "CheckUnloading", "loadedChunkCount", loadedChunkCount(), "singleChunkLifecycleManagerCount", singleChunkLifecycleManagerCount());

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
        MC_TRACE_INSTANT("server.chunk", "UnloadChunk", "x", chunkId.x, "z", chunkId.z);
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
