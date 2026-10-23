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

#include "SingleLevelStorageManager.hpp"
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/profiler/TraceCategories.hpp"
#include "common/profiler/TraceEvents.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/nbt/Nbt.hpp"
#include "common/util/thread/ITask.hpp"
#include "common/util/thread/UniversalWorkerPool.hpp"
#include "common/world/WorldConstants.hpp"
#include "common/world/blockentity/BlockEntity.hpp"
#include "common/world/chunk/data/BiomeContainer.hpp"
#include "common/world/chunk/data/ChunkData.hpp"
#include "server/scoreboard/storage/ScoreboardDataManager.hpp"
#include "server/world/storage/backend/BedrockLDBBackend.hpp"
#include "server/world/storage/backend/JavaAnvilBackend.hpp"
#include "server/world/storage/blockentity/BlockEntityStorageManager.hpp"
#include "server/world/storage/core/LevelDatCodec.hpp"
#include "server/world/storage/core/SaveFormat.hpp"
#include "server/world/storage/core/WorldSessionLock.hpp"
#include "server/world/storage/db/ConsistencyMode.hpp"
#include "server/world/storage/db/RocksDBConfig.hpp"
#include "server/world/storage/db/RocksDBDatabase.hpp"
#include "server/world/storage/db/SectionCodec.hpp"
#include "server/world/storage/db/SectionKey.hpp"
#include "server/world/storage/entity/EntityStorageManager.hpp"
#include "server/world/storage/player/PlayerDataManager.hpp"
#include "server/world/storage/player/PlayerSaveData.hpp"
#include "server/world/storage/section/SectionManager.hpp"
#include "server/world/storage/snapshot/BackupManager.hpp"
#include "server/world/storage/task/StorageTask.hpp"
#include "server/world/storage/task/StorageTaskManager.hpp"
#include <array>
#include <atomic>
#include <cstddef>
#include <exception>
#include <filesystem>
#include <fstream>
#include <functional>
#include <future>
#include <ios>
#include <memory>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>
#include <fmt/format.h>
#include <nlohmann/json_fwd.hpp>
#include <spdlog/spdlog.h>

using namespace mc::trace;

namespace mc::world::storage {
namespace {

Result<void> _validateSectionFormat(RocksDBDatabase& database, bool readonly)
{
    const rocksdb::Slice key("native-section-format");
    constexpr std::string_view format = "compact-v1";
    auto marker = database.get(cf::META, key);
    if (marker.success()) {
        const auto& value = marker.value();
        if (std::string_view(reinterpret_cast<const char*>(value.data()), value.size()) == format) {
            return {};
        }
        return Error(ErrorCode::InvalidData, "Unsupported native section storage format");
    }
    if (marker.error().code() != ErrorCode::NotFound) {
        return marker.error();
    }
    for (const auto* name : {cf::SECTIONS_OVERWORLD, cf::SECTIONS_NETHER, cf::SECTIONS_THE_END}) {
        auto iterator = database.newIterator(name);
        iterator->SeekToFirst();
        if (iterator->Valid()) {
            return Error(ErrorCode::InvalidData,
                "Native section format marker is missing; existing section records are unsupported");
        }
        if (!iterator->status().ok()) {
            return Error(ErrorCode::FileReadFailed, iterator->status().ToString());
        }
    }
    if (readonly) {
        return Error(ErrorCode::InvalidData, "Native section format marker is missing");
    }
    return database.put(cf::META, key, rocksdb::Slice(format.data(), format.size()), true);
}

Result<rocksdb::WriteBatch> _captureSections(SectionManager& manager, std::span<const ChunkData* const> chunks)
{
    // 一次预留最终批次，避免追加大量段时反复扩容和复制已捕获的数据。
    // 每条额外预留 32 字节，覆盖标签、列族、键与长度变长整数；批头占 12 字节。
    size_t reservedBytes = 12;
    for (const ChunkData* chunk : chunks) {
        MC_ASSERT_RELEASE(chunk != nullptr);
        const auto lock = chunk->lockForLightRead();
        for (i32 index = 0; index < world::CHUNK_SECTIONS; ++index) {
            reservedBytes += SectionCodec::encodedSize(chunk->getSection(index)) + 32;
        }
    }
    rocksdb::WriteBatch batch(reservedBytes);
    for (const ChunkData* chunk : chunks) {
        auto result = manager.appendChunk(batch, *chunk);
        if (result.failed()) {
            return result.error();
        }
    }
    return batch;
}

} // namespace

SingleLevelStorageManager::SingleLevelStorageManager() = default;

SingleLevelStorageManager::~SingleLevelStorageManager()
{
    close();
}

SingleLevelStorageManager::SingleLevelStorageManager(SingleLevelStorageManager&& other) noexcept
    : m_db(std::move(other.m_db))
    , m_sessionLock(std::move(other.m_sessionLock))
    , m_backupManager(std::move(other.m_backupManager))
    , m_playerDataManager(std::move(other.m_playerDataManager))
    , m_entityStorage(std::move(other.m_entityStorage))
    , m_blockEntityStorage(std::move(other.m_blockEntityStorage))
    , m_scoreboardDataManager(std::move(other.m_scoreboardDataManager))
    , m_taskManager(std::move(other.m_taskManager))
    , m_sectionManagers(std::move(other.m_sectionManagers))
    , m_config(std::move(other.m_config))
    , m_worldPath(std::move(other.m_worldPath))
{

    m_ioWorkerPool = other.m_ioWorkerPool;
    other.m_ioWorkerPool = nullptr;

    m_computeWorkerPool = other.m_computeWorkerPool;
    other.m_computeWorkerPool = nullptr;
}

SingleLevelStorageManager& SingleLevelStorageManager::operator=(SingleLevelStorageManager&& other) noexcept
{
    if (this != &other) {
        close();
        m_db = std::move(other.m_db);
        m_sessionLock = std::move(other.m_sessionLock);
        m_backupManager = std::move(other.m_backupManager);
        m_playerDataManager = std::move(other.m_playerDataManager);
        m_entityStorage = std::move(other.m_entityStorage);
        m_scoreboardDataManager = std::move(other.m_scoreboardDataManager);
        m_taskManager = std::move(other.m_taskManager);
        m_sectionManagers = std::move(other.m_sectionManagers);
        m_config = std::move(other.m_config);
        m_worldPath = std::move(other.m_worldPath);

        m_ioWorkerPool = other.m_ioWorkerPool;
        other.m_ioWorkerPool = nullptr;

        m_computeWorkerPool = other.m_computeWorkerPool;
        other.m_computeWorkerPool = nullptr;
    }
    return *this;
}

void SingleLevelStorageManager::setIoWorkerPool(util::UniversalWorkerPool* workerPool)
{
    m_ioWorkerPool = workerPool;

    if (m_ioWorkerPool) {
        m_taskManager = std::make_unique<StorageTaskManager>(*m_ioWorkerPool);
    } else {
        m_taskManager.reset();
    }
}

Result<void> SingleLevelStorageManager::open(
    const std::filesystem::path& worldPath, const SingleLevelStorageConfig& config)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Server.World, "SingleLevelStorageManager::open", "path", worldPath.string());

    if (isOpen()) {
        return Error(ErrorCode::InvalidState, "Storage already open");
    }

    m_config = config;
    m_worldPath = worldPath;

    // 检测存档格式
    auto formatResult = SaveFormatDetector::detect(worldPath);
    if (formatResult.failed()) {
        return formatResult.error();
    }
    m_config.formatInfo = formatResult.value();

    // Native 格式必须可写：detect 只对外来格式（JavaAnvil / BedrockLDB）置 readonly。
    // 一旦出现 Native + readonly，说明检测逻辑把本可写的世界降级成了只读，此后所有落盘
    // 都会静默失败（readonly 下 saveChunk/saveEntity 直接返回），世界数据不保存却毫无征兆——
    // 新建世界被误判为外来 Java 格式时正是这种情况。
    MC_ASSERT_RELEASE_MSG(m_config.formatInfo.format != SaveFormat::Native || !m_config.formatInfo.readonly,
        "SingleLevelStorageManager::open: Native world detected as readonly; persistence would be silently discarded");

    // 外来格式：使用后端读取，强制只读
    if (m_config.formatInfo.format != SaveFormat::Native) {
        m_config.readonly = true;
        return _openForeignFormat(worldPath);
    }

    return _openNativeFormat(worldPath);
}

