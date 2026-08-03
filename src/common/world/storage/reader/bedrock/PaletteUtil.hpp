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
#include <cstddef>
#include <vector>

namespace mc::world::storage::reader::bedrock::palette {

/**
 * @brief 空调色板位深度标识
 *
 * 在基岩版区块存储格式中，当调色板为空时使用的特殊位深度值。
 * 该值用于标识区块段中没有方块数据的情况。
 */
inline constexpr i32 EMPTY_PALETTE_BITS = 127;

/**
 * @brief 从压缩数据中读取紧凑索引数组
 *
 * 基岩版区块存储格式使用位打包（bit-packing）方式存储调色板索引。
 * 该函数从原始字节数据中解包出索引值数组。
 *
 * @param data 原始字节数据
 * @param pos 当前读取位置（会被更新）
 * @param bitsPerEntry 每个索引的位深度
 * @param entryCount 索引数量
 * @param wordBits 字的位深度（通常为32位）
 * @return Result<std::vector<u32>> 解包后的索引数组，或错误信息
 */
[[nodiscard]] Result<std::vector<u32>> readPackedIndices(
    const std::vector<u8>& data, size_t& pos, i32 bitsPerEntry, i32 entryCount, i32 wordBits);

/**
 * @brief 读取基岩版变长无符号整数
 *
 * 基岩版使用VarInt格式存储变长整数，每个字节的低7位为数据位，
 * 最高位为继续标志位（1表示还有后续字节）。
 *
 * @param data 原始字节数据
 * @param pos 当前读取位置（会被更新）
 * @return Result<u32> 解码后的无符号整数，或错误信息
 */
[[nodiscard]] Result<u32> readVarUint(const std::vector<u8>& data, size_t& pos);

} // namespace mc::world::storage::reader::bedrock::palette
