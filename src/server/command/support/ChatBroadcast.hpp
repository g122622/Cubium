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

#include "common/util/text/ITextComponent.hpp"

namespace mc {

namespace server {
class IServer;
}

namespace command::support {

/**
 * @brief 把系统聊天组件广播给所有在线玩家
 *
 * 对应原版 `PlayerList.broadcastSystemMessage(component, overlay=false)`：/say、/me、
 * 队伍消息等命令共用。组件只序列化一次，所有接收者复用同一份 NBT 字节。
 *
 * @param server 服务器实例
 * @param message 聊天组件（translate 结构原样下发，由客户端本地化）
 */
void broadcastSystemChat(server::IServer& server, const text::ITextComponent& message);

} // namespace command::support
} // namespace mc