Result<void> SingleLevelStorageManager::_openNativeFormat(const std::filesystem::path& worldPath)
{
    std::filesystem::path dbPath = worldPath / "db";
    std::filesystem::path backupPath = worldPath / "backups";

    try {
        std::filesystem::create_directories(worldPath);
        std::filesystem::create_directories(dbPath);
        if (m_config.enableBackup) {
            std::filesystem::create_directories(backupPath);
        }
    }
    catch (const std::exception& e) {
        return Error(ErrorCode::FileWriteFailed, fmt::format("Failed to create directories: {}", e.what()));
    }

    auto lockResult =
        m_config.readonly ? WorldSessionLock::acquireReadOnly(worldPath) : WorldSessionLock::acquire(worldPath);
    if (!lockResult.success()) {
        return lockResult.error();
    }
    m_sessionLock.emplace(std::move(lockResult.value()));

    RocksDBConfig dbConfig = m_config.rocksdbConfig.value_or(RocksDBConfig{});
    dbConfig.consistencyMode = m_config.consistencyMode;

    auto dbResult = m_config.readonly ? RocksDBDatabase::openReadOnly(dbPath) : RocksDBDatabase::open(dbPath, dbConfig);
    if (!dbResult.success()) {
        m_sessionLock.reset();
        return dbResult.error();
    }
    m_db = dbResult.value();

    auto formatResult = _validateSectionFormat(*m_db, m_config.readonly);
    if (formatResult.failed()) {
        m_db.reset();
        m_sessionLock.reset();
        return formatResult.error();
    }

    if (m_config.enableBackup && !m_config.readonly) {
        auto backupResult = BackupManager::open(backupPath);
        if (backupResult.success()) {
            m_backupManager = backupResult.value();
        } else {
            spdlog::warn("Failed to initialize backup manager: {}", backupResult.error().message());
        }
    }

    m_playerDataManager = std::make_unique<PlayerDataManager>(*m_db);
    m_entityStorage = std::make_unique<EntityStorageManager>(*m_db);
    m_blockEntityStorage = std::make_unique<BlockEntityStorageManager>(*m_db);
    m_scoreboardDataManager = std::make_unique<mc::scoreboard::ScoreboardDataManager>(*this);

    if (!m_ioWorkerPool) {
        m_taskManager.reset();
    } else if (!m_taskManager) {
        m_taskManager = std::make_unique<StorageTaskManager>(*m_ioWorkerPool);
    }

    m_sectionManagers.clear();

    spdlog::info("SingleLevelStorageManager opened at {} (format: Native, consistency: {})",
        worldPath.string(),
        static_cast<i32>(m_config.consistencyMode));

    return {};
}

Result<void> SingleLevelStorageManager::_openForeignFormat(const std::filesystem::path& worldPath)
{
    auto lockResult = WorldSessionLock::acquireReadOnly(worldPath);
    if (!lockResult.success()) {
        return lockResult.error();
    }
    m_sessionLock.emplace(std::move(lockResult.value()));

    // 根据检测到的格式创建对应后端
    switch (m_config.formatInfo.format) {
        case SaveFormat::JavaAnvil:
            m_backend = std::make_unique<JavaAnvilBackend>();
            break;
        case SaveFormat::BedrockLDB:
            m_backend = std::make_unique<BedrockLDBBackend>();
            break;
        default:
            return Error(ErrorCode::InvalidState,
                fmt::format("Unsupported foreign save format: {}", m_config.formatInfo.formatName));
    }

    auto openResult = m_backend->open(worldPath, m_config.formatInfo);
    if (openResult.failed()) {
        m_backend.reset();
        m_sessionLock.reset();
        return openResult.error();
    }

    spdlog::info("SingleLevelStorageManager opened at {} (format: {}, readonly: true)",
        worldPath.string(),
        m_config.formatInfo.formatName);

    return {};
}

void SingleLevelStorageManager::_beginAsyncOperation()
{
    std::lock_guard lock(m_asyncOperationMutex);
    ++m_asyncOperationCount;
}

void SingleLevelStorageManager::_finishAsyncOperation()
{
    std::lock_guard lock(m_asyncOperationMutex);
    MC_ASSERT_RELEASE(m_asyncOperationCount > 0);
    if (--m_asyncOperationCount == 0) {
        m_asyncOperationCondition.notify_all();
    }
}

