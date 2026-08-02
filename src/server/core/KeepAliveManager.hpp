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

#include "common/core/Constants.hpp"
#include "common/core/Types.hpp"
#include <vector>

namespace mc::server::core {

// 前向声明
class PlayerManager;

/**
 * @brief 心跳管理器
 *
 * 负责心跳计时、超时检测、ping 计算。
 * 与 PlayerManager 协同工作。
 *
 * 心跳间隔与超时阈值采用 vanilla Java 硬编码值
 * （mc::network::KEEP_ALIVE_INTERVAL_MS=15000ms、KEEP_ALIVE_TIMEOUT_MS=30000ms，
 * 对齐 vanilla Java Connection#KEEP_ALIVE_TIME / ServerGamePacketListenerImpl 滞后判定），
 * 不可配置：原版本就硬编码不可配，移除配置项可彻底杜绝配置残留导致发送间隔/超时脱钩的 bug。
 * 时序基于 wall-clock 毫秒（util::TimeUtils::getCurrentTimeMs），不依赖 TPS/tick 计数，
 * 避免低 TPS 下发送间隔被拉长到超过超时阈值而误踢玩家。
 *
 * 使用示例：
 * @code
 * KeepAliveManager kaMgr(playerManager);
 *
 * // 每个 tick 检查是否需要发送心跳（wall-clock 毫秒）
 * auto players = kaMgr.getPlayersNeedingKeepAlive(currentTickMs);
 * for (auto playerId : players) {
 *     // ... 发送 KeepAlive 包，并调 recordKeepAliveSent
 * }
 *
 * // 处理心跳响应
 * kaMgr.handleKeepAliveResponse(playerId, timestamp, currentTimeMs);
 *
 * // 检查超时（每 tick 调用）
 * auto timeouts = kaMgr.getTimedOutPlayers(currentTickMs);
 * @endcode
 */
class KeepAliveManager {
public:
    /**
     * @brief 构造心跳管理器
     * @param playerManager 玩家管理器引用
     *
     * 心跳间隔/超时采用 vanilla 硬编码常量（见类注释），不可配置。
     */
    explicit KeepAliveManager(PlayerManager& playerManager);

    // ========== 心跳发送 ==========

    /**
     * @brief 检查玩家是否需要发送心跳
     * @param playerId 玩家ID
     * @param currentTickMs 当前时间戳（毫秒）
     * @return true 如果需要发送心跳
     */
    [[nodiscard]] bool needsKeepAlive(PlayerId playerId, u64 currentTickMs) const;

    /**
     * @brief 获取需要发送心跳的玩家列表
     * @param currentTickMs 当前时间戳（毫秒）
     * @return 需要发送心跳的玩家ID列表
     */
    [[nodiscard]] std::vector<PlayerId> getPlayersNeedingKeepAlive(u64 currentTickMs) const;

    /**
     * @brief 记录心跳发送时间
     * @param playerId 玩家ID
     * @param timestamp 发送时间戳（毫秒）
     * @param tick 发送时的 tick
     */
    void recordKeepAliveSent(PlayerId playerId, u64 timestamp, u64 tick);

    // ========== 心跳响应 ==========

    /**
     * @brief 处理心跳响应
     * @param playerId 玩家ID
     * @param timestamp 响应时间戳（与发送时相同）
     * @param currentTimeMs 当前时间戳（毫秒），用于计算 ping
     */
    void handleKeepAliveResponse(PlayerId playerId, u64 timestamp, u64 currentTimeMs);

    /**
     * @brief 更新心跳时间戳（简化版本）
     * @param playerId 玩家ID
     * @param timestamp 接收时间戳
     */
    void updateKeepAlive(PlayerId playerId, u64 timestamp);

    // ========== 超时检测 ==========

    /**
     * @brief 检查玩家是否超时
     * @param playerId 玩家ID
     * @param currentTickMs 当前时间戳（毫秒）
     * @return true 如果玩家超时
     */
    [[nodiscard]] bool isTimedOut(PlayerId playerId, u64 currentTickMs) const;

    /**
     * @brief 获取超时玩家列表
     * @param currentTickMs 当前时间戳（毫秒）
     * @return 超时玩家ID列表
     */
    [[nodiscard]] std::vector<PlayerId> getTimedOutPlayers(u64 currentTickMs) const;

    // ========== 状态查询 ==========

    /**
     * @brief 获取玩家 ping
     * @param playerId 玩家ID
     * @return ping 值（毫秒），如果玩家不存在返回 0
     */
    [[nodiscard]] u32 getPlayerPing(PlayerId playerId) const;

    /**
     * @brief 获取玩家最后发送心跳时间
     * @param playerId 玩家ID
     * @return 时间戳（毫秒），如果玩家不存在返回 0
     */
    [[nodiscard]] u64 getLastKeepAliveSent(PlayerId playerId) const;

    /**
     * @brief 获取玩家最后接收心跳时间
     * @param playerId 玩家ID
     * @return 时间戳（毫秒），如果玩家不存在返回 0
     */
    [[nodiscard]] u64 getLastKeepAliveReceived(PlayerId playerId) const;

private:
    PlayerManager& m_playerManager;
};

} // namespace mc::server::core
