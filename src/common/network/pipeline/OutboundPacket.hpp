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
#include "common/network/ir/IrPacket.hpp"
#include "common/network/protocol/PacketFlow.hpp"

#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <tuple>
#include <vector>

namespace mc::network::pipeline {

/**
 * @brief 可跨连接共享的出站快照；缓存加密前的帧，避免广播重复编码和压缩。
 *
 * 构造时复制仍可变的共享 NBT，其他字段由传入包转移所有权。
 * 缓存按协议表、方向和压缩阈值隔离；流式加密始终由各连接单独执行。
 */
class OutboundPacket final {
public:
    using Bytes = std::shared_ptr<const std::vector<u8>>;
    using Encoder = std::function<Result<std::vector<u8>>()>;

    explicit OutboundPacket(ir::IrPacket packet);
    /** @brief 获取仅供读取的包快照。 */
    [[nodiscard]] const ir::IrPacket& packet() const noexcept { return m_packet; }
    /** @brief 返回对应编码配置的缓存帧，首次访问时执行编码。 */
    [[nodiscard]] Result<Bytes> prepare(
        const void* tables, protocol::PacketFlow flow, i32 threshold, const Encoder& encoder) const;

private:
    ir::IrPacket m_packet;
    mutable std::mutex m_mutex;
    mutable std::map<std::tuple<const void*, protocol::PacketFlow, i32>, Bytes> m_frames;
};

} // namespace mc::network::pipeline