void SingleLevelStorageManager::close()
{
    if (!isOpen()) {
        return;
    }

    MC_TRACE_SCOPED_EVENT(TraceEvents.Server.World, "SingleLevelStorageManager::close");

    // 只等待本存档请求，不排空外部共享池；完成计数覆盖固定读取句柄的整个生命周期。
    {
        std::unique_lock lock(m_asyncOperationMutex);
        m_asyncOperationCondition.wait(lock, [this] {
            return m_asyncOperationCount == 0;
        });
    }

    {
        std::lock_guard<std::mutex> lock(m_sectionManagersMutex);
        m_sectionManagers.clear();
    }

    m_backupManager.reset();
    m_scoreboardDataManager.reset();
    m_entityStorage.reset();
    m_blockEntityStorage.reset();
    m_playerDataManager.reset();
    m_db.reset();
    m_backend.reset();
    m_taskManager.reset();
    m_ioWorkerPool = nullptr;
    m_computeWorkerPool = nullptr;
    m_sessionLock.reset();

    spdlog::info("SingleLevelStorageManager closed");

    m_worldPath.clear();
}

Result<size_t> SingleLevelStorageManager::saveChunks(
    DimensionId dimension, std::span<const ChunkData* const> chunks, bool sync)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Server.World, "SingleLevelStorageManager::saveChunks", "count", chunks.size());
    if (!isOpen()) {
        return Error(ErrorCode::InvalidState, "Storage not open");
    }
    if (m_config.readonly || m_backend || chunks.empty()) {
        return size_t{0};
    }
    auto& manager = _sectionManager(dimension);
    auto captured = _captureSections(manager, chunks);
    if (captured.failed()) {
        return captured.error();
    }
    return manager.saveBatch(captured.value(), sync);
}

Result<size_t> SingleLevelStorageManager::flushPlayerData()
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Server.World, "SingleLevelStorageManager::flushPlayerData");

    if (!isOpen()) {
        return Error(ErrorCode::InvalidState, "Storage not open");
    }

    if (m_config.readonly) {
        return Result<size_t>(0);
    }

    if (!m_playerDataManager) {
        return 0;
    }

    auto playerResult = m_playerDataManager->saveAllDirty();
    if (playerResult.failed()) {
        return playerResult.error();
    }
    return playerResult.value();
}

Result<void> SingleLevelStorageManager::saveChunk(const ChunkData& chunk, DimensionId dimension)
{
    if (!isOpen()) {
        return Error(ErrorCode::InvalidState, "Storage not open");
    }

    if (m_config.readonly) {
        return Result<void>::ok();
    }

    auto& manager = _sectionManager(dimension);

    const std::array<const ChunkData*, 1> chunks{&chunk};
    auto captured = _captureSections(manager, chunks);
    if (captured.failed()) {
        return captured.error();
    }
    auto saveResult = manager.saveBatch(captured.value(), false);
    if (saveResult.failed()) {
        return saveResult.error();
    }

    // 保存方块实体
    if (m_blockEntityStorage) {
        auto blockEntities = chunk.getAllBlockEntities();
        if (!blockEntities.empty()) {
            for (const auto* blockEntity : blockEntities) {
                auto beResult = m_blockEntityStorage->saveBlockEntity(*blockEntity, dimension);
                if (beResult.failed()) {
                    return beResult.error();
                }
            }
        }
    }

    return Result<void>::ok();
}

Result<std::optional<ChunkData>> SingleLevelStorageManager::loadChunk(ChunkCoord x, ChunkCoord z, DimensionId dimension)
{
    if (!isOpen()) {
        return Error(ErrorCode::InvalidState, "Storage not open");
    }

    // 外来格式：委托给后端读取
    if (m_backend) {
        auto result = m_backend->loadChunk(x, z, dimension);
        if (result.success()) {
            auto& chunk = result.value();
            if (chunk.has_value()) {
                chunk->initializeMissingHeightmaps();
            }
        }
        return result;
    }

    // Native 格式：通过 RocksDB SectionManager 读取
    auto& manager = _sectionManager(dimension);
    ChunkData chunk(x, z);
    bool hasAnySection = false;

    std::vector<SectionKey> keys;
    keys.reserve(world::CHUNK_SECTIONS);
    for (i8 sectionY = 0; sectionY < world::CHUNK_SECTIONS; ++sectionY) {
        keys.emplace_back(x, z, static_cast<i8>(world::sectionIndexToCoord(sectionY)), dimension);
    }

    auto loadResult = manager.loadSectionsSync(keys);
    if (loadResult.failed()) {
        return loadResult.error();
    }

    const auto& sections = loadResult.value();
    MC_ASSERT_RELEASE(sections.size() == keys.size());

    for (size_t i = 0; i < sections.size(); ++i) {
        const auto& sectionData = sections[i];
        if (!sectionData) {
            continue;
        }

        const i32 sectionIndex = world::sectionCoordToIndex(static_cast<i32>(keys[i].sectionY));
        auto applyResult = SectionCodec::restore(
            {reinterpret_cast<const u8*>(sectionData->data()), sectionData->size()}, chunk, sectionIndex);
        if (applyResult.failed()) {
            return applyResult.error();
        }

        hasAnySection = true;
    }

    if (!hasAnySection) {
        return std::optional<ChunkData>{};
    }

    // 加载方块实体
    if (m_blockEntityStorage) {
        auto beResult = m_blockEntityStorage->loadBlockEntitiesInChunk(x, z, dimension);
        if (beResult.success()) {
            for (auto& blockEntity : beResult.value()) {
                if (blockEntity != nullptr) {
                    // 【求值顺序】必须先取出位置再移交所有权：C++ 实参求值顺序未指定，
                    // 若 std::move(blockEntity) 先完成参数移动构造（置空该指针），
                    // 随后的 blockEntity->getPos() 就是对空指针解引用（实测崩溃点：读地址 0x14）。
                    const BlockPos blockEntityPos = blockEntity->getPos();
                    chunk.setBlockEntity(blockEntityPos, std::move(blockEntity));
                }
            }
        }
        // 方块实体加载失败不影响区块加载
    }

    chunk.initializeMissingHeightmaps();
    chunk.setLoaded(true);
    chunk.setFullyGenerated(true);
    chunk.setDirty(false);
    return std::optional<ChunkData>(std::move(chunk));
}

