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
 * @brief 封禁 IP 条目
 *
 * 存储单个被封禁 IP 地址的信息。
 */
struct BannedIpEntry {
    std::string ip;      ///< IP 地址
    std::string created; ///< 封禁创建时间（格式：yyyy-MM-dd HH:mm:ss Z）
    std::string source;  ///< 封禁执行者名称
    std::string expires; ///< 过期时间（"forever" 表示永久封禁）
    std::string reason;  ///< 封禁原因

    /**
     * @brief 默认构造函数
     */
    BannedIpEntry() = default;

    /**
     * @brief 构造函数
     * @param ipAddress IP 地址
     * @param banCreated 创建时间
     * @param banSource 封禁执行者
     * @param banExpires 过期时间（空字符串表示永久）
     * @param banReason 封禁原因
     */
    BannedIpEntry(std::string ipAddress,
        std::string banCreated,
        std::string banSource,
        std::string banExpires,
        std::string banReason)
        : ip(std::move(ipAddress))
        , created(std::move(banCreated))
        , source(std::move(banSource))
        , expires(std::move(banExpires))
        , reason(std::move(banReason))
    {}

    /**
     * @brief 检查条目是否有效
     * @return true 如果 IP 不为空
     */
    [[nodiscard]] bool isValid() const { return !ip.empty(); }

    /**
     * @brief 检查封禁是否已过期
     * @return true 如果封禁已过期
     */
    [[nodiscard]] bool hasExpired() const;

    /**
     * @brief 获取显示名称
     * @return IP 地址
     */
    [[nodiscard]] std::string getDisplayName() const { return ip; }
};

/**
 * @brief IP 封禁列表管理器
 *
 * 管理服务器 IP 封禁列表，包括：
 * - 添加/移除封禁条目
 * - 检查 IP 是否被封禁
 * - 从文件加载/保存封禁列表
 * - 自动清理过期封禁
 *
 * 封禁列表存储格式（JSON 数组）：
 * @code
 * [
 *   {
 *     "ip": "192.168.1.100",
 *     "created": "2024-01-15 12:00:00 +0800",
 *     "source": "ServerAdmin",
 *     "expires": "forever",
 *     "reason": "DDoS attack"
 *   }
 * ]
 * @endcode
 *
 * 使用示例：
 * @code
 * BannedIpList banList;
 * banList.load("banned-ips.json");
 *
 * // 检查 IP 是否被封禁
 * if (banList.isBanned(ip)) {
 *     auto entry = banList.getEntry(ip);
 *     // 显示封禁原因
 * }
 *
 * // 封禁 IP
 * banList.addEntry(BannedIpEntry(ip, created, source, expires, reason));
 * banList.save();
 * @endcode
 */
class BannedIpList {
public:
    /**
     * @brief 构造封禁列表管理器
     */
    BannedIpList();

    /**
     * @brief 析构函数
     */
    ~BannedIpList() = default;

    // 禁止拷贝
    BannedIpList(const BannedIpList&) = delete;
    BannedIpList& operator=(const BannedIpList&) = delete;

    // 禁止移动（包含 std::mutex）
    BannedIpList(BannedIpList&&) = delete;
    BannedIpList& operator=(BannedIpList&&) = delete;

    // ========== 条目管理 ==========

    /**
     * @brief 添加 IP 到封禁列表
     * @param entry 封禁条目
     * @return true 如果添加成功，false 如果 IP 已被封禁
     */
    bool addEntry(const BannedIpEntry& entry);

    /**
     * @brief 移除封禁
     * @param ip IP 地址
     * @return true 如果移除成功，false 如果 IP 未被封禁
     */
    bool removeEntry(const std::string& ip);

    /**
     * @brief 检查 IP 是否被封禁
     * @param ip IP 地址
     * @return true 如果 IP 被封禁（且未过期）
     * @note 会自动清理过期的封禁条目
     */
    [[nodiscard]] bool isBanned(const std::string& ip) const;

    /**
     * @brief 获取封禁条目
     * @param ip IP 地址
     * @return 条目，如果不存在或已过期则返回空
     */
    [[nodiscard]] std::optional<BannedIpEntry> getEntry(const std::string& ip) const;

    /**
     * @brief 获取所有封禁条目
     * @return 封禁条目列表
     */
    [[nodiscard]] std::vector<BannedIpEntry> getAllEntries() const;

    /**
     * @brief 获取所有封禁 IP 地址
     * @return IP 地址列表
     */
    [[nodiscard]] std::vector<std::string> getAllBannedIps() const;

    /**
     * @brief 获取封禁 IP 数量
     * @return 封禁 IP 数量
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
    mutable std::unordered_map<std::string, BannedIpEntry> m_entries; ///< IP -> 条目
    std::filesystem::path m_filePath;
};

} // namespace mc::server::core
