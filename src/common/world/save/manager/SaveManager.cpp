#include "SaveManager.hpp"
#include "../core/SessionLock.hpp"
#include "../serializer/ChunkSerializer.hpp"
#include "../io/FileUtil.hpp"
#include <filesystem>

namespace mc::world::save {

// ========== 构造函数/析构函数 ==========

SaveManager::SaveManager(std::unique_ptr<LevelSave> levelSave,
                         std::unique_ptr<data::LevelData> levelData)
    : m_levelSave(std::move(levelSave))
    , m_levelData(std::move(levelData))
{
}

SaveManager::~SaveManager() {
    close();
}

SaveManager::SaveManager(SaveManager&& other) noexcept
    : m_levelSave(std::move(other.m_levelSave))
    , m_levelData(std::move(other.m_levelData))
    , m_workers(std::move(other.m_workers))
    , m_playerCache(std::move(other.m_playerCache))
    , m_closed(other.m_closed.load())
    , m_gameTime(other.m_gameTime.load())
{
    other.m_closed = true;
}

SaveManager& SaveManager::operator=(SaveManager&& other) noexcept {
    if (this != &other) {
        close();

        m_levelSave = std::move(other.m_levelSave);
        m_levelData = std::move(other.m_levelData);
        m_workers = std::move(other.m_workers);
        m_playerCache = std::move(other.m_playerCache);
        m_closed = other.m_closed.load();
        m_gameTime = other.m_gameTime.load();

        other.m_closed = true;
    }
    return *this;
}

// ========== 静态工厂方法 ==========

Result<std::unique_ptr<SaveManager>>
SaveManager::createNew(const std::filesystem::path& savesDir,
                       const String& worldName,
                       const data::WorldSettings& settings) {
    // 创建世界目录结构
    auto levelSaveResult = LevelSave::create(savesDir, worldName);
    if (levelSaveResult.failed()) {
        return levelSaveResult.error();
    }

    auto levelSave = std::move(levelSaveResult.value());

    // 创建默认的世界元数据
    auto levelData = std::make_unique<data::LevelData>();
    levelData->levelName = worldName;
    levelData->seed = settings.seed;
    levelData->generatorName = settings.generatorName;
    levelData->generateFeatures = settings.generateFeatures;
    levelData->bonusChest = settings.bonusChest;
    levelData->gameType = settings.gameType;
    levelData->hardcore = settings.hardcore;
    levelData->allowCommands = settings.allowCommands;
    levelData->initialized = true;

    // 设置生成点
    levelData->spawnX = settings.spawnX;
    levelData->spawnY = settings.spawnY;
    levelData->spawnZ = settings.spawnZ;

    auto saveManager = std::unique_ptr<SaveManager>(
        new SaveManager(std::move(levelSave), std::move(levelData))
    );

    // 保存初始 level.dat
    auto saveResult = saveManager->saveLevelData();
    if (saveResult.failed()) {
        return saveResult.error();
    }

    return saveManager;
}

Result<std::unique_ptr<SaveManager>>
SaveManager::load(const std::filesystem::path& worldDir) {
    // 打开世界目录
    auto levelSaveResult = LevelSave::open(worldDir);
    if (levelSaveResult.failed()) {
        return levelSaveResult.error();
    }

    auto levelSave = std::move(levelSaveResult.value());

    // 加载 level.dat
    auto levelDataResult = serializer::LevelDataSerializer::load(
        levelSave->levelDatPath()
    );
    if (levelDataResult.failed()) {
        return levelDataResult.error();
    }

    auto saveManager = std::unique_ptr<SaveManager>(
        new SaveManager(std::move(levelSave), std::move(levelDataResult.value()))
    );

    return saveManager;
}

// ========== 区块操作 ==========

std::future<Result<std::unique_ptr<ChunkData>>>
SaveManager::loadChunkAsync(ChunkCoord x, ChunkCoord z, i32 dimension) {
    auto promise = std::promise<Result<std::unique_ptr<ChunkData>>>();
    auto future = promise.get_future();

    if (m_closed.load()) {
        promise.set_value(Error(ErrorCode::InvalidState, "SaveManager is closed"));
        return future;
    }

    // 获取或创建 IOWorker
    io::IOWorker* worker = getOrCreateWorker(dimension);
    if (worker == nullptr) {
        promise.set_value(Error(ErrorCode::OutOfMemory, "Failed to create IOWorker"));
        return future;
    }

    // 异步加载区块
    auto loadFuture = worker->loadChunk(x, z);

    // 将结果转换为 ChunkData
    // 注意：这里需要在线程中处理
    std::thread([promise = std::move(promise),
                 loadFuture = std::move(loadFuture)]() mutable {
        auto result = loadFuture.get();
        if (result.failed()) {
            promise.set_value(result.error());
            return;
        }

        auto& nbtOpt = result.value();
        if (!nbtOpt.has_value()) {
            // 区块不存在
            promise.set_value(nullptr);
            return;
        }

        // 反序列化 NBT
        auto chunkResult = serializer::ChunkSerializer::deserialize(nbtOpt.value());
        if (chunkResult.failed()) {
            promise.set_value(chunkResult.error());
            return;
        }

        promise.set_value(std::move(chunkResult.value()));
    }).detach();

    return future;
}

std::future<Result<void>>
SaveManager::saveChunkAsync(const ChunkData& chunk, i32 dimension) {
    auto promise = std::promise<Result<void>>();
    auto future = promise.get_future();

    if (m_closed.load()) {
        promise.set_value(Error(ErrorCode::InvalidState, "SaveManager is closed"));
        return future;
    }

    // 获取或创建 IOWorker
    io::IOWorker* worker = getOrCreateWorker(dimension);
    if (worker == nullptr) {
        promise.set_value(Error(ErrorCode::OutOfMemory, "Failed to create IOWorker"));
        return future;
    }

    // 序列化区块到 NBT
    auto nbt = serializer::ChunkSerializer::serialize(chunk, m_gameTime.load());

    // 异步保存
    auto saveFuture = worker->saveChunk(chunk.x(), chunk.z(), std::move(nbt));

    // 等待保存完成
    std::thread([promise = std::move(promise),
                 saveFuture = std::move(saveFuture)]() mutable {
        auto result = saveFuture.get();
        promise.set_value(result);
    }).detach();

    return future;
}

bool SaveManager::hasChunk(ChunkCoord x, ChunkCoord z, i32 dimension) const {
    std::lock_guard<std::mutex> lock(m_workersMutex);

    auto it = m_workers.find(dimension);
    if (it == m_workers.end()) {
        // 检查文件是否存在
        std::filesystem::path regionDir = m_levelSave->regionDir(dimension);
        i32 regionX = x >> 5;
        i32 regionZ = z >> 5;
        std::filesystem::path regionPath = regionDir /
            ("r." + std::to_string(regionX) + "." + std::to_string(regionZ) + ".mca");
        return std::filesystem::exists(regionPath);
    }

    return it->second->hasChunk(x, z);
}

// ========== 玩家操作 ==========

Result<std::unique_ptr<data::PlayerData>>
SaveManager::loadPlayer(const String& playerId) {
    if (m_closed.load()) {
        return Error(ErrorCode::InvalidState, "SaveManager is closed");
    }

    // 先检查缓存
    {
        std::lock_guard<std::mutex> lock(m_playerCacheMutex);
        auto it = m_playerCache.find(playerId);
        if (it != m_playerCache.end()) {
            // 返回拷贝
            auto data = std::make_unique<data::PlayerData>(*it->second);
            return data;
        }
    }

    // 从文件加载
    std::filesystem::path playerPath = m_levelSave->playerDataPath(playerId);
    if (!std::filesystem::exists(playerPath)) {
        // 玩家数据不存在，返回 nullptr
        return nullptr;
    }

    auto loadResult = serializer::PlayerDataSerializer::load(playerPath);
    if (loadResult.failed()) {
        return loadResult.error();
    }

    // 存入缓存
    {
        std::lock_guard<std::mutex> lock(m_playerCacheMutex);
        m_playerCache[playerId] = std::make_unique<data::PlayerData>(*loadResult.value());
    }

    return loadResult;
}

Result<void> SaveManager::savePlayer(const data::PlayerData& playerData) {
    if (m_closed.load()) {
        return Error(ErrorCode::InvalidState, "SaveManager is closed");
    }

    // 确保目录存在
    std::filesystem::path playerDir = m_levelSave->playerDataDir();
    if (!std::filesystem::exists(playerDir)) {
        try {
            std::filesystem::create_directories(playerDir);
        } catch (const std::filesystem::filesystem_error& e) {
            return Error(ErrorCode::FileWriteFailed,
                         "Failed to create player data directory: " + String(e.what()));
        }
    }

    // 保存到文件
    std::filesystem::path playerPath = m_levelSave->playerDataPath(playerData.uuid.toString());
    auto saveResult = serializer::PlayerDataSerializer::save(playerPath, playerData);
    if (saveResult.failed()) {
        return saveResult.error();
    }

    // 更新缓存
    {
        std::lock_guard<std::mutex> lock(m_playerCacheMutex);
        m_playerCache[playerData.uuid.toString()] =
            std::make_unique<data::PlayerData>(playerData);
    }

    return {};
}

bool SaveManager::hasPlayerData(const String& playerId) const {
    std::filesystem::path playerPath = m_levelSave->playerDataPath(playerId);
    return std::filesystem::exists(playerPath);
}

// ========== 世界数据 ==========

Result<void> SaveManager::saveLevelData() {
    if (m_closed.load()) {
        return Error(ErrorCode::InvalidState, "SaveManager is closed");
    }

    // 更新最后游玩时间
    m_levelData->lastPlayed = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();

    // 备份旧的 level.dat
    std::filesystem::path levelDatPath = m_levelSave->levelDatPath();
    std::filesystem::path levelDatOldPath = m_levelSave->levelDatOldPath();

    if (std::filesystem::exists(levelDatPath)) {
        try {
            std::filesystem::copy_file(levelDatPath, levelDatOldPath,
                std::filesystem::copy_options::overwrite_existing);
        } catch (...) {
            // 忽略备份错误
        }
    }

    // 保存新的 level.dat
    return serializer::LevelDataSerializer::save(levelDatPath, *m_levelData);
}

std::filesystem::path SaveManager::getDimensionPath(i32 dimension) const {
    return m_levelSave->regionDir(dimension);
}

std::filesystem::path SaveManager::worldDir() const {
    return m_levelSave->worldDir();
}

const String& SaveManager::worldName() const {
    return m_levelSave->worldName();
}

// ========== 同步与关闭 ==========

Result<void> SaveManager::sync() {
    if (m_closed.load()) {
        return Error(ErrorCode::InvalidState, "SaveManager is closed");
    }

    // 同步所有 IOWorker
    std::vector<std::future<Result<void>>> syncFutures;

    {
        std::lock_guard<std::mutex> lock(m_workersMutex);
        for (auto& pair : m_workers) {
            syncFutures.push_back(pair.second->sync());
        }
    }

    // 等待所有同步完成
    for (auto& future : syncFutures) {
        auto result = future.get();
        if (result.failed()) {
            return result.error();
        }
    }

    // 同步 level.dat
    return saveLevelData();
}

void SaveManager::close() {
    if (m_closed.exchange(true)) {
        return;  // 已经关闭
    }

    // 同步所有数据
    sync();

    // 关闭所有 IOWorker
    {
        std::lock_guard<std::mutex> lock(m_workersMutex);
        for (auto& pair : m_workers) {
            pair.second->close();
        }
        m_workers.clear();
    }

    // 清空缓存
    {
        std::lock_guard<std::mutex> lock(m_playerCacheMutex);
        m_playerCache.clear();
    }
}

// ========== 私有方法 ==========

io::IOWorker* SaveManager::getOrCreateWorker(i32 dimension) {
    std::lock_guard<std::mutex> lock(m_workersMutex);

    auto it = m_workers.find(dimension);
    if (it != m_workers.end()) {
        return it->second.get();
    }

    // 创建新的 IOWorker
    std::filesystem::path regionDir = m_levelSave->regionDir(dimension);

    // 确保目录存在
    if (!std::filesystem::exists(regionDir)) {
        try {
            std::filesystem::create_directories(regionDir);
        } catch (...) {
            return nullptr;
        }
    }

    auto worker = std::make_unique<io::IOWorker>(regionDir, 1);
    auto* ptr = worker.get();
    m_workers[dimension] = std::move(worker);

    return ptr;
}

} // namespace mc::world::save