std::future<Result<std::optional<ChunkData>>> SingleLevelStorageManager::loadChunkAsync(ChunkCoord x,
    ChunkCoord z,
    DimensionId dimension,
    std::shared_ptr<std::atomic<bool>> abortSignal,
    util::TaskPriority priority)
{
    auto promise = std::make_shared<std::promise<Result<std::optional<ChunkData>>>>();
    auto future = promise->get_future();
    _loadChunkAsyncCore(
        x,
        z,
        dimension,
        std::move(abortSignal),
        [promise](Result<std::optional<ChunkData>> result) {
            promise->set_value(std::move(result));
        },
        priority);
    return future;
}

void SingleLevelStorageManager::loadChunkAsyncCallback(ChunkCoord x,
    ChunkCoord z,
    DimensionId dimension,
    std::function<void(ChunkCoord, ChunkCoord, DimensionId, Result<std::optional<ChunkData>>)> callback,
    std::shared_ptr<std::atomic<bool>> abortSignal,
    util::TaskPriority priority)
{
    _loadChunkAsyncCore(
        x,
        z,
        dimension,
        std::move(abortSignal),
        [x, z, dimension, cb = std::move(callback)](Result<std::optional<ChunkData>> result) {
            if (cb) {
                cb(x, z, dimension, std::move(result));
            }
        },
        priority);
}

// chunk 按值持有：调用方以 move 交出共享所有权，传入后本函数与 ServerIO 任务共同持有一份
// NOLINTNEXTLINE(performance-unnecessary-value-param)
void SingleLevelStorageManager::saveChunkAsyncCallback(std::shared_ptr<const ChunkData> chunk,
    DimensionId dimension,
    std::function<void(std::optional<ChunkPos>, Result<void>)> callback,
    util::TaskPriority priority)
{
    if (!chunk) {
        if (callback) {
            // 空区块没有真实坐标，回传空 optional，避免调用方把失败误记为坐标 (0, 0)
            callback(std::nullopt, Error(ErrorCode::InvalidArgument, "saveChunkAsyncCallback: null chunk"));
        }
        return;
    }

    const ChunkCoord x = chunk->x();
    const ChunkCoord z = chunk->z();

    if (!isOpen() || m_config.readonly || m_backend) {
        // 未打开 / 只读 / 外来格式：外来后端 saveChunk 为只读空操作，等价于 ok。
        // 与 saveChunk 的外来格式语义一致（外来格式不保存区块 section）。
        Result<void> result =
            (!isOpen()) ? Result<void>(Error(ErrorCode::InvalidState, "Storage not open")) : Result<void>::ok();
        if (callback) {
            callback(ChunkPos(x, z), std::move(result));
        }
        return;
    }

    // 同步降级：无 taskManager（测试/独立模式）在当前线程直接执行。
    if (!m_taskManager) {
        auto result = saveChunk(*chunk, dimension);
        if (callback) {
            callback(ChunkPos(x, z), std::move(result));
        }
        return;
    }

    // 调用线程完成唯一一次快照拷贝：直接从紧凑数组收集进 WriteBatch。
    // 批次拥有数据，交接后后台线程不再触碰 ChunkData。
    const std::array<const ChunkData*, 1> chunks{chunk.get()};
    auto captured = _captureSections(_sectionManager(dimension), chunks);
    if (captured.failed()) {
        if (callback) {
            callback(ChunkPos(x, z), captured.error());
        }
        return;
    }
    auto sectionSnapshot = std::make_shared<rocksdb::WriteBatch>(std::move(captured.value()));

    // 方块实体：在主线程同步保存。方块实体数量少（每区块通常 < 50），NBT 序列化 + 单条 RocksDB put
    // 开销远小于整列区块的持久化工作；同步保存避免在 ServerIO 读取 ChunkData 的方块实体表
    // 与 setBlockState 修改方块实体表的数据竞争。
    Result<void> blockEntityResult = Result<void>::ok();
    if (m_blockEntityStorage) {
        auto blockEntities = chunk->getAllBlockEntities();
        for (const auto* blockEntity : blockEntities) {
            auto beResult = m_blockEntityStorage->saveBlockEntity(*blockEntity, dimension);
            if (beResult.failed()) {
                blockEntityResult = beResult;
                break;
            }
        }
    }
    if (blockEntityResult.failed()) {
        if (callback) {
            callback(ChunkPos(x, z), blockEntityResult.error());
        }
        return;
    }

    // 注册进行中保存：后续对同一区块的 loadChunkAsync 在读盘前等待此 promise，
    // 确保读到保存后的新数据（对齐 Moonrise GenericDataLoadTask 等待 UnloadTask）。
    _beginAsyncOperation();
    PendingChunkSave pendingSave = _registerPendingChunkSave(x, z, dimension);

    // ServerIO 只提交已经拥有数据的批次，不再进行拼接或区块访问。
    auto& sectionManager = _sectionManager(dimension);
    auto executor = [sectionSnapshot = std::move(sectionSnapshot), &sectionManager, x, z](
                        const std::atomic<bool>& abortSig) -> bool {
        if (abortSig.load(std::memory_order::acquire)) {
            return false;
        }
        return sectionManager.saveBatch(*sectionSnapshot, false).success();
    };

    SectionKey descKey{x, z, 0, dimension};
    auto task = StorageTask::createSaveTask(descKey, false, std::move(executor));
    // 保存完成回调（ServerIO 线程）负责唤醒等待此区块读盘的加载任务并通知调用方。
    // 它在任务结束后调用（成功/失败/取消都走这里），保证加载不会因等待而永久挂起。
    util::TaskCallback onSaveDone = [this, pendingSave, x, z, cb = std::move(callback)](
                                        bool success, util::ITask* /*task*/) {
        _completePendingChunkSave(pendingSave);
        _finishAsyncOperation();
        if (cb) {
            cb(ChunkPos(x, z),
                success ? Result<void>::ok() : Error(ErrorCode::InvalidState, "Save chunk failed or cancelled"));
        }
    };
    // 线程池拒绝提交时同样立即执行失败回调，由同一个收尾入口清理保存追踪。
    m_taskManager->submit(std::move(task), priority, std::move(onSaveDone), nullptr);
}

