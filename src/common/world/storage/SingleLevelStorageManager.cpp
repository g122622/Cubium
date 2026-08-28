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
#include "common/world/storage/blockentity/BlockEntityStorageManager.hpp"
#include "common/world/storage/core/LevelDatCodec.hpp"
#include "common/world/storage/core/SaveFormat.hpp"
#include "common/world/storage/core/WorldSessionLock.hpp"
#include "common/world/storage/db/ConsistencyMode.hpp"
#include "common/world/storage/db/RocksDBConfig.hpp"
#include "common/world/storage/db/RocksDBDatabase.hpp"
#include "common/world/storage/db/SectionKey.hpp"
#include "common/world/storage/entity/EntityStorageManager.hpp"
#include "common/world/storage/player/PlayerDataManager.hpp"
#include "common/world/storage/player/PlayerSaveData.hpp"
#include "common/world/storage/section/SectionCache.hpp"
#include "common/world/storage/section/SectionManager.hpp"
#include "common/world/storage/snapshot/BackupManager.hpp"
#include "common/world/storage/task/StorageTask.hpp"
#include "common/world/storage/task/StorageTaskManager.hpp"
#include "scoreboard/storage/ScoreboardDataManager.hpp"
#include "world/storage/backend/BedrockLDBBackend.hpp"
#include "world/storage/backend/JavaAnvilBackend.hpp"
#include "world/storage/db/SectionCodec.hpp"
#include "world/storage/save/AutoSave.hpp"
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
    for (auto& [dim, manager] : m_sectionManagers) {
        if (manager) {
            manager->setTaskManager(m_taskManager.get());
        }
    }

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

        for (auto& [dim, manager] : m_sectionManagers) {
            if (manager) {
                manager->setTaskManager(m_taskManager.get());
            }
        }

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

    std::lock_guard<std::mutex> lock(m_sectionManagersMutex);
    for (auto& [dim, manager] : m_sectionManagers) {
        if (manager) {
            manager->setTaskManager(m_taskManager.get());
        }
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

void SingleLevelStorageManager::close()
{
    if (!isOpen()) {
        return;
    }

    MC_TRACE_SCOPED_EVENT(TraceEvents.Server.World, "SingleLevelStorageManager::close");

    {
        std::lock_guard<std::mutex> lock(m_sectionManagersMutex);
        m_sectionManagers.clear();
    }

    if (m_autoSave) {
        m_autoSave->stop();
    }
    m_backupManager.reset();
    m_scoreboardDataManager.reset();
    m_entityStorage.reset();
    m_blockEntityStorage.reset();
    m_playerDataManager.reset();
    m_autoSave.reset();
    m_autoSaveInitialized = false;
    m_db.reset();
    m_backend.reset();
    m_taskManager.reset();
    m_ioWorkerPool = nullptr;
    m_sessionLock.reset();

    spdlog::info("SingleLevelStorageManager closed");

    m_worldPath.clear();
}

Result<size_t> SingleLevelStorageManager::flushAllDirty()
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Server.World, "SingleLevelStorageManager::flushAllDirty");

    if (!isOpen()) {
        return Error(ErrorCode::InvalidState, "Storage not open");
    }

    if (m_config.readonly) {
        return Result<size_t>(0);
    }

    size_t totalFlushed = 0;

    {
        std::lock_guard<std::mutex> lock(m_sectionManagersMutex);
        for (auto& [dim, manager] : m_sectionManagers) {
            auto result = manager->flushDirtySections();
            if (!result.success()) {
                return result.error();
            }
            totalFlushed += result.value();
        }
    }

    if (m_playerDataManager) {
        auto playerResult = m_playerDataManager->saveAllDirty();
        if (playerResult.failed()) {
            spdlog::error("Failed to flush dirty player data: {}", playerResult.error().message());
        } else {
            totalFlushed += playerResult.value();
        }
    }

    return totalFlushed;
}

