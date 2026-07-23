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
#include "common/network/ir/IrPacketBase.hpp"

#include <string>

namespace mc::network::ir::play {

/**
 * @brief KeepAlive（双向，心跳）
 *
 * S→C 发 id，C→S 原样回 id。用于超时检测。id 通常为服务端当前 tick 计数。
 */
struct KeepAlive {
    i64 id;
    BedrockMeta bedrock{};
};

/**
 * @brief Disconnect（S→C，踢出）
 *
 * reason 为 JSON 文本组件字符串。
 */
struct Disconnect {
    std::string reason;
    BedrockMeta bedrock{};
};

/**
 * @brief MovePlayerPos（C→S，玩家位置移动）
 *
 * 玩家位移时上报新坐标与 onGround 标志。对应 Java ServerboundMovePlayerPosPacket。
 * TODO(Phase3): 补 PosRot/Rot/StatusOnly 变体与协议相对/绝对移动标志。
 */
struct MovePlayerPos {
    f64 x;
    f64 y;
    f64 z;
    bool onGround;
    BedrockMeta bedrock{};
};

/**
 * @brief Chat（C→S，玩家聊天）
 *
 * message 为玩家输入文本，timestamp 为发送时间戳（毫秒）。
 * TODO(Phase3): 补 signature/salt/lastSeen 等签名链字段（1.21.11 聊天签名）。
 */
struct Chat {
    std::string message;
    i64 timestamp;
    BedrockMeta bedrock{};
};

} // namespace mc::network::ir::play
