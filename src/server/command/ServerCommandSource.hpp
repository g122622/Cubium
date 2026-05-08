#pragma once

#include "common/command/CommandSource.hpp"
#include "common/command/CommandDispatcher.hpp"
#include "common/core/Types.hpp"
#include <memory>
#include <vector>
#include <functional>

namespace mc {

// 前向声明
class ServerPlayer;
namespace server {
class ServerWorld;
class IServer;
}

namespace command {

/**
 * @brief 服务端命令源
 *
 * 扩展 CommandSource，提供服务端特有的功能：
 * - 玩家在线检查
 * - 世界访问
 * - 服务器实例访问
 *
 * 参考 MC 的 ServerCommandSource 类
 */
class ServerCommandSource : public ICommandSource {
public:
    /**
     * @brief 构造服务端命令源
     * @param server 服务器实例
     * @param player 玩家实例（可选，控制台时为空）
     * @param world 世界实例（可选）
     * @param position 执行位置
     * @param rotation 朝向
     * @param permissionLevel 权限等级 (0-4)
     */
    ServerCommandSource(
        server::IServer* server,
        ServerPlayer* player = nullptr,
        server::ServerWorld* world = nullptr,
        const Vector3d& position = Vector3d(0, 0, 0),
        const Vector2f& rotation = Vector2f(0, 0),
        i32 permissionLevel = 0,
        PlayerId playerId = 0,
        std::string playerName = ""
    );

    // ========== ICommandSource 接口实现 ==========

    void sendMessage(const std::string& message,
                    const std::optional<Uuid>& senderUuid = std::nullopt) override;

    /**
     * @brief 发送错误消息
     * @param message 错误消息内容
     *
     * 错误消息会以红色显示（对于支持的客户端）
     */
    void sendError(const std::string& message);

    bool shouldReceiveFeedback() const override;
    bool shouldReceiveErrors() const override;
    bool allowLogging() const override;

    // ========== 服务器访问 ==========

    /**
     * @brief 获取服务器实例
     */
    [[nodiscard]] server::IServer* server() const noexcept { return m_server; }

    /**
     * @brief 获取玩家实例
     * @return 玩家指针，如果不是玩家则返回 nullptr
     */
    [[nodiscard]] ServerPlayer* player() const noexcept { return m_player; }

    /**
     * @brief 获取逻辑玩家ID
     */
    [[nodiscard]] PlayerId playerId() const noexcept { return m_playerId; }

    /**
     * @brief 获取世界实例
     */
    [[nodiscard]] server::ServerWorld* world() const noexcept { return m_world; }

    // ========== 位置和朝向 ==========

    [[nodiscard]] const Vector3d& position() const noexcept { return m_position; }
    [[nodiscard]] const Vector2f& rotation() const noexcept { return m_rotation; }

    // ========== 权限 ==========

    [[nodiscard]] i32 permissionLevel() const noexcept { return m_permissionLevel; }

    /**
     * @brief 检查是否有指定权限等级
     * @param level 要求的权限等级
     * @return true 如果有足够权限
     */
    [[nodiscard]] bool hasPermission(i32 level) const noexcept {
        return m_permissionLevel >= level;
    }

    // ========== 显示名称 ==========

    /**
     * @brief 获取显示名称
     */
    [[nodiscard]] const std::string& name() const noexcept { return m_name; }

    // ========== 实体检查 ==========

    /**
     * @brief 是否是玩家
     */
    [[nodiscard]] bool isPlayer() const noexcept { return m_player != nullptr || m_playerId != 0; }

    /**
     * @brief 断言是玩家
     * @throws CommandException 如果不是玩家
     */
    [[nodiscard]] ServerPlayer& assertPlayer() const;

    // ========== 派生命令源 ==========

    /**
     * @brief 创建以指定玩家为源的新命令源
     */
    [[nodiscard]] ServerCommandSource withPlayer(ServerPlayer* player) const;

    /**
     * @brief 创建指定位置的新命令源
     */
    [[nodiscard]] ServerCommandSource withPosition(const Vector3d& pos) const;

    /**
     * @brief 创建指定朝向的新命令源
     */
    [[nodiscard]] ServerCommandSource withRotation(const Vector2f& rot) const;

    /**
     * @brief 创建指定世界的新命令源
     */
    [[nodiscard]] ServerCommandSource withWorld(server::ServerWorld* world) const;

    /**
     * @brief 创建禁用反馈的新命令源
     */
    [[nodiscard]] ServerCommandSource withFeedbackDisabled() const;

    /**
     * @brief 创建抑制输出的新命令源
     */
    [[nodiscard]] ServerCommandSource withSuppressedOutput() const;

    /**
     * @brief 创建指定权限等级的新命令源
     */
    [[nodiscard]] ServerCommandSource withPermissionLevel(i32 level) const;

    /**
     * @brief 创建权限不低于指定值的新命令源
     */
    [[nodiscard]] ServerCommandSource withMaximumPermission(i32 level) const;

    // ========== 反馈控制 ==========

    [[nodiscard]] bool isFeedbackDisabled() const noexcept { return m_feedbackDisabled; }
    void setFeedbackDisabled(bool disabled) { m_feedbackDisabled = disabled; }

    // ========== 静态工厂方法 ==========

    /**
     * @brief 创建控制台命令源
     */
    static ServerCommandSource forConsole(server::IServer* server);

private:
    server::IServer* m_server;
    ServerPlayer* m_player;
    PlayerId m_playerId;
    server::ServerWorld* m_world;
    Vector3d m_position;
    Vector2f m_rotation;
    i32 m_permissionLevel;
    std::string m_name;
    bool m_feedbackDisabled;
};

} // namespace command
} // namespace mc
