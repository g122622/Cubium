/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without including limitation the rights to use, copy, modify,
 * merge, publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
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

#include "common/command/ICommandSource.hpp" // for Uuid and UuidHash
#include "common/core/Types.hpp"
#include <cstddef>
#include <string>
#include <unordered_map>

namespace mc::client {

/**
 * @brief 玩家身份注册表
 *
 * 统一维护 UUID ↔ EntityInstanceId ↔ PlayerId ↔ username 的多向映射，
 * 消除历史代码中 `static_cast<EntityInstanceId>(playerId)` 的反模式。
 *
 * ## 数据来源
 *
 * 三条信息流原本相互孤立，本注册表是唯一的汇合点：
 * - **LoginResponsePacket**（本地玩家）：playerId / entityId / uuid / username 一次性到齐。
 * - **PlayerListEntry**（玩家列表）：uuid / username，可能早于或晚于 spawn。
 * - **PlayerSpawnPacket**（网络玩家实体）：playerId / username / 位置，无 uuid，也无 entityId
 *   （历史反模式用 `static_cast<EntityInstanceId>(playerId)` 凑数）。
 *
 * ## 解决的问题
 *
 * 1. 本地玩家原先无 UUID（皮肤/玩家列表无法用 UUID 索引）。
 * 2. 网络玩家 spawn 时无 UUID，无法与 PlayerListEntry 关联。
 * 3. 渲染层需要按 EntityInstanceId 查询皮肤，但皮肤按 UUID 索引——中间缺一层翻译。
 *
 * ## 线程安全
 *
 * 非线程安全。调用者需保证在正确的线程访问（网络回调线程与渲染线程的同步由调用方负责）。
 */
class PlayerIdentityRegistry {
public:
    PlayerIdentityRegistry() = default;
    ~PlayerIdentityRegistry() = default;

    // 禁止拷贝
    PlayerIdentityRegistry(const PlayerIdentityRegistry&) = delete;
    PlayerIdentityRegistry& operator=(const PlayerIdentityRegistry&) = delete;

    // 允许移动
    PlayerIdentityRegistry(PlayerIdentityRegistry&&) noexcept = default;
    PlayerIdentityRegistry& operator=(PlayerIdentityRegistry&&) noexcept = default;

    // ========== 注册 ==========

    /**
     * @brief 注册本地玩家（登录成功时调用）
     *
     * LoginResponsePacket 一次性提供全部身份字段。
     *
     * @param entityId 世界实体标识
     * @param playerId 网络会话标识
     * @param uuid 玩家持久 UUID（服务端 generateOfflineUuid 生成）
     * @param username 用户名
     */
    void registerLocalPlayer(
        EntityInstanceId entityId, PlayerId playerId, const Uuid& uuid, const std::string& username);

    /**
     * @brief 注册网络玩家实体（PlayerSpawnPacket 到达时调用）
     *
     * spawn 包无 UUID，UUID 通过 username 在已注册的 PlayerListEntry 中反查。
     * 若 username 尚未通过 registerPlayerListUuid 注册，UUID 留空，待
     * PlayerListEntry 到达后再补全（见 tryResolveUuid）。
     *
     * @param entityId 世界实体标识（由客户端 EntityManager 分配，非 playerId 强转）
     * @param playerId 网络会话标识
     * @param username 用户名
     */
    void registerNetworkPlayer(EntityInstanceId entityId, PlayerId playerId, const std::string& username);

    /**
     * @brief 记录 PlayerListEntry 的 uuid↔username（玩家列表包到达时调用）
     *
     * 可能早于或晚于对应的 registerNetworkPlayer：
     * - 早于：暂存 uuid↔username，待 spawn 时由 registerNetworkPlayer 取用补全。
     * - 晚于：直接补全已注册实体条目的 UUID。
     *
     * @param uuid 玩家 UUID
     * @param username 用户名
     */
    void registerPlayerListUuid(const Uuid& uuid, const std::string& username);

    /**
     * @brief 显式补全某 entityId 的 UUID（网络玩家实体已注册但 UUID 后到时调用）
     *
     * @param entityId 世界实体标识
     * @param uuid 玩家 UUID
     * @return true 补全成功；false 该 entityId 未注册
     */
    bool assignUuidToEntity(EntityInstanceId entityId, const Uuid& uuid);

    // ========== 移除 ==========

    /**
     * @brief 按实体ID移除
     */
    void removeByEntityId(EntityInstanceId entityId);

    /**
     * @brief 按玩家ID移除
     */
    void removeByPlayerId(PlayerId playerId);

    /**
     * @brief 按UUID移除（玩家列表移除时调用）
     */
    void removeByUuid(const Uuid& uuid);

    /**
     * @brief 清除全部（登出/断开连接时调用）
     */
    void clear();

    // ========== 查询 ==========

    /**
     * @brief 按实体ID查询 UUID
     * @return UUID 指针；未注册或 UUID 未补全返回 nullptr
     */
    [[nodiscard]] const Uuid* uuidOf(EntityInstanceId entityId) const;

    /**
     * @brief 按 UUID 查询实体ID
     * @return 实体ID；未注册返回 INVALID_ENTITY_ID
     */
    [[nodiscard]] EntityInstanceId entityIdOf(const Uuid& uuid) const;

    /**
     * @brief 按实体ID查询玩家ID
     * @return 玩家ID；未注册返回 0
     */
    [[nodiscard]] PlayerId playerIdOf(EntityInstanceId entityId) const;

    /**
     * @brief 按用户名查询 UUID
     * @return UUID 指针；未知返回 nullptr
     */
    [[nodiscard]] const Uuid* uuidByUsername(const std::string& username) const;

    /**
     * @brief 按用户名查询实体ID
     * @return 实体ID；未知返回 INVALID_ENTITY_ID
     */
    [[nodiscard]] EntityInstanceId entityIdByUsername(const std::string& username) const;

    /**
     * @brief 该实体ID是否为本地玩家
     */
    [[nodiscard]] bool isLocal(EntityInstanceId entityId) const;

    /**
     * @brief 已注册条目数量
     */
    [[nodiscard]] size_t size() const { return m_byEntity.size(); }

private:
    struct Entry {
        EntityInstanceId entityId;
        PlayerId playerId;
        Uuid uuid{};
        std::string username;
        bool isLocal;
        bool hasUuid;
    };

    /// 实体ID → 条目（权威存储）
    std::unordered_map<EntityInstanceId, Entry> m_byEntity;
    /// 玩家ID → 实体ID
    std::unordered_map<PlayerId, EntityInstanceId> m_entityByPlayer;
    /// UUID → 实体ID（仅 hasUuid 的条目）
    std::unordered_map<Uuid, EntityInstanceId, UuidHash> m_entityByUuid;
    /// 用户名 → 实体ID（仅已注册实体）
    std::unordered_map<std::string, EntityInstanceId> m_entityByUsername;
    /// PlayerListEntry 的 username → UUID（实体注册前暂存，spawn 时取用）
    std::unordered_map<std::string, Uuid> m_uuidByUsername;

    EntityInstanceId m_localEntityId = INVALID_ENTITY_ID;

    void _indexEntry(const Entry& entry);
    void _unindexEntry(const Entry& entry);
};

} // namespace mc::client
