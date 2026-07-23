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
#include <vector>

namespace mc::network::ir::configuration {

/**
 * @brief RegistryData（S→C，注册表数据同步）
 *
 * 1.21.11 服务端在 Configuration 阶段推送维度/生物群系/聊天类型等注册表数据。
 * TODO(Phase3): 完整字段（registryEntry 列表等）待对齐 1.21.11 时补全。
 */
struct RegistryData {
    std::string registryKey;
    std::vector<u8> payload; // 暂存原始字节，Phase3 细化
    BedrockMeta bedrock{};
};

/**
 * @brief FinishConfiguration（S→C / C→S，配置完成，terminal）
 *
 * 双向 terminal 包：服务端发完所有配置数据后发 S→C 版，客户端回 C→S 版确认，
 * 双方处理后切到 Play 阶段。
 */
struct FinishConfiguration {
    static constexpr bool kTerminal = true;

    BedrockMeta bedrock{};
};

} // namespace mc::network::ir::configuration
