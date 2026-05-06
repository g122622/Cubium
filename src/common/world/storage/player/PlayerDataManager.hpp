#pragma once

#include "PlayerSaveData.hpp"
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <string>
#include <functional>
#include <mutex>

namespace mc {

// 前向声明
class ServerPlayer;
namespace server { struct ServerPlayerData; }

namespace world::storage {

// 前向声明
class RocksDBDatabase;

/**
 * @brief 玩家数据管理器
 *
 * 负责玩家数据的加载、保存和缓存。
 * 使用 RocksDB 的 players 列族进行持久化。
 *
 * 使用示例：
 * @code
 * PlayerDataManager playerMgr(db);
 *
 * // 加载玩家数据
 * auto dataResult = playerMgr.loadPlayer("player-uuid");
 * if (dataResult.success()) {
 *     PlayerSaveData& data = dataResult.value();
 *     // 使用数据
 * }
 *
 * // 保存玩家数据
 * PlayerSaveData data;
 * data.uuid = "player-uuid";
 * data.username = "Steve";
 * auto saveResult = playerMgr.savePlayer(data);
 *
 * // 保存所有脏数据
 * playerMgr.saveAllDirty();
 * @endcode
 */
class PlayerDataManager {
public:
    /**
     * @brief 玩家数据变更回调
     */
    using PlayerDataCallback = std::function<void(const String& uuid)>;

    /**
     * @brief 构造函数
     * @param db RocksDB 数据库引用
     */
    explicit PlayerDataManager(RocksDBDatabase& db);

    /**
     * @brief 析构函数
     *
     * 自动保存所有脏数据。
     */
    ~PlayerDataManager();

    // 禁止拷贝
    PlayerDataManager(const PlayerDataManager&) = delete;
    PlayerDataManager& operator=(const PlayerDataManager&) = delete;

    // 允许移动
    PlayerDataManager(PlayerDataManager&&) noexcept = default;
    PlayerDataManager& operator=(PlayerDataManager&&) noexcept = default;

    // ========== 玩家数据操作 ==========

    /**
     * @brief 加载玩家数据
     *
     * 从数据库加载玩家数据。如果缓存中存在，返回缓存数据。
     *
     * @param uuid 玩家 UUID
     * @return 玩家数据指针，如果不存在返回 nullptr
     */
    [[nodiscard]] Result<PlayerSaveData*> loadPlayer(const String& uuid);

    /**
     * @brief 保存玩家数据
     *
     * 将玩家数据标记为脏，等待异步保存。
     *
     * @param data 玩家数据
     * @return 成功或错误
     */
    Result<void> savePlayer(const PlayerSaveData& data);

    /**
     * @brief 立即保存玩家数据
     *
     * 同步写入数据库。
     *
     * @param data 玩家数据
     * @return 成功或错误
     */
    Result<void> savePlayerImmediate(const PlayerSaveData& data);

    /**
     * @brief 删除玩家数据
     *
     * 从数据库和缓存中删除玩家数据。
     *
     * @param uuid 玩家 UUID
     * @return 成功或错误
     */
    Result<void> deletePlayer(const String& uuid);

    /**
     * @brief 检查玩家数据是否存在
     *
     * @param uuid 玩家 UUID
     * @return true 如果存在
     */
    [[nodiscard]] bool hasPlayer(const String& uuid) const;

    /**
     * @brief 获取缓存的玩家数据
     *
     * 仅从缓存获取，不加载数据库。
     *
     * @param uuid 玩家 UUID
     * @return 玩家数据指针，如果未缓存返回 nullptr
     */
    [[nodiscard]] PlayerSaveData* getCachedPlayer(const String& uuid);
    [[nodiscard]] const PlayerSaveData* getCachedPlayer(const String& uuid) const;

    /**
     * @brief 从服务器玩家数据创建保存数据
     *
     * 将运行时的玩家状态转换为可持久化的格式。
     *
     * @param playerData 服务器玩家数据
     * @return 保存数据
     */
    [[nodiscard]] static PlayerSaveData fromServerPlayerData(
        const server::ServerPlayerData& playerData);

    /**
     * @brief 从玩家实体创建保存数据
     *
     * 将 Player 实体的状态转换为可持久化的格式。
     *
     * @param player 玩家实体
     * @return 保存数据
     */
    [[nodiscard]] static PlayerSaveData fromPlayer(const ServerPlayer& player);

    // ========== 批量操作 ==========

    /**
     * @brief 保存所有脏数据
     *
     * 将所有标记为脏的玩家数据写入数据库。
     *
     * @return 成功保存的玩家数量，或错误
     */
    Result<size_t> saveAllDirty();

    /**
     * @brief 保存所有缓存数据
     *
     * 将所有缓存的玩家数据写入数据库，无论是否脏。
     *
     * @return 成功保存的玩家数量，或错误
     */
    Result<size_t> saveAll();

    /**
     * @brief 标记玩家数据为脏
     *
     * @param uuid 玩家 UUID
     */
    void markDirty(const String& uuid);

    /**
     * @brief 获取脏玩家数量
     */
    [[nodiscard]] size_t dirtyCount() const;

    /**
     * @brief 获取脏玩家 UUID 列表
     */
    [[nodiscard]] std::vector<String> getDirtyUuids() const;

    /**
     * @brief 清空缓存
     *
     * 警告：这会丢弃所有未保存的数据！
     */
    void clearCache();

    // ========== 统计 ==========

    /**
     * @brief 获取缓存大小
     */
    [[nodiscard]] size_t cacheSize() const;

    // ========== 回调 ==========

    /**
     * @brief 设置玩家数据保存回调
     *
     * 当玩家数据被保存时触发。
     *
     * @param callback 回调函数
     */
    void setOnPlayerSaved(PlayerDataCallback callback) {
        m_onPlayerSaved = std::move(callback);
    }

private:
    /**
     * @brief 生成数据库键
     * @param uuid 玩家 UUID
     * @return 数据库键
     */
    [[nodiscard]] static std::vector<u8> makeKey(const String& uuid);

    /**
     * @brief 从缓存中移除玩家
     * @param uuid 玩家 UUID
     */
    void removeFromCache(const String& uuid);

private:
    RocksDBDatabase& m_db;

    mutable std::mutex m_cacheMutex;
    std::unordered_map<String, std::unique_ptr<PlayerSaveData>> m_cache;
    std::unordered_set<String> m_dirtyUuids;

    PlayerDataCallback m_onPlayerSaved;
};

} // namespace world::storage
} // namespace mc
