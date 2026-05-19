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

#include "WorldStorageService.hpp"
#include "perfetto/TraceEvents.hpp"
#include "scoreboard/storage/ScoreboardDataManager.hpp"
#include "world/storage/db/ColumnFamilies.hpp"
#include "world/storage/db/SectionCodec.hpp"
#include <stdexcept>
#include <spdlog/spdlog.h>

namespace mc::world::storage {

// ============================================================================
// 构造与析构
// ============================================================================

WorldStorageService::WorldStorageService()
    : m_worldListService(std::make_unique<WorldListService>(WorldStoragePaths::defaultPaths()))
{}

WorldStorageService::~WorldStorageService()
{
    close();
}

WorldStorageService::WorldStorageService(WorldStorageService&& other) noexcept
    : m_db(std::move(other.m_db))
    , m_worldListService(std::move(other.m_worldListService))
    , m_sessionLock(std::move(other.m_sessionLock))
    , m_backupManager(std::move(other.m_backupManager))
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

WorldStorageService& WorldStorageService::operator=(WorldStorageService&& other) noexcept
{
    if (this != &other) {
        close();
        m_db = std::move(other.m_db);
        m_worldListService = std::move(other.m_worldListService);
        m_sessionLock = std::move(other.m_sessionLock);
        m_backupManager = std::move(other.m_backupManager);
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

void WorldStorageService::setIoWorkerPool(util::ServerWorkerPool* workerPool)
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

// ============================================================================
// 生命周期管理
// ============================================================================

Result<void> WorldStorageService::open(const std::filesystem::path& worldPath, const WorldStorageConfig& config)
{
    MC_TRACE_EVENT("server.world", "WorldStorageService::open", "path", worldPath.string());

    if (isOpen()) {
        return Error(ErrorCode::InvalidState, "Storage already open");
    }

    m_config = config;
    m_worldPath = worldPath;

    // 1. 创建目录结构
    std::filesystem::path dbPath = worldPath / "db";
    std::filesystem::path backupPath = worldPath / "backups";

    try {
        std::filesystem::create_directories(worldPath);
        std::filesystem::create_directories(dbPath);
        if (config.enableBackup) {
            std::filesystem::create_directories(backupPath);
        }
    }
    catch (const std::exception& e) {
        return Error(ErrorCode::FileWriteFailed, fmt::format("Failed to create directories: {}", e.what()));
    }

    // 2. 获取会话锁
    auto lockResult = WorldSessionLock::acquire(worldPath);
    if (!lockResult.success()) {
        return lockResult.error();
    }
    m_sessionLock.emplace(std::move(lockResult.value()));

    // 3. 打开数据库
    RocksDBConfig dbConfig = config.rocksdbConfig.value_or(RocksDBConfig{});
    dbConfig.consistencyMode = config.consistencyMode;

    auto dbResult = RocksDBDatabase::open(dbPath, dbConfig);
    if (!dbResult.success()) {
        m_sessionLock.reset();
        return dbResult.error();
    }
    m_db = dbResult.value();

    // 4. 初始化备份管理器
    if (config.enableBackup) {
        auto backupResult = BackupManager::open(backupPath);
        if (backupResult.success()) {
            m_backupManager = backupResult.value();
        } else {
            spdlog::warn("Failed to initialize backup manager: {}", backupResult.error().message());
        }
    }

    // 4.1 初始化玩家数据管理器
    m_playerDataManager = std::make_unique<PlayerDataManager>(*m_db);
    m_scoreboardDataManager = std::make_unique<mc::scoreboard::ScoreboardDataManager>(*this);

    // 4.2 存储任务管理器由服务器注入的 IO Worker 池驱动
    if (!m_ioWorkerPool) {
        m_taskManager.reset();
    } else if (!m_taskManager) {
        m_taskManager = std::make_unique<StorageTaskManager>(*m_ioWorkerPool);
    }

    // 5. 清空 SectionManager 缓存
    m_sectionManagers.clear();

    spdlog::info("WorldStorageService opened at {} (consistency: {})",
        worldPath.string(),
        static_cast<i32>(config.consistencyMode));

    return {};
}

void WorldStorageService::close()
{
    if (!isOpen()) {
        return;
    }

    MC_TRACE_EVENT("server.world", "WorldStorageService::close");

    // 1. 刷新所有脏数据
    auto flushResult = flushAllDirty();
    if (!flushResult.success()) {
        spdlog::error("Failed to flush dirty sections: {}", flushResult.error().message());
    }

    // 2. 清空 SectionManager
    {
        std::lock_guard<std::mutex> lock(m_sectionManagersMutex);
        m_sectionManagers.clear();
    }

    // 3. 关闭备份管理器
    m_backupManager.reset();
    m_scoreboardDataManager.reset();

    // 4. 关闭数据库
    m_db.reset();

    // 5. 断开 IO Worker 池引用，池本身由 MinecraftServer 统一管理
    m_taskManager.reset();
    m_ioWorkerPool = nullptr;

    // 6. 释放会话锁
    m_sessionLock.reset();

    spdlog::info("WorldStorageService closed");

    m_worldPath.clear();
}

Result<size_t> WorldStorageService::flushAllDirty()
{
    MC_TRACE_EVENT("server.world", "WorldStorageService::flushAllDirty");

    if (!isOpen()) {
        return Error(ErrorCode::InvalidState, "Storage not open");
    }

    size_t totalFlushed = 0;

    // 保存脏Section
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

    // 保存脏玩家数据
    if (m_playerDataManager) {
        auto playerResult = m_playerDataManager->saveAllDirty();
        if (playerResult.failed()) {
            spdlog::error("Failed to flush dirty player data: {}", playerResult.error().message());
        } else {
            totalFlushed += playerResult.value();
        }
    }

    if (totalFlushed > 0) {
        spdlog::debug("Flushed {} dirty sections and player data", totalFlushed);
    }

    return totalFlushed;
}

Result<size_t> WorldStorageService::saveAll()
{
    MC_TRACE_EVENT("server.world", "WorldStorageService::saveAll");

    if (!isOpen()) {
        return Error(ErrorCode::InvalidState, "Storage not open");
    }

    size_t totalSaved = 0;

    // 保存所有Section
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

    // 保存所有玩家数据
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

Result<void> WorldStorageService::saveChunk(const ChunkData& chunk, DimensionId dimension)
{
    if (!isOpen()) {
        return Error(ErrorCode::InvalidState, "Storage not open");
    }

    auto& manager = sectionManager(dimension);

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

        SectionKey key(chunk.x(), chunk.z(), sectionY, dimension);
        auto sectionDataResult = SectionCodec::fromChunkSection(*section, key, biomes);
        if (sectionDataResult.failed()) {
            return sectionDataResult.error();
        }

        auto saveResult = manager.saveSectionSync(key, sectionDataResult.value());
        if (saveResult.failed()) {
            return saveResult.error();
        }
    }

    return Result<void>::ok();
}

Result<std::optional<ChunkData>> WorldStorageService::loadChunk(ChunkCoord x, ChunkCoord z, DimensionId dimension)
{
    if (!isOpen()) {
        return Error(ErrorCode::InvalidState, "Storage not open");
    }

    auto& manager = sectionManager(dimension);
    ChunkData chunk(x, z);
    bool hasAnySection = false;
    bool hasBiomes = false;
    BiomeContainer biomeContainer;

    for (i8 sectionY = 0; sectionY < world::CHUNK_SECTIONS; ++sectionY) {
        SectionKey key(x, z, sectionY, dimension);
        auto loadResult = manager.loadSectionSync(key);
        if (loadResult.failed()) {
            return loadResult.error();
        }
        if (!loadResult.value()) {
            continue;
        }

        const auto& sectionData = loadResult.value();
        if (!hasBiomes && sectionData->biomes.size() == BiomeContainer::BIOME_SIZE) {
            for (i32 biomeY = 0; biomeY < BiomeContainer::BIOME_HEIGHT; ++biomeY) {
                for (i32 biomeZ = 0; biomeZ < BiomeContainer::BIOME_DEPTH; ++biomeZ) {
                    for (i32 biomeX = 0; biomeX < BiomeContainer::BIOME_WIDTH; ++biomeX) {
                        const size_t biomeIndex = static_cast<size_t>(
                            biomeY * BiomeContainer::BIOME_WIDTH * BiomeContainer::BIOME_DEPTH +
                            biomeZ * BiomeContainer::BIOME_WIDTH + biomeX);
                        biomeContainer.setBiome(biomeX, biomeY, biomeZ, sectionData->biomes[biomeIndex]);
                    }
                }
            }
            hasBiomes = true;
        }

        ChunkSection* section = chunk.createSection(sectionY);
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

    chunk.setLoaded(true);
    chunk.setFullyGenerated(true);
    chunk.setDirty(false);
    return std::optional<ChunkData>(std::move(chunk));
}

// ============================================================================
// 子服务访问
// ============================================================================

SectionManager& WorldStorageService::sectionManager(DimensionId dimension)
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

    // 创建新的 SectionManager
    auto* manager = createSectionManager(dimension);
    if (!manager) {
        throw std::runtime_error(
            fmt::format("Failed to create SectionManager for dimension {}", static_cast<i32>(dimension)));
    }
    return *manager;
}

const SectionManager& WorldStorageService::sectionManager(DimensionId dimension) const
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

bool WorldStorageService::hasSectionManager(DimensionId dimension) const
{
    std::lock_guard<std::mutex> lock(m_sectionManagersMutex);
    return m_sectionManagers.find(dimension) != m_sectionManagers.end();
}

SectionManager* WorldStorageService::createSectionManager(DimensionId dimension)
{
    MC_TRACE_EVENT(
        "server.world", "WorldStorageService::createSectionManager", "dimension", static_cast<i32>(dimension));

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

    spdlog::debug("Created SectionManager for dimension {} with cache capacity {}",
        static_cast<i32>(dimension),
        m_config.sectionCacheCapacity);

    return it->second.get();
}

// ============================================================================
// 配置
// ============================================================================

void WorldStorageService::setConsistencyMode(ConsistencyMode mode)
{
    m_config.consistencyMode = mode;
    spdlog::info("Changed consistency mode to {}", static_cast<i32>(mode));
}

std::filesystem::path WorldStorageService::resolveWorldPath(const std::string& worldName) const
{
    return m_worldListService->paths().worldDir(worldName);
}

std::filesystem::path WorldStorageService::savesDirectory() const
{
    return m_worldListService->paths().savesDir();
}

// ============================================================================
// 统计信息
// ============================================================================

std::unordered_map<DimensionId, SectionCache::CacheStats> WorldStorageService::getCacheStats() const
{
    std::unordered_map<DimensionId, SectionCache::CacheStats> stats;

    std::lock_guard<std::mutex> lock(m_sectionManagersMutex);
    for (const auto& [dim, manager] : m_sectionManagers) {
        stats[dim] = manager->getCacheStats();
    }

    return stats;
}

size_t WorldStorageService::getTotalDirtyCount() const
{
    size_t total = 0;

    std::lock_guard<std::mutex> lock(m_sectionManagersMutex);
    for (const auto& [dim, manager] : m_sectionManagers) {
        total += manager->getDirtyCount();
    }

    return total;
}

std::vector<DimensionId> WorldStorageService::getOpenDimensions() const
{
    std::vector<DimensionId> dimensions;

    std::lock_guard<std::mutex> lock(m_sectionManagersMutex);
    dimensions.reserve(m_sectionManagers.size());
    for (const auto& [dim, manager] : m_sectionManagers) {
        dimensions.push_back(dim);
    }

    return dimensions;
}

// ============================================================================
// Section 缓存管理
// ============================================================================

void WorldStorageService::setCacheCapacity(DimensionId dimension, size_t capacity)
{
    sectionManager(dimension).setCacheCapacity(capacity);
}

void WorldStorageService::clearCache(DimensionId dimension)
{
    std::lock_guard<std::mutex> lock(m_sectionManagersMutex);
    auto it = m_sectionManagers.find(dimension);
    if (it != m_sectionManagers.end()) {
        it->second->clearCache();
    }
}

void WorldStorageService::clearAllCaches()
{
    std::lock_guard<std::mutex> lock(m_sectionManagersMutex);
    for (auto& [dim, manager] : m_sectionManagers) {
        manager->clearCache();
    }
}

// ============================================================================
// 备份管理
// ============================================================================

Result<BackupID> WorldStorageService::createBackup(const std::string& name, const std::string& description)
{
    if (!m_backupManager) {
        return Error(ErrorCode::InvalidState, "Backup manager not enabled");
    }

    if (!m_db) {
        return Error(ErrorCode::InvalidState, "Database not open");
    }

    return m_backupManager->createBackup(*m_db, name, description);
}

Result<size_t> WorldStorageService::pruneOldBackups(size_t keepCount)
{
    if (!m_backupManager) {
        return Error(ErrorCode::InvalidState, "Backup manager not enabled");
    }

    return m_backupManager->pruneOldBackups(keepCount);
}

} // namespace mc::world::storage
