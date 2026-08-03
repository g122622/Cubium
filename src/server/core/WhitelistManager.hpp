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
#include <algorithm>
#include <cctype>
#include <cstddef>
#include <filesystem>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace mc::server::core {

/**
 * @brief 白名单条目
 *
 * 存储单个白名单玩家的信息。
 */
struct WhitelistEntry {
    std::string uuid; ///< 玩家 UUID（格式：xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx）
    std::string name; ///< 玩家名称

    /**
     * @brief 默认构造函数
     */
    WhitelistEntry() = default;

    /**
     * @brief 构造函数
     * @param playerUuid 玩家 UUID
     * @param playerName 玩家名称
     */
    WhitelistEntry(std::string playerUuid, std::string playerName)
        : uuid(std::move(playerUuid))
        , name(std::move(playerName))
    {}

    /**
     * @brief 检查条目是否有效
     * @return true 如果 UUID 和名称都不为空
     */
    [[nodiscard]] bool isValid() const { return !uuid.empty() && !name.empty(); }
};

/**
 * @brief 白名单管理器
 *
 * 管理服务器白名单，包括：
 * - 白名单开关状态
 * - 添加/移除玩家
 * - 从文件加载/保存白名单
 * - 检查玩家是否在白名单中
 *
 * 白名单存储格式（JSON 数组）：
 * @code
 * [
 *   {"uuid": "xxx-xxx-xxx", "name": "Player1"},
 *   {"uuid": "yyy-yyy-yyy", "name": "Player2"}
 * ]
 * @endcode
 *
 * 使用示例：
 * @code
 * WhitelistManager whitelist;
 * whitelist.load("whitelist.json");
 *
 * // 添加玩家
 * whitelist.addEntry(WhitelistEntry("uuid", "Player"));
 *
 * // 检查玩家
 * if (whitelist.isWhitelisted("uuid")) {
 *     // 允许登录
 * }
 *
 * // 保存更改
 * whitelist.save();
 * @endcode
 */
class WhitelistManager {
public:
    /**
     * @brief 构造白名单管理器
     */
    WhitelistManager();

    /**
     * @brief 析构函数
     */
    ~WhitelistManager() = default;

    // 禁止拷贝
    WhitelistManager(const WhitelistManager&) = delete;
    WhitelistManager& operator=(const WhitelistManager&) = delete;

    // 禁止移动（包含 std::mutex）
    WhitelistManager(WhitelistManager&&) = delete;
    WhitelistManager& operator=(WhitelistManager&&) = delete;

    // ========== 白名单开关 ==========

    /**
     * @brief 检查白名单是否启用
     * @return true 如果白名单启用
     */
    [[nodiscard]] bool isEnabled() const;

    /**
     * @brief 设置白名单开关
     * @param enabled 是否启用
     */
    void setEnabled(bool enabled);

    // ========== 条目管理 ==========

    /**
     * @brief 添加玩家到白名单
     * @param entry 白名单条目
     * @return true 如果添加成功，false 如果玩家已在白名单中
     */
    bool addEntry(const WhitelistEntry& entry);

    /**
     * @brief 通过 UUID 移除玩家
     * @param uuid 玩家 UUID
     * @return true 如果移除成功，false 如果玩家不在白名单中
     */
    bool removeEntry(const std::string& uuid);

    /**
     * @brief 通过名称移除玩家
     * @param name 玩家名称
     * @return true 如果移除成功，false 如果玩家不在白名单中
     */
    bool removeEntryByName(const std::string& name);

    /**
     * @brief 检查玩家是否在白名单中（通过 UUID）
     * @param uuid 玩家 UUID
     * @return true 如果玩家在白名单中
     */
    [[nodiscard]] bool isWhitelisted(const std::string& uuid) const;

    /**
     * @brief 检查玩家名称是否在白名单中
     * @param name 玩家名称
     * @return true 如果玩家名称在白名单中
     * @note 名称检查不区分大小写（MC 1.16.5 行为）
     */
    [[nodiscard]] bool isNameWhitelisted(const std::string& name) const;

    /**
     * @brief 获取白名单条目
     * @param uuid 玩家 UUID
     * @return 条目，如果不存在则返回空
     */
    [[nodiscard]] std::optional<WhitelistEntry> getEntry(const std::string& uuid) const;

    /**
     * @brief 通过名称获取白名单条目
     * @param name 玩家名称
     * @return 条目，如果不存在则返回空
     */
    [[nodiscard]] std::optional<WhitelistEntry> getEntryByName(const std::string& name) const;

    /**
     * @brief 获取所有白名单条目
     * @return 白名单条目列表
     */
    [[nodiscard]] std::vector<WhitelistEntry> getAllEntries() const;

    /**
     * @brief 获取所有白名单玩家名称
     * @return 名称列表
     */
    [[nodiscard]] std::vector<std::string> getAllNames() const;

    /**
     * @brief 获取白名单玩家数量
     * @return 玩家数量
     */
    [[nodiscard]] size_t size() const;

    /**
     * @brief 检查白名单是否为空
     * @return true 如果白名单为空
     */
    [[nodiscard]] bool empty() const;

    /**
     * @brief 清空白名单
     */
    void clear();

    // ========== 文件操作 ==========

    /**
     * @brief 从文件加载白名单
     * @param path 白名单文件路径
     * @return 成功或错误
     */
    [[nodiscard]] Result<void> load(const std::filesystem::path& path);

    /**
     * @brief 保存白名单到文件
     * @param path 白名单文件路径（可选，默认使用上次加载的路径）
     * @return 成功或错误
     */
    [[nodiscard]] Result<void> save(const std::filesystem::path& path = {});

    /**
     * @brief 重新加载白名单
     * @return 成功或错误
     * @note 使用上次加载的路径
     */
    [[nodiscard]] Result<void> reload();

    /**
     * @brief 获取白名单文件路径
     * @return 文件路径
     */
    [[nodiscard]] std::filesystem::path filePath() const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_filePath;
    }

private:
    /**
     * @brief 将字符串转换为小写
     * @param str 输入字符串
     * @return 小写字符串
     */
    static std::string _toLower(std::string str)
    {
        std::transform(str.begin(), str.end(), str.begin(), ::tolower);
        return str;
    }

    mutable std::mutex m_mutex;
    std::unordered_map<std::string, WhitelistEntry> m_entriesByUuid; ///< UUID -> 条目
    std::unordered_map<std::string, std::string> m_nameToUuid;       ///< 名称（小写）-> UUID
    std::filesystem::path m_filePath;
    bool m_enabled = false;
};

} // namespace mc::server::core
