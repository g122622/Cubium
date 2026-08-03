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
#include <chrono>
#include <cstddef>
#include <filesystem>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace mc::server::core {

/**
 * @brief 封禁玩家条目
 *
 * 存储单个被封禁玩家的信息。
 */
struct BannedPlayerEntry {
    std::string uuid;    ///< 玩家 UUID（格式：xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx）
    std::string name;    ///< 玩家名称
    std::string created; ///< 封禁创建时间（格式：yyyy-MM-dd HH:mm:ss Z）
    std::string source;  ///< 封禁执行者名称
    std::string expires; ///< 过期时间（"forever" 表示永久封禁）
    std::string reason;  ///< 封禁原因

    /**
     * @brief 默认构造函数
     */
    BannedPlayerEntry() = default;

    /**
     * @brief 构造函数
     * @param playerUuid 玩家 UUID
     * @param playerName 玩家名称
     * @param banCreated 创建时间
     * @param banSource 封禁执行者
     * @param banExpires 过期时间（空字符串表示永久）
     * @param banReason 封禁原因
     */
    BannedPlayerEntry(std::string playerUuid,
        std::string playerName,
        std::string banCreated,
        std::string banSource,
        std::string banExpires,
        std::string banReason)
        : uuid(std::move(playerUuid))
        , name(std::move(playerName))
        , created(std::move(banCreated))
        , source(std::move(banSource))
        , expires(std::move(banExpires))
        , reason(std::move(banReason))
    {}

    /**
     * @brief 检查条目是否有效
     * @return true 如果 UUID 和名称都不为空
     */
    [[nodiscard]] bool isValid() const { return !uuid.empty() && !name.empty(); }

    /**
     * @brief 检查封禁是否已过期
     * @return true 如果封禁已过期
     */
    [[nodiscard]] bool hasExpired() const;

    /**
     * @brief 获取显示名称
     * @return 用于显示的名称
     */
    [[nodiscard]] std::string getDisplayName() const { return name.empty() ? uuid : name; }
};

/**
 * @brief 玩家封禁列表管理器
 *
 * 管理服务器玩家封禁列表，包括：
 * - 添加/移除封禁条目
 * - 检查玩家是否被封禁
 * - 从文件加载/保存封禁列表
 * - 自动清理过期封禁
 *
 * 封禁列表存储格式（JSON 数组）：
 * @code
 * [
 *   {
 *     "uuid": "xxx-xxx-xxx",
 *     "name": "Player1",
 *     "created": "2024-01-15 10:30:00 +0800",
 *     "source": "ServerAdmin",
 *     "expires": "forever",
 *     "reason": "Griefing"
 *   }
 * ]
 * @endcode
 *
 * 使用示例：
 * @code
 * BannedPlayerList banList;
 * banList.load("banned-players.json");
 *
 * // 检查玩家是否被封禁
 * if (banList.isBanned(uuid)) {
 *     auto entry = banList.getEntry(uuid);
 *     // 显示封禁原因
 * }
 *
 * // 封禁玩家
 * banList.addEntry(BannedPlayerEntry(uuid, name, created, source, expires, reason));
 * banList.save();
 * @endcode
 */
class BannedPlayerList {
public:
    /**
     * @brief 构造封禁列表管理器
     */
    BannedPlayerList();

    /**
     * @brief 析构函数
     */
    ~BannedPlayerList() = default;

    // 禁止拷贝
    BannedPlayerList(const BannedPlayerList&) = delete;
    BannedPlayerList& operator=(const BannedPlayerList&) = delete;

    // 禁止移动（包含 std::mutex）
    BannedPlayerList(BannedPlayerList&&) = delete;
    BannedPlayerList& operator=(BannedPlayerList&&) = delete;

    // ========== 条目管理 ==========

    /**
     * @brief 添加玩家到封禁列表
     * @param entry 封禁条目
     * @return true 如果添加成功，false 如果玩家已被封禁
     */
    bool addEntry(const BannedPlayerEntry& entry);

    /**
     * @brief 通过 UUID 移除封禁
     * @param uuid 玩家 UUID
     * @return true 如果移除成功，false 如果玩家未被封禁
     */
    bool removeEntry(const std::string& uuid);

    /**
     * @brief 通过名称移除封禁
     * @param name 玩家名称
     * @return true 如果移除成功，false 如果玩家未被封禁
     */
    bool removeEntryByName(const std::string& name);

    /**
     * @brief 检查玩家是否被封禁（通过 UUID）
     * @param uuid 玩家 UUID
     * @return true 如果玩家被封禁（且未过期）
     * @note 会自动清理过期的封禁条目
     */
    [[nodiscard]] bool isBanned(const std::string& uuid) const;

    /**
     * @brief 检查玩家名称是否被封禁
     * @param name 玩家名称
     * @return true 如果玩家被封禁（且未过期）
     * @note 名称检查不区分大小写（MC 1.16.5 行为）
     */
    [[nodiscard]] bool isNameBanned(const std::string& name) const;

    /**
     * @brief 获取封禁条目
     * @param uuid 玩家 UUID
     * @return 条目，如果不存在或已过期则返回空
     */
    [[nodiscard]] std::optional<BannedPlayerEntry> getEntry(const std::string& uuid) const;

    /**
     * @brief 通过名称获取封禁条目
     * @param name 玩家名称
     * @return 条目，如果不存在或已过期则返回空
     */
    [[nodiscard]] std::optional<BannedPlayerEntry> getEntryByName(const std::string& name) const;

    /**
     * @brief 获取所有封禁条目
     * @return 封禁条目列表
     */
    [[nodiscard]] std::vector<BannedPlayerEntry> getAllEntries() const;

    /**
     * @brief 获取所有封禁玩家名称
     * @return 名称列表
     */
    [[nodiscard]] std::vector<std::string> getAllBannedNames() const;

    /**
     * @brief 获取封禁玩家数量
     * @return 封禁玩家数量
     */
    [[nodiscard]] size_t size() const;

    /**
     * @brief 检查封禁列表是否为空
     * @return true 如果封禁列表为空
     */
    [[nodiscard]] bool empty() const;

    /**
     * @brief 清空封禁列表
     */
    void clear();

    // ========== 文件操作 ==========

    /**
     * @brief 从文件加载封禁列表
     * @param path 封禁列表文件路径
     * @return 成功或错误
     */
    [[nodiscard]] Result<void> load(const std::filesystem::path& path);

    /**
     * @brief 保存封禁列表到文件
     * @param path 封禁列表文件路径（可选，默认使用上次加载的路径）
     * @return 成功或错误
     */
    [[nodiscard]] Result<void> save(const std::filesystem::path& path = {});

    /**
     * @brief 重新加载封禁列表
     * @return 成功或错误
     * @note 使用上次加载的路径
     */
    [[nodiscard]] Result<void> reload();

    /**
     * @brief 获取封禁列表文件路径
     * @return 文件路径
     */
    [[nodiscard]] const std::filesystem::path& filePath() const { return m_filePath; }

private:
    /**
     * @brief 清理过期的封禁条目
     */
    void _removeExpired() const;

    /**
     * @brief 获取当前时间的格式化字符串
     * @return 格式化的时间字符串
     */
    static std::string _getCurrentTimeString();

    mutable std::mutex m_mutex;
    mutable std::unordered_map<std::string, BannedPlayerEntry> m_entriesByUuid; ///< UUID -> 条目
    mutable std::unordered_map<std::string, std::string> m_nameToUuid;          ///< 名称（小写）-> UUID
    std::filesystem::path m_filePath;
};

} // namespace mc::server::core
