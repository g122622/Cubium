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

#include "common/network/backend/IProtocolBackend.hpp"
#include "common/network/buffer/RegistryByteBuf.hpp"

namespace mc::network::backend::java {

/**
 * @brief Java 1.21.11 协议版本常量（对应 Java DataVersion / protocol version）
 */
inline constexpr i32 kJavaProtocolVersion = 774;

/**
 * @brief Java 1.21.11 协议后端
 *
 * 把 ir::IrPacket 编解码成 Java 1.21.11 线协议字节（VarInt 长度前缀帧 + VarInt packet id
 * + payload，大端，经 RSA+AES-CFB8 加密、zlib 压缩）。provideProtocolTables 委托
 * JavaProtocolTables 构建 5 阶段包表（addPacket 顺序严格对齐 GameProtocols.java）。
 *
 * TODO(Phase3): JavaProtocolTables 填充各阶段 addPacket 链 + 各包 codec。
 */
class JavaBackend final : public IProtocolBackend<buffer::RegistryByteBuf> {
public:
    [[nodiscard]] std::string name() const override { return "java"; }

    [[nodiscard]] std::vector<i32> supportedProtocolVersions() const override { return {kJavaProtocolVersion}; }

    [[nodiscard]] std::shared_ptr<pipeline::ProtocolTableSet<buffer::RegistryByteBuf>> provideProtocolTables() override;
};

} // namespace mc::network::backend::java
