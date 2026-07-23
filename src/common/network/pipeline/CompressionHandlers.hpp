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
#include "common/network/crypto/ZlibCodec.hpp"

#include <vector>

namespace mc::network::pipeline {

/**
 * @brief 压缩层 handler（出站：packetID+payload → VarInt(数据长度)+压缩流）
 *
 * 对应 MC Java CompressionEncoder。持有当前阈值，send 路径每包经 ZlibCodec::encode。
 * threshold = -1 时禁用（不装本 handler），故本 handler 激活时 threshold >= 0。
 * 装在加密层之内：出站先压缩再加长度前缀再加密。
 */
class CompressionEncoder {
public:
    explicit CompressionEncoder(i32 threshold) noexcept
        : m_threshold(threshold)
    {}

    [[nodiscard]] i32 threshold() const noexcept { return m_threshold; }

    /**
     * @brief 压缩一个包（packetID+payload）
     *
     * @param input 待压缩的 packetID+payload
     * @param output 写出 VarInt(数据长度) + 压缩流/原文
     */
    [[nodiscard]] Result<void> encode(const std::vector<u8>& input, std::vector<u8>& output);

private:
    i32 m_threshold;
};

/**
 * @brief 解压层 handler（入站：VarInt(数据长度)+payload → packetID+payload）
 *
 * 对应 MC Java CompressionDecoder。recv 路径每包经 ZlibCodec::decode。
 * 装在解密层之内：入站先解密再加长度前缀解帧再解压。
 */
class CompressionDecoder {
public:
    explicit CompressionDecoder(i32 threshold) noexcept
        : m_threshold(threshold)
    {}

    [[nodiscard]] i32 threshold() const noexcept { return m_threshold; }

    /**
     * @brief 解压一个压缩层包
     *
     * @param input 已解帧的完整压缩层字节（VarInt(数据长度) + payload）
     * @param output 写出解压后的 packetID+payload
     */
    [[nodiscard]] Result<void> decode(const std::vector<u8>& input, std::vector<u8>& output);

private:
    i32 m_threshold;
};

} // namespace mc::network::pipeline
