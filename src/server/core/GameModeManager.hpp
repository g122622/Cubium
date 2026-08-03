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
#include "common/network/protocol/GameActions.hpp"
#include <functional>
#include <utility>

namespace mc::server::core {

// 前向声明
class PlayerManager;
class ConnectionManager;

/**
 * @brief 游戏模式管理器
 *
 * 集中管理玩家游戏模式的切换和网络同步。
 *
 * 职责:
 * - 设置玩家游戏模式并更新能力
 * - 发送 ir::play::GameStateChange(ChangeGameMode) 到客户端
 * - 发送 ir::play::PlayerAbilities 到客户端
 * - 广播游戏模式变化给其他玩家（多人模式）
 *
 * 使用示例：
 * @code
 * GameModeManager gmMgr(playerManager, connectionManager);
 * gmMgr.setGameMode(playerId, GameMode::Creative);
 * // 会自动更新玩家状态并发送网络包
 * @endcode
 */
class GameModeManager {
public:
    /**
     * @brief 游戏模式变化回调类型
     * @param playerId 玩家ID
     * @param oldMode 旧游戏模式
     * @param newMode 新游戏模式
     */
    using GameModeChangeCallback = std::function<void(PlayerId, GameMode, GameMode)>;

    /**
     * @brief 构造游戏模式管理器
     * @param playerManager 玩家管理器引用
     * @param connectionManager 连接管理器引用
     */
    GameModeManager(PlayerManager& playerManager, ConnectionManager& connectionManager);

    // ========== 游戏模式管理 ==========

    /**
     * @brief 设置玩家游戏模式（带网络同步）
     *
     * 此方法会：
     * 1. 更新玩家数据的游戏模式
     * 2. 更新玩家能力（飞行、无敌等）
     * 3. 发送 ir::play::GameStateChange 到客户端
     * 4. 发送 ir::play::PlayerAbilities 到客户端
     * 5. 调用注册的回调
     *
     * @param playerId 玩家ID
     * @param mode 新游戏模式
     * @return true 如果设置成功
     */
    bool setGameMode(PlayerId playerId, GameMode mode);

    /**
     * @brief 仅设置本地游戏模式（不发送网络包）
     *
     * 用于玩家登录时设置初始模式。
     * 仅更新玩家数据中的游戏模式字段。
     *
     * @param playerId 玩家ID
     * @param mode 新游戏模式
     * @return true 如果设置成功
     */
    bool setGameModeLocal(PlayerId playerId, GameMode mode);

    /**
     * @brief 获取玩家游戏模式
     * @param playerId 玩家ID
     * @return 游戏模式，如果玩家不存在返回 NotSet
     */
    [[nodiscard]] GameMode getGameMode(PlayerId playerId) const noexcept;

    // ========== 能力同步 ==========

    /**
     * @brief 同步玩家能力到客户端
     *
     * 发送 PlayerAbilitiesPacket 到指定玩家。
     * 用于玩家登录后同步初始能力。
     *
     * @param playerId 玩家ID
     * @return true 如果发送成功
     */
    bool syncAbilities(PlayerId playerId);

    /**
     * @brief 根据游戏模式获取默认能力
     * @param mode 游戏模式
     * @return 能力标志位
     */
    [[nodiscard]] static u8 getAbilitiesForGameMode(GameMode mode) noexcept;

    // ========== 回调 ==========

    /**
     * @brief 设置游戏模式变化回调
     * @param callback 回调函数
     */
    void setOnGameModeChange(GameModeChangeCallback callback) noexcept { m_onGameModeChange = std::move(callback); }

private:
    /**
     * @brief 发送游戏模式变化包
     * @param playerId 目标玩家ID
     * @param mode 新游戏模式
     * @return true 如果发送成功
     */
    bool _sendGameModeChangePacket(PlayerId playerId, GameMode mode);

    /**
     * @brief 发送玩家能力包
     * @param playerId 目标玩家ID
     * @param mode 游戏模式（用于确定能力）
     * @return true 如果发送成功
     */
    bool _sendAbilitiesPacket(PlayerId playerId, GameMode mode);

    PlayerManager& m_playerManager;
    ConnectionManager& m_connectionManager;
    GameModeChangeCallback m_onGameModeChange;
};

} // namespace mc::server::core
