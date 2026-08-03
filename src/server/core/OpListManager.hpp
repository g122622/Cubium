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

namespace mc::server::core {

/**
 * @brief OP 权限等级
 *
 * 参考 MC 1.16.5 的权限等级系统：
 * - 0: 普通玩家，无特殊权限
 * - 1: 可以绕过重生点保护
 * - 2: 可以使用游戏管理命令（/gamemode, /tp, /give 等）
 * - 3: 可以使用服务器管理命令（/ban, /kick, /op, /whitelist 等）
 * - 4: 可以使用所有命令（控制台级别，包括 /stop）
 */
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
    [[nodiscard]] bool isValid() const noexcept { return !uuid.empty() && !name.empty(); }

    /**
     * @brief 获取显示名称
     * @return 用于显示的名称
     */
    [[nodiscard]] std::string getDisplayName() const { return name.empty() ? uuid : name; }

    /**
     * @brief 获取权限等级数值
     * @return 权限等级 (0-4)
     */
    [[nodiscard]] i32 getLevelValue() const noexcept { return static_cast<i32>(level); }
};

/**
 * @brief OP 列表管理器
 *
 * 管理服务器 OP 列表，包括：
 * - 添加/移除 OP
 * - 查询玩家权限等级
 * - 从文件加载/保存 OP 列表
 *
 * OP 列表存储格式（JSON 数组）：
 * @code
 * [
 *   {
 *     "uuid": "xxx-xxx-xxx",
 *     "name": "Player1",
 *     "level": 4,
 *     "bypassesPlayerLimit": false
 *   }
 * ]
 * @endcode
 *
 * 使用示例：
 * @code
 * OpListManager opList;
 * opList.load("ops.json");
 *
 * // 添加 OP
 * opList.addEntry(OpEntry("uuid", "Player", OpLevel::Admin));
 *
 * // 检查权限
 * if (opList.isOp("uuid")) {
 *     i32 level = opList.getLevel("uuid");
 * }
 *
 * // 保存更改
 * opList.save();
 * @endcode
 */
class OpListManager {
public:
    /**
     * @brief 构造 OP 列表管理器
     */
    OpListManager();

    /**
     * @brief 析构函数
     */
    ~OpListManager() = default;

    // 禁止拷贝
    OpListManager(const OpListManager&) = delete;
    OpListManager& operator=(const OpListManager&) = delete;

    // 禁止移动（包含 std::mutex）
    OpListManager(OpListManager&&) = delete;
    OpListManager& operator=(OpListManager&&) = delete;

    // ========== 条目管理 ==========

    /**
     * @brief 添加或更新 OP 条目
     * @param entry OP 条目
     * @return true 如果添加或更新成功
     * @note 如果玩家已是 OP，将更新其权限等级
     */
    bool setEntry(const OpEntry& entry);

    /**
     * @brief 通过 UUID 移除 OP
     * @param uuid 玩家 UUID
     * @return true 如果移除成功，false 如果玩家不是 OP
     */
    bool removeEntry(const std::string& uuid);

    /**
     * @brief 通过名称移除 OP
     * @param name 玩家名称
     * @return true 如果移除成功，false 如果玩家不是 OP
     * @note 名称检查不区分大小写（MC 1.16.5 行为）
     */
    bool removeEntryByName(const std::string& name);

    /**
     * @brief 检查玩家是否是 OP
     * @param uuid 玩家 UUID
     * @return true 如果玩家是 OP
     */
    [[nodiscard]] bool isOp(const std::string& uuid) const;

    /**
     * @brief 检查玩家名称是否是 OP
     * @param name 玩家名称
     * @return true 如果玩家是 OP
     * @note 名称检查不区分大小写
     */
    [[nodiscard]] bool isNameOp(const std::string& name) const;

    /**
     * @brief 获取玩家权限等级
     * @param uuid 玩家 UUID
     * @return 权限等级，如果不是 OP 返回 OpLevel::Normal (0)
     */
    [[nodiscard]] OpLevel getLevel(const std::string& uuid) const;

    /**
     * @brief 通过名称获取权限等级
     * @param name 玩家名称
     * @return 权限等级，如果不是 OP 返回 OpLevel::Normal (0)
     * @note 名称检查不区分大小写
     */
    [[nodiscard]] OpLevel getLevelByName(const std::string& name) const;

    /**
     * @brief 检查玩家是否可以绕过玩家数量限制
     * @param uuid 玩家 UUID
     * @return true 如果可以绕过限制
     */
    [[nodiscard]] bool bypassesPlayerLimit(const std::string& uuid) const;

    /**
     * @brief 获取 OP 条目
     * @param uuid 玩家 UUID
     * @return 条目，如果不存在则返回空
     */
    [[nodiscard]] std::optional<OpEntry> getEntry(const std::string& uuid) const;

    /**
     * @brief 通过名称获取 OP 条目
     * @param name 玩家名称
     * @return 条目，如果不存在则返回空
     * @note 名称检查不区分大小写
     */
    [[nodiscard]] std::optional<OpEntry> getEntryByName(const std::string& name) const;

    /**
     * @brief 获取所有 OP 条目
     * @return OP 条目列表
     */
    [[nodiscard]] std::vector<OpEntry> getAllEntries() const;

    /**
     * @brief 获取所有 OP 玩家名称
     * @return 名称列表
     */
    [[nodiscard]] std::vector<std::string> getAllNames() const;

    /**
     * @brief 获取 OP 玩家数量
     * @return OP 玩家数量
     */
    [[nodiscard]] size_t size() const;

    /**
     * @brief 检查 OP 列表是否为空
     * @return true 如果 OP 列表为空
     */
    [[nodiscard]] bool empty() const;

    /**
     * @brief 清空 OP 列表
     */
    void clear() noexcept;

    // ========== 文件操作 ==========

    /**
     * @brief 从文件加载 OP 列表
     * @param path OP 列表文件路径
     * @return 成功或错误
     */
    [[nodiscard]] Result<void> load(const std::filesystem::path& path);

    /**
     * @brief 保存 OP 列表到文件
     * @param path OP 列表文件路径（可选，默认使用上次加载的路径）
     * @return 成功或错误
     */
    [[nodiscard]] Result<void> save(const std::filesystem::path& path = {});

    /**
     * @brief 重新加载 OP 列表
     * @return 成功或错误
     * @note 使用上次加载的路径
     */
    [[nodiscard]] Result<void> reload();

    /**
     * @brief 获取 OP 列表文件路径
     * @return 文件路径
     */
    [[nodiscard]] const std::filesystem::path& filePath() const { return m_filePath; }

private:
    mutable std::mutex m_mutex;
    std::unordered_map<std::string, OpEntry> m_entriesByUuid;  ///< UUID -> 条目
    std::unordered_map<std::string, std::string> m_nameToUuid; ///< 名称（小写）-> UUID
    std::filesystem::path m_filePath;

    /**
     * @brief 将名称转换为小写（用于名称映射）
     * @param name 原始名称
     * @return 小写名称
     */
    static std::string _toLowerName(const std::string& name);
};

} // namespace mc::server::core