Result<size_t> SingleLevelStorageManager::saveAll()
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Server.World, "SingleLevelStorageManager::saveAll");

    if (!isOpen()) {
        return Error(ErrorCode::InvalidState, "Storage not open");
    }

    if (m_config.readonly) {
        return Result<size_t>(0);
    }

    size_t totalSaved = 0;

    {
        std::lock_guard<std::mutex> lock(m_sectionManagersMutex);
        for (auto& [dim, manager] : m_sectionManagers) {
            auto result = manager->saveAll();
            if (!result.success()) {
                return result.error();
            }
            totalSaved += result.value();
        }
    }

    if (m_playerDataManager) {
        auto playerResult = m_playerDataManager->saveAll();
        if (playerResult.failed()) {
            spdlog::error("Failed to save all player data: {}", playerResult.error().message());
        } else {
            totalSaved += playerResult.value();
        }
    }

    if (totalSaved > 0) {
        spdlog::info("Saved {} cached sections and player data", totalSaved);
    }

    return totalSaved;
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

    std::vector<BiomeId> biomes;
    const auto biomeBytes = chunk.getBiomes().serialize();
    biomes.reserve(biomeBytes.size() / 2);
    for (size_t i = 0; i + 1 < biomeBytes.size(); i += 2) {
        const u16 low = static_cast<u16>(biomeBytes[i]);
        const u16 high = static_cast<u16>(biomeBytes[i + 1]);
        biomes.push_back(static_cast<BiomeId>(low | (high << 8)));
    }

    for (i8 sectionY = 0; sectionY < world::CHUNK_SECTIONS; ++sectionY) {
        const ChunkSection* section = chunk.getSection(sectionY);
        if (!section) {
            continue;
        }

        SectionKey key(chunk.x(), chunk.z(), static_cast<i8>(world::sectionIndexToCoord(sectionY)), dimension);
        auto sectionDataResult = SectionCodec::fromChunkSection(*section, key, biomes);
        if (sectionDataResult.failed()) {
            return sectionDataResult.error();
        }

        auto saveResult = manager.saveSectionSync(key, sectionDataResult.value());
        if (saveResult.failed()) {
            return saveResult.error();
        }
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
        return m_backend->loadChunk(x, z, dimension);
    }

    // Native 格式：通过 RocksDB SectionManager 读取
    auto& manager = _sectionManager(dimension);
    ChunkData chunk(x, z);
    bool hasAnySection = false;
    bool hasBiomes = false;
    BiomeContainer biomeContainer;

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

        if (sectionData->biomes.size() == SectionData::BIOME_COUNT) {
            const i32 biomeSectionIndex = world::sectionCoordToIndex(static_cast<i32>(keys[i].sectionY));
            if (biomeSectionIndex >= 0 && biomeSectionIndex < BiomeContainer::SECTION_COUNT) {
                for (i32 biomeY = 0; biomeY < BiomeContainer::VERT_SIZE; ++biomeY) {
                    for (i32 biomeZ = 0; biomeZ < BiomeContainer::HORIZ_SIZE; ++biomeZ) {
                        for (i32 biomeX = 0; biomeX < BiomeContainer::HORIZ_SIZE; ++biomeX) {
                            const size_t biomeIndex =
                                static_cast<size_t>(biomeY * BiomeContainer::HORIZ_SIZE * BiomeContainer::HORIZ_SIZE +
                                    biomeZ * BiomeContainer::HORIZ_SIZE + biomeX);
                            biomeContainer.setBiome(
                                biomeSectionIndex, biomeX, biomeY, biomeZ, sectionData->biomes[biomeIndex]);
                        }
                    }
                }
                hasBiomes = true;
            }
        }

        const i32 sectionIndex = world::sectionCoordToIndex(static_cast<i32>(keys[i].sectionY));
        if (sectionIndex < 0 || sectionIndex >= world::CHUNK_SECTIONS) {
            return Error(ErrorCode::ChunkCorrupted,
                fmt::format("Section coord {} maps outside chunk section range [0, {})",
                    static_cast<i32>(keys[i].sectionY),
                    world::CHUNK_SECTIONS));
        }

        ChunkSection* section = chunk.createSection(sectionIndex);
        MC_ASSERT_RELEASE(section != nullptr);
        auto applyResult = SectionCodec::toChunkSection(*sectionData, *section);
        if (applyResult.failed()) {
            return applyResult.error();
        }

        hasAnySection = true;
    }

    if (!hasAnySection) {
        return std::optional<ChunkData>{};
    }

    if (hasBiomes) {
        chunk.setBiomes(std::move(biomeContainer));
    }

    // 加载方块实体
    if (m_blockEntityStorage) {
        auto beResult = m_blockEntityStorage->loadBlockEntitiesInChunk(x, z, dimension);
        if (beResult.success()) {
            for (auto& blockEntity : beResult.value()) {
                if (blockEntity != nullptr) {
                    chunk.setBlockEntity(blockEntity->getPos(), std::move(blockEntity));
                }
            }
        }
        // 方块实体加载失败不影响区块加载
    }

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
        [promise](Result<std::optional<ChunkData>> result) { promise->set_value(std::move(result)); },
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

