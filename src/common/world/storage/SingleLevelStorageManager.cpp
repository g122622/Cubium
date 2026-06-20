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
#include "common/world/WorldConstants.hpp"
#include "perfetto/TraceEvents.hpp"
#include "scoreboard/storage/ScoreboardDataManager.hpp"
#include "world/storage/backend/BedrockLDBBackend.hpp"
#include "world/storage/backend/JavaAnvilBackend.hpp"
#include "world/storage/db/SectionCodec.hpp"
#include "world/storage/save/AutoSave.hpp"
#include <stdexcept>
#include <spdlog/spdlog.h>

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
    }
    return *this;
}

void SingleLevelStorageManager::setIoWorkerPool(util::ServerWorkerPool* workerPool)
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
    MC_TRACE_EVENT("server.world", "SingleLevelStorageManager::open", "path", worldPath.string());

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

    MC_TRACE_EVENT("server.world", "SingleLevelStorageManager::close");

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
    MC_TRACE_EVENT("server.world", "SingleLevelStorageManager::flushAllDirty");

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
    MC_TRACE_EVENT("server.world", "SingleLevelStorageManager::saveAll");

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
    bool thundering)
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
            thundering);
    }

    // 外来格式：只读，不保存
    return Result<void>::ok();
}

Result<std::unique_ptr<nbt::tags::compound_list_tag>> SingleLevelStorageManager::loadScheduledEvents()
{
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
    MC_TRACE_EVENT(
        "server.world", "SingleLevelStorageManager::createSectionManager", "dimension", static_cast<i32>(dimension));

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
    MC_TRACE_EVENT("server.initialization", "SingleLevelStorageManager::initializeAutoSave");

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

    auto result = saveAll();
    if (result.failed()) {
        spdlog::error("Failed to save data during auto-save shutdown: {}", result.error().message());
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
