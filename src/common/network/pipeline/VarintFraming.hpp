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

#include <vector>

namespace mc::network::pipeline {

/**
 * @brief Java VarInt21 长度前缀帧编解码
 *
 * 对应 MC Java Varint21FrameDecoder/Prepender：每条消息 = VarInt(payload 长度) + payload。
 * 属于 Java wire 格式，由 Connection 流水线持有（不在通用 ITransport 里——
 * LocalTransport 零拷贝、RakNetTransport 自有分帧都不需要它）。
 *
 * 与压缩/加密层的相对位置（对齐 Java 出站 compress→frame→encrypt）：
 * - 出站：先压缩得 (VarInt(数据长度)+data)，再 frame 得 (VarInt(帧长)+前述)，
 *   最后 encrypt 整个帧。
 * - 入站：先 decrypt 整个帧，再 deframe 取出 (VarInt(数据长度)+data)，最后 decompress。
 */
class VarintFraming {
public:
    /**
     * @brief 给 payload 加 VarInt 长度前缀，输出完整帧
     */
    static void encodeFrame(const u8* payload, usize size, std::vector<u8>& output);

    /**
     * @brief 从流缓冲尝试切出完整帧
     *
     * @param buffer 输入输出：含已读入但未切帧的残留字节；返回时移除已消费部分
     * @param frameOut 输出：切出的完整 payload（不含长度前缀）；无完整帧时不修改
     * @return true 切出一帧；false 数据不足
     */
    [[nodiscard]] static bool tryDecodeFrame(std::vector<u8>& buffer, std::vector<u8>& frameOut);
};

} // namespace mc::network::pipeline
