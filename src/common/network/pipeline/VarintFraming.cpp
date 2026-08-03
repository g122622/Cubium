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

#include "common/network/pipeline/VarintFraming.hpp"
#include "common/core/Types.hpp"
#include <cstddef>
#include <vector>

namespace mc::network::pipeline {

namespace {

void writeVarUInt(std::vector<u8>& out, u32 value)
{
    while (true) {
        if ((value & ~static_cast<u32>(0x7F)) == 0) {
            out.push_back(static_cast<u8>(value));
            return;
        }
        out.push_back(static_cast<u8>((value & 0x7Fu) | 0x80u));
        value >>= 7;
    }
}

// VarInt 读取失败原因
bool tryReadVarUInt(const u8* data, usize size, u32& outValue, usize& outConsumed)
{
    u32 value = 0;
    for (usize i = 0; i < 5; ++i) {
        if (i >= size) {
            return false; // 数据不足
        }
        const u8 byte = data[i];
        value |= static_cast<u32>(byte & 0x7Fu) << (7 * i);
        if ((byte & 0x80u) == 0) {
            outValue = value;
            outConsumed = i + 1;
            return true;
        }
    }
    return false; // 超过 5 字节，非法 VarInt
}

} // namespace

void VarintFraming::encodeFrame(const u8* payload, usize size, std::vector<u8>& output)
{
    writeVarUInt(output, static_cast<u32>(size));
    if (payload != nullptr && size > 0) {
        output.insert(output.end(), payload, payload + size);
    }
}

bool VarintFraming::tryDecodeFrame(std::vector<u8>& buffer, std::vector<u8>& frameOut)
{
    if (buffer.empty()) {
        return false;
    }

    u32 frameLength = 0;
    usize varIntSize = 0;
    if (!tryReadVarUInt(buffer.data(), buffer.size(), frameLength, varIntSize)) {
        return false; // 长度 VarInt 不完整
    }

    const usize totalFrameSize = varIntSize + frameLength;
    if (buffer.size() < totalFrameSize) {
        return false; // payload 不完整
    }

    frameOut.assign(buffer.begin() + static_cast<std::ptrdiff_t>(varIntSize),
        buffer.begin() + static_cast<std::ptrdiff_t>(totalFrameSize));
    buffer.erase(buffer.begin(), buffer.begin() + static_cast<std::ptrdiff_t>(totalFrameSize));
    return true;
}

} // namespace mc::network::pipeline
