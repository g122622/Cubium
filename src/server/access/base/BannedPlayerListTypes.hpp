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

namespace mc::server {
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
    [[nodiscard]] bool isValid() const
    {
        return !uuid.empty() && !name.empty();
    }

    /**
     * @brief 检查封禁是否已过期
     * @return true 如果封禁已过期
     */
    [[nodiscard]] bool hasExpired() const;

    /**
     * @brief 获取显示名称
     * @return 用于显示的名称
     */
    [[nodiscard]] std::string getDisplayName() const
    {
        return name.empty() ? uuid : name;
    }
};

} // namespace mc::server
