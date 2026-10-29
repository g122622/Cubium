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

#include "TerminalCommandSource.hpp"

#include "common/util/assert/AssertAll.hpp"
#include "server/player/ServerPlayer.hpp"

#include <algorithm>
#include <utility>

namespace mc::server::console {

namespace {

/// 控制台权限等级：与原版控制台/RCON 一致（可执行全部命令）。
constexpr i32 CONSOLE_PERMISSION_LEVEL = 4;

/// 控制台命令源显示名。
constexpr const char* CONSOLE_NAME = "Console";

} // namespace

TerminalCommandSource::TerminalCommandSource(server::IServer* server, EventRingBuffer& events)
    : mc::command::ServerCommandSource(server,
          nullptr, // 无玩家：与原版控制台/RCON 一致
          nullptr, // 无执行实体
          0,       // 无 playerId
          0,       // 主世界
          Vector3d(0, 0, 0),
          Vector2f(0, 0),
          CONSOLE_PERMISSION_LEVEL,
          CONSOLE_NAME,
          false, // 反馈开启：控制台命令的回执需要显示在终端
          mc::command::EntityAnchorType::Feet)
    , m_events(&events)
{}

void TerminalCommandSource::sendMessage(const std::string& message, const std::optional<Uuid>& senderUuid)
{
    // 玩家反馈（/execute as 切换执行者后的情形）照常送到玩家；控制台自身的回执由
    // onConsoleMessage 负责，避免重复记录。
    mc::command::ServerCommandSource::sendMessage(message, senderUuid);
}

void TerminalCommandSource::sendError(const std::string& message)
{
    // 基类会给错误加 §c 前缀；终端不需要样式码，_emit 统一剥离。
    _emit(message, TerminalEventKind::Error);
}

void TerminalCommandSource::onConsoleMessage(const std::string& message)
{
    // 基类对错误消息加 §c 前缀，据此区分错误与普通回执。
    const bool isError = message.rfind("§c", 0) == 0;
    _emit(message, isError ? TerminalEventKind::Error : TerminalEventKind::Command);
}

void TerminalCommandSource::_emit(const std::string& message, TerminalEventKind kind) const
{
    std::string text = stripFormattingCodes(message);
    if (text.empty()) {
        return;
    }
    m_events->push(kind, std::move(text));
}

// 以下派生方法保持"终端命令源"类型不变，理由见头文件。每个都只改单一字段，
// 与 ServerCommandSource 的同名实现逐字段对齐。

TerminalCommandSource TerminalCommandSource::withPlayer(mc::ServerPlayer* player) const
{
    TerminalCommandSource source(*this);
    source.m_player = player;
    source.m_entity = static_cast<Entity*>(player);
    if (player != nullptr) {
        source.m_playerId = player->playerId();
        source.m_name = player->username();
    }
    return source;
}

TerminalCommandSource TerminalCommandSource::withEntity(Entity& entity) const
{
    TerminalCommandSource source(*this);
    source.m_entity = &entity;

    auto* serverPlayer = dynamic_cast<mc::ServerPlayer*>(&entity);
    if (serverPlayer != nullptr) {
        source.m_player = serverPlayer;
        source.m_playerId = serverPlayer->playerId();
        source.m_name = serverPlayer->username();
    } else if (entity.hasCustomName()) {
        source.m_name = entity.customNameText();
    } else {
        source.m_name = entity.getTypeId();
    }

    return source;
}

TerminalCommandSource TerminalCommandSource::withPosition(const Vector3d& pos) const
{
    TerminalCommandSource source(*this);
    source.m_position = pos;
    return source;
}

TerminalCommandSource TerminalCommandSource::withRotation(const Vector2f& rot) const
{
    TerminalCommandSource source(*this);
    source.m_rotation = rot;
    return source;
}

TerminalCommandSource TerminalCommandSource::withDimension(DimensionId dimensionId) const
{
    TerminalCommandSource source(*this);
    source.m_dimensionId = dimensionId;
    return source;
}

TerminalCommandSource TerminalCommandSource::withFeedbackDisabled() const
{
    TerminalCommandSource source(*this);
    source.m_feedbackDisabled = true;
    return source;
}

TerminalCommandSource TerminalCommandSource::withSuppressedOutput() const
{
    return withFeedbackDisabled();
}

TerminalCommandSource TerminalCommandSource::withAnchor(mc::command::EntityAnchorType anchor) const
{
    TerminalCommandSource source(*this);
    source.m_anchor = anchor;
    return source;
}

TerminalCommandSource TerminalCommandSource::withPermissionLevel(i32 level) const
{
    TerminalCommandSource source(*this);
    source.m_permissionLevel = level;
    return source;
}

TerminalCommandSource TerminalCommandSource::withMaximumPermission(i32 level) const
{
    TerminalCommandSource source(*this);
    source.m_permissionLevel = std::max(source.m_permissionLevel, level);
    return source;
}

} // namespace mc::server::console
