#pragma once

#include "common/command/ICommandSource.hpp"
#include "common/command/exceptions/CommandExceptions.hpp"
#include "common/core/Types.hpp"
#include "common/util/math/Vector2.hpp"
#include "common/util/math/Vector3.hpp"
#include <functional>
#include <memory>
#include <optional>
#include <string>

namespace mc {

// 前向声明
class Entity;
class Player;
class ServerPlayer;
class MinecraftServer;
namespace server {
class ServerWorld;
}

namespace command {

class ICommandSource;

/**
 * @brief 实体锚点类型
 *
 * 用于确定实体的参考点（眼睛或脚）
 */
enum class EntityAnchorType {
    Feet, // 脚部
    Eyes  // 眼睛
};

/**
 * @brief 命令源
 *
 * 包含命令执行的完整上下文信息：
 * - 执行者（玩家/控制台/命令方块）
 * - 位置和朝向
 * - 所在世界
 * - 权限等级
 *
 * 参考 MC 的 CommandSource 类设计
 */
class CommandSource {
public:
    /**
     * @brief 构造命令源
     * @param source 底层命令源接口
     * @param position 执行位置
     * @param rotation 朝向（yaw, pitch）
     * @param world 所在世界（可选）
     * @param permissionLevel 权限等级 (0-4)
     * @param name 显示名称
     * @param server 服务器实例（可选）
     * @param entity 关联实体（可选）
     */
    CommandSource(std::shared_ptr<ICommandSource> source,
        const Vector3d& position,
        const Vector2f& rotation,
        server::ServerWorld* world,
        i32 permissionLevel,
        const std::string& name,
        class MinecraftServer* server,
        Entity* entity = nullptr);

    // ========== 静态工厂方法 ==========

    /**
     * @brief 创建控制台命令源
     */
    static CommandSource forConsole(class MinecraftServer* server);

    // ========== 位置和朝向 ==========

    [[nodiscard]] const Vector3d& position() const noexcept { return m_position; }
    [[nodiscard]] const Vector2f& rotation() const noexcept { return m_rotation; }
    [[nodiscard]] server::ServerWorld* world() const noexcept { return m_world; }

    // ========== 实体信息 ==========

    [[nodiscard]] Entity* entity() const noexcept { return m_entity; }
    [[nodiscard]] const std::string& name() const noexcept { return m_name; }

    /**
     * @brief 获取玩家实体
     * @return 玩家指针，如果不是玩家则返回 nullptr
     */
    [[nodiscard]] ServerPlayer* player() const;

    /**
     * @brief 断言执行者是实体
     * @throws CommandException 如果不是实体
     */
    [[nodiscard]] Entity& assertEntity() const;

    /**
     * @brief 断言执行者是玩家
     * @throws CommandException 如果不是玩家
     */
    [[nodiscard]] ServerPlayer& assertPlayer() const;

    // ========== 权限 ==========

    [[nodiscard]] i32 permissionLevel() const noexcept { return m_permissionLevel; }

    /**
     * @brief 检查是否有指定权限等级
     */
    [[nodiscard]] bool hasPermission(i32 level) const noexcept { return m_permissionLevel >= level; }

    // ========== 服务器 ==========

    [[nodiscard]] MinecraftServer* server() const noexcept { return m_server; }

    // ========== 消息发送 ==========

    /**
     * @brief 发送成功反馈
     * @param message 消息
     * @param logToConsole 是否记录到控制台
     */
    void sendFeedback(const std::string& message, bool logToConsole = true) const;

    /**
     * @brief 发送错误消息
     */
    void sendError(const std::string& message) const;

    // ========== 派生命令源 ==========

    /**
     * @brief 创建以指定实体为源的新命令源
     */
    [[nodiscard]] CommandSource withEntity(Entity& entity) const;

    /**
     * @brief 创建指定位置的新命令源
     */
    [[nodiscard]] CommandSource withPosition(const Vector3d& pos) const;

    /**
     * @brief 创建指定朝向的新命令源
     */
    [[nodiscard]] CommandSource withRotation(const Vector2f& rot) const;

    /**
     * @brief 创建指定世界的新命令源
     */
    [[nodiscard]] CommandSource withWorld(server::ServerWorld* world) const;

    /**
     * @brief 创建禁用反馈的新命令源
     */
    [[nodiscard]] CommandSource withFeedbackDisabled() const;

    /**
     * @brief 创建指定权限等级的新命令源
     */
    [[nodiscard]] CommandSource withPermissionLevel(i32 level) const;

    /**
     * @brief 创建指定锚点类型的新命令源
     */
    [[nodiscard]] CommandSource withAnchor(EntityAnchorType anchor) const;

    // ========== 状态 ==========

    [[nodiscard]] bool isFeedbackDisabled() const noexcept { return m_feedbackDisabled; }
    [[nodiscard]] EntityAnchorType anchor() const noexcept { return m_anchor; }

private:
    std::shared_ptr<ICommandSource> m_source;
    Vector3d m_position;
    Vector2f m_rotation; // (yaw, pitch)
    server::ServerWorld* m_world;
    i32 m_permissionLevel;
    std::string m_name;
    MinecraftServer* m_server;
    Entity* m_entity;
    bool m_feedbackDisabled;
    EntityAnchorType m_anchor;
};

// ========== 静态异常定义 ==========

class CommandExceptions {
public:
    static CommandException requiresPlayer()
    {
        return CommandException(CommandErrorType::PermissionDenied, "commands.requires.player");
    }

    static CommandException requiresEntity()
    {
        return CommandException(CommandErrorType::PermissionDenied, "commands.requires.entity");
    }

    static CommandException permissionDenied([[maybe_unused]] i32 required)
    {
        return CommandException(CommandErrorType::PermissionDenied, "commands.permission.denied");
    }
};

} // namespace command
} // namespace mc
