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

#include "common/command/CommandDispatcher.hpp"
#include "common/command/CommandSource.hpp"
#include "common/command/ICommandSource.hpp"
#include "common/core/Types.hpp"
#include "common/util/math/Vector2.hpp"
#include "common/util/math/Vector3.hpp"
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace mc {

// 前向声明
class ServerPlayer;
class Entity;
namespace server {
class ServerWorld;
class IServer;
} // namespace server

namespace command {

/**
 * @brief 服务端命令源
 *
 * 扩展 CommandSource，提供服务端特有的功能：
 * - 玩家在线检查
 * - 世界访问
 * - 服务器实例访问
 */
class ServerCommandSource : public ICommandSource {
public:
    /**
     * @brief 构造服务端命令源
     * @param server 服务器实例
     * @param player 玩家实例（可选，控制台时为空）
     * @param dimensionId 维度ID
     * @param position 执行位置
     * @param rotation 朝向
     * @param permissionLevel 权限等级 (0-4)
     * @param playerId 玩家逻辑ID（仅当 player 为空时使用）
     * @param playerName 玩家名称（仅当 player 为空时使用）
     * @param entity 执行实体（可选，控制台/命令方块时为空）
     *
     * entity 字段代表命令的执行实体，
     * 可以是任何 Entity（玩家、僵尸、矿车等），也可以为 nullptr（控制台/命令方块/RCON）。
     * 当 player 非空时，entity 应与 player 一致（或由调用者显式设置为 player）。
     */
    ServerCommandSource(server::IServer* server,
        ServerPlayer* player = nullptr,
        DimensionId dimensionId = 0,
        const Vector3d& position = Vector3d(0, 0, 0),
        const Vector2f& rotation = Vector2f(0, 0),
        i32 permissionLevel = 0,
        PlayerId playerId = 0,
        std::string playerName = "",
        Entity* entity = nullptr);

    // ========== ICommandSource 接口实现 ==========

    void sendMessage(const std::string& message, const std::optional<Uuid>& senderUuid = std::nullopt) override;

    /**
     * @brief 发送错误消息
     * @param message 错误消息内容
     *
     * 错误消息会以红色显示（对于支持的客户端）
     */
    void sendError(const std::string& message) override;

    bool shouldReceiveFeedback() const noexcept override;
    bool shouldReceiveErrors() const noexcept override;
    bool allowLogging() const noexcept override;

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
     * @brief 获取执行实体
     * @return 执行实体指针。
     *         当命令由玩家执行时返回该玩家实体；由控制台/命令方块执行时返回 nullptr；
     *         通过 /execute as @e 切换后可返回非玩家实体。
     */
    [[nodiscard]] Entity* entity() const noexcept { return m_entity; }

    /**
     * @brief 获取执行实体，如果为空则抛出异常
     * @return 执行实体引用
     * @throws CommandException 如果没有关联实体（如控制台执行命令）
     */
    [[nodiscard]] Entity& entityOrException() const;

    /**
     * @brief 获取逻辑玩家ID
     */
    [[nodiscard]] PlayerId playerId() const noexcept { return m_playerId; }

    /**
     * @brief 获取世界实例
     */
    [[nodiscard]] server::ServerWorld* world() const noexcept;

    /**
     * @brief 获取维度ID
     */
    [[nodiscard]] DimensionId dimensionId() const noexcept { return m_dimensionId; }

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
    [[nodiscard]] bool hasPermission(i32 level) const noexcept { return m_permissionLevel >= level; }

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
     *
     * 同时更新 entity 为该玩家。
     */
    [[nodiscard]] ServerCommandSource withPlayer(ServerPlayer* player) const;

    /**
     * @brief 创建以指定实体为源的新命令源
     *
     * 替换执行实体和显示名称，
     * 但保留位置、旋转、维度等其他字段不变。
     * 如果传入的实体是 ServerPlayer，则同时更新 m_player 和 m_playerId。
     *
     * @param entity 新的执行实体（必须非空）
     */
    [[nodiscard]] ServerCommandSource withEntity(Entity& entity) const;

    /**
     * @brief 创建指定位置的新命令源
     */
    [[nodiscard]] ServerCommandSource withPosition(const Vector3d& pos) const;

    /**
     * @brief 创建指定朝向的新命令源
     */
    [[nodiscard]] ServerCommandSource withRotation(const Vector2f& rot) const;

    /**
     * @brief 创建指定维度的新命令源
     */
    [[nodiscard]] ServerCommandSource withDimension(DimensionId dimensionId) const;

    /**
     * @brief 创建禁用反馈的新命令源
     */
    [[nodiscard]] ServerCommandSource withFeedbackDisabled() const;

    /**
     * @brief 创建抑制输出的新命令源
     */
    [[nodiscard]] ServerCommandSource withSuppressedOutput() const;

    /**
     * @brief 创建指定锚点类型的新命令源
     */
    [[nodiscard]] ServerCommandSource withAnchor(EntityAnchorType anchor) const;

    /**
     * @brief 创建指定权限等级的新命令源
     */
    [[nodiscard]] ServerCommandSource withPermissionLevel(i32 level) const;

    /**
     * @brief 创建权限不低于指定值的新命令源
     */
    [[nodiscard]] ServerCommandSource withMaximumPermission(i32 level) const;

    // ========== 锚点 ==========

    /**
     * @brief 获取实体锚点类型
     */
    [[nodiscard]] EntityAnchorType anchor() const noexcept { return m_anchor; }

    // ========== 反馈控制 ==========

    [[nodiscard]] bool isFeedbackDisabled() const noexcept { return m_feedbackDisabled; }
    void setFeedbackDisabled(bool disabled) noexcept { m_feedbackDisabled = disabled; }

    // ========== 静态工厂方法 ==========

    /**
     * @brief 创建控制台命令源
     */
    static ServerCommandSource forConsole(server::IServer* server);

private:
    server::IServer* m_server;
    ServerPlayer* m_player; ///< 关联的玩家指针（控制台时为 nullptr）
    Entity* m_entity;       ///< 执行实体。玩家执行时与 m_player 一致，
                            ///< 控制台/命令方块时为 nullptr，/execute as @e 后可为非玩家实体。
    PlayerId m_playerId;
    DimensionId m_dimensionId;
    Vector3d m_position;
    Vector2f m_rotation;
    i32 m_permissionLevel;
    std::string m_name;
    bool m_feedbackDisabled;
    EntityAnchorType m_anchor = EntityAnchorType::Feet;
};

} // namespace command
} // namespace mc