void SingleLevelStorageManager::saveChunkAsyncCallback(std::shared_ptr<const ChunkData> chunk,
    DimensionId dimension,
    std::function<void(ChunkCoord, ChunkCoord, Result<void>)> callback,
    util::TaskPriority priority)
{
    if (!chunk) {
        if (callback) {
            callback(0, 0, Error(ErrorCode::InvalidArgument, "saveChunkAsyncCallback: null chunk"));
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
            callback(x, z, std::move(result));
        }
        return;
    }

    // 同步降级：无 taskManager（测试/独立模式）在当前线程直接执行。
    if (!m_taskManager) {
        auto result = saveChunk(*chunk, dimension);
        if (callback) {
            callback(x, z, std::move(result));
        }
        return;
    }

    // stage1（调用线程，主线程）：序列化区块为快照（vector<SectionData> + biomes + 方块实体副本）。
    // 序列化读取 ChunkData，与 setBlockState 等 chunk 修改者均在主线程串行执行，无数据竞争。
    // 快照脱离 ChunkData 后，后续 chunk 修改不影响已捕获的保存数据。
    std::vector<SectionData> sectionSnapshot;
    sectionSnapshot.reserve(world::CHUNK_SECTIONS);

    std::vector<BiomeId> biomes;
    const auto biomeBytes = chunk->getBiomes().serialize();
    biomes.reserve(biomeBytes.size() / 2);
    for (size_t i = 0; i + 1 < biomeBytes.size(); i += 2) {
        const u16 low = static_cast<u16>(biomeBytes[i]);
        const u16 high = static_cast<u16>(biomeBytes[i + 1]);
        biomes.push_back(static_cast<BiomeId>(low | (high << 8)));
    }

    for (i8 sectionY = 0; sectionY < world::CHUNK_SECTIONS; ++sectionY) {
        const ChunkSection* section = chunk->getSection(sectionY);
        if (!section) {
            continue;
        }
        SectionKey key(x, z, static_cast<i8>(world::sectionIndexToCoord(sectionY)), dimension);
        auto sectionDataResult = SectionCodec::fromChunkSection(*section, key, biomes);
        if (sectionDataResult.failed()) {
            if (callback) {
                callback(x, z, sectionDataResult.error());
            }
            return;
        }
        sectionSnapshot.push_back(std::move(sectionDataResult.value()));
    }

    // 方块实体：在主线程同步保存。方块实体数量少（每区块通常 < 50），NBT 序列化 + 单条 RocksDB put
    // 开销远小于 24 section 的 ZSTD+WriteBatch；同步保存避免在 ServerIO 读取 ChunkData 的方块实体表
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
            callback(x, z, blockEntityResult.error());
        }
        return;
    }

    // 注册进行中保存：后续对同一区块的 loadChunkAsync 在读盘前等待此 promise，
    // 确保读到保存后的新数据（对齐 Moonrise GenericDataLoadTask 等待 UnloadTask）。
    auto savePromise = _registerPendingChunkSave(x, z, dimension);

    // stage2（ServerIO）：对快照执行 saveSectionSync × 24（ZSTD 压缩 + RocksDB WriteBatch）。
    // 仅触及快照与 SectionManager，不触及 ChunkData，与主线程 chunk 修改无数据竞争。
    auto& sectionManager = _sectionManager(dimension);
    auto executor = [sectionSnapshot = std::move(sectionSnapshot),
                        dimension,
                        &sectionManager,
                        x,
                        z,
                        cb = std::move(callback),
                        savePromise](const std::atomic<bool>& abortSig) -> bool {
        // savePromise 在任务结束（含取消）时 set_value，确保等待此保存的加载能继续；
        // set_value 后 shared_future 变就绪，loadChunkAsync 的 _waitPendingChunkSave 返回。
        auto fulfill = [&] {
            try {
                savePromise->set_value();
            }
            catch (...) {
                // 重复 set_value 抛异常（不应发生），忽略。
            }
        };

        if (abortSig.load(std::memory_order::acquire)) {
            fulfill();
            if (cb) {
                cb(x, z, Error(ErrorCode::InvalidState, "Save chunk task cancelled"));
            }
            return false;
        }

        Result<void> result = Result<void>::ok();
        for (auto& sectionData : sectionSnapshot) {
            if (abortSig.load(std::memory_order::acquire)) {
                result = Error(ErrorCode::InvalidState, "Save chunk task cancelled");
                break;
            }
            SectionKey key(x, z, sectionData.key.sectionY, dimension);
            auto saveResult = sectionManager.saveSectionSync(key, sectionData);
            if (saveResult.failed()) {
                result = saveResult;
                break;
            }
        }

        fulfill();
        if (cb) {
            cb(x, z, std::move(result));
        }
        return true;
    };

    SectionKey descKey{x, z, 0, dimension};
    auto task = StorageTask::createSaveTask(descKey, false, std::move(executor));
    m_taskManager->submit(std::move(task), priority, nullptr, std::make_shared<std::atomic<bool>>(false));
}

