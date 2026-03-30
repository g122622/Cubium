#pragma once

#include "../../common/entity/entities/player/Player.hpp"
#include <memory>

namespace mc {

class ServerWorld;
class MinecraftServer;

/**
 * @brief 服务端玩家实体
 *
 * 扩展 Player 类，添加服务端特有的功能：
 * - 连接状态管理
 * - 数据包发送
 * - 服务端权限检查
 */
class ServerPlayer : public Player {
public:
    /**
     * @brief 构造服务端玩家
     * @param id 实体ID
     * @param name 玩家名称
     */
    ServerPlayer(EntityId id, const String& name);
    ~ServerPlayer() override = default;

    // ========== 网络相关 ==========

    /**
     * @brief 发送聊天消息给玩家
     */
    void sendChatMessage(const String& message);

    /**
     * @brief 发送系统消息给玩家
     */
    void sendSystemMessage(const String& message);

    // ========== 世界相关 ==========

    /**
     * @brief 设置所在世界
     */
    void setWorld(ServerWorld* world) { m_world = world; }

    /**
     * @brief 获取所在世界
     */
    [[nodiscard]] ServerWorld* getWorld() const { return m_world; }

    // ========== 连接状态 ==========

    /**
     * @brief 检查玩家是否在线
     */
    [[nodiscard]] bool isOnline() const { return m_online; }

    /**
     * @brief 设置在线状态
     */
    void setOnline(bool online) { m_online = online; }

private:
    ServerWorld* m_world = nullptr;
    bool m_online = true;
};

} // namespace mc
