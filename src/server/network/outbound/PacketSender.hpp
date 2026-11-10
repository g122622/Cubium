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

#include "common/core/Types.hpp"
#include "common/network/ir/IrPacket.hpp"
#include "server/player/runtime/ServerPlayerData.hpp"
#include <cstddef>
#include <string>
#include <vector>

namespace mc::server {

// 前向声明
class PlayerManager;

/**
 * @brief 面向玩家的 IR 包投递入口
 *
 * 单播和共享包广播通过玩家连接投递，不负责关闭连接或移除玩家记录。
 */
class PacketSender {
public:
    /**
     * @brief 构造包投递服务
     * @param playerManager 玩家管理器引用
     */
    explicit PacketSender(PlayerManager& playerManager);

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

private:
    PlayerManager& m_playerManager;
};

} // namespace mc::server