PendingChunkSave SingleLevelStorageManager::_registerPendingChunkSave(ChunkCoord x, ChunkCoord z, DimensionId dimension)
{
    auto owner = std::make_shared<const PendingChunkSave::Owner>(PendingChunkSave::Owner{x, z, dimension});
    SectionKey descKey{x, z, 0, dimension};
    {
        std::lock_guard<std::mutex> lock(m_pendingChunkSavesMutex);
        // 覆盖旧条目：ServerChunkManager 保证同一区块同一时刻仅有一个进行中卸载保存
        // （unloadChunkSync stage1 检查 m_unloadSaveInProgress，stage3 才完成移除）。
        auto& entry = m_pendingChunkSaves[descKey];
        entry.save = owner;
        entry.waiters.clear();
    }
    return PendingChunkSave{std::move(owner)};
}

void SingleLevelStorageManager::_completePendingChunkSave(PendingChunkSave save)
{
    std::vector<std::unique_ptr<StorageTask>> waiters;
    {
        std::lock_guard<std::mutex> lock(m_pendingChunkSavesMutex);
        SectionKey descKey{save.owner->x, save.owner->z, 0, save.owner->dimension};
        auto it = m_pendingChunkSaves.find(descKey);
        if (it == m_pendingChunkSaves.end()) {
            return;
        }
        // 该区块可能已被新的保存覆盖：只有仍是本次保存时才移走等待者，
        // 否则由新的保存完成时负责唤醒
        if (!(it->second.save == save.owner)) {
            return;
        }
        waiters = std::move(it->second.waiters);
        m_pendingChunkSaves.erase(it);
    }

    // 等待者重新投递到 ServerIO 池：保存已落盘，此时的读盘任务一定读到新数据
    for (auto& waiter : waiters) {
        m_taskManager->submit(std::move(waiter), util::TaskPriority::Normal, nullptr, nullptr);
    }
    MC_ASSERT_RELEASE(waiters.empty());
}

void SingleLevelStorageManager::_abandonPendingChunkSave(PendingChunkSave save)
{
    // 保存任务从未启动（池未启动/队列被清空）：唤醒等待者，避免加载永久挂起
    _completePendingChunkSave(std::move(save));
}

void SingleLevelStorageManager::_removePendingChunkSave(ChunkCoord x, ChunkCoord z, DimensionId dimension)
{
    SectionKey descKey{x, z, 0, dimension};
    std::lock_guard<std::mutex> lock(m_pendingChunkSavesMutex);
    m_pendingChunkSaves.erase(descKey);
}

