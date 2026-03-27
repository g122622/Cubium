#pragma once

#include "LevelSave.hpp"
#include "../io/IOWorker.hpp"
#include "../serializer/ChunkSerializer.hpp"
#include "../data/LevelData.hpp"
#include "../data/PlayerData.hpp"
#include "../../../core/Types.hpp"
#include "../../../core/Result.hpp"
#include "../../../world/chunk/ChunkData.hpp"
#include "../../../world/chunk/ChunkPos.hpp"
#include <memory>
#include <future>
#include <unordered_map>
#include <mutex>

namespace mc::world::save {

/**
 * @brief 存档管理器 - 存档系统的主入口
 *
 * 负责创建、加载、管理世界存档。
 * 每个世界对应一个 SaveManager 实例。
 *
 * ## 功能
 *
 * - 创建/加载世界存档
 * - 异步区块读写
 * - 玩家数据管理
 * - 世界元数据管理
 * - 多维度支持
 *
 * ## 使用示例
 *
 * ```cpp
 * // 创建新世界
 * WorldSettings settings;
 * settings.levelName = "MyWorld";
 * settings.seed = 12345;
 * auto createResult = SaveManager::createNew("saves/", "MyWorld", settings);
 * if (createResult.success()) {
 *     auto& saveManager = createResult.value();
 *     // ...
 * }
 *
 * // 加载现有世界
 * auto loadResult = SaveManager::load("saves/MyWorld");
 * if (loadResult.success()) {
 *     auto& saveManager = loadResult.value();
 *
 *     // 加载区块
 *     auto chunkFuture = saveManager->loadChunkAsync(0, 0);
 *     auto chunkResult = chunkFuture.get();
 *
 *     // 保存区块
 *     auto saveFuture = saveManager->saveChunkAsync(*chunkData);
 *     saveFuture.get();
 * }
 * ```
 *
 * ## 线程安全
 *
 * 所有公共方法都是线程安全的。
 * 区块操作使用异步 I/O，不会阻塞主线程。
 */
class SaveManager {
public:
    /**
     * @brief 创建新世界存档
     *
     * @param savesDir 存档根目录（如 "saves/"）
     * @param worldName 世界名称
     * @param settings 世界设置
     * @return 成功返回 SaveManager，失败返回错误
     */
    [[nodiscard]] static Result<std::unique_ptr<SaveManager>>
    createNew(const std::filesystem::path& savesDir,
              const String& worldName,
              const data::WorldSettings& settings);

    /**
     * @brief 加载现有世界存档
     *
     * @param worldDir 世界目录路径
     * @return 成功返回 SaveManager，失败返回错误
     */
    [[nodiscard]] static Result<std::unique_ptr<SaveManager>>
    load(const std::filesystem::path& worldDir);

    ~SaveManager();

    // 禁止拷贝
    SaveManager(const SaveManager&) = delete;
    SaveManager& operator=(const SaveManager&) = delete;

    // 允许移动
    SaveManager(SaveManager&& other) noexcept;
    SaveManager& operator=(SaveManager&& other) noexcept;

    // ========== 区块操作 ==========

    /**
     * @brief 异步加载区块
     *
     * @param x 区块 X 坐标（世界坐标）
     * @param z 区块 Z 坐标（世界坐标）
     * @param dimension 维度 ID（默认 0 = 主世界）
     * @return 区块数据的 Future
     *
     * @note 返回的 Future 在 IOWorker 线程完成
     */
    [[nodiscard]] std::future<Result<std::unique_ptr<ChunkData>>>
    loadChunkAsync(ChunkCoord x, ChunkCoord z, i32 dimension = 0);

    /**
     * @brief 异步保存区块
     *
     * @param chunk 区块数据
     * @param dimension 维度 ID（默认 0 = 主世界）
     * @return 保存操作的 Future
     */
    std::future<Result<void>>
    saveChunkAsync(const ChunkData& chunk, i32 dimension = 0);

    /**
     * @brief 检查区块是否存在
     *
     * @param x 区块 X 坐标
     * @param z 区块 Z 坐标
     * @param dimension 维度 ID
     * @return 如果区块存在返回 true
     */
    [[nodiscard]] bool hasChunk(ChunkCoord x, ChunkCoord z, i32 dimension = 0) const;

    // ========== 玩家操作 ==========

    /**
     * @brief 加载玩家数据
     *
     * @param playerId 玩家 UUID 字符串
     * @return 玩家数据（不存在返回 nullptr）
     */
    [[nodiscard]] Result<std::unique_ptr<data::PlayerData>>
    loadPlayer(const String& playerId);

    /**
     * @brief 保存玩家数据
     *
     * @param playerData 玩家数据
     * @return 成功返回 void，失败返回错误
     */
    Result<void> savePlayer(const data::PlayerData& playerData);

    /**
     * @brief 检查玩家数据是否存在
     *
     * @param playerId 玩家 UUID 字符串
     * @return 如果存在返回 true
     */
    [[nodiscard]] bool hasPlayerData(const String& playerId) const;

    // ========== 世界数据 ==========

    /**
     * @brief 获取世界元数据
     */
    [[nodiscard]] const data::LevelData& levelData() const { return *m_levelData; }

    /**
     * @brief 获取可变的世界元数据
     */
    [[nodiscard]] data::LevelData& levelData() { return *m_levelData; }

    /**
     * @brief 保存世界元数据
     *
     * 将 level.dat 和 level.dat_old 写入磁盘。
     *
     * @return 成功返回 void，失败返回错误
     */
    Result<void> saveLevelData();

    /**
     * @brief 获取维度 Region 目录路径
     *
     * @param dimension 维度 ID（0=主世界, -1=下界, 1=末地）
     * @return Region 目录路径
     */
    [[nodiscard]] std::filesystem::path getDimensionPath(i32 dimension) const;

    /**
     * @brief 获取世界目录路径
     */
    [[nodiscard]] std::filesystem::path worldDir() const;

    /**
     * @brief 获取世界名称
     */
    [[nodiscard]] const String& worldName() const;

    // ========== 同步与关闭 ==========

    /**
     * @brief 同步所有待写入的数据到磁盘
     *
     * 等待所有异步操作完成。
     *
     * @return 成功返回 void，失败返回错误
     */
    Result<void> sync();

    /**
     * @brief 关闭存档（自动同步）
     *
     * 释放所有资源，关闭文件句柄。
     */
    void close();

    /**
     * @brief 检查是否已关闭
     */
    [[nodiscard]] bool isClosed() const { return m_closed; }

private:
    explicit SaveManager(std::unique_ptr<LevelSave> levelSave,
                         std::unique_ptr<data::LevelData> levelData);

    /**
     * @brief 获取或创建维度的 IOWorker
     */
    io::IOWorker* getOrCreateWorker(i32 dimension);

    // ========== 成员变量 ==========

    std::unique_ptr<LevelSave> m_levelSave;
    std::unique_ptr<data::LevelData> m_levelData;

    /// 各维度的 IOWorker（键为维度 ID）
    std::unordered_map<i32, std::unique_ptr<io::IOWorker>> m_workers;
    mutable std::mutex m_workersMutex;

    /// 玩家数据缓存
    std::unordered_map<String, std::unique_ptr<data::PlayerData>> m_playerCache;
    mutable std::mutex m_playerCacheMutex;

    /// 是否已关闭
    std::atomic<bool> m_closed{false};

    /// 当前游戏时间（用于区块序列化）
    std::atomic<i64> m_gameTime{0};
};

} // namespace mc::world::save
