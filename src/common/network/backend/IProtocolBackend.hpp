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

#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/network/ir/IrPacket.hpp"
#include "common/network/pipeline/ProtocolTableSet.hpp"

#include <memory>
#include <string>
#include <vector>

namespace mc::network::backend {

/**
 * @brief 协议后端接口（IR ↔ wire 的后端）
 *
 * "LLVM IR"架构里的后端：把协议无关的 ir::IrPacket 编解码成具体后端的 wire 字节。
 * Java 后端产出 Java 1.21.11 线协议字节；基岩后端产出 RakNet/GamePacket 字节（stub）。
 * Connection 持有后端提供的 ProtocolTableSet，编解码经它完成。
 *
 * 设计要点：
 * - name() 标识后端（"java"/"bedrock"）。
 * - supportedProtocolVersions() 列出支持的协议版本（Java 774；基岩 685..898）。
 * - provideProtocolTables() 构建并返回 5 阶段包表（ProtocolTableSet），注入 Connection。
 *
 * @tparam B 缓冲类型
 */
template <typename B>
class IProtocolBackend {
public:
    virtual ~IProtocolBackend() = default;

    /**
     * @brief 后端标识名（"java" / "bedrock"）
     */
    [[nodiscard]] virtual std::string name() const = 0;

    /**
     * @brief 支持的协议版本列表
     */
    [[nodiscard]] virtual std::vector<i32> supportedProtocolVersions() const = 0;

    /**
     * @brief 构建并返回 5 阶段包表，注入 Connection 的 ProtocolTableSet
     */
    [[nodiscard]] virtual std::shared_ptr<pipeline::ProtocolTableSet<B>> provideProtocolTables() = 0;
};

} // namespace mc::network::backend
