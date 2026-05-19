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

#pragma once

#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "world/storage/core/WorldSessionLock.hpp"
#include "world/storage/db/ConsistencyMode.hpp"
#include "world/storage/db/RocksDBConfig.hpp"
#include "world/storage/db/SectionKey.hpp"
#include "world/storage/player/PlayerDataManager.hpp"
#include "world/storage/section/SectionManager.hpp"
#include "world/storage/snapshot/BackupManager.hpp"
#include "world/storage/task/StorageTaskManager.hpp"
#include <filesystem>
#include <memory>
#include <optional>
#include <unordered_map>

namespace mc::scoreboard {
class ScoreboardDataManager;
}

namespace mc::world::storage {

struct AutoSaveConfig;
class AutoSave;

/**
 * @brief 单存档存储配置
 */
struct SingleLevelStorageConfig {
    ConsistencyMode consistencyMode = ConsistencyMode::Eventual;
    size_t sectionCacheCapacity = 1024;
    bool enableBackup = true;
    std::optional<RocksDBConfig> rocksdbConfig;
    bool computeHash = false;
};

/**
 * @brief 单存档运行时存储门面
 *
 * 一个实例只对应一个已打开的存档。
 * 负责数据库、会话锁、SectionManager、玩家数据、记分板和备份等运行时状态。
 */
class SingleLevelStorageManager {
    friend class mc::scoreboard::ScoreboardDataManager;

public:
    SingleLevelStorageManager();
    ~SingleLevelStorageManager();

    SingleLevelStorageManager(const SingleLevelStorageManager&) = delete;
    SingleLevelStorageManager& operator=(const SingleLevelStorageManager&) = delete;
    SingleLevelStorageManager(SingleLevelStorageManager&&) noexcept;
    SingleLevelStorageManager& operator=(SingleLevelStorageManager&&) noexcept;

    Result<void> open(const std::filesystem::path& worldPath, const SingleLevelStorageConfig& config);
    void close();

    [[nodiscard]] bool isOpen() const { return m_db != nullptr; }

    Result<size_t> flushAllDirty();
    Result<size_t> saveAll();
    Result<void> saveChunk(const ChunkData& chunk, DimensionId dimension);
    [[nodiscard]] Result<std::optional<ChunkData>> loadChunk(ChunkCoord x, ChunkCoord z, DimensionId dimension);

    [[nodiscard]] SectionManager& sectionManager(DimensionId dimension);
    [[nodiscard]] const SectionManager& sectionManager(DimensionId dimension) const;
    [[nodiscard]] bool hasSectionManager(DimensionId dimension) const;

    [[nodiscard]] WorldSessionLock* sessionLock()
    {
        return m_sessionLock.has_value() ? &m_sessionLock.value() : nullptr;
    }

    [[nodiscard]] const WorldSessionLock* sessionLock() const
    {
        return m_sessionLock.has_value() ? &m_sessionLock.value() : nullptr;
    }

    [[nodiscard]] BackupManager* backupManager() { return m_backupManager.get(); }
    [[nodiscard]] const BackupManager* backupManager() const { return m_backupManager.get(); }

    [[nodiscard]] util::ServerWorkerPool* ioWorkerPool() { return m_ioWorkerPool; }
    [[nodiscard]] const util::ServerWorkerPool* ioWorkerPool() const { return m_ioWorkerPool; }
    void setIoWorkerPool(util::ServerWorkerPool* workerPool);

    [[nodiscard]] StorageTaskManager* taskManager() { return m_taskManager.get(); }
    [[nodiscard]] const StorageTaskManager* taskManager() const { return m_taskManager.get(); }

    [[nodiscard]] PlayerDataManager* playerDataManager() { return m_playerDataManager.get(); }
    [[nodiscard]] const PlayerDataManager* playerDataManager() const { return m_playerDataManager.get(); }

    [[nodiscard]] mc::scoreboard::ScoreboardDataManager* scoreboardDataManager() { return m_scoreboardDataManager.get(); }
    [[nodiscard]] const mc::scoreboard::ScoreboardDataManager* scoreboardDataManager() const
    {
        return m_scoreboardDataManager.get();
    }

    Result<BackupID> createBackup(const std::string& name, const std::string& description = "");
    Result<size_t> pruneOldBackups(size_t keepCount);

    void initializeAutoSave(const AutoSaveConfig& config);
    void shutdownAutoSave();
    void startAutoSave();
    void stopAutoSave();
    [[nodiscard]] bool isAutoSaveRunning() const;
    [[nodiscard]] AutoSave* autoSave() { return m_autoSave.get(); }
    [[nodiscard]] const AutoSave* autoSave() const { return m_autoSave.get(); }
    void tickAutoSave(u64 tickCount);
    Result<size_t> saveNow();
    Result<size_t> saveNowWithSnapshot(const std::string& snapshotName);

    [[nodiscard]] const SingleLevelStorageConfig& config() const { return m_config; }
    void setConsistencyMode(ConsistencyMode mode);
    [[nodiscard]] const std::filesystem::path& worldPath() const { return m_worldPath; }

    [[nodiscard]] std::unordered_map<DimensionId, SectionCache::CacheStats> getCacheStats() const;
    [[nodiscard]] size_t getTotalDirtyCount() const;
    [[nodiscard]] std::vector<DimensionId> getOpenDimensions() const;

    void setCacheCapacity(DimensionId dimension, size_t capacity);
    void clearCache(DimensionId dimension);
    void clearAllCaches();

private:
    SectionManager* createSectionManager(DimensionId dimension);

    [[nodiscard]] RocksDBDatabase* database() { return m_db.get(); }
    [[nodiscard]] const RocksDBDatabase* database() const { return m_db.get(); }

private:
    std::unique_ptr<RocksDBDatabase> m_db;
    std::optional<WorldSessionLock> m_sessionLock;
    std::unique_ptr<BackupManager> m_backupManager;
    std::unique_ptr<PlayerDataManager> m_playerDataManager;
    std::unique_ptr<mc::scoreboard::ScoreboardDataManager> m_scoreboardDataManager;
    std::unique_ptr<AutoSave> m_autoSave;
    bool m_autoSaveInitialized = false;
    util::ServerWorkerPool* m_ioWorkerPool = nullptr;
    std::unique_ptr<StorageTaskManager> m_taskManager;

    mutable std::mutex m_sectionManagersMutex;
    std::unordered_map<DimensionId, std::unique_ptr<SectionManager>> m_sectionManagers;

    SingleLevelStorageConfig m_config;
    std::filesystem::path m_worldPath;
};

} // namespace mc::world::storage
