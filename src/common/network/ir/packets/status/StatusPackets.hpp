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

namespace mc::network::ir::status {

/**
 * @brief 状态请求（C→S，无字段）
 *
 * 客户端请求服务端状态 JSON。服务端响应 StatusResponse。
 */
struct StatusRequest {
    BedrockMeta bedrock{};
    [[nodiscard]] friend bool operator==(const StatusRequest&, const StatusRequest&) noexcept = default;
};

/**
 * @brief 状态响应（S→C，服务端状态 JSON）
 *
 * 包含版本、玩家数、描述、图标等。JSON 字符串原样传输。
 */
struct StatusResponse {
    std::string json;
    BedrockMeta bedrock{};
    [[nodiscard]] friend bool operator==(const StatusResponse&, const StatusResponse&) noexcept = default;
};

/**
 * @brief Ping（C→S，携带时间戳）
 *
 * 客户端发时间戳，服务端原样回 Pong，用于测 RTT。
 */
struct PingRequest {
    i64 payload;
    BedrockMeta bedrock{};
    [[nodiscard]] friend bool operator==(const PingRequest&, const PingRequest&) noexcept = default;
};

/**
 * @brief Pong（S→C，回显时间戳）
 */
struct PingResponse {
    i64 payload;
    BedrockMeta bedrock{};
    [[nodiscard]] friend bool operator==(const PingResponse&, const PingResponse&) noexcept = default;
};

} // namespace mc::network::ir::status
