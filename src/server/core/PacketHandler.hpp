#pragma once

#include "common/core/Result.hpp"
#include "common/network/packet/Packet.hpp"
#include "common/network/connection/IServerConnection.hpp"
#include "ServerCoreConfig.hpp"
#include <functional>

namespace mc::network {
class PacketDeserializer;
class LoginRequestPacket;
class PlayerMovePacket;
class TeleportConfirmPacket;
class KeepAlivePacket;
class ChatMessagePacket;
}

namespace mc::server::core {

// 前向声明
class PlayerManager;
class ConnectionManager;
class TeleportManager;
class KeepAliveManager;
class PositionTracker;
class TimeManager;

/**
 * @brief 数据包处理结果
 */
enum class PacketHandleResult {
    Success,        ///< 处理成功
    Ignore,         ///< 忽略（未登录等）
    Disconnect,     ///< 需要断开连接
    Error           ///< 处理错误
};

/**
 * @brief 登录结果
 */
struct LoginResult {
    bool success = false;
    PlayerId playerId = 0;
    String username;
    String message;
};

/**
 * @brief 数据包处理器
 *
 * 统一处理所有入站数据包，包括登录、移动、心跳、传送确认、聊天消息等。
 * 协调各管理器完成数据包处理逻辑。
 *
 * 使用示例：
 * @code
 * PacketHandler handler(playerMgr, connMgr, tpMgr, kaMgr, posMgr, timeMgr, config);
 *
 * // 处理数据包
 * auto result = handler.handlePacket(sessionId, data, size);
 * if (result == PacketHandleResult::Disconnect) {
 *     // 断开连接
 * }
 * @endcode
 */
class PacketHandler {
public:
    /// 登录成功回调类型
    using LoginCallback = std::function<void(PlayerId, const String&)>;

    /// 登录失败回调类型
    using LoginFailCallback = std::function<void(u32, const String&)>;

    /// 断开连接回调类型
    using DisconnectCallback = std::function<void(PlayerId, const String&)>;

    /// 聊天消息回调类型
    using ChatCallback = std::function<void(PlayerId, const String&, const String&)>;

    /**
     * @brief 构造数据包处理器
     * @param playerManager 玩家管理器引用
     * @param connectionManager 连接管理器引用
     * @param teleportManager 传送管理器引用
     * @param keepAliveManager 心跳管理器引用
     * @param positionTracker 位置追踪器引用
     * @param timeManager 时间管理器引用
     * @param config 配置引用
     */
    PacketHandler(PlayerManager& playerManager,
                  ConnectionManager& connectionManager,
                  TeleportManager& teleportManager,
                  KeepAliveManager& keepAliveManager,
                  PositionTracker& positionTracker,
                  TimeManager& timeManager,
                  const ServerCoreConfig& config);

    // ========== 数据包处理 ==========

    /**
     * @brief 处理数据包
     * @param sessionId 会话ID
     * @param data 数据包数据（包含头部）
     * @param size 数据大小
     * @return 处理结果
     */
    PacketHandleResult handlePacket(u32 sessionId, const u8* data, size_t size);

    /**
     * @brief 处理登录请求
     * @param sessionId 会话ID
     * @param connection 连接接口
     * @param data 数据包负载（不含头部）
     * @param size 负载大小
     * @return 登录结果
     */
    LoginResult handleLoginRequest(u32 sessionId, network::ConnectionPtr connection,
                                    const u8* data, size_t size);

    /**
     * @brief 处理玩家移动
     * @param sessionId 会话ID
     * @param data 数据包负载
     * @param size 负载大小
     * @return 处理结果
     */
    PacketHandleResult handlePlayerMove(u32 sessionId, const u8* data, size_t size);

    /**
     * @brief 处理玩家骑乘输入
     * @param sessionId 会话ID
     * @param data 数据包负载
     * @param size 负载大小
     * @return 处理结果
     */
    PacketHandleResult handlePlayerInput(u32 sessionId, const u8* data, size_t size);

    /**
     * @brief 处理载具移动同步
     * @param sessionId 会话ID
     * @param data 数据包负载
     * @param size 负载大小
     * @return 处理结果
     */
    PacketHandleResult handleMoveVehicle(u32 sessionId, const u8* data, size_t size);

    /**
     * @brief 处理实体动作
     * @param sessionId 会话ID
     * @param data 数据包负载
     * @param size 负载大小
     * @return 处理结果
     */
    PacketHandleResult handleEntityAction(u32 sessionId, const u8* data, size_t size);

    /**
     * @brief 处理传送确认
     * @param sessionId 会话ID
     * @param data 数据包负载
     * @param size 负载大小
     * @return 处理结果
     */
    PacketHandleResult handleTeleportConfirm(u32 sessionId, const u8* data, size_t size);

    /**
     * @brief 处理心跳响应
     * @param sessionId 会话ID
        * @param data 完整数据包（包含头部）
        * @param size 数据包大小
     * @param currentTimeMs 当前时间戳（毫秒）
     * @return 处理结果
     */
    PacketHandleResult handleKeepAlive(u32 sessionId, const u8* data, size_t size, u64 currentTimeMs);

    /**
     * @brief 处理聊天消息
     * @param sessionId 会话ID
     * @param data 数据包负载
     * @param size 负载大小
     * @return 处理结果
     */
    PacketHandleResult handleChatMessage(u32 sessionId, const u8* data, size_t size);

    // ========== 回调设置 ==========

    /**
     * @brief 设置登录成功回调
     */
    void setOnLoginSuccess(LoginCallback callback) { m_onLoginSuccess = std::move(callback); }

    /**
     * @brief 设置登录失败回调
     */
    void setOnLoginFail(LoginFailCallback callback) { m_onLoginFail = std::move(callback); }

    /**
     * @brief 设置断开连接回调
     */
    void setOnDisconnect(DisconnectCallback callback) { m_onDisconnect = std::move(callback); }

    /**
     * @brief 设置聊天消息回调
     */
    void setOnChat(ChatCallback callback) { m_onChat = std::move(callback); }

private:
    PlayerManager& m_playerManager;
    ConnectionManager& m_connectionManager;
    TeleportManager& m_teleportManager;
    KeepAliveManager& m_keepAliveManager;
    PositionTracker& m_positionTracker;
    TimeManager& m_timeManager;
    const ServerCoreConfig& m_config;

    // 回调
    LoginCallback m_onLoginSuccess;
    LoginFailCallback m_onLoginFail;
    DisconnectCallback m_onDisconnect;
    ChatCallback m_onChat;
};

} // namespace mc::server::core
