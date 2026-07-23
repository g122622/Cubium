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

namespace mc::network::backend {

/**
 * @brief 基岩版协议后端 stub
 *
 * 基岩版基于 RakNet（UDP），线协议为 GamePacket（0xfe）+ 子客户端 ID + batch/zlib，
 * 握手走 JWT/ECDH。本 stub 实现后端接口但 provideProtocolTables 返回空表，
 * supportedProtocolVersions 返回基岩 1.21.x 常量预留（685..898）。
 *
 * 基岩后端落地时：实现 RakNet 传输 + GamePacket codec + JWT 握手 + 各阶段包表。
 *
 * TODO(bedrock): 全部实现。
 */
template <typename B>
class BedrockBackend final : public IProtocolBackend<B> {
public:
    [[nodiscard]] std::string name() const override { return "bedrock"; }

    [[nodiscard]] std::vector<i32> supportedProtocolVersions() const override
    {
        // 基岩 1.21.x 协议版本区间预留（685..898）。
        return {685,
            686,
            687,
            688,
            689,
            690,
            691,
            692,
            693,
            694,
            695,
            696,
            697,
            698,
            699,
            700,
            712,
            729,
            748,
            766,
            771,
            776,
            786,
            800,
            818,
            836,
            845,
            856,
            871,
            886,
            898};
    }

    [[nodiscard]] std::shared_ptr<pipeline::ProtocolTableSet<B>> provideProtocolTables() override
    {
        // TODO(bedrock): 构建基岩 5 阶段包表。当前返回空表（所有 ProtocolInfo 为 nullptr）。
        return std::make_shared<pipeline::ProtocolTableSet<B>>();
    }
};

} // namespace mc::network::backend
