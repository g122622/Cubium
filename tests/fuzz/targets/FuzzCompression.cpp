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

// Fuzz 目标：压缩层（pipeline/CompressionHandlers → crypto/ZlibCodec）。
//
// 输入语义：第 1 字节为阈值选择器（-1 表示禁用压缩，0..254 为实际阈值），其余为压缩层字节
// （VarInt(数据长度) + 原文或 zlib 流）。
// 覆盖要点：
//   - 声明长度 VarInt 被截断 / 超 5 字节；
//   - 声明长度 > kMaxUncompressed（8MB）被拒；
//   - 声明的压缩包长度小于阈值被拒（Java CompressionDecoder 的校验）；
//   - 压缩层尾部有残留字节（consumed != input.size()）；
//   - 解压实际长度与声明长度不一致；
//   - zlib 流畸形（inflate 失败）；
//   - 合法输入的 encode/decode 往返（含"数据小于阈值则原样透传"分支）。

#include "common/network/pipeline/CompressionHandlers.hpp"

#include <cstddef>
#include <cstdint>
#include <vector>

using namespace mc;
using namespace mc::network;

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data, std::size_t size)
{
    if (size < 2) {
        return 0;
    }

    // 第 1 字节取阈值：-1 禁用压缩；0..254 为实际阈值。
    const i32 threshold = static_cast<i32>(data[0]) - 1;

    std::vector<u8> input(data + 1, data + size);
    std::vector<u8> decoded;

    pipeline::CompressionDecoder decoder(threshold);
    if (!decoder.decode(input, decoded).success()) {
        // 畸形输入在此返回——这条分支本身就是被覆盖的路径。
        return 0;
    }

    // 合法输入：做一次 encode → decode 往返，覆盖 encoder 的"压缩"与"原文透传"两个分支。
    pipeline::CompressionEncoder encoder(threshold);
    std::vector<u8> recompressed;
    if (encoder.encode(decoded, recompressed).success()) {
        std::vector<u8> roundTrip;
        pipeline::CompressionDecoder roundTripDecoder(threshold);
        (void)roundTripDecoder.decode(recompressed, roundTrip);
    }

    // 再对原始输入直接压缩：输入通常小于阈值，走"写 0 + 原文"透传分支。
    std::vector<u8> passthrough;
    if (encoder.encode(input, passthrough).success()) {
        std::vector<u8> passthroughOut;
        pipeline::CompressionDecoder passthroughDecoder(threshold);
        (void)passthroughDecoder.decode(passthrough, passthroughOut);
    }

    return 0;
}
