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
#include <cstddef>
#include <filesystem>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace mc::server {
enum class OpLevel : u8 {
    Normal = 0,     ///< 普通玩家
    Moderator = 1,  ///< 版主（可绕过重生点保护）
    GameMaster = 2, ///< 游戏管理员（可使用游戏管理命令）
    Admin = 3,      ///< 服务器管理员（可使用服务器管理命令）
    Owner = 4       ///< 服务器所有者（控制台级别权限）
};

/**
 * @brief 在 OP 列表等级之上叠加单机主机作弊提升。
 *
 * 单机主机在开启作弊时，运行时视为 OP（与原版 PlayerList.isOp 中
 * isSingleplayerOwner && isAllowCommands 的判定一致，不写 ops.json）。
 *
 * @param opListLevel 来自 OP 列表的基础权限等级。
 * @param isOwner 是否为单机世界主机。
 * @param cheatsEnabled 单机世界是否开启作弊（allowCommands）。
 * @return 提升后的权限等级；非主机或未开作弊时原样返回。
 */
[[nodiscard]] inline i32 applyOwnerCheatsBoost(i32 opListLevel, bool isOwner, bool cheatsEnabled) noexcept
{
    if (isOwner && cheatsEnabled) {
        return std::max(opListLevel, static_cast<i32>(OpLevel::Owner));
    }
    return opListLevel;
}

/**
 * @brief OP 列表条目
 *
 * 存储单个 OP 玩家的信息。
 * 参考 MC 1.16.5 的 OpEntry。
 */
struct OpEntry {
    std::string uuid;                    ///< 玩家 UUID（格式：xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx）
    std::string name;                    ///< 玩家名称
    OpLevel level = OpLevel::GameMaster; ///< 权限等级，默认为 2
    bool bypassesPlayerLimit = false;    ///< 是否绕过玩家数量限制

    /**
     * @brief 默认构造函数
     */
    OpEntry() = default;

    /**
     * @brief 构造函数
     * @param playerUuid 玩家 UUID
     * @param playerName 玩家名称
     * @param opLevel 权限等级
     * @param bypassLimit 是否绕过玩家限制
     */
    OpEntry(
        std::string playerUuid, std::string playerName, OpLevel opLevel = OpLevel::GameMaster, bool bypassLimit = false)
        : uuid(std::move(playerUuid))
        , name(std::move(playerName))
        , level(opLevel)
        , bypassesPlayerLimit(bypassLimit)
    {}

    /**
     * @brief 检查条目是否有效
     * @return true 如果 UUID 和名称都不为空
     */
    [[nodiscard]] bool isValid() const noexcept
    {
        return !uuid.empty() && !name.empty();
    }

    /**
     * @brief 获取显示名称
     * @return 用于显示的名称
     */
    [[nodiscard]] std::string getDisplayName() const
    {
        return name.empty() ? uuid : name;
    }

    /**
     * @brief 获取权限等级数值
     * @return 权限等级 (0-4)
     */
    [[nodiscard]] i32 getLevelValue() const noexcept
    {
        return static_cast<i32>(level);
    }
};

} // namespace mc::server
