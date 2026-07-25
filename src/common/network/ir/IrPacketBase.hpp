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

#include <optional>

namespace mc::network::ir {

/**
 * @brief IR 包公共基类特征（非虚基类，仅提供静态/可选字段约定）
 *
 * IR 包是 plain struct，不继承虚基类（游戏逻辑用 std::visit 消费，零虚函数开销）。
 * 本文件仅约定两类公共特征：
 * - kTerminal：terminal 包（ClientIntention/LoginAcknowledged/FinishConfiguration 等）驱动
 *   状态机阶段切换。各 terminal 包 struct 内声明 `static constexpr bool kTerminal = true`。
 * - 基岩预留字段：相关包 struct 加 std::optional 基岩专属字段（subclientSender/Target 等），
 *   Java 后端编解码时忽略（默认 nullopt），基岩后端将来填充，避免冲击 IR 类型系统。
 *
 * 提供 BedrockMeta 辅助结构体集中基岩预留字段，包 struct 按需组合进成员。
 */
struct BedrockMeta {
    /**
     * @brief 基岩版子客户端发送方 ID（Java 无此概念，默认 nullopt）
     */
    std::optional<u8> subclientSender;

    /**
     * @brief 基岩版子客户端接收方 ID
     */
    std::optional<u8> subclientTarget;

    /**
     * @brief 基岩版运行时方块 ID（Java 用方块状态 ID，基岩用 runtime id，需查表）
     */
    std::optional<i32> runtimeBlockId;

    // 默认 ==：三个 std::optional 成员均可比较，故默认 == 可用。本结构被大量 IR 包 struct
    // 作为 bedrock 预留成员嵌入——若不声明 ==，则外层 struct 的 `= default` == 会被隐式
    // 删除（无法比较 BedrockMeta），导致测试端 EXPECT_EQ 编译失败。
    [[nodiscard]] friend bool operator==(const BedrockMeta&, const BedrockMeta&) noexcept = default;
};

} // namespace mc::network::ir
