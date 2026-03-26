#pragma once

#include "ServerPlayerData.hpp"
#include "common/network/packet/Packet.hpp"
#include "common/network/packet/PacketSerializer.hpp"
#include <vector>

namespace mc::server::core {

// 前向声明
class PlayerManager;

/**
 * @brief 连接管理器
 *
 * 负责消息发送、广播、连接断开、数据包封装。
 * 与 PlayerManager 协同工作，提供网络通信功能。
 *
 * 使用示例：
 * @code
 * ConnectionManager connMgr(playerManager);
 * connMgr.broadcastPacket(PacketType::Chat, payload);
 * connMgr.sendToPlayer(playerId, PacketType::Teleport, teleportPayload);
 * @endcode
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
     * @brief 向指定玩家发送原始数据
     * @param playerId 玩家ID
     * @param data 数据指针
     * @param size 数据大小
     * @return true 如果发送成功
     */
    bool sendToPlayer(PlayerId playerId, const u8* data, size_t size);

    /**
     * @brief 向指定玩家发送数据包（封装完整数据包）
     * @param playerId 玩家ID
     * @param type 数据包类型
     * @param payload 数据包负载
     * @return true 如果发送成功
     */
    bool sendPacketToPlayer(PlayerId playerId, network::PacketType type, const std::vector<u8>& payload);

    /**
     * @brief 向指定玩家发送已序列化的数据包
     * @param playerId 玩家ID
     * @param serializedPacket 已封装完整数据包（包含头部）
     * @return true 如果发送成功
     */
    bool sendSerializedPacket(PlayerId playerId, const std::vector<u8>& serializedPacket);

    // ========== 广播 ==========

    /**
     * @brief 向所有玩家广播原始数据
     * @param data 数据指针
     * @param size 数据大小
     */
    void broadcast(const u8* data, size_t size);

    /**
     * @brief 向所有玩家广播数据包
     * @param type 数据包类型
     * @param payload 数据包负载
     */
    void broadcastPacket(network::PacketType type, const std::vector<u8>& payload);

    /**
     * @brief 向除指定玩家外的所有玩家广播原始数据
     * @param excludePlayerId 排除的玩家ID
     * @param data 数据指针
     * @param size 数据大小
     */
    void broadcastExcept(PlayerId excludePlayerId, const u8* data, size_t size);

    /**
     * @brief 向除指定玩家外的所有玩家广播数据包
     * @param excludePlayerId 排除的玩家ID
     * @param type 数据包类型
     * @param payload 数据包负载
     */
    void broadcastPacketExcept(PlayerId excludePlayerId, network::PacketType type, const std::vector<u8>& payload);

    // ========== 连接管理 ==========

    /**
     * @brief 断开玩家连接
     * @param playerId 玩家ID
     * @param reason 断开原因
     */
    void disconnectPlayer(PlayerId playerId, const String& reason = "");

    /**
     * @brief 断开所有玩家连接
     * @param reason 断开原因
     */
    void disconnectAll(const String& reason = "");

    /**
     * @brief 清理已断开连接的玩家
     * @param removedPlayers 输出参数，存放被移除的玩家ID列表
     * @return 清理的玩家数量
     */
    size_t cleanupDisconnectedPlayers(std::vector<PlayerId>* removedPlayers = nullptr);

    // ========== 数据包封装 ==========

    /**
     * @brief 封装完整数据包（包含头部）
     * @param type 数据包类型
     * @param payload 数据包负载
     * @return 封装后的数据包
     */
    [[nodiscard]] static std::vector<u8> encapsulatePacket(network::PacketType type, const std::vector<u8>& payload);

    /**
     * @brief 封装完整数据包到序列化器
     * @param type 数据包类型
     * @param payload 数据包负载
     * @param out 输出序列化器
     */
    static void encapsulatePacket(network::PacketType type, const std::vector<u8>& payload, network::PacketSerializer& out);

private:
    PlayerManager& m_playerManager;
};

} // namespace mc::server::core
