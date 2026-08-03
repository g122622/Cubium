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

#include "common/core/Types.hpp"
#include "server/network/ServerNetwork.hpp"
#include <cstddef>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace mc {
class Player;
class ServerPlayer;
} // namespace mc

namespace mc::server {
class ServerWorld;
class IServer;
} // namespace mc::server

namespace mc::server {

/**
 * @brief 服务端玩家实体管理器
 *
 * 负责创建和管理服务端玩家实体，整合：
 * - 实体创建（ServerPlayer 对象）
 * - 世界实体池管理（EntityManager）
 * - 实体追踪（EntityTracker）
 * - PlayerId ↔ EntityInstanceId 双向映射
 *
 * ## 设计原则
 *
 * - PlayerId：网络会话标识，由 PlayerManager 分配
 * - EntityInstanceId：世界实体标识，由 EntityManager 分配
 * - 玩家实体被纳入世界实体池，与其他实体统一管理
 * - EntityTracker 追踪玩家实体，自动同步给其他玩家
 * - createPlayerEntity 创建的是 ServerPlayer（而非基类 Player），
 *   以便成就系统、命令系统、选择器等通过 Player::asServerPlayer()
 *   获取 ServerPlayer* 并访问其特有功能（PlayerAdvancements、网络通信等）
 *
 * ## 线程安全
 *
 * 所有公共方法都是线程安全的。
 */
class ServerPlayerEntityManager {
public:
    /**
     * @brief 构造函数
     */
    ServerPlayerEntityManager() = default;

    /**
     * @brief 析构函数
     */
    ~ServerPlayerEntityManager() = default;

    // 禁止拷贝
    ServerPlayerEntityManager(const ServerPlayerEntityManager&) = delete;
    ServerPlayerEntityManager& operator=(const ServerPlayerEntityManager&) = delete;

    // 允许移动
    ServerPlayerEntityManager(ServerPlayerEntityManager&&) noexcept = default;
    ServerPlayerEntityManager& operator=(ServerPlayerEntityManager&&) noexcept = default;

    // ========== 玩家实体生命周期 ==========

    /**
     * @brief 创建玩家实体并加入世界
     *
     * 此方法执行以下操作：
     * 1. 创建 ServerPlayer 对象（携带 PlayerAdvancements、末影箱回调等服务端特有状态）
     * 2. 设置玩家的 PlayerId
     * 3. 将玩家加入世界的 EntityManager（分配 EntityInstanceId）
     * 4. 注入服务端上下文：setServer、setWorld、setConnection，
     *    使成就触发、网络发包、末影箱自动保存等路径立即可用
     * 5. 将玩家加入 EntityTracker（开始同步）
     * 6. 建立 PlayerId ↔ EntityInstanceId 映射
     *
     * @param playerId 玩家ID（由 PlayerManager 分配）
     * @param username 用户名
     * @param world 目标世界
     * @param server 服务器接口指针（用于 ServerPlayer::setServer）
     * @param connection 网络连接（可为 nullptr，用于 ServerPlayer::setConnection）
     * @param spawnX 生成点 X 坐标
     * @param spawnY 生成点 Y 坐标
     * @param spawnZ 生成点 Z 坐标
     * @return 创建的玩家实体指针，失败返回 nullptr
     *
     * @pre playerId != 0
     * @pre world != nullptr
     * @pre server != nullptr
     */
    Player* createPlayerEntity(PlayerId playerId,
        const std::string& username,
        ServerWorld& world,
        IServer* server,
        mc::server::net::ServerClientConnection* connection,
        f32 spawnX,
        f32 spawnY,
        f32 spawnZ);

    /**
     * @brief 移除玩家实体
     *
     * 此方法执行以下操作：
     * 1. 从 EntityTracker 移除追踪
     * 2. 从 EntityManager 移除实体
     * 3. 清除 PlayerId ↔ EntityInstanceId 映射
     *
     * @param playerId 玩家ID
     * @param world 世界引用
     */
    void removePlayerEntity(PlayerId playerId, ServerWorld& world);

    /**
     * @brief 清除所有玩家实体
     *
     * 在服务器关闭时调用。
     *
     * @param world 世界引用
     */
    void clearAll(ServerWorld& world);

    // ========== 查询 ==========

    /**
     * @brief 获取玩家的 EntityInstanceId
     * @param playerId 玩家ID
     * @return EntityInstanceId，未找到返回 INVALID_ENTITY_ID
     */
    [[nodiscard]] EntityInstanceId getPlayerEntityId(PlayerId playerId) const;

    /**
     * @brief 通过 EntityInstanceId 获取 PlayerId
     * @param entityId 实体ID
     * @return PlayerId，未找到返回 0
     */
    [[nodiscard]] PlayerId getPlayerIdByEntityId(EntityInstanceId entityId) const;

    /**
     * @brief 获取玩家的实体指针
     *
     * 注意：返回的指针可能在下次实体操作后失效，
     * 调用者应立即使用，不要长期持有。
     *
     * @param playerId 玩家ID
     * @param world 世界引用
     * @return Player 指针，未找到返回 nullptr
     */
    [[nodiscard]] Player* getPlayerEntity(PlayerId playerId, ServerWorld& world) const;

    /**
     * @brief 检查玩家是否存在
     * @param playerId 玩家ID
     * @return true 如果玩家存在
     */
    [[nodiscard]] bool hasPlayer(PlayerId playerId) const;

    /**
     * @brief 获取玩家数量
     * @return 当前玩家数量
     */
    [[nodiscard]] size_t playerCount() const;

    /**
     * @brief 获取所有玩家ID列表
     * @return 玩家ID列表
     */
    [[nodiscard]] std::vector<PlayerId> getPlayerIds() const;

private:
    mutable std::mutex m_mutex;

    /// PlayerId → EntityInstanceId 映射
    std::unordered_map<PlayerId, EntityInstanceId> m_playerToEntity;

    /// EntityInstanceId → PlayerId 映射
    std::unordered_map<EntityInstanceId, PlayerId> m_entityToPlayer;
};

} // namespace mc::server
