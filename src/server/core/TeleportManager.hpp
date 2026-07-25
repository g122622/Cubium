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
#include <atomic>
#include <vector>

namespace mc::server::core {

// 前向声明
class PlayerManager;
class ConnectionManager;

/**
 * @brief 传送管理器
 *
 * 负责玩家传送请求、确认、ID 生成。
 * 与 PlayerManager 和 ConnectionManager 协同工作。
 *
 * 使用示例：
 * @code
 * TeleportManager tpMgr(playerManager, connManager);
 * u32 teleportId = tpMgr.requestTeleport(playerId, 100.0, 64.0, 200.0);
 * // 等待客户端确认
 * bool confirmed = tpMgr.confirmTeleport(playerId, teleportId);
 * @endcode
 */
class TeleportManager {
public:
    /**
     * @brief 构造传送管理器
     * @param playerManager 玩家管理器引用
     */
    explicit TeleportManager(PlayerManager& playerManager);

    // ========== 传送请求 ==========

    /**
     * @brief 请求传送玩家
     * @param playerId 玩家ID
     * @param x 目标X坐标
     * @param y 目标Y坐标
     * @param z 目标Z坐标
     * @param yaw 偏航角（默认0）
     * @param pitch 俯仰角（默认0）
     * @return 传送ID，如果玩家不存在返回0
     */
    u32 requestTeleport(PlayerId playerId, f64 x, f64 y, f64 z, f32 yaw = 0.0f, f32 pitch = 0.0f);

    /**
     * @brief 确认传送
     * @param playerId 玩家ID
     * @param teleportId 传送ID
     * @return true 如果确认成功
     */
    bool confirmTeleport(PlayerId playerId, u32 teleportId);

    // ========== 状态查询 ==========

    /**
     * @brief 检查玩家是否在等待传送确认
     * @param playerId 玩家ID
     * @return true 如果玩家在等待传送确认
     */
    [[nodiscard]] bool isWaitingForConfirm(PlayerId playerId) const;

    /**
     * @brief 获取玩家当前的传送ID
     * @param playerId 玩家ID
     * @return 传送ID，如果没有返回0
     */
    [[nodiscard]] u32 getPendingTeleportId(PlayerId playerId) const;

private:
    PlayerManager& m_playerManager;
    std::atomic<u32> m_nextTeleportId{1};
};

} // namespace mc::server::core