std::shared_ptr<std::promise<void>> SingleLevelStorageManager::_registerPendingChunkSave(
    ChunkCoord x, ChunkCoord z, DimensionId dimension)
{
    auto promise = std::make_shared<std::promise<void>>();
    std::shared_future<void> future = promise->get_future().share();
    SectionKey descKey{x, z, 0, dimension};
    {
        std::lock_guard<std::mutex> lock(m_pendingChunkSavesMutex);
        // 覆盖旧条目：ServerChunkManager 保证同一区块同一时刻仅有一个进行中卸载保存
        // （unloadChunkSync stage1 检查 m_unloadSaveInProgress，stage3 才完成移除）。
        m_pendingChunkSaves[descKey] = future;
    }
    return promise;
}

void SingleLevelStorageManager::_waitPendingChunkSave(ChunkCoord x, ChunkCoord z, DimensionId dimension)
{
    std::shared_future<void> future;
    {
        SectionKey descKey{x, z, 0, dimension};
        std::lock_guard<std::mutex> lock(m_pendingChunkSavesMutex);
        auto it = m_pendingChunkSaves.find(descKey);
        if (it == m_pendingChunkSaves.end()) {
            return; // 无进行中保存，直接读盘
        }
        future = it->second; // 拷贝 shared_future（引用计数 +1），锁外等待
    }
    // 在 ServerIO 线程等待保存完成，不阻塞主线程。
    // 保存任务的 savePromise->set_value() 使 future 就绪；future 拷贝脱离 map，
    // 即使保存完成覆盖/移除 map 条目，本 future 仍可安全 wait（promise 共享状态存活至所有 future 释放）。
    future.wait();
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

    if (!isOpen()) {
        completion(Error(ErrorCode::InvalidState, "Storage not open"));
        return;
    }

    // 外来格式：在 ServerIO worker 内加锁同步执行 m_backend->loadChunk（外来后端非线程安全）
    if (m_backend) {
        if (!m_taskManager) {
            // 无线程池：同步降级
            std::lock_guard<std::mutex> lock(m_foreignReadMutex);
            completion(m_backend->loadChunk(x, z, dimension));
            return;
        }
        auto executor = [this, x, z, dimension, completion](const std::atomic<bool>& abortSig) {
            if (abortSig.load(std::memory_order::acquire)) {
                completion(Error(ErrorCode::InvalidState, "Load chunk (foreign) cancelled"));
                return false;
            }
            std::lock_guard<std::mutex> lock(m_foreignReadMutex);
            completion(m_backend->loadChunk(x, z, dimension));
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
        std::atomic<int> pending;
        std::mutex mutex;
        Result<std::vector<std::shared_ptr<const SectionData>>> sectionResult{
            Error(ErrorCode::Unknown, "uninitialized")};
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
    int initialPending = m_blockEntityStorage ? 2 : 1;
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
            bool hasBiomes = false;
            BiomeContainer biomeContainer;

            for (size_t i = 0; i < sections.size(); ++i) {
                const auto& sectionData = sections[i];
                if (!sectionData) {
                    continue;
                }

                if (sectionData->biomes.size() == SectionData::BIOME_COUNT) {
                    const i32 biomeSectionIndex = world::sectionCoordToIndex(static_cast<i32>(state->keys[i].sectionY));
                    if (biomeSectionIndex >= 0 && biomeSectionIndex < BiomeContainer::SECTION_COUNT) {
                        for (i32 biomeY = 0; biomeY < BiomeContainer::VERT_SIZE; ++biomeY) {
                            for (i32 biomeZ = 0; biomeZ < BiomeContainer::HORIZ_SIZE; ++biomeZ) {
                                for (i32 biomeX = 0; biomeX < BiomeContainer::HORIZ_SIZE; ++biomeX) {
                                    const size_t biomeIndex = static_cast<size_t>(
                                        biomeY * BiomeContainer::HORIZ_SIZE * BiomeContainer::HORIZ_SIZE +
                                        biomeZ * BiomeContainer::HORIZ_SIZE + biomeX);
                                    biomeContainer.setBiome(
                                        biomeSectionIndex, biomeX, biomeY, biomeZ, sectionData->biomes[biomeIndex]);
                                }
                            }
                        }
                        hasBiomes = true;
                    }
                }

                const i32 sectionIndex = world::sectionCoordToIndex(static_cast<i32>(state->keys[i].sectionY));
                if (sectionIndex < 0 || sectionIndex >= world::CHUNK_SECTIONS) {
                    return Error(ErrorCode::ChunkCorrupted,
                        fmt::format("Section coord {} maps outside chunk section range [0, {})",
                            static_cast<i32>(state->keys[i].sectionY),
                            world::CHUNK_SECTIONS));
                }

                ChunkSection* section = chunk.createSection(sectionIndex);
                MC_ASSERT_RELEASE(section != nullptr);
                auto applyResult = SectionCodec::toChunkSection(*sectionData, *section);
                if (applyResult.failed()) {
                    return applyResult.error();
                }

                hasAnySection = true;
            }

            if (!hasAnySection) {
                return std::optional<ChunkData>{};
            }

            if (hasBiomes) {
                chunk.setBiomes(std::move(biomeContainer));
            }

            // 路径 B 结果：方块实体（失败不影响区块加载，与同步 loadChunk 行为一致）
            if (state->blockEntityReady && state->blockEntityResult.success()) {
                for (auto& blockEntity : state->blockEntityResult.value()) {
                    if (blockEntity != nullptr) {
                        chunk.setBlockEntity(blockEntity->getPos(), std::move(blockEntity));
                    }
                }
            }

            chunk.setLoaded(true);
            chunk.setFullyGenerated(true);
            chunk.setDirty(false);
            return std::optional<ChunkData>(std::move(chunk));
        }();

        state->completion(std::move(result));
    };

    // 完成检查：减少 pending 计数，归零者把组装（反序列化）投递到 ServerCompute 线程池。
    // 反序列化（SectionCodec::toChunkSection × 24 + biomes 组装）是 CPU 密集型，放在 ServerCompute
    // 与 ServerIO 读盘分离（对齐 Moonrise ProcessOffMainTask 跑在 loadExecutor）。
    // 无 Compute 池（测试/独立模式）时降级为在 ServerIO worker 内联组装。
    // abortSignal 透传给 Compute 任务，使 SCLM 取消能中断组装阶段。
    auto checkComplete = [this, state, _assemble, abortSignal, priority]() {
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
            m_computeWorkerPool->submit(std::move(computeTask),
                /*callback=*/nullptr,
                priority,
                abortSignal);
        } else {
            // 降级：无 Compute 池，在当前线程（ServerIO worker 或测试线程）内联组装。
            std::atomic<bool> dummySig{false};
            runAssemble(dummySig);
        }
    };

    // 路径 A：section 数据。提交 StorageTask 到 ServerIO，内部同步调 loadSectionsSync。
    auto sectionExecutor = [this, state, keys = std::move(keys), checkComplete, x, z, dimension](
                               const std::atomic<bool>& abortSig) {
        if (abortSig.load(std::memory_order::acquire)) {
            {
                std::lock_guard<std::mutex> lock(state->mutex);
                state->sectionResult = Error(ErrorCode::InvalidState, "Load sections cancelled");
                state->sectionReady = true;
            }
            checkComplete();
            return false;
        }
        // 等待该区块进行中的卸载保存完成，确保读到保存后的新数据（对齐 Moonrise
        // GenericDataLoadTask 等待 chunkDataUnload）。在 ServerIO 线程等待，不阻塞主线程。
        _waitPendingChunkSave(x, z, dimension);
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
    i32 spawnX,
    i32 spawnY,
    i32 spawnZ,
    f32 spawnAngle,
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
            spawnX,
            spawnY,
            spawnZ,
            spawnAngle,
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

    SectionManager::Config config;
    config.cacheCapacity = m_config.sectionCacheCapacity;
    config.computeHash = m_config.computeHash;
    config.consistencyMode = m_config.consistencyMode;

    auto manager = std::make_unique<SectionManager>(*m_db, dimension, config);
    manager->setTaskManager(m_taskManager.get());

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

std::unordered_map<DimensionId, SectionCache::CacheStats> SingleLevelStorageManager::getCacheStats() const
{
    std::unordered_map<DimensionId, SectionCache::CacheStats> stats;
    std::lock_guard<std::mutex> lock(m_sectionManagersMutex);
    for (const auto& [dim, manager] : m_sectionManagers) {
        stats[dim] = manager->getCacheStats();
    }
    return stats;
}

size_t SingleLevelStorageManager::getTotalDirtyCount() const
{
    size_t total = 0;
    std::lock_guard<std::mutex> lock(m_sectionManagersMutex);
    for (const auto& [dim, manager] : m_sectionManagers) {
        total += manager->getDirtyCount();
    }
    return total;
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

void SingleLevelStorageManager::setCacheCapacity(DimensionId dimension, size_t capacity)
{
    _sectionManager(dimension).setCacheCapacity(capacity);
}

void SingleLevelStorageManager::clearCache(DimensionId dimension)
{
    std::lock_guard<std::mutex> lock(m_sectionManagersMutex);
    auto it = m_sectionManagers.find(dimension);
    if (it != m_sectionManagers.end()) {
        it->second->clearCache();
    }
}

void SingleLevelStorageManager::clearAllCaches()
{
    std::lock_guard<std::mutex> lock(m_sectionManagersMutex);
    for (auto& [dim, manager] : m_sectionManagers) {
        manager->clearCache();
    }
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

void SingleLevelStorageManager::initializeAutoSave(const AutoSaveConfig& config)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Initialization, "SingleLevelStorageManager::initializeAutoSave");

    if (m_autoSaveInitialized) {
        return;
    }

    m_autoSave = std::make_unique<AutoSave>(*this);
    m_autoSave->setConfig(config);
    m_autoSaveInitialized = true;
    spdlog::info("AutoSave initialized");
}

void SingleLevelStorageManager::shutdownAutoSave()
{
    if (!m_autoSaveInitialized) {
        return;
    }

    if (m_autoSave) {
        m_autoSave->stop();
    }

    m_autoSave.reset();
    m_autoSaveInitialized = false;
    spdlog::info("AutoSave shutdown complete");
}

void SingleLevelStorageManager::startAutoSave()
{
    MC_ASSERT_RELEASE(m_autoSave != nullptr);
    m_autoSave->start();
}

void SingleLevelStorageManager::stopAutoSave()
{
    if (m_autoSave) {
        m_autoSave->stop();
    }
}

bool SingleLevelStorageManager::isAutoSaveRunning() const
{
    return m_autoSave != nullptr && m_autoSave->isRunning();
}

void SingleLevelStorageManager::tickAutoSave(u64 tickCount)
{
    if (m_autoSave && m_autoSave->isRunning()) {
        m_autoSave->tick(tickCount);
    }
}

Result<size_t> SingleLevelStorageManager::saveNow()
{
    if (!isOpen()) {
        return Error(ErrorCode::InvalidState, "Storage not open");
    }
    return flushAllDirty();
}

Result<size_t> SingleLevelStorageManager::saveNowWithSnapshot(const std::string& snapshotName)
{
    if (!isOpen()) {
        return Error(ErrorCode::InvalidState, "Storage not open");
    }
    if (!m_autoSave) {
        return Error(ErrorCode::InvalidState, "AutoSave not initialized");
    }
    return m_autoSave->saveNowWithSnapshot(snapshotName);
}

} // namespace mc::world::storage
