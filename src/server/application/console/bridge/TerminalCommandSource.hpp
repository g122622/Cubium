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

#include "server/application/console/buffer/EventRingBuffer.hpp"
#include "server/command/ServerCommandSource.hpp"

#include <optional>
#include <string>

namespace mc::server::console {

/**
 * @brief 面向终端的控制台命令源
 *
 * 行为与 ServerCommandSource::forConsole 一致（权限 4、无实体、固定位置/维度），
 * 差别只在消息出口：命令的反馈与错误不再写进 spdlog 日志，而是剥离 § 样式码后
 * 作为终端事件进入事件区，避免"回执又变成一行日志"无法区分。
 *
 * 不改公共 ServerCommandSource 的行为：玩家执行命令时的聊天栏着色保持原样。
 */
class TerminalCommandSource : public mc::command::ServerCommandSource {
public:
    /**
     * @brief 创建控制台终端命令源
     * @param server 服务器实例
     * @param events 事件区缓冲（输出目标，须比命令源存活更久）
     */
    TerminalCommandSource(server::IServer* server, EventRingBuffer& events);

    void sendMessage(const std::string& message, const std::optional<Uuid>& senderUuid = std::nullopt) override;
    void sendError(const std::string& message) override;

    /**
     * @brief 非玩家源的输出出口：把回执送入事件区
     *
     * 不用 sendMessage 覆写：命令执行者经 /execute as 换成玩家时，sendMessage 会改为给
     * 玩家发聊天（正确行为），但控制台作为发起者仍应在终端看到一条回执。
     */
    void onConsoleMessage(const std::string& message) override;

    /**
     * @brief 派生命令源的工厂：复制当前上下文，仅替换输出目标
     *
     * 命令内部会通过 withEntity/withPosition 等派生出新命令源再执行（如 /execute as），
     * 若派生出的源退回基类，子命令的反馈就会丢失或重新写进日志。故每个 with* 都覆写为
     * 返回本类型。
     */
    [[nodiscard]] TerminalCommandSource withPlayer(mc::ServerPlayer* player) const;
    [[nodiscard]] TerminalCommandSource withEntity(Entity& entity) const;
    [[nodiscard]] TerminalCommandSource withPosition(const Vector3d& pos) const;
    [[nodiscard]] TerminalCommandSource withRotation(const Vector2f& rot) const;
    [[nodiscard]] TerminalCommandSource withDimension(DimensionId dimensionId) const;
    [[nodiscard]] TerminalCommandSource withFeedbackDisabled() const;
    [[nodiscard]] TerminalCommandSource withSuppressedOutput() const;
    [[nodiscard]] TerminalCommandSource withAnchor(mc::command::EntityAnchorType anchor) const;
    [[nodiscard]] TerminalCommandSource withPermissionLevel(i32 level) const;
    [[nodiscard]] TerminalCommandSource withMaximumPermission(i32 level) const;

private:
    /**
     * @brief 输出一条消息
     * @param message 原始文本（可能带 § 样式码）
     * @param kind 事件分类
     */
    void _emit(const std::string& message, TerminalEventKind kind) const;

    EventRingBuffer* m_events;
};

} // namespace mc::server::console
