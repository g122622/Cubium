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

#include "PaletteUtil.hpp"
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include <cstddef>
#include <vector>
#include <fmt/format.h>

namespace mc::world::storage::reader::bedrock::palette {

Result<std::vector<u32>> readPackedIndices(
    const std::vector<u8>& data, size_t& pos, i32 bitsPerEntry, i32 entryCount, i32 wordBits)
{
    // 验证位深度参数
    if (bitsPerEntry <= 0 || bitsPerEntry > wordBits) {
        return Error(ErrorCode::ChunkCorrupted, fmt::format("Invalid bits per entry: {}", bitsPerEntry));
    }

    // 计算每个字可以存储多少个索引值
    const i32 valuesPerWord = wordBits / bitsPerEntry;
    if (valuesPerWord <= 0) {
        return Error(ErrorCode::ChunkCorrupted, "Invalid palette packing parameters");
    }

    // 计算需要读取的字数量和字节数
    const i32 wordCount = (entryCount + valuesPerWord - 1) / valuesPerWord;
    const size_t bytesNeeded = static_cast<size_t>(wordCount) * static_cast<size_t>(wordBits / 8);

    // 验证数据长度
    if (pos + bytesNeeded > data.size()) {
        return Error(ErrorCode::ChunkCorrupted, "Packed palette indices truncated");
    }

    // 预分配结果数组并计算位掩码
    std::vector<u32> indices(static_cast<size_t>(entryCount), 0);
    const u32 mask = (bitsPerEntry == 32) ? 0xFFFFFFFFu : ((1u << bitsPerEntry) - 1u);

    // 逐字解包索引值
    i32 index = 0;
    for (i32 word = 0; word < wordCount && index < entryCount; ++word) {
        // 以小端序读取32位字
        u32 value = static_cast<u32>(data[pos]) | (static_cast<u32>(data[pos + 1]) << 8) |
            (static_cast<u32>(data[pos + 2]) << 16) | (static_cast<u32>(data[pos + 3]) << 24);
        pos += 4;

        // 从当前字中提取所有索引值
        for (i32 bit = 0; bit < valuesPerWord && index < entryCount; ++bit) {
            indices[static_cast<size_t>(index++)] = (value >> (bit * bitsPerEntry)) & mask;
        }
    }

    return indices;
}

Result<u32> readVarUint(const std::vector<u8>& data, size_t& pos)
{
    u32 value = 0;
    i32 shift = 0;

    // VarInt格式：每个字节的低7位为数据，最高位为继续标志
    // 最多读取5个字节（35位，但u32只用32位）
    while (pos < data.size() && shift < 35) {
        const u8 byte = data[pos++];
        value |= static_cast<u32>(byte & 0x7F) << shift;

        // 最高位为0表示这是最后一个字节
        if ((byte & 0x80) == 0) {
            return value;
        }
        shift += 7;
    }

    return Error(ErrorCode::ChunkCorrupted, "Invalid Bedrock varuint");
}

} // namespace mc::world::storage::reader::bedrock::palette
