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

namespace mc::network::crypto {

/**
 * @brief MC Java 网络压缩/解压（zlib + threshold）
 *
 * 对应 MC Java CompressionEncoder/Decoder：
 * - 压缩层线格式：每个包 = VarInt(数据长度) + payload。
 *   - 数据长度 == 0：后接未压缩的（packetID + payload）。
 *   - 数据长度 > 0：该值是解压后总长度，后接 zlib 压缩流，解压得（packetID + payload）。
 * - threshold 含义：未压缩数据长度 < threshold 时不压缩（写 0）；>= threshold 时压缩。
 *   threshold = -1 表示禁用压缩（不装压缩层）。
 * - 校验常量：MAXIMUM_UNCOMPRESSED_LENGTH = 8388608（8MB），MAXIMUM_COMPRESSED_LENGTH = 2097152（2MB）。
 *
 * 本类是无状态工具：encode/decode 各接受阈值，状态由 pipeline handler 持有。
 * 压缩用 zlib Deflate（默认级别），解压用 zlib Inflate（windowBits=15）。
 */
class ZlibCodec {
public:
    static constexpr i32 kDisabled = -1;             // 阈值 -1 表示禁用压缩
    static constexpr u32 kMaxUncompressed = 8388608; // 8MB
    static constexpr u32 kMaxCompressed = 2097152;   // 2MB

    /**
     * @brief 压缩一个包（packetID + payload）
     *
     * @param threshold 压缩阈值（<0 视为不压缩）
     * @param input 待压缩的 packetID+payload
     * @param output 写出 VarInt(数据长度) + (未压缩原文 或 zlib 压缩流)
     *
     * dataLength 写入规则：input.size() < threshold → 写 0 + 原文；
     * 否则写 input.size() + zlib(input)。
     */
    [[nodiscard]] static Result<void> encode(i32 threshold, const std::vector<u8>& input, std::vector<u8>& output);

    /**
     * @brief 解压一个压缩层包
     *
     * @param input VarInt(数据长度) + payload 起始字节
     * @param inputSize input 可用字节
     * @param threshold 当前阈值（用于校验：声明长度不应小于阈值）
     * @param dataLengthOut 输出读到的数据长度（0=未压缩）
     * @param output 写出解压后的（packetID + payload）
     * @param consumedOut 输出 input 中被本函数消费的字节数（VarInt + payload）
     *
     * 注意：本函数假设调用方已按 VarInt 帧长度切出完整压缩层数据；
     * consumedOut 用于多包连包场景推进游标。
     */
    [[nodiscard]] static Result<void> decode(const u8* input,
        usize inputSize,
        i32 threshold,
        i32& dataLengthOut,
        std::vector<u8>& output,
        usize& consumedOut);

private:
    [[nodiscard]] static Result<std::vector<u8>> deflateBytes(const u8* data, usize size);
    [[nodiscard]] static Result<std::vector<u8>> inflateBytes(const u8* data, usize size, u32 maxOut);
};

} // namespace mc::network::crypto
