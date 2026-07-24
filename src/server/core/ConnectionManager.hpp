/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
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

#include "ServerPlayerData.hpp"
#include "common/network/ir/IrPacket.hpp"
#include <string>
#include <vector>

namespace mc::server::core {

// 前向声明
class PlayerManager;

/**
 * @brief 连接管理器（新网络层 IR 版本）
 *
 * 新网络层（1.21.11 IR）下的发送/广播门面。原 12 字节头封装已删除：
 * 游戏逻辑直接构造 ir::IrPacket 交由本类，经各 ServerPlayerData::send →
 * ServerClientConnection::send 出站。
 *
 * 本类是过渡期保留的薄门面：所有发送最终委托给 ServerPlayerData::send(ir::IrPacket)。
 * Step5 删旧体系后，调用方可逐步直接使用 IServer::broadcastPacket/sendPacketToPlayer。
 */
class ConnectionManager {
public:
    /**
     * @brief 构造连接管理器
     * @param playerManager 玩家管理器引用
     */
    explicit ConnectionManager(PlayerManager& playerManager);

    // ========== 发送数据 ==========

    /**
     * @brief 向指定玩家发送 IR 包
     * @param playerId 玩家ID
     * @param packet IR 包
     * @return true 如果发送成功
     */
    bool sendToPlayer(PlayerId playerId, const mc::network::ir::IrPacket& packet);

    // ========== 广播 ==========

    /**
     * @brief 向所有在线玩家广播 IR 包
     * @param packet IR 包
     */
    void broadcast(const mc::network::ir::IrPacket& packet);

    /**
     * @brief 向除指定玩家外的所有在线玩家广播 IR 包
     * @param excludePlayerId 排除的玩家ID
     * @param packet IR 包
     */
    void broadcastExcept(PlayerId excludePlayerId, const mc::network::ir::IrPacket& packet);

    // ========== 连接管理 ==========

    /**
     * @brief 断开玩家连接
     * @param playerId 玩家ID
     * @param reason 断开原因（仅用于日志）
     */
    void disconnectPlayer(PlayerId playerId, const std::string& reason = "");

    /**
     * @brief 断开所有玩家连接
     * @param reason 断开原因（仅用于日志）
     */
    void disconnectAll(const std::string& reason = "");

    /**
     * @brief 清理已断开连接的玩家
     * @param removedPlayers 输出参数，存放被移除的玩家ID列表
     * @return 清理的玩家数量
     */
    size_t cleanupDisconnectedPlayers(std::vector<PlayerId>* removedPlayers = nullptr);

private:
    PlayerManager& m_playerManager;
};

} // namespace mc::server::core