void SingleLevelStorageManager::_loadChunkAsyncCore(ChunkCoord x,
    ChunkCoord z,
    DimensionId dimension,
    std::shared_ptr<std::atomic<bool>> abortSignal,
    std::function<void(Result<std::optional<ChunkData>>)> completion,
    util::TaskPriority priority)
{
    if (!completion) {
        return;
    }

    // 在加载线程独占数据时补齐高度图。回传后主线程只需安装区块与发布状态，
    // 不再在一次登录中串行扫描整个视距的存档区块；外来格式已有的高度值保持不变。
    _beginAsyncOperation();
    completion = [this, callback = std::move(completion)](Result<std::optional<ChunkData>> result) {
        if (result.success() && result.value().has_value()) {
            result.value()->initializeMissingHeightmaps();
        }
        _finishAsyncOperation();
        callback(std::move(result));
    };

    if (!isOpen()) {
        completion(Error(ErrorCode::InvalidState, "Storage not open"));
        return;
    }

    // 外来格式：在 ServerIO worker 内加锁同步执行 m_backend->loadChunk（外来后端非线程安全）
    if (m_backend) {
        if (!m_taskManager) {
            // 无线程池：同步降级
            auto result = [&] {
                std::lock_guard<std::mutex> lock(m_foreignReadMutex);
                return m_backend->loadChunk(x, z, dimension);
            }();
            completion(std::move(result));
            return;
        }
        auto executor = [this, x, z, dimension, completion](const std::atomic<bool>& abortSig) {
            if (abortSig.load(std::memory_order::acquire)) {
                completion(Error(ErrorCode::InvalidState, "Load chunk (foreign) cancelled"));
                return false;
            }
            auto result = [&] {
                std::lock_guard<std::mutex> lock(m_foreignReadMutex);
                return m_backend->loadChunk(x, z, dimension);
            }();
            completion(std::move(result));
            return true;
        };
        SectionKey descKey{x, z, 0, dimension};
        auto task = StorageTask::createLoadTask(descKey, std::move(executor));
        auto signal = abortSignal ? std::move(abortSignal) : std::make_shared<std::atomic<bool>>(false);
        m_taskManager->submit(std::move(task), priority, nullptr, std::move(signal));
        return;
    }

    // Native 格式：无 taskManager 时同步降级
    if (!m_taskManager) {
        completion(loadChunk(x, z, dimension));
        return;
    }

    // Native 格式有 taskManager：两路并行 I/O，均在 ServerIO 线程执行同步读取。
    // 路径 A：section 数据（SectionManager::loadSectionsSync，线程安全）
    // 路径 B：方块实体（BlockEntityStorageManager::loadBlockEntitiesInChunk，RocksDB 迭代器线程安全）
    // 两路写入共享 AsyncLoadState，最后完成的一路组装 ChunkData 并调用 completion。
    auto& sectionManager = _sectionManager(dimension);

    std::vector<SectionKey> keys;
    keys.reserve(world::CHUNK_SECTIONS);
    for (i8 sectionY = 0; sectionY < world::CHUNK_SECTIONS; ++sectionY) {
        keys.emplace_back(x, z, static_cast<i8>(world::sectionIndexToCoord(sectionY)), dimension);
    }

    struct AsyncLoadState {
        std::atomic<i32> pending;
        std::mutex mutex;
        Result<PinnedValues> sectionResult{Error(ErrorCode::Unknown, "uninitialized")};
        bool sectionReady = false;
        Result<std::vector<std::unique_ptr<BlockEntity>>> blockEntityResult{Error(ErrorCode::Unknown, "uninitialized")};
        bool blockEntityReady = false;
        std::function<void(Result<std::optional<ChunkData>>)> completion;
        ChunkCoord x;
        ChunkCoord z;
        DimensionId dimension;
        std::vector<SectionKey> keys;
        SectionManager* sectionManager;
        BlockEntityStorageManager* blockEntityStorage;
    };

    // pending 初始值：路径 A 必有，路径 B 仅在有 blockEntityStorage 时 +1
    i32 initialPending = m_blockEntityStorage ? 2 : 1;
    auto state = std::make_shared<AsyncLoadState>();
    state->pending.store(initialPending, std::memory_order::relaxed);
    state->completion = std::move(completion);
    state->x = x;
    state->z = z;
    state->dimension = dimension;
    state->keys = keys;
    state->sectionManager = &sectionManager;
    state->blockEntityStorage = m_blockEntityStorage.get();

    // 组装函数：两路都完成后调用，组装 ChunkData 并调用 completion。调用方持 state->mutex。
    auto _assemble = [state]() {
        Result<std::optional<ChunkData>> result = [&]() -> Result<std::optional<ChunkData>> {
            if (state->sectionResult.failed()) {
                return state->sectionResult.error();
            }
            const auto& sections = state->sectionResult.value();

            ChunkData chunk(state->x, state->z);
            bool hasAnySection = false;

            for (size_t i = 0; i < sections.size(); ++i) {
                const auto& sectionData = sections[i];
                if (!sectionData) {
                    continue;
                }

                const i32 sectionIndex = world::sectionCoordToIndex(static_cast<i32>(state->keys[i].sectionY));
                auto applyResult = SectionCodec::restore(
                    {reinterpret_cast<const u8*>(sectionData->data()), sectionData->size()}, chunk, sectionIndex);
                if (applyResult.failed()) {
                    return applyResult.error();
                }

                hasAnySection = true;
            }

            if (!hasAnySection) {
                return std::optional<ChunkData>{};
            }

            // 路径 B 结果：方块实体（失败不影响区块加载，与同步 loadChunk 行为一致）
            if (state->blockEntityReady && state->blockEntityResult.success()) {
                for (auto& blockEntity : state->blockEntityResult.value()) {
                    if (blockEntity != nullptr) {
                        // 同同步路径：先取位置再移动所有权，避免求值顺序导致对空指针解引用
                        const BlockPos blockEntityPos = blockEntity->getPos();
                        chunk.setBlockEntity(blockEntityPos, std::move(blockEntity));
                    }
                }
            }

            chunk.setLoaded(true);
            chunk.setFullyGenerated(true);
            chunk.setDirty(false);
            return std::optional<ChunkData>(std::move(chunk));
        }();

        state->sectionResult = PinnedValues{};
        state->completion(std::move(result));
    };

    // 完成检查：减少 pending 计数，归零者把组装（反序列化）投递到 ServerCompute 线程池。
    // 紧凑数组恢复与缺失高度图重建在 ServerCompute 执行，与 ServerIO 读盘分离。
    // 无 Compute 池（测试/独立模式）时降级为在 ServerIO worker 内联组装。
    // abortSignal 透传给 Compute 任务，使 SCLM 取消能中断组装阶段。
    auto checkComplete = [this, state, _assemble, priority]() {
        if (state->pending.fetch_sub(1, std::memory_order::acq_rel) != 1) {
            return;
        }
        // 两路 I/O 均完成，组装阶段投递到 ServerCompute。
        auto runAssemble = [state, _assemble](const std::atomic<bool>& /*abortSig*/) {
            std::lock_guard<std::mutex> lock(state->mutex);
            _assemble();
            return true;
        };
        if (m_computeWorkerPool != nullptr) {
            auto computeTask = std::make_unique<util::FunctionTask>(util::TaskType::ChunkLoad,
                fmt::format("ChunkLoadAssemble({},{})", state->x, state->z),
                std::move(runAssemble),
                "server.chunk");
            // 组装阶段不绑定取消信号：它是本次加载的收尾环节，负责调用 completion 通知提交方
            // （ServerChunkManager 据此清除"存档解析在途"条目并推进等待者）。组装本身不做 I/O，
            // 且 _assemble 会按已收集到的取消结果（sectionResult 为错误）正常产出失败结果；
            // 若绑定取消信号，被取消的加载会跳过组装从而永不通知提交方，提交方将永久停留在
            // "存档解析在途"状态（区块请求再也无法完成）。
            const auto taskId = m_computeWorkerPool->submit(std::move(computeTask), nullptr, priority, nullptr);
            if (taskId == 0) {
                // 恢复是收尾操作：池拒绝提交时内联执行，释放固定读取句柄并结清请求。
                std::lock_guard lock(state->mutex);
                _assemble();
            }
        } else {
            // 降级：无 Compute 池，在当前线程（ServerIO worker 或测试线程）内联组装。
            std::atomic<bool> dummySig{false};
            runAssemble(dummySig);
        }
    };

    // 路径 A：section 数据。提交 StorageTask 到 ServerIO，内部同步调 loadSectionsSync。
    auto sectionExecutor = [this, state, keys = std::move(keys), checkComplete, x, z, dimension](
                               const std::atomic<bool>& abortSig) mutable {
        if (abortSig.load(std::memory_order::acquire)) {
            {
                std::lock_guard<std::mutex> lock(state->mutex);
                state->sectionResult = Error(ErrorCode::InvalidState, "Load sections cancelled");
                state->sectionReady = true;
            }
            checkComplete();
            return false;
        }
        // 若该区块有进行中的卸载保存，把"读盘"阶段作为等待者登记，由保存完成回调重新投递，
        // 确保读到保存后的新数据（对齐 Moonrise GenericDataLoadTask 等待 chunkDataUnload）。
        // 不能在池线程里阻塞等待保存：保存任务也在同一个池里排队，全部线程被等待占满时
        // 保存任务永远轮不到执行，形成线程饥饿死锁。
        auto runLoadSections = [state, keys = std::move(keys), checkComplete](const std::atomic<bool>& loadAbortSig) {
            if (loadAbortSig.load(std::memory_order::acquire)) {
                {
                    std::lock_guard<std::mutex> lock(state->mutex);
                    state->sectionResult = Error(ErrorCode::InvalidState, "Load sections cancelled");
                    state->sectionReady = true;
                }
                checkComplete();
                return false;
            }
            auto secResult = state->sectionManager->loadSectionsSync(keys);
            {
                std::lock_guard<std::mutex> lock(state->mutex);
                state->sectionResult = std::move(secResult);
                state->sectionReady = true;
            }
            checkComplete();
            return true;
        };

        SectionKey sectionDescKey{x, z, 0, dimension};
        {
            std::lock_guard<std::mutex> lock(m_pendingChunkSavesMutex);
            auto it = m_pendingChunkSaves.find(sectionDescKey);
            if (it != m_pendingChunkSaves.end() && it->second.save != nullptr) {
                it->second.waiters.push_back(StorageTask::createLoadTask(sectionDescKey, std::move(runLoadSections)));
                // 走到这里时 m_taskManager 必然存在（路径 A 由它提交），等待者由保存完成回调重新投递
                return true;
            }
        }
        // 无进行中保存：直接在本次池任务里读盘
        return runLoadSections(abortSig);
    };
    SectionKey sectionDescKey{x, z, 0, dimension};
    auto sectionTask = StorageTask::createLoadTask(sectionDescKey, std::move(sectionExecutor));
    auto sectionSignal = abortSignal ? abortSignal : std::make_shared<std::atomic<bool>>(false);
    m_taskManager->submit(std::move(sectionTask), priority, nullptr, sectionSignal);

    // 路径 B：方块实体（仅有 blockEntityStorage 时）。
    if (m_blockEntityStorage) {
        auto beExecutor = [state, checkComplete](const std::atomic<bool>& abortSig) {
            if (abortSig.load(std::memory_order::acquire)) {
                {
                    std::lock_guard<std::mutex> lock(state->mutex);
                    state->blockEntityResult = Error(ErrorCode::InvalidState, "Load block entities cancelled");
                    state->blockEntityReady = true;
                }
                checkComplete();
                return false;
            }
            auto beResult = state->blockEntityStorage->loadBlockEntitiesInChunk(state->x, state->z, state->dimension);
            {
                std::lock_guard<std::mutex> lock(state->mutex);
                state->blockEntityResult = std::move(beResult);
                state->blockEntityReady = true;
            }
            checkComplete();
            return true;
        };
        SectionKey beDescKey{x, z, 0, dimension};
        auto beTask = StorageTask::createLoadTask(beDescKey, std::move(beExecutor));
        // 路径 B 共享同一 abortSignal（共享 ptr）
        m_taskManager->submit(std::move(beTask), priority, nullptr, abortSignal);
    }
}

