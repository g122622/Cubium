#include "WorldStorageService.hpp"
#include "world/storage/db/ColumnFamilies.hpp"
#include "perfetto/TraceEvents.hpp"
#include <spdlog/spdlog.h>
#include <stdexcept>

namespace mc::world::storage {

// ============================================================================
// 构造与析构
// ============================================================================

WorldStorageService::WorldStorageService()
    : m_worldListService(std::make_unique<WorldListService>(WorldStoragePaths::defaultPaths()))
{
}

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

Result<void> WorldStorageService::open(const std::filesystem::path& worldPath,
                                        const WorldStorageConfig& config)
{
    MC_TRACE_EVENT("server.world", "WorldStorageService::open",
                   "path", worldPath.string());

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
    } catch (const std::exception& e) {
        return Error(ErrorCode::FileWriteFailed,
                     fmt::format("Failed to create directories: {}", e.what()));
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
            spdlog::warn("Failed to initialize backup manager: {}",
                         backupResult.error().message());
        }
    }

    // 4.1 初始化玩家数据管理器
    m_playerDataManager = std::make_unique<PlayerDataManager>(*m_db);

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
        spdlog::error("Failed to flush dirty sections: {}",
                      flushResult.error().message());
    }

    // 2. 清空 SectionManager
    {
        std::lock_guard<std::mutex> lock(m_sectionManagersMutex);
        m_sectionManagers.clear();
    }

    // 3. 关闭备份管理器
    m_backupManager.reset();

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
            spdlog::error("Failed to flush dirty player data: {}",
                         playerResult.error().message());
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
            spdlog::error("Failed to save all player data: {}",
                         playerResult.error().message());
        } else {
            totalSaved += playerResult.value();
        }
    }

    if (totalSaved > 0) {
        spdlog::info("Saved {} cached sections and player data", totalSaved);
    }

    return totalSaved;
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
        throw std::runtime_error(fmt::format("Failed to create SectionManager for dimension {}",
                                              static_cast<i32>(dimension)));
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
        throw std::runtime_error(fmt::format("SectionManager not found for dimension {}",
                                              static_cast<i32>(dimension)));
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
    MC_TRACE_EVENT("server.world", "WorldStorageService::createSectionManager",
                   "dimension", static_cast<i32>(dimension));

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
                  static_cast<i32>(dimension), m_config.sectionCacheCapacity);

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

Result<BackupID> WorldStorageService::createBackup(const std::string& name,
                                                    const std::string& description)
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
