#pragma once

#include "common/core/Types.hpp"
#include "common/network/packet/GameStateChangePacket.hpp"
#include "common/network/packet/PlayerAbilitiesPacket.hpp"
#include <functional>

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
 * - 发送 GameStateChangePacket(ChangeGameMode) 到客户端
 * - 发送 PlayerAbilitiesPacket 到客户端
 * - 广播游戏模式变化给其他玩家（多人模式）
 *
 * 参考 MC 1.16.5 PlayerList / PlayerInteractionManager
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
     * 3. 发送 GameStateChangePacket 到客户端
     * 4. 发送 PlayerAbilitiesPacket 到客户端
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
    [[nodiscard]] GameMode getGameMode(PlayerId playerId) const;

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
    [[nodiscard]] static u8 getAbilitiesForGameMode(GameMode mode);

    // ========== 回调 ==========

    /**
     * @brief 设置游戏模式变化回调
     * @param callback 回调函数
     */
    void setOnGameModeChange(GameModeChangeCallback callback) { m_onGameModeChange = std::move(callback); }

private:
    /**
     * @brief 发送游戏模式变化包
     * @param playerId 目标玩家ID
     * @param mode 新游戏模式
     * @return true 如果发送成功
     */
    bool sendGameModeChangePacket(PlayerId playerId, GameMode mode);

    /**
     * @brief 发送玩家能力包
     * @param playerId 目标玩家ID
     * @param mode 游戏模式（用于确定能力）
     * @return true 如果发送成功
     */
    bool sendAbilitiesPacket(PlayerId playerId, GameMode mode);

    PlayerManager& m_playerManager;
    ConnectionManager& m_connectionManager;
    GameModeChangeCallback m_onGameModeChange;
};

} // namespace mc::server::core
