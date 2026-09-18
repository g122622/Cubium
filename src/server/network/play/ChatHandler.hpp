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

#include "common/core/Types.hpp"
#include "common/network/ir/IrPacket.hpp"
#include "server/network/play/base/PlayHandlerBase.hpp"

#include <string>

namespace mc::server::net {

/**
 * @brief 聊天与命令执行包族
 *
 * Chat 与 ChatCommand 两条入站路径都汇聚到 `_executePlayerCommand`：前者在消息以 '/'
 * 开头时转为命令，后者是客户端命令通道（不含 '/' 前缀，CommandDispatcher::parse 两种
 * 写法都能剥）。
 */
class ChatHandler : public PlayHandlerBase {
public:
    explicit ChatHandler(MinecraftServer& server)
        : PlayHandlerBase(server)
    {}

    /// Chat：普通聊天；以 '/' 开头则按命令处理
    void handleChatMessagePacket(PlayerId playerId, const mc::network::ir::IrPacket& packet);

    /// ChatCommand：客户端命令通道（command 不含 '/' 前缀）
    void handleChatCommandPacket(PlayerId playerId, const mc::network::ir::IrPacket& packet);

private:
    /// 执行玩家命令。commandInput 含或不含 '/' 前缀均可（CommandDispatcher::parse 自动剥离）。
    void _executePlayerCommand(PlayerId playerId, const std::string& commandInput);
};

} // namespace mc::server::net
