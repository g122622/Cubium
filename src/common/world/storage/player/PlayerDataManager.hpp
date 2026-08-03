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

#include "PlayerSaveData.hpp"
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include <cstddef>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace mc {

// 前向声明
class Player;
namespace server {
struct ServerPlayerData;
}

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
    using PlayerDataCallback = std::function<void(const std::string& uuid)>;

    /**
     * @brief 构造函数
     * @param db RocksDB 数据库引用
     */
    explicit PlayerDataManager(RocksDBDatabase& db);

    /**
     * @brief 析构函数
     *
     * 不在析构中隐式保存。
     * 保存职责由上层共享存储/服务器关闭流程统一负责。
     */
    ~PlayerDataManager();

    // 禁止拷贝
    PlayerDataManager(const PlayerDataManager&) = delete;
    PlayerDataManager& operator=(const PlayerDataManager&) = delete;

    // 禁止移动（有引用成员和 mutex）
    PlayerDataManager(PlayerDataManager&&) noexcept = delete;
    PlayerDataManager& operator=(PlayerDataManager&&) noexcept = delete;

    // ========== 玩家数据操作 ==========

    /**
     * @brief 加载玩家数据
     *
     * 从数据库加载玩家数据。如果缓存中存在，返回缓存数据。
     *
     * @param uuid 玩家 UUID
     * @return 玩家数据指针，如果不存在返回 nullptr
     */
    [[nodiscard]] Result<PlayerSaveData*> loadPlayer(const std::string& uuid);

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
    Result<void> deletePlayer(const std::string& uuid);

    /**
     * @brief 检查玩家数据是否存在
     *
     * @param uuid 玩家 UUID
     * @return true 如果存在
     */
    [[nodiscard]] bool hasPlayer(const std::string& uuid) const;

    /**
     * @brief 获取缓存的玩家数据
     *
     * 仅从缓存获取，不加载数据库。
     *
     * @param uuid 玩家 UUID
     * @return 玩家数据指针，如果未缓存返回 nullptr
     */
    [[nodiscard]] PlayerSaveData* getCachedPlayer(const std::string& uuid);
    [[nodiscard]] const PlayerSaveData* getCachedPlayer(const std::string& uuid) const;

    /**
     * @brief 从服务器玩家数据创建保存数据
     *
     * 将运行时的玩家状态转换为可持久化的格式。
     *
     * @param playerData 服务器玩家数据
     * @return 保存数据
     */
    [[nodiscard]] static PlayerSaveData fromServerPlayerData(const server::ServerPlayerData& playerData);

    /**
     * @brief 从玩家实体创建保存数据
     *
     * 将 Player 实体的状态转换为可持久化的格式。
     *
     * @param player 玩家实体
     * @return 保存数据
     */
    [[nodiscard]] static PlayerSaveData fromPlayer(const Player& player);

    /**
     * @brief 将保存数据应用到玩家实体
     *
     * 将 PlayerSaveData 中的所有字段恢复到 Player 对象，
     * 用于玩家登录时从存档恢复状态。
     *
     * @param player 目标玩家实体
     * @param data 保存数据
     */
    static void applyToPlayer(Player& player, const PlayerSaveData& data);

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
    void markDirty(const std::string& uuid);

    /**
     * @brief 获取脏玩家数量
     */
    [[nodiscard]] size_t dirtyCount() const;

    /**
     * @brief 获取脏玩家 UUID 列表
     */
    [[nodiscard]] std::vector<std::string> getDirtyUuids() const;

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
    void setOnPlayerSaved(PlayerDataCallback callback) { m_onPlayerSaved = std::move(callback); }

private:
    /**
     * @brief 生成数据库键
     * @param uuid 玩家 UUID
     * @return 数据库键
     */
    [[nodiscard]] static std::vector<u8> _makeKey(const std::string& uuid);

    /**
     * @brief 从缓存中移除玩家
     * @param uuid 玩家 UUID
     */
    void _removeFromCache(const std::string& uuid);

private:
    RocksDBDatabase& m_db;

    mutable std::mutex m_cacheMutex;
    std::unordered_map<std::string, std::unique_ptr<PlayerSaveData>> m_cache;
    std::unordered_set<std::string> m_dirtyUuids;

    PlayerDataCallback m_onPlayerSaved;
};

} // namespace world::storage
} // namespace mc