Result<std::optional<PlayerSaveData>> SingleLevelStorageManager::loadPlayer(const std::string& uuid)
{
    if (!isOpen()) {
        return Error(ErrorCode::InvalidState, "Storage not open");
    }

    if (m_backend) {
        return m_backend->loadPlayer(uuid);
    }

    if (!m_playerDataManager) {
        return Error(ErrorCode::InvalidState, "Player data manager not initialized");
    }

    auto loadResult = m_playerDataManager->loadPlayer(uuid);
    if (loadResult.failed()) {
        return loadResult.error();
    }

    PlayerSaveData* playerData = loadResult.value();
    if (!playerData) {
        return std::optional<PlayerSaveData>{};
    }

    return std::optional<PlayerSaveData>(*playerData);
}

Result<LevelRuntimeData> SingleLevelStorageManager::loadLevelData()
{
    // 父级 MinecraftServer::initializeWorld 已带 trace；此处作为 subpart 量化 level.dat 运行时数据读取耗时。
    MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Initialization, "SingleLevelStorageManager::loadLevelData");

    if (!isOpen()) {
        return Error(ErrorCode::InvalidState, "Storage not open");
    }

    if (m_backend) {
        return m_backend->loadLevelData();
    }

    return LevelDatCodec::readRuntimeData(m_worldPath);
}

Result<void> SingleLevelStorageManager::saveLevelData(i64 gameTime,
    i64 dayTime,
    const RespawnData& spawnData,
    i32 clearWeatherTime,
    i32 rainTime,
    bool raining,
    i32 thunderTime,
    bool thundering,
    bool initialized,
    Difficulty difficulty,
    bool difficultyLocked)
{
    if (!isOpen()) {
        return Error(ErrorCode::InvalidState, "Storage not open");
    }

    if (m_config.readonly) {
        return Result<void>::ok();
    }

    // Native 格式：直接写入 level.dat
    if (!m_backend) {
        return LevelDatCodec::updateRuntimeData(m_worldPath,
            gameTime,
            dayTime,
            spawnData,
            clearWeatherTime,
            rainTime,
            raining,
            thunderTime,
            thundering,
            initialized,
            difficulty,
            difficultyLocked);
    }

    // 外来格式：只读，不保存
    return Result<void>::ok();
}

Result<std::unique_ptr<nbt::tags::compound_list_tag>> SingleLevelStorageManager::loadScheduledEvents()
{
    // 父级 MinecraftServer::initializeWorld 已带 trace；此处作为 subpart 量化调度事件反序列化耗时。
    MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Initialization, "SingleLevelStorageManager::loadScheduledEvents");

    if (!isOpen()) {
        return Error(ErrorCode::InvalidState, "Storage not open");
    }

    // 外来格式返回空列表
    if (m_backend) {
        return std::make_unique<nbt::tags::compound_list_tag>();
    }

    return LevelDatCodec::readScheduledEvents(m_worldPath);
}

Result<void> SingleLevelStorageManager::saveScheduledEvents(const nbt::tags::compound_list_tag& events)
{
    if (!isOpen()) {
        return Error(ErrorCode::InvalidState, "Storage not open");
    }

    if (m_config.readonly) {
        return Result<void>::ok();
    }

    // 外来格式：只读，不保存
    if (m_backend) {
        return Result<void>::ok();
    }

    return LevelDatCodec::updateScheduledEvents(m_worldPath, events);
}

Result<std::optional<nlohmann::json>> SingleLevelStorageManager::loadAddonWorldProperties()
{
    if (!isOpen()) {
        return Error(ErrorCode::InvalidState, "Storage not open");
    }
    if (m_backend) {
        return std::nullopt;
    }

    auto bytes = m_db->get(cf::META, rocksdb::Slice("addon-world-properties"));
    if (bytes.failed()) {
        if (bytes.error().code() == ErrorCode::NotFound) {
            return std::nullopt;
        }
        return bytes.error();
    }

    auto saved = nlohmann::json::parse(bytes.value().begin(), bytes.value().end(), nullptr, false);
    if (saved.is_discarded() || !saved.is_object()) {
        return Error(ErrorCode::InvalidData, "Invalid saved addon world properties");
    }
    return saved;
}

