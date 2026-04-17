#pragma once

#include "../../common/entity/entities/player/Player.hpp"
#include "../../common/network/connection/IServerConnection.hpp"
#include "../../common/network/packet/ExperiencePackets.hpp"
#include <memory>
#include <utility>
#include <vector>

namespace mc {

class ServerWorld;

/**
 * @brief 服务端玩家实体。
 *
 * 扩展 Player 类，添加服务端特有的网络同步与在线状态管理能力。
 */
class ServerPlayer : public Player {
public:
    /**
     * @brief 构造服务端玩家。
     * @param id 实体ID。
     * @param name 玩家名称。
     */
    ServerPlayer(EntityId id, const String& name);
    ~ServerPlayer() override = default;

    // ========== 网络相关 ==========

    /**
     * @brief 发送聊天消息给玩家。
     * @param message 聊天内容。
     */
    void sendChatMessage(const String& message);

    /**
     * @brief 发送系统消息给玩家。
     * @param message 系统消息内容。
     */
    void sendSystemMessage(const String& message);

    /**
     * @brief 同步经验状态到客户端。
     * @note 仅在连接可用时发送网络包。
     */
    void syncExperience();

    // ========== 重写经验方法 ==========

    /**
     * @brief 添加经验并同步到客户端。
     * @param amount 增加的经验值。
     */
    void addExperience(i32 amount) override;

    /**
     * @brief 设置经验等级并同步到客户端。
     * @param level 目标等级。
     */
    void setExperienceLevel(i32 level) override;

    /**
     * @brief 绑定网络连接。
     * @param connection 玩家连接（可为 nullptr）。
     */
    void setConnection(network::ConnectionPtr connection) { m_connection = std::move(connection); }

    /**
     * @brief 获取网络连接。
     * @return 网络连接共享指针的常量引用。
     * @note 返回值可能为 nullptr，调用方需结合 hasConnection() 使用。
     */
    [[nodiscard]] const network::ConnectionPtr& connection() const { return m_connection; }

    /**
     * @brief 检查网络连接是否可用。
     * @return true 表示连接存在且仍处于连接状态。
     */
    [[nodiscard]] bool hasConnection() const { return m_connection && m_connection->isConnected(); }

    // ========== 世界相关 ==========

    /**
     * @brief 设置所在世界。
     * @param world 世界指针。
     */
    void setWorld(ServerWorld* world) { m_world = world; }

    /**
     * @brief 获取所在世界。
     * @return 当前所在世界指针。
     */
    [[nodiscard]] ServerWorld* getWorld() const { return m_world; }

    // ========== 连接状态 ==========

    /**
     * @brief 检查玩家是否在线。
     * @return true 表示在线。
     */
    [[nodiscard]] bool isOnline() const { return m_online; }

    /**
     * @brief 设置在线状态。
     * @param online 新的在线状态。
     */
    void setOnline(bool online) { m_online = online; }

private:
    /**
     * @brief 发送完整封包到当前玩家连接。
     * @param packet 已包含协议头的完整封包字节流。
     * @return true 表示已成功投递到底层连接。
     * @note 当玩家连接不存在或已断开时返回 false，不抛出异常。
     */
    [[nodiscard]] bool sendFullPacket(const std::vector<u8>& packet) const;

private:
    network::ConnectionPtr m_connection;
    ServerWorld* m_world = nullptr;
    bool m_online = true;
};

} // namespace mc