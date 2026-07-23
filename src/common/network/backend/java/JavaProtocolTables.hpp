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

#include "common/network/buffer/RegistryByteBuf.hpp"
#include "common/network/pipeline/ProtocolTableSet.hpp"

#include <memory>

namespace mc::network::backend::java {

/**
 * @brief Java 1.21.11 五阶段包表构建器
 *
 * 用 ProtocolInfoBuilder 链式 addPacket 构建 5 阶段 × 2 流向共 10 张包表。
 * addPacket 顺序严格对齐 GameProtocols.java（注册顺序 = packet id）。
 *
 * TODO(Phase3): 填充各阶段 addPacket 链 + 各 IR 包的 StreamCodec<RegistryByteBuf, IrStruct>。
 *               当前 build() 返回空表（所有 ProtocolInfo 为 nullptr），骨架阶段占位。
 */
class JavaProtocolTables {
public:
    /**
     * @brief 构建并返回完整的 5 阶段包表集合
     */
    [[nodiscard]] static std::shared_ptr<pipeline::ProtocolTableSet<buffer::RegistryByteBuf>> build();
};

} // namespace mc::network::backend::java