Result<void> SingleLevelStorageManager::saveAddonWorldProperties(const nlohmann::json& data, bool sync)
{
    if (!isOpen()) {
        return Error(ErrorCode::InvalidState, "Storage not open");
    }
    if (m_config.readonly || m_backend) {
        return Error(ErrorCode::PermissionDenied, "Cannot save addon properties to readonly storage");
    }
    if (!data.is_object()) {
        return Error(ErrorCode::InvalidArgument, "Addon world properties must be an object");
    }

    // 由现有数据库的一次原子写入保存整个快照，不另建文件或维护磁盘缓存。
    const auto encoded = data.dump();
    return m_db->put(cf::META, rocksdb::Slice("addon-world-properties"), rocksdb::Slice(encoded), sync);
}

Result<std::optional<nlohmann::json>> SingleLevelStorageManager::loadDragonFightData()
{
    if (!isOpen()) {
        return Error(ErrorCode::InvalidState, "Storage not open");
    }

    // 外来格式：返回空
    if (m_backend) {
        return std::nullopt;
    }

    const auto filePath = m_worldPath / "data" / "end_dragon_fight.json";
    if (!std::filesystem::exists(filePath)) {
        return std::nullopt;
    }

    try {
        std::ifstream file(filePath, std::ios::in);
        if (!file.is_open()) {
            return std::nullopt;
        }
        nlohmann::json data = nlohmann::json::parse(file);
        return data;
    }
    catch (const std::exception& e) {
        spdlog::warn("Failed to load end_dragon_fight.json: {}", e.what());
        return std::nullopt;
    }
}

Result<void> SingleLevelStorageManager::saveDragonFightData(const nlohmann::json& data)
{
    if (!isOpen()) {
        return Error(ErrorCode::InvalidState, "Storage not open");
    }

    if (m_config.readonly) {
        return Result<void>::ok();
    }

    // 外来格式：只读，不保存
    if (m_backend) {
        return Result<void>::ok();
    }

    try {
        const auto dataDir = m_worldPath / "data";
        if (!std::filesystem::exists(dataDir)) {
            std::filesystem::create_directories(dataDir);
        }

        const auto filePath = dataDir / "end_dragon_fight.json";
        std::ofstream file(filePath, std::ios::out | std::ios::trunc);
        if (!file.is_open()) {
            return Error(ErrorCode::FileOpenFailed, "Failed to open end_dragon_fight.json for writing");
        }
        file << data.dump(2);
        return Result<void>::ok();
    }
    catch (const std::exception& e) {
        return Error(ErrorCode::FileWriteFailed, fmt::format("Failed to save end_dragon_fight.json: {}", e.what()));
    }
}

SectionManager& SingleLevelStorageManager::_sectionManager(DimensionId dimension)
{
    if (!isOpen()) {
        throw std::runtime_error("Storage not open");
    }

    {
        std::lock_guard<std::mutex> lock(m_sectionManagersMutex);
        auto it = m_sectionManagers.find(dimension);
        if (it != m_sectionManagers.end()) {
            return *it->second;
        }
    }

    auto* manager = _createSectionManager(dimension);
    if (!manager) {
        throw std::runtime_error(
            fmt::format("Failed to create SectionManager for dimension {}", static_cast<i32>(dimension)));
    }
    return *manager;
}

const SectionManager& SingleLevelStorageManager::_sectionManager(DimensionId dimension) const
{
    if (!isOpen()) {
        throw std::runtime_error("Storage not open");
    }

    std::lock_guard<std::mutex> lock(m_sectionManagersMutex);
    auto it = m_sectionManagers.find(dimension);
    if (it == m_sectionManagers.end()) {
        throw std::runtime_error(fmt::format("SectionManager not found for dimension {}", static_cast<i32>(dimension)));
    }
    return *it->second;
}

bool SingleLevelStorageManager::_hasSectionManager(DimensionId dimension) const
{
    std::lock_guard<std::mutex> lock(m_sectionManagersMutex);
    return m_sectionManagers.find(dimension) != m_sectionManagers.end();
}

SectionManager* SingleLevelStorageManager::_createSectionManager(DimensionId dimension)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Server.World,
        "SingleLevelStorageManager::createSectionManager",
        "dimension",
        static_cast<i32>(dimension));

    auto manager = std::make_unique<SectionManager>(*m_db, dimension, m_config.consistencyMode);

    std::lock_guard<std::mutex> lock(m_sectionManagersMutex);
    auto [it, inserted] = m_sectionManagers.emplace(dimension, std::move(manager));
    if (!inserted) {
        return nullptr;
    }
    return it->second.get();
}

void SingleLevelStorageManager::setConsistencyMode(ConsistencyMode mode)
{
    m_config.consistencyMode = mode;
    spdlog::info("Changed consistency mode to {}", static_cast<i32>(mode));
}

std::vector<DimensionId> SingleLevelStorageManager::getOpenDimensions() const
{
    std::vector<DimensionId> dimensions;
    std::lock_guard<std::mutex> lock(m_sectionManagersMutex);
    dimensions.reserve(m_sectionManagers.size());
    for (const auto& [dim, manager] : m_sectionManagers) {
        dimensions.push_back(dim);
    }
    return dimensions;
}

Result<BackupID> SingleLevelStorageManager::createBackup(const std::string& name, const std::string& description)
{
    if (m_config.readonly) {
        return Error(ErrorCode::PermissionDenied, "Cannot create backup in readonly mode");
    }

    if (!m_backupManager) {
        return Error(ErrorCode::InvalidState, "Backup manager not enabled");
    }

    if (!m_db) {
        return Error(ErrorCode::InvalidState, "Database not open");
    }

    return m_backupManager->createBackup(*m_db, name, description);
}

Result<size_t> SingleLevelStorageManager::pruneOldBackups(size_t keepCount)
{
    if (!m_backupManager) {
        return Error(ErrorCode::InvalidState, "Backup manager not enabled");
    }

    return m_backupManager->pruneOldBackups(keepCount);
}

} // namespace mc::world::storage
